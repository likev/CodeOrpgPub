/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/06/04 16:12:37 $
 * $Id: hci_options.c,v 1.104 2014/06/04 16:12:37 steves Exp $
 * $Revision: 1.104 $
 * $State: Exp $
 */ 

/************************************************************************
 *									*
 *	Module:       hci_options
 *									*
 *	Description:  Process options for an hci task
 *									*
 ************************************************************************/

/* Local include files */

#include <hci.h>
#include <hci_options.h>
#include <hci_icon.h>

/* Enums */

enum { BREAK_BY_TIME, BREAK_BY_CHECK, BREAK_BY_CANCEL };
enum { NO_RESTART_HCI_TASK, RESTART_HCI_TASK };

/* Macros */

#define	NUM_LE_LOG_MSGS		5000    /* num msgs in log file */
#define	CONNECT_CHECK_TIME	10      /* connectivity check period (sec) */
#define	RECONNECT_LAG		10      /* lag after connectivity detected */
#define	MRPG_STATE_MSGID	1       /* msg id of Mrpg_state_t */
#define	MRPG_SYNC_LAG		10      /* time until mrpg synchronization */
#define	DISPLAY_TIMER		100     /* timer for countdown dialog (ms) */
#define SHUTDOWN_WAIT_TIME	5       /* time to wait for app shutdown */
#define	MRPG_STARTUP_WAIT	180     /* time to wait for MRPG to startup */
#define	NETWORK_CONNECT_WAIT	180     /* time to wait for intial network */
#define	NETWORK_RECONNECT_WAIT	360     /* time to wait for network reconnect */
#define	MAX_NUM_SHELLS		10	/* max number of shells to handle. */
#define	HCI_LE_LB_TYPE_FLAG	0
#define	HCI_LE_INSTANCE_FLAG	-1
#define	HCI_LE_SYSTEM_LOG_FLAG	0

typedef struct
{
  Widget w;
  char title[HCI_BUF_128];
} Hci_Shell_widget_t;

/* Static/global variables */

static Widget Top_widget = ( Widget ) NULL;
static XtAppContext App_context = NULL;       /* Application context */
static Display *HCI_display = ( Display * ) NULL;
static Screen *HCI_screen = ( Screen * ) NULL;
static Colormap HCI_colormap;
static Dimension HCI_depth;
static char Top_widget_title[HCI_BUF_128] = "default";
static int (*HCI_user_destroy_callback)() = NULL;
static void (*HCI_user_timer_proc)() = NULL;
static Cursor Busy_cursor = ( Cursor ) NULL;
static Cursor Selectable_cursor = ( Cursor ) NULL;
static int Site_info_node = HCI_UNKNOWN_NODE;
static int Site_info_system = HCI_UNKNOWN_SYSTEM;
static int Site_info_channel_number = 1;
static int Rpg_connected = 1; /* Connection flag to mrpg host */
static int Sys_cfg_changed = 0; /* Detected change to system config */
static int Channel_number = 0; /* Channel number */
static int Started_by_hci = 0; /* this app is started by hci */
static int Chan_num_for_syscfg = 0; /* channel from ORPGMGR_setup_sys_cfg */
static unsigned int Rpga_ip = -1; /* IP of RPGA node */
static int Low_bandwidth_flag = 0; /* TRUE if running in low bandwidth */
static int Satellite_connection_flag = 0; /* TRUE if satellite connection. */
static int Simulation_speed = -1; /* Bandwidth simulation speed - low bw only */
static char *Orpg_machine_name = ""; /* name of the host where mrpg runs */
static int Transaction_timeout = 0; /* Time out for LB transactions */
static int Object_index = -1; /* Index of GUI object in task that starts app */
static char *Restart_cmd = NULL; /* String used to restart this app */
static int Restart_flag = NO_RESTART_HCI_TASK;
static int Restarted_by_hci = 0;
static int HCI_argc;          /* command line argument count */
static char **HCI_argv;               /* command line arguments */
static char HCI_custom_args[128];     /* custom command line arguments */
static int (*HCI_custom_args_callback)( char, char * ) = NULL;
static void (*HCI_custom_args_help_callback)() = NULL;
static int Top_widget_minimized_flag = HCI_NO_FLAG;
static int Time_interval_count = -1;
static int User_time_interval_count = -1;
static int HCI_task_index = -1;
static int HCI_site_def_mode_A_vcp = -1;
static int HCI_site_def_mode_B_vcp = -1;
static int HCI_site_def_wx_mode = -1;
static int HCI_site_bdds_flag = -1;
static int HCI_site_mlos_flag = -1;
static int HCI_site_rms_flag = -1;
static int HCI_site_rda_elev = -1;
static int HCI_site_rda_lat = -1;
static int HCI_site_rda_lon = -1;
static int HCI_site_rpg_id = -1;
static int HCI_site_orda_flag = -1;
static char HCI_site_rpg_name[ HCI_BUF_128 ];
static int Redundant_status_updated_flag = HCI_NO_FLAG;
static int Partial_init_flag = HCI_NO_FLAG;
static Hci_Shell_widget_t Hci_Shell[MAX_NUM_SHELLS];
static int Hci_Shell_count = 0;

static char* HCI_task_titles[ NUM_HCI_TASKS ] = {
"RPG Control/Status", /* HCI_TASK */
"RPG Product Generation Table Editor", /* HCI_PROD_TASK */
"RDA Performance Data", /* HCI_PERF_TASK */
"Environmental Data Editor", /* HCI_WIND_TASK */
"Clutter Regions Editor", /* HCI_CCZ_TASK */
"VCP and Mode Control", /* HCI_VCP_TASK */
"RPG Base Data Display", /* HCI_BASEDATA_TASK */
"PRF Control", /* HCI_HCI_PRF_TASK */
"Clutter Bypass Map Display", /* HCI_CBM_TASK */
"Alert Threshold Editor", /* HCI_ALT_TASK */
"Products in Database", /* HCI_PSTAT_TASK */
"RPG Status", /* HCI_STATUS_TASK */
"Archive II", /* HCI_ARCHII_TASK */
"RPG Product Priority (Load Shed Products)", /* HCI_PROD_PRIORITY_TASK */
"Product Distribution Comms Status", /* HCI_NB_TASK */
"Edit Selectable Product Parameters", /* HCI_SPP_TASK */
"Load HUB Router Files", /* HCI_HUB_RTR_LOAD_TASK */
"Configure HUB Router", /* HCI_CONFIGURE_HUB_RTR_TASK */
"HCI Passwords", /* HCI_PASSWD_TASK */
"Precipitation Status", /* HCI_PRECIP_STATUS_TASK */
"RDA/RPG Interface Control/Status", /* HCI_RDA_LINK_TASK */
"RDA Control/Status", /* HCI_RDC_TASK */
"RDA Alarms", /* HCI_RDA_TASK */
"RPG Control", /* HCI_RPC_TASK */
"Load Shed Categories", /* HCI_LOAD_TASK */
"RPG Product Distribution Control", /* HCI_PDC_TASK */
"Algorithms", /* HCI_APPS_ADAPT_TASK */
"RDA Performance Data", /* HCI_ORDA_PMD_TASK */
"Mode Automation Status", /* HCI_MODE_STATUS_TASK */
"Misc", /* HCI_MISC_TASK */
"Restore Adaptation Data", /* HCI_RESTORE_ADAPT_TASK */
"Save Adaptation Data", /* HCI_SAVE_ADAPT_TASK */
"Merge Adaptation Data", /* HCI_MERGE_TASK */
"Blockage Data Display", /* HCI_BLOCKAGE_TASK */
"Hardware Configuration", /* HCI_HARDWARE_CONFIG_TASK */
"View Log File", /* HCI_LOG_TASK */
"RPG Base Data Display Tool", /* HCI_BASEDATA_TOOL */
"RPG Status Print Tool", /* HCI_PRINT_STATUS_TOOL */
"RPG Terrain/Blockage Display Tool", /* HCI_TRD_TOOL */
"RDA Simulator GUI Tool", /* HCI_RDASIM_GUI_TOOL */
"Model Data Viewer", /* HCI_ENVIROVIEWER_TASK */
"SAILS Control" /* HCI_SAILS_TASK */
};

/*      Months                          */

static char *HCI_months [] = { "", "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                   "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
static void (*Check_install) (void) = NULL;

/* Function prototypes */

static void Destroy_callback( Widget, XtPointer, XtPointer );
static int  Detect_connectivity();
static void Shutdown_this_app( char *, int );
static void Set_up_system_config();
static int Display_dialog( char *, char *, int, int, int (*)() ); 
static void Syscfg_change_cb( EN_id_t, char *, int, void * ); 
static int Read_command_line_options();
static void Wait_for_connected();
static int Check_for_connected();
static void Wait_for_mrpg();
static int Check_for_mrpg();
static void Set_top_widget_title();
static void Set_system_info();
static void Cancel_cb( Widget, XtPointer, XtPointer );
static int Post_task_started_event();
static XtTimerCallbackProc Timer_proc( Widget, XtIntervalId );
static XtTimerCallbackProc Partial_timer_proc( Widget, XtIntervalId );
static int HCI_XError_handler( Display *, XErrorEvent * );
static int HCI_XIOError_handler( Display * );
static void Map_event_callback( Widget, XtPointer, XEvent *, Boolean * );
static void HCI_LE_multiline( int, const char *, va_list * );
static void Set_site_info();
static void Set_shell_title( Widget, char * );
static void Update_shell_titles();
static void Redundant_status_updated( int, LB_id_t, int, void * );

/*************************************************************************

  Initialize HCI task. This function requires adaptaion data be installed,
  thus, this is only for RPG-related HCIs. The hci_task_flag variable must
  have a valid value.

*************************************************************************/

void HCI_init( int argc, char **argv, int hci_task_flag )
{   
  Atom wm_delete_window;
  ArgList args = (ArgList) NULL;
  Pixmap bitmap;
  int status;
  int i;

  /* Validate/set task flag. */

  HCI_task_index = hci_task_flag;
  if( HCI_task_index < 0 || HCI_task_index >= NUM_HCI_TASKS )
  {
    fprintf( stderr, "Invalid HCI_task_index" );
    HCI_task_exit( HCI_EXIT_FAIL );
  }

  /* Save original argc/argv for use outside this function. An original
     copy has to be saved because XtAppInitialize will remove any
     Xt-specific arguments. */

  HCI_argc = argc;
  HCI_argv = (char **) malloc( sizeof( char* ) * HCI_argc );
  if( argv == NULL )
  {
    fprintf( stderr, "HCI_argv malloc failed\n" );
    HCI_task_exit( HCI_EXIT_FAIL );
  }
  for( i = 0; i < HCI_argc; i++ )
  {
    HCI_argv[i] = malloc( sizeof( char ) * strlen( argv[i])  + 1 );
    strcpy( HCI_argv[i], argv[i] );
  }

  /* Initialize the Xt toolkit and create the top level widget. */

  Top_widget = XtAppInitialize( &App_context, Top_widget_title,
                                NULL, 0, &argc, argv, NULL, args, 0 );
  if( Top_widget == NULL )
  {
    fprintf( stderr, "HCI_init failed to create Top_widget\n" );
    HCI_task_exit( HCI_EXIT_FAIL );
  }

  HCI_display = XtDisplay( Top_widget );
  HCI_screen = XtScreen( Top_widget );
  HCI_colormap = DefaultColormap( HCI_display, DefaultScreen( HCI_display ) );
  HCI_depth = XDefaultDepth( HCI_display, DefaultScreen( HCI_display ) );

  /* Initialize colors and fonts. */

  hci_initialize_read_colors( HCI_display, HCI_colormap );
  hci_initialize_fonts( HCI_display );

  /* Register for any X errors. */

  XSetErrorHandler( HCI_XError_handler );
  XSetIOErrorHandler( HCI_XIOError_handler );

  /* Register termination handler to dump stack. */

  if( HCI_register_term_hdlr( NULL ) < 0 )
  {
    HCI_LE_error( "Unable to register termination handler" );
  }

  /* Read command-line options and set flags accordingly. */

  if( Read_command_line_options() < 0 )
  {
    fprintf( stderr, "Invalid options\n" );
    HCI_task_exit( HCI_EXIT_FAIL );
  }

  /* Set write permission for various DATA LBs. */

  ORPGDA_write_permission( ORPGDAT_HCI_DATA );

  if( Site_info_system == HCI_FAA_SYSTEM )
  {
    ORPGDA_write_permission( ORPGDAT_REDMGR_CMDS );
    ORPGDA_write_permission( ORPGDAT_REDMGR_CHAN_MSGS );
  }

  /* Set restart flag if this is main HCI task. */

  if( HCI_task_index == HCI_TASK )
  {
    Restart_flag = RESTART_HCI_TASK;
    HCI_LE_error( "Application will restart if network connectivity is lost" );
  }
  else
  {
    Restart_flag = NO_RESTART_HCI_TASK;
    HCI_LE_error( "Application will not restart if network connectivity is lost" );
  }

  /* Set system info. */

  Set_system_info();

  /* Initialize DEAU DB. */

  ORPGMISC_deau_init();

  /* Set site info. */

  Set_site_info();

  /* Set title. */

  Set_top_widget_title();

  /* Define the icon to use when minimizing window. */

  bitmap = XCreatePixmapFromBitmapData( HCI_display,
           RootWindowOfScreen( XtScreen( Top_widget ) ),
           ( char * ) hci_icon_bits, hci_icon_width, hci_icon_height,
           1, 0, 1 );

  /* Define busy and selectable cursors. */

  Busy_cursor = XCreateFontCursor( HCI_display, XC_watch );
  Selectable_cursor = XCreateFontCursor( HCI_display, XC_hand2 );

  /* Set top widget attributes.  */

  XtVaSetValues( Top_widget,
                 XmNdeleteResponse, XmDO_NOTHING,
                 XmNiconic, False,
                 XmNiconPixmap, bitmap,
                 XmNallowShellResize, True,
                 XmNmwmFunctions, MWM_FUNC_ALL | MWM_FUNC_MAXIMIZE,
                 NULL );

  XtAddCallback( Top_widget, XmNdestroyCallback, Destroy_callback, NULL );
  XtAddEventHandler( Top_widget, StructureNotifyMask, False, Map_event_callback, NULL );

  /* If window is destroyed other than using "Close" button,
     call destory callback f(x). */

  wm_delete_window = XmInternAtom( HCI_display,
                                   "WM_DELETE_WINDOW", False );
  XmAddWMProtocolCallback( Top_widget, wm_delete_window,
                           Destroy_callback, NULL );

  /* For FAA, register for Local/Redundant Status changes. */

  if( Site_info_system == HCI_FAA_SYSTEM )
  {
    status = ORPGDA_UN_register( ORPGDAT_REDMGR_CHAN_MSGS,
                                 ORPGRED_CHANNEL_STATUS_MSG,
                                 Redundant_status_updated );

    if( status != LB_SUCCESS )
    {
      HCI_LE_error( "Unable to register for local ch status (%d)", status );
      HCI_task_exit( HCI_EXIT_FAIL );
    }

    status = ORPGDA_UN_register( ORPGDAT_REDMGR_CHAN_MSGS,
                                 ORPGRED_REDUN_CHANL_STATUS_MSG,
                                 Redundant_status_updated );

    if( status != LB_SUCCESS )
    {
      HCI_LE_error( "Unable to register for redundant ch status (%d)", status );
      HCI_task_exit( HCI_EXIT_FAIL );
    }
  }

  /* Block RPG event processing until timer event. */
  
  (void) EN_cntl_block();
} 

void HCI_set_check_install (void (*check_install)(void)) {
  Check_install = check_install;
}

/*************************************************************************

  Partially Initialize HCI task, such that adaptation data isn't
  required to be installed to run. This is useful for HCIs that
  have to run before adaptation data is installed or hardware
  devices are configured. A negative task flag implies the task
  will handle the HCI title.

*************************************************************************/

void HCI_partial_init( int argc, char **argv, int hci_task_flag )
{   
  Atom wm_delete_window;
  ArgList args = (ArgList) NULL;
  Pixmap bitmap;
  int i;
  int ret;
  char le_label[HCI_BUF_256], le_fname[HCI_BUF_256];

  /* Set flag to indicate this is a partial init. */

  Partial_init_flag = HCI_YES_FLAG;

  /* Validate/set task flag. */

  HCI_task_index = hci_task_flag;
  if( HCI_task_index >= NUM_HCI_TASKS )
  {
    fprintf( stderr, "Invalid HCI_task_index" );
    HCI_task_exit( HCI_EXIT_FAIL );
  }

  /* Save original argc/argv for use outside this function. An original
     copy has to be saved because XtAppInitialize will remove any
     Xt-specific arguments. */

  HCI_argc = argc;
  HCI_argv = (char **) malloc( sizeof( char* ) * HCI_argc );
  if( argv == NULL )
  {
    fprintf( stderr, "HCI_argv malloc failed\n" );
    HCI_task_exit( HCI_EXIT_FAIL );
  }
  for( i = 0; i < HCI_argc; i++ )
  {
    HCI_argv[i] = malloc( sizeof( char ) * strlen( argv[i] ) + 1 );
    strcpy( HCI_argv[i], argv[i] );
  }

  /* Initialize the Xt toolkit and create the top level widget. */

  Top_widget = XtAppInitialize( &App_context, Top_widget_title,
                                NULL, 0, &argc, argv, NULL, args, 0 );
  if( Top_widget == NULL )
  {
    fprintf( stderr, "HCI_init failed to create Top_widget\n" );
    HCI_task_exit( HCI_EXIT_FAIL );
  }

  HCI_display = XtDisplay( Top_widget );
  HCI_screen = XtScreen( Top_widget );
  HCI_colormap = DefaultColormap( HCI_display, DefaultScreen( HCI_display ) );
  HCI_depth = XDefaultDepth( HCI_display, DefaultScreen( HCI_display ) );

  /* Initialize colors and fonts. */

  hci_initialize_read_colors( HCI_display, HCI_colormap );
  hci_initialize_fonts( HCI_display );

  /* Register for any X errors. */

  XSetErrorHandler( HCI_XError_handler );
  XSetIOErrorHandler( HCI_XIOError_handler );

  /* Initialize LE logging. Determine if this HCI task is on channel two.
     If so, have LE log create a second log to keep channel-specific
     logging separate. */

  if( HCI_argv[0] == NULL )
  {
    fprintf( stderr, "HCI_argv[0] is NULL\n" );
    HCI_task_exit( HCI_EXIT_FAIL );
  }

  strcpy (le_label, MISC_basename (HCI_argv[0]));
  strcpy (le_fname, HCI_STRING);
  for( i = 0; i < HCI_argc; i++ )
  {
    if( strcmp( HCI_argv[i], "-A" ) == 0 &&
        i + 1 < HCI_argc &&
        sscanf( HCI_argv[i + 1], "%d", &Channel_number ) == 1 &&
        Channel_number == 2 )
    {
      strcat (le_label, ".2");
      strcat (le_fname, ".2");
      break;
    }
  }

  LE_set_option( "label", le_label );
  LE_set_option( "LE name", le_fname );
  if ( ( ret = ORPGMISC_LE_init( HCI_argc, HCI_argv, HCI_NUM_LE_LOG_MSGS, HCI_LE_LB_TYPE_FLAG, HCI_LE_INSTANCE_FLAG, HCI_LE_SYSTEM_LOG_FLAG ) ) != 0 )
  {
    fprintf( stderr, "ORPGMISC_LE_init failed (%d)\n", ret );
    HCI_task_exit( HCI_EXIT_FAIL );
  }

  if( HCI_task_index < 0 )
  {
    HCI_LE_log( "Launching task" );
  }
  else
  {
    HCI_LE_log( "Launching %s", HCI_task_titles[ HCI_task_index ] );
  }

  /* Register termination handler to dump stack. */

  if( HCI_register_term_hdlr( NULL ) < 0 )
  {
    HCI_LE_error( "Unable to register termination handler" );
  }

  /* Set system info. */

  Set_system_info();

  /* Define the icon to use when minimizing window. */

  bitmap = XCreatePixmapFromBitmapData( HCI_display,
           RootWindowOfScreen( XtScreen( Top_widget ) ),
           ( char * ) hci_icon_bits, hci_icon_width, hci_icon_height,
           1, 0, 1 );

  /* Define busy and selectable cursors. */

  Busy_cursor = XCreateFontCursor( HCI_display, XC_watch );
  Selectable_cursor = XCreateFontCursor( HCI_display, XC_hand2 );

  /* Set top widget attributes.  */

  XtVaSetValues( Top_widget,
                 XmNdeleteResponse, XmDO_NOTHING,
                 XmNiconic, False,
                 XmNiconPixmap, bitmap,
                 XmNallowShellResize, True,
                 XmNmwmFunctions, MWM_FUNC_ALL | MWM_FUNC_MAXIMIZE,
                 NULL );

  XtAddCallback( Top_widget, XmNdestroyCallback, Destroy_callback, NULL );
  XtAddEventHandler( Top_widget, StructureNotifyMask, False, Map_event_callback, NULL );

  /* If window is destroyed other than using "Close" button,
     call destory callback f(x). */

  wm_delete_window = XmInternAtom( HCI_display,
                                   "WM_DELETE_WINDOW", False );
  XmAddWMProtocolCallback( Top_widget, wm_delete_window,
                           Destroy_callback, NULL );

  /* Block RPG event processing until timer event. */
  
  (void) EN_cntl_block();
} 

/*************************************************************************

  HCI initialization for a non-RPG task. No RPG-specific references
  shoud be made.

*************************************************************************/

void HCI_init_non_RPG( int argc, char **argv )
{   
  Atom wm_delete_window;
  ArgList args = (ArgList) NULL;
  Pixmap bitmap;
  int i;

  /* Set flag to indicate this is a partial init. */

  Partial_init_flag = HCI_YES_FLAG;

  /* Save original argc/argv for use outside this function. An original
     copy has to be saved because XtAppInitialize will remove any
     Xt-specific arguments. */

  HCI_argc = argc;
  HCI_argv = (char **) malloc( sizeof( char* ) * HCI_argc );
  if( argv == NULL )
  {
    fprintf( stderr, "HCI_argv malloc failed\n" );
    HCI_task_exit( HCI_EXIT_FAIL );
  }
  for( i = 0; i < HCI_argc; i++ )
  {
    HCI_argv[i] = malloc( sizeof( char ) * strlen( argv[i] ) + 1 );
    strcpy( HCI_argv[i], argv[i] );
  }

  /* Initialize the Xt toolkit and create the top level widget. */

  Top_widget = XtAppInitialize( &App_context, Top_widget_title,
                                NULL, 0, &argc, argv, NULL, args, 0 );
  if( Top_widget == NULL )
  {
    fprintf( stderr, "HCI_init failed to create Top_widget\n" );
    HCI_task_exit( HCI_EXIT_FAIL );
  }

  HCI_display = XtDisplay( Top_widget );
  HCI_screen = XtScreen( Top_widget );
  HCI_colormap = DefaultColormap( HCI_display, DefaultScreen( HCI_display ) );
  HCI_depth = XDefaultDepth( HCI_display, DefaultScreen( HCI_display ) );

  /* Initialize colors and fonts. */

  hci_initialize_read_colors( HCI_display, HCI_colormap );
  hci_initialize_fonts( HCI_display );

  /* Register for any X errors. */

  XSetErrorHandler( HCI_XError_handler );
  XSetIOErrorHandler( HCI_XIOError_handler );

  /* Initialize LE logging. */

  if( HCI_argv[0] == NULL )
  {
    fprintf( stderr, "HCI_argv[0] is NULL\n" );
    HCI_task_exit( HCI_EXIT_FAIL );
  }

  LE_set_option( "LB size", HCI_NUM_LE_LOG_MSGS );
  LE_init( HCI_argc, HCI_argv );

  HCI_LE_log( "Launching task" );

  /* Register termination handler to dump stack. */

  if( HCI_register_term_hdlr( NULL ) < 0 )
  {
    HCI_LE_error( "Unable to register termination handler" );
  }

  /* Define the icon to use when minimizing window. */

  bitmap = XCreatePixmapFromBitmapData( HCI_display,
           RootWindowOfScreen( XtScreen( Top_widget ) ),
           ( char * ) hci_icon_bits, hci_icon_width, hci_icon_height,
           1, 0, 1 );

  /* Define busy and selectable cursors. */

  Busy_cursor = XCreateFontCursor( HCI_display, XC_watch );
  Selectable_cursor = XCreateFontCursor( HCI_display, XC_hand2 );

  /* Set top widget attributes.  */

  XtVaSetValues( Top_widget,
                 XmNdeleteResponse, XmDO_NOTHING,
                 XmNiconic, False,
                 XmNiconPixmap, bitmap,
                 XmNallowShellResize, True,
                 XmNmwmFunctions, MWM_FUNC_ALL | MWM_FUNC_MAXIMIZE,
                 NULL );

  XtAddCallback( Top_widget, XmNdestroyCallback, Destroy_callback, NULL );
  XtAddEventHandler( Top_widget, StructureNotifyMask, False, Map_event_callback, NULL );

  /* If window is destroyed other than using "Close" button,
     call destory callback f(x). */

  wm_delete_window = XmInternAtom( HCI_display,
                                   "WM_DELETE_WINDOW", False );
  XmAddWMProtocolCallback( Top_widget, wm_delete_window,
                           Destroy_callback, NULL );

  /* Block RPG event processing until timer event. */
  
  (void) EN_cntl_block();
} 

/*************************************************************************

  Starts HCI task.

*************************************************************************/

void HCI_start( void (*timer_cb)(), int timer_loop, int resize_flag  )
{
  Dimension width, height;

  /* Hide/reset any progress meters created during initialization. */

  HCI_PM_hide();

  /* Check if window size is allowed to change. */

  if( resize_flag == NO_RESIZE_HCI )
  {
    XtVaGetValues( Top_widget,
                   XmNwidth, &width,
                   XmNheight, &height, NULL );

    XtVaSetValues( Top_widget,
                   XmNminHeight, height,
                   XmNminWidth, width,
                   XmNmaxHeight, height,
                   XmNmaxWidth, width, NULL );
  }

  /* Set interval counters. The timer callback is executed every
     quarter second. We don't want the code executed that often.
     Determine the number of time intervals to skip before executing
     the timer callback code. This goes for the user-defined callback
     as well. Make sure the user-defined value is a multiple of a
     quarter second. */

  Time_interval_count = HCI_ONE_AND_HALF_SECOND / HCI_QUARTER_SECOND;

  if( timer_cb != NULL )
  {
    HCI_user_timer_proc = timer_cb;
    if( timer_loop > 0 )
    {
      if( timer_loop % HCI_QUARTER_SECOND == 0 )
      {
        User_time_interval_count = timer_loop / HCI_QUARTER_SECOND;
      }
      else
      {
        HCI_LE_error( "Timer interval (%d) should be multiple of %d", timer_loop, HCI_QUARTER_SECOND );
        HCI_task_exit( HCI_EXIT_FAIL );
      }
    }
    else
    {
      HCI_LE_error( "Timer interval is negative (%d)", timer_loop );
      HCI_task_exit( HCI_EXIT_FAIL );
    }
  }
  else
  {
    HCI_LE_log( "No user-defined timer process" );
  }

  /* Set up initial timer event for this library. */

  if( Partial_init_flag == HCI_YES_FLAG )
  {
    XtAppAddTimeOut( App_context,
                     HCI_QUARTER_SECOND,
                     ( XtTimerCallbackProc ) Partial_timer_proc,
                     ( XtPointer ) NULL );
  }
  else
  {
    XtAppAddTimeOut( App_context,
                     HCI_QUARTER_SECOND,
                     ( XtTimerCallbackProc ) Timer_proc,
                     ( XtPointer ) NULL );
  }

  /* Notify that task has started and enter X event monitoring loop. */

  Post_task_started_event();
  XtAppMainLoop( App_context );

  /* Exit after leaving X event monitoring loop. */

  HCI_task_exit( HCI_EXIT_SUCCESS );
}

/**************************************************************************

    Description: Posts an event indicating the task has started.

 **************************************************************************/

int Post_task_started_event()
{
  hci_child_started_event_t task_data;

  memset( &task_data, 0, sizeof( hci_child_started_event_t ) );

  task_data.child_id = Object_index;

  EN_post( ORPGEVT_HCI_CHILD_IS_STARTED, &task_data,
            sizeof( hci_child_started_event_t ), 0 );

  return( Object_index );
}

/*************************************************************************

  Finishes HCI task.

*************************************************************************/

void HCI_finish()
{
  HCI_task_exit( HCI_EXIT_SUCCESS );
}

/**************************************************************************

    Terminates this process.

 **************************************************************************/

void HCI_task_exit( int error_code )
{
  ORPGTASK_exit( error_code );
}

/**************************************************************************

    Redirect infrastructure log errors to the log files.

 **************************************************************************/

static void hci_report_infr_err( const char *msg )
{
  if( msg != NULL ){ HCI_LE_log(  msg ); }
}

/**************************************************************************

    Read the command line options. Returns 1 on success or a negative 
    error code. The process is terminated on "-h" option.

 **************************************************************************/

static int Read_command_line_options()
{
  int input, i;
  char *endstr;
  char options[HCI_BUF_256], opt_str[8];
  char le_label[HCI_BUF_256], le_fname[HCI_BUF_256];
  int channel_num;
  int retval = 1;

  /* Initialize LE logging. Determine if this HCI task is on channel two.
     If so, have LE log create a second log to keep channel-specific
     logging separate. */

  strcpy (le_label, MISC_basename (HCI_argv[0]));
  strcpy (le_fname, HCI_STRING);
  for( i = 0; i < HCI_argc; i++ )
  {
    if( strcmp( HCI_argv[i], "-A" ) == 0 &&
        i + 1 < HCI_argc &&
        sscanf( HCI_argv[i + 1], "%d", &channel_num ) == 1 &&
        channel_num == 2 )
    {
      strcat (le_label, ".2");
      strcat (le_fname, ".2");
      break;
    }
  }

  LE_set_option( "label", le_label );
  LE_set_option( "LE name", le_fname );
  if ( ( retval = ORPGMISC_LE_init( HCI_argc, HCI_argv, HCI_NUM_LE_LOG_MSGS, HCI_LE_LB_TYPE_FLAG, HCI_LE_INSTANCE_FLAG, HCI_LE_SYSTEM_LOG_FLAG ) ) != 0 )
  {
    fprintf( stderr, "ORPGMISC_LE_init failed (%d)\n", retval );
    HCI_task_exit( HCI_EXIT_FAIL );
  }

  HCI_LE_log( "Launching %s", HCI_task_titles[ HCI_task_index ] );

  /* Parse command line arguments with the getopts function. The string
     to be used to restart the HCI task also needs to be created. Since
     getopts only handles single character flags (i.e. -A, -I, etc.),
     remove the "-name" flag before processing and use it in the restart
     string. The rest of the restart string will be created once getopts
     verifies the validity of the other command line arguments. */

  strcpy( options, "lhA:O:s:N:RS" );
  if( HCI_custom_args != NULL ){ strcat( options, HCI_custom_args ); }

  Restart_cmd = STR_copy( Restart_cmd, HCI_argv[0] );
  Restart_cmd = STR_cat( Restart_cmd, " -R " );

  for( i = 0; i < HCI_argc; i++ )
  {
    if( strcmp( HCI_argv[i], "-name" ) == 0 )
    {
      /* Make sure -name is followed by something valid. */
      if( (i+1) < HCI_argc && strlen( HCI_argv[i+1] ) > 0 )
      {
        /* Copy to restart string. */
        Restart_cmd = STR_cat( Restart_cmd, "-name \"" );
        Restart_cmd = STR_cat( Restart_cmd, HCI_argv[i+1] );
        Restart_cmd = STR_cat( Restart_cmd, "\" " );
        /* Remove -name and its value from command line arguments. */
        HCI_argc = HCI_argc - 2;
        for( ; i < HCI_argc; i++ )
        {
          HCI_argv[i] = realloc( HCI_argv[i], strlen( HCI_argv[i+2] ) + 1 );
          strcpy( HCI_argv[i], HCI_argv[i+2] );
        }
        break;
      }
      else
      {
        HCI_LE_error( "Option -name needs argument" );
        HCI_task_exit(HCI_EXIT_FAIL);
      }
    }
  }

  /* Parse rest of command line. */

  while( ( input = getopt( HCI_argc, HCI_argv, options ) ) != -1 )
  { 
    sprintf( opt_str, "-%c ", (char) input );
    switch( input )
    {
      case 'N':
        Started_by_hci = 1;
        Chan_num_for_syscfg = strtol( optarg, &endstr, 10 );
        if( endstr == optarg ){ Chan_num_for_syscfg = 0; }
        break;

      case 'R':
        Restarted_by_hci = 1;
        break;

      case 'A': 
        Channel_number = strtol( optarg, &endstr, 10 );
        if( endstr == optarg ){ Channel_number = 0; }
        Restart_cmd = STR_cat( Restart_cmd, opt_str );
        Restart_cmd = STR_cat( Restart_cmd, optarg );
        Restart_cmd = STR_cat( Restart_cmd, " " );
        break;
    
      case 'O':
        Object_index = strtol( optarg, &endstr, 10 ); 
        if( endstr == optarg ){ Object_index = -1; }
        Restart_cmd = STR_cat( Restart_cmd, opt_str );
        Restart_cmd = STR_cat( Restart_cmd, optarg );
        Restart_cmd = STR_cat( Restart_cmd, " " );
        break;

      case 'l':
        Low_bandwidth_flag = 1;
        Restart_cmd = STR_cat( Restart_cmd, opt_str );
        Restart_cmd = STR_cat( Restart_cmd, " " );
        break;

      case 'S':
        Satellite_connection_flag = 1;
        Restart_cmd = STR_cat( Restart_cmd, opt_str );
        Restart_cmd = STR_cat( Restart_cmd, " " );
        break;

      case 's':
        Simulation_speed = strtol( optarg, &endstr, 10 );
        if( endstr == optarg ){ Simulation_speed = -1; }
        if( Simulation_speed > 0 )
        {
          Restart_cmd = STR_cat( Restart_cmd, opt_str );
          Restart_cmd = STR_cat( Restart_cmd, optarg );
          Restart_cmd = STR_cat( Restart_cmd, " " );
        }
        break;

      default:
        if( HCI_custom_args != NULL && HCI_custom_args_callback != NULL &&
            strchr( HCI_custom_args, input ) != NULL )
        {
          retval = HCI_custom_args_callback( input, optarg );
        }
        else if( input != 'h' ){ retval = -1; }
        else
        {
          printf( "common hci options:\n" );
          printf( "-h (this help message)\n" );
          printf( "-l (low bandwidth mode)\n" );
          printf( "-name screen_title (Specifies screen title)\n" );
          printf( "-s baud_rate (Sets simulated baud rate)\n" );
          printf( "-S (Run on satellite connected MSCF)\n" );
          printf( "-A channel_number (redundant channel number;\n" );
          printf( "   0 for local channel; The default is 0)\n" );
          printf( "-O instance_number (hci internal use)\n" );
          printf( "-R (Restart by hci, hci internal use)\n" );
          printf( "-N chan_number (started by hci, hci internal use)\n" );
          printf( "    -s is ignored without -l;\n" );
          printf( "    -l and -s are ignored on RPG active node;\n" );
          if( HCI_custom_args != NULL && HCI_custom_args_help_callback != NULL )
          {
            HCI_custom_args_help_callback();
          }
          HCI_task_exit(HCI_EXIT_SUCCESS);
        }
        break;
    } /* End switch. */
  } /* End while loop. */


  /* Make sure network connection to RPG is available. */

  if( !Started_by_hci && Restarted_by_hci && Channel_number > 0 )
  {
    Wait_for_connected();
  }

  /* Make sure MRPG has finished startup and is ready. */

  if( Restarted_by_hci ){ Wait_for_mrpg(); }

  /* Set up system config file. */

  Set_up_system_config();

  /* Set low bandwidth variables, if applicable. */

  if( Low_bandwidth_flag )
  {
    Transaction_timeout = 60;
    HCI_LE_log( "HCI: Low bandwidth mode" );
  }
  else
  {
    Transaction_timeout = 15;
    Simulation_speed = -1;
  }

  if( Low_bandwidth_flag )
  {
     RMT_set_compression( RMT_COMPRESSION_ON );
     HCI_LE_log( "Transaction time out time = %d", Transaction_timeout );
     HCI_LE_log( "Max restart wait time = %d", NETWORK_RECONNECT_WAIT );
  }

  /* Set time RMT call waits for completion before failing. */

  RMT_time_out( Transaction_timeout );
    
  /* Restart command should go to background. */

  Restart_cmd = STR_cat( Restart_cmd, "&" );

  /*  Make infrastructure error messages go to the hci log file  */

  MISC_log_reg_callback( (void (*)())hci_report_infr_err );

  /* Initialize the log-error messaging services. */

  if( ORPGMISC_init( HCI_argc, HCI_argv, 1000, 0, -1, 0 ) < 0 )
  {
    HCI_LE_error( "ORPGMISC_init failed" );
    HCI_task_exit (HCI_EXIT_FAIL);
  }

  return retval;
}

/*************************************************************************

    Sets up the system configuration and the run-time environment 
    accordingly.

**************************************************************************/

static void Set_up_system_config()
{
  char buf[HCI_BUF_256];
  int chan = 0, ret = 0;

  if( !Started_by_hci && Channel_number == 0 )
  {
    /* This HCI task was started by hand on the local machine and the
       user did not specify a channel to logically connect to. Assume
       a fast connection, and since no channel was specified, the channel 
       of the local machine will be used to set up the system config file. */
    Low_bandwidth_flag = 0;
    Simulation_speed = -1;
    ret = EN_register( ORPGEVT_DATA_STORE_CREATED, Syscfg_change_cb );
    if( ret < 0 )
    {
      HCI_LE_error( "EN_register ORPGEVT_DATA_STORE_CREATED failed (%d)", ret );
      HCI_task_exit(HCI_EXIT_FAIL);
    }
    return;
  }
  else if( Started_by_hci )
  {
    /* This HCI task was started by a different HCI task. The channel to
       logically connect to was specified on the command line. Use that
       channel when setting up the system config file. */
    if( Satellite_connection_flag )
    {
      chan = Chan_num_for_syscfg | ORPGMGR_SAT_CONN;
    }
    else
    {
      chan = Chan_num_for_syscfg;
    }
    ret = ORPGMGR_setup_sys_cfg( chan, 1, Syscfg_change_cb );
  }
  else
  {
    /* The HCI task was started by hand on the local machine and the
       user specified a channel to logically connect to. Use that
       channel when setting up the system config file. */ 
    if( Channel_number < 1 || Channel_number > 2 )
    {
      HCI_LE_error( "Bad channel number (-A) %d", Channel_number );
      HCI_task_exit(HCI_EXIT_FAIL);
    }
    else if( Satellite_connection_flag )
    {
      chan = Channel_number | ORPGMGR_SAT_CONN;
    }
    else
    {
      chan = Channel_number;
    }
    ret = ORPGMGR_setup_sys_cfg( chan, 0, Syscfg_change_cb );
    if (ret < 0 && Check_install != NULL)
	Check_install ();
  }

  /* If ORPGMGR_setup_sys_cfg failed, we must exit. */

  if( ret < 0 )
  {
    sprintf( buf, "RPG system configuration for channel %d not found - \nhci cannot start", chan^ORPGMGR_SAT_CONN );
    Display_dialog( "System Configuration Error", buf, SHUTDOWN_WAIT_TIME, 0, NULL ); 
    HCI_task_exit(HCI_EXIT_FAIL);
  }
  
  /* ORPGMGR_setup_sys_cfg was successful. Make sure there is a connection
     to the MRPG host machine. */

  Chan_num_for_syscfg = ret;
  ret = ORPGMGR_get_mrpg_host_name( buf, HCI_BUF_256 );
  if( ret < 0 )
  { 
    HCI_LE_error( "ORPGMGR_get_mrpg_host_name failed (%d)", ret );
    HCI_task_exit(HCI_EXIT_FAIL);
  } 
  if( ret == 0 )
  {
    /* This machine is not an active RPG node */
    Orpg_machine_name = MISC_malloc( strlen(buf) + 1 );
    strcpy( Orpg_machine_name, buf );
    Rpga_ip = NET_get_ip_by_name( Orpg_machine_name );
    if( Rpga_ip == INADDR_NONE )
    {
      HCI_LE_error( "NET_get_ip_by_name returned INADDR_NONE" );
      HCI_task_exit(HCI_EXIT_FAIL);
    }
  }
  else
  {
    Low_bandwidth_flag = 0; /* disable -s and -l options */
    Simulation_speed = -1;
  }

  /* Register for ORPGEVT_DATA_STORE_CREATED event. If received, the
     HCI task should be restarted to re-synchronize with changes to
     the system configuration. */
  ret = EN_register( ORPGEVT_DATA_STORE_CREATED, Syscfg_change_cb );
  if( ret < 0 )
  {
    HCI_LE_error( "EN_register ORPGEVT_DATA_STORE_CREATED failed (%d)", ret );
    HCI_task_exit(HCI_EXIT_FAIL);
  }
}

/*************************************************************************

    The system configuration update event callback function.

**************************************************************************/

static void Syscfg_change_cb( EN_id_t event, char *msg, int msg_len, void *arg )
{
  HCI_LE_log( "Received ORPGEVT_DATA_STORE_CREATED event" );
  Sys_cfg_changed = 1;
}

/***********************************************************************

    This is the local timer callback function.

***********************************************************************/

static XtTimerCallbackProc Timer_proc( Widget w, XtIntervalId id )
{
  static int cnt_num = 0;
  static int user_cnt_num = 0;
  static double previous_RDA_Status_time = 0;
  double current_RDA_Status_time;

  /*  Unblock EN events so any ORPG events can be processed.  */

  (void) EN_cntl_unblock();
  (void) EN_cntl_block();

  if( cnt_num < Time_interval_count )
  {
    cnt_num++;
  }
  else
  {
    cnt_num = 0;

    if( Site_info_system == HCI_FAA_SYSTEM )
    {
      current_RDA_Status_time = ORPGRDA_last_status_update();
      if( Redundant_status_updated_flag == HCI_YES_FLAG ||
          current_RDA_Status_time != previous_RDA_Status_time )
      {
        Redundant_status_updated_flag = HCI_NO_FLAG;
        previous_RDA_Status_time = current_RDA_Status_time;
        Set_top_widget_title();
        Update_shell_titles();
      }
    }
    else if( Site_info_system == HCI_NWSR_SYSTEM )
    {
      current_RDA_Status_time = ORPGRDA_last_status_update();
      if( current_RDA_Status_time != previous_RDA_Status_time )
      {
        previous_RDA_Status_time = current_RDA_Status_time;
        Set_top_widget_title();
        Update_shell_titles();
      }
    }

    if( !Rpg_connected )
    {
      if( Orpg_machine_name[0] != '\0' )
      {
        Shutdown_this_app( "Connection to RPG lost", NETWORK_RECONNECT_WAIT );
      }
    }

    if( Sys_cfg_changed )
    {
      Shutdown_this_app( "System config changed", SHUTDOWN_WAIT_TIME );
    }

    Detect_connectivity();
  }

  if( HCI_user_timer_proc != NULL )
  {
    if( user_cnt_num < User_time_interval_count )
    {
      user_cnt_num++;
    }
    else
    {
      user_cnt_num = 0;

      /* Call user-defined timer process. */

      HCI_user_timer_proc();

      /* Hide/reset any progress meters created during the interval. */

      HCI_PM_hide();
    }
  }

  XtAppAddTimeOut( App_context,
                   HCI_QUARTER_SECOND,
                   ( XtTimerCallbackProc ) Timer_proc,
                   ( XtPointer ) NULL );

  return ( ( XtTimerCallbackProc ) 0 );
}

/***********************************************************************

    This is the local timer callback function for partial initialization.

***********************************************************************/

static XtTimerCallbackProc Partial_timer_proc( Widget w, XtIntervalId id )
{
  static int user_cnt_num = 0;

  /* Unblock EN events so any ORPG events can be processed. */

  (void) EN_cntl_unblock();
  (void) EN_cntl_block();

  if( HCI_user_timer_proc != NULL )
  {
    if( user_cnt_num < User_time_interval_count )
    {
      user_cnt_num++;
    }
    else
    {
      user_cnt_num = 0;

      /* Call user-defined timer process. */

      HCI_user_timer_proc();

      /* Hide/reset any progress meters created during the interval. */

      HCI_PM_hide();
    }
  }

  XtAppAddTimeOut( App_context,
                   HCI_QUARTER_SECOND,
                   ( XtTimerCallbackProc ) Partial_timer_proc,
                   ( XtPointer ) NULL );

  return ( ( XtTimerCallbackProc ) 0 );
}

/************************************************************************

    Shuts down this application and, if restart flag is set, starts
    a new instance of this application.

 ***********************************************************************/

static void Shutdown_this_app( char *reason, int wait_time )
{
  char buf[HCI_BUF_256];

  if( Restart_flag == RESTART_HCI_TASK )
  {
    HCI_LE_log( "Start a new instance of this application" );
    HCI_LE_log( " with command: %s", Restart_cmd );
    sprintf( buf,
            "%s -\n\nThis application must terminate and restart. If\nthe application has not restarted by the end of\nthe countdown, a major error has occurred. Call\nthe WSR-88D Hotline for help.\n", reason );
    Display_dialog( "HCI Restart", buf, wait_time, 0, Detect_connectivity );
    system( Restart_cmd );
    HCI_task_exit( HCI_EXIT_SUCCESS );
  }
  else
  {
    HCI_LE_log( "Terminate this application" );
    sprintf( buf, "%s - this application must be shut down", reason );
    Display_dialog( "HCI Shutdown", buf, SHUTDOWN_WAIT_TIME, 0, NULL );
    HCI_task_exit( HCI_EXIT_SUCCESS );
  }
}

/************************************************************************

    Checks the connection to the RPG host and sets Rpg_connected. This is
    called by a timer.

************************************************************************/

static int Detect_connectivity()
{
  static time_t previous_time = 0, reconnect_time = 0;
  static int cnt = 0, start_lag_timer = 0;
  static int prev_num_disconnects = 0;
  int num_disconnects = 0;
  int ret = 0;
  time_t current_time;
  int ip_stat = 0;
  int connect_flag = 0;

  /* We work at most every second, else return previous value. */
  cnt++;
  if( cnt * DISPLAY_TIMER <= 1000 ){ return Rpg_connected; }

  /* We check every CONNECT_CHECK_TIME seconds, else return previous value. */
  cnt = 0;
  current_time = MISC_systime( NULL );
  if( current_time < previous_time + CONNECT_CHECK_TIME )
  {
    return Rpg_connected;
  }
  previous_time = current_time;
    
  /* If Orpg_machine_name isn't defined, return previous value. */
  if( strlen( Orpg_machine_name ) == 0 ){ return Rpg_connected; }

  /* Actual check for connectivity. */
  EN_cntl_unblock();
  ret = ORPGMGR_check_connectivity( 1, &Rpga_ip, &ip_stat );
  EN_cntl_block();
  if( ret == 1 )
  { 
    /* If LSB of ip_stat is 1, good connection. Make sure the
       disconnect counter is the same as before. If it isn't,
       then there has been a disconnect/reconnect since the last
       function call. Assume disconnected, since file descriptors,
       sockets, etc may be in an invalid state. */
    num_disconnects = ip_stat >> 1;
    if( ( ip_stat & 1 ) && ( num_disconnects == prev_num_disconnects ) )
    {
      connect_flag = 1;
    }
    else
    {
      prev_num_disconnects = num_disconnects;
    }
  }

  if ( !connect_flag )
  {
    /* Not connected. */
    Rpg_connected = 0;
    start_lag_timer = 0;
    HCI_LE_log( "Connection to mrpg host lost." );
  }
  else
  {
    /* Connected, but don't change connection flag until
       consistent connection for RECONNECT_LAG seconds. */
    if( !Rpg_connected && !start_lag_timer )
    {
      reconnect_time = current_time;
      start_lag_timer = 1;
      HCI_LE_log( "Connection to mrpg host detected, lag timer started" );
    }
  
    /* If connection is newly detected and lag timer is not expired... */
    if( !Rpg_connected && start_lag_timer )
    {
      /* Lag timer expired, assume connection is OK and stable. */
      if( current_time >= ( reconnect_time + RECONNECT_LAG ) )
      {
        HCI_LE_log( "Connection to mrpg host is OK" );
        Rpg_connected = 1;
        start_lag_timer = 0;
      }
    }
  }

  return Rpg_connected;
}

/************************************************************************

    Pops up a dialog window and displays "text" in the window for 
    "wait_seconds" seconds. Then pops down the dialog and return. It 
    returns 1 if the use cancels the waiting, or 0 otherwise. "title" is
    the window title. If "can_cancel" is true, the cancel button is 
    enabled.

************************************************************************/

static int Display_dialog( char *title, char *text, int wait_seconds,
                           int can_cancel, int (*check_cb)() )
{
  XmString str;
  Widget wait_dialog;
  int wait_cancelled = 0, status, i;
  time_t start_time, current_time;
  char buf[HCI_BUF_512];

  /* Set dialog text. */
  strcpy( buf, text );
  sprintf( buf + strlen( buf ), "\n%d seconds remaining", wait_seconds );

  /* Create popup. */
  wait_dialog = hci_custom_popup( Top_widget, title, buf, NULL, NULL, "Cancel",
                     Cancel_cb, NULL, NULL, WARNING_COLOR, TEXT_FOREGROUND );

  /* Countdown loop. */
  status = BREAK_BY_TIME;
  start_time = MISC_systime( NULL );
  while(1)
  {
    /* Process up to 50 pending X events.XXX */
    for( i = 0; i < 50; i++ )
    {
      if( ( XtAppPending( App_context ) & XtIMAll ) == 0 ){ break; }
      XtAppProcessEvent( App_context, XtIMAll );
    }

    /* Break out of loop if user clicked Cancel button. */
    if( wait_cancelled )
    {
      status = BREAK_BY_CANCEL;
      break;
    }

    /* Wait a bit. */
    msleep( DISPLAY_TIMER );
    current_time = MISC_systime( NULL );

    /* Check callback if one is defined. */
    if( check_cb != NULL && check_cb() != 0 )
    {
      status = BREAK_BY_CHECK;
      break;
    }

    /* Countdown timer expired. */
    if( current_time >= start_time + wait_seconds )
    {
      HCI_LE_log( "Countdown timer exceeded" );
      break;
    }

    /* No reason to stop loop, so keep going. */
    sprintf( buf, "%s\n%d seconds remaining",
             text, wait_seconds - (int)(current_time - start_time) );
    str = XmStringCreateLocalized( buf );
    XtVaSetValues( wait_dialog, XmNmessageString, str, NULL );
    XmStringFree( str );
  }

  XtPopdown( XtParent( wait_dialog ) );
  return status;

}

/**********************************************************************

    The cancel button callback function.

**********************************************************************/

static void Cancel_cb( Widget w, XtPointer client_data, XtPointer call_data )
{
  *((int*)client_data) = 1;
}

/**************************************************************************

    Waits for connection to RPG is established.

 **************************************************************************/

static void Wait_for_connected()
{
  if (Check_for_connected() == 0 )
  {
    char buf[HCI_BUF_256];
    int ret;
    HCI_LE_log( "Waiting for network connection..." );
    sprintf( buf, "Node \"RPG\" is not reachable. Waiting for\na valid network connection. HCI will exit\nafter 3 minutes." );
    ret = Display_dialog( "Waiting for network connectivity",
                          buf, NETWORK_CONNECT_WAIT, 1, Check_for_connected );
    if( ret != BREAK_BY_CHECK )
    {
      if( ret == BREAK_BY_TIME )
                HCI_LE_log( "RPG connection check timed out" );
      if( ret == BREAK_BY_CANCEL )
                HCI_LE_log( "RPG connection waiting cancelled" );
      HCI_task_exit(HCI_EXIT_FAIL);
    }
  }
    
  return;
}   
    
/**************************************************************************
  
    Checks if connection to RPG is established.
  
 **************************************************************************/

static int Check_for_connected()
{
  static time_t last_tm = 0;
  time_t tm;

  tm = MISC_systime( NULL );
  if( tm >= last_tm + 10 )
  {
    int ret = 0;
    int chan = 0;
    last_tm = tm;
    if( Satellite_connection_flag )
    {
      chan = Channel_number | ORPGMGR_SAT_CONN;
    }
    else
    {
      chan = Channel_number;
    }
    ret = ORPGMGR_setup_sys_cfg( chan, 0, NULL );
    if( ret >= 0 ){ return 1; }
    if (ret == ORPGMGR_FATAL_ERROR)
    {
      char buf[256];
      sprintf( buf, "RPG system configuration can not be installed - \nhci cannot start");
      Display_dialog( "System Configuration Error", buf, SHUTDOWN_WAIT_TIME, 0, NULL ); 
      HCI_task_exit(HCI_EXIT_FAIL);
    }
  }

  return 0;
}

/**************************************************************************

    Waits to synchronize with MRPG.

 **************************************************************************/
  
static void Wait_for_mrpg()
{
  if( Check_for_mrpg() == 0 )
  {
    char buf[HCI_BUF_256];
    int ret;
    HCI_LE_log( "Waiting to synchronize with MRPG..." );
    sprintf( buf, "Waiting for internal data to synchronize. HCI will\nexit if not synchronized by end of countdown." );
    ret = Display_dialog( "Waiting to synchronize data",
                          buf, MRPG_STARTUP_WAIT, 1, Check_for_mrpg );
    if( ret != BREAK_BY_CHECK )
    {
      if( ret == BREAK_BY_TIME )
        HCI_LE_log( "Wait_for_mrpg check timed out" );
      if( ret == BREAK_BY_CANCEL )
        HCI_LE_log( "Wait_for_mrpg cancelled" );
      HCI_task_exit(HCI_EXIT_FAIL);
    }
  }

  return;
}

/**************************************************************************

    Checks if data is synchronized with MRPG.

 **************************************************************************/

static int Check_for_mrpg()
{
  static int mrpg_trans_flag = 0; /* Has MRPG previously entered TRANSITION? */
  static int cnt = 0;
  static int mrpg_finished = 0; /* Has MRPG finished startup? */
  static time_t non_trans_detect_time = 0;
  Mrpg_state_t mrpg_info;
  time_t current_time;
  int lb_read_size = -1;

  /* We work at most every second, else return previous value. */

  cnt++;
  if( cnt * DISPLAY_TIMER <= 1000 ){ return mrpg_finished; }
  cnt = 0;

  current_time = MISC_systime( NULL );

  /* Read Mrpg_state_t to retrieve state info. */
  if( ( lb_read_size = ORPGDA_read( ORPGDAT_TASK_STATUS, (char *) &mrpg_info, sizeof( Mrpg_state_t ), MRPG_RPG_STATE_MSGID ) ) == sizeof( Mrpg_state_t ) ) 
  {
    /* If state is initially TRANSITION, then any change in state will
       be seen as synchronization. If the state is initially not
       TRANSITION, then wait MRPG_SYNC_LAG seconds to see if state goes
       to TRANSITION. If it does not, then assume synchronization. If
       it does, then wait for change out of TRANSITION. */
    if( mrpg_info.state == MRPG_ST_TRANSITION && mrpg_trans_flag == 0 )
    {
      mrpg_trans_flag = 1;
    }
    else if( mrpg_info.state != MRPG_ST_TRANSITION && mrpg_trans_flag == 0 )
    {
      if( non_trans_detect_time == 0 )
      {
	non_trans_detect_time = current_time;
      }
      else if( ( current_time - non_trans_detect_time ) > MRPG_SYNC_LAG )
      {
	non_trans_detect_time = 0;
	mrpg_finished = 1; 
      }
    }
    else if( mrpg_info.state != MRPG_ST_TRANSITION && mrpg_trans_flag == 1 )
    {
      mrpg_trans_flag = 0;
      mrpg_finished = 1;
    }
  }
  else
  {
    HCI_LE_log( "LB_read(RPG State) unexpected size of %d", lb_read_size );
  }

  return mrpg_finished;
} 

/**********************************************************************

    Destroy callback.

**********************************************************************/

static void Destroy_callback( Widget w, XtPointer cl, XtPointer ca )
{
  /* If user defines a destroy callback and it returns non-zero,
     do not exit. This is useful in ensuring a task doesn't exit
     before it needs to. */
  if( HCI_user_destroy_callback != NULL && HCI_user_destroy_callback() )
  {
    return;
  }
  HCI_task_exit( HCI_EXIT_SUCCESS );
}

/**********************************************************************

    Initialize various system information flags.

**********************************************************************/

static void Set_system_info()
{
  char *node_buf;
  char *system_buf;
  char *channel_buf;

  /* Determine node this is running on. */

  if( ( node_buf = ORPGMISC_get_site_name( "type" ) ) == NULL )
  {
    HCI_LE_error( "ORPGMISC_get_site_name(\"type\") is NULL" );
    HCI_LE_error( "Site_info_node is UNKNOWN" );
  }
  else if( strcmp( node_buf, "rpga" ) == 0 )
  {
    Site_info_node = HCI_RPGA_NODE;
  }
  else if( strcmp( node_buf, "rpgb" ) == 0 )
  {
    Site_info_node = HCI_RPGB_NODE;
  }
  else if( strcmp( node_buf, "mscf" ) == 0 )
  {
    Site_info_node = HCI_MSCF_NODE;
  }
  else
  {
    HCI_LE_error( "ORPGMISC_get_site_name(\"type\") is invalid: %s", node_buf );
    HCI_task_exit( HCI_EXIT_FAIL );
  }

  /* Determine configuration this is running on. */

  if( ( system_buf = ORPGMISC_get_site_name( "system" ) ) == NULL )
  {
    HCI_LE_error( "ORPGMISC_get_site_name(\"system\") is NULL" );
    HCI_LE_error( "Site_info_system is UNKNOWN" );
  }
  else if( strcmp( system_buf, "NWS" ) == 0 )
  {
    Site_info_system = HCI_NWS_SYSTEM;
  }
  else if( strcmp( system_buf, "NWSR" ) == 0 )
  {
    Site_info_system = HCI_NWSR_SYSTEM;
  }
  else if( strcmp( system_buf, "FAA" ) == 0 )
  {
    Site_info_system = HCI_FAA_SYSTEM;
  }
  else if( strcmp( system_buf, "DODAN" ) == 0 )
  {
    Site_info_system = HCI_DODAN_SYSTEM;
  }
  else if( strcmp( system_buf, "DODFR" ) == 0 )
  {
    Site_info_system = HCI_DODFR_SYSTEM;
  }
  else
  {
    HCI_LE_error( "ORPGMISC_get_site_name(\"system\") is invalid: %s", system_buf );
    HCI_task_exit( HCI_EXIT_FAIL );
  }

  /* Determine channel this is running on. */

  if( Site_info_node == HCI_MSCF_NODE )
  {
    Site_info_channel_number = Channel_number;
  }
  else if( Channel_number == 0 )
  {
    if( ( channel_buf = ORPGMISC_get_site_name( "channel_num" ) ) == NULL )
    {
      HCI_LE_error( "ORPGMISC_get_site_name(\"channel_num\") is NULL" );
      HCI_task_exit(  HCI_EXIT_FAIL );
    }
    else if( strcmp( channel_buf, "1" ) == 0 )
    {
      Site_info_channel_number = 1;
    }
    else if( strcmp( channel_buf, "2" ) == 0 )
    {
      Site_info_channel_number = 2;
    }
    else
    {
      HCI_LE_error( "ORPGMISC_get_site_name(\"channel_num\") is invalid" );
      HCI_task_exit(  HCI_EXIT_FAIL );
    }
  }
  else
  {
    Site_info_channel_number = Channel_number;
  } 
} 

/**********************************************************************

    Initialize various site information flags.

**********************************************************************/

static void Set_site_info()
{
  double d;
  char *c;

  /* Default Mode A VCP */
  if( DEAU_get_values( "site_info.def_mode_A_vcp", &d, 1 ) < 0 )
  {
    HCI_LE_error( "Unable to read site_info.def_mode_A_vcp");
    HCI_task_exit( HCI_EXIT_FAIL );
  }
  else
  {
    HCI_site_def_mode_A_vcp = (int) d;
  }
  /* Default Mode B VCP */
  if( DEAU_get_values( "site_info.def_mode_B_vcp", &d, 1 ) < 0 )
  {
    HCI_LE_error( "Unable to read site_info.def_mode_B_vcp");
    HCI_task_exit( HCI_EXIT_FAIL );
  }
  else
  {
    HCI_site_def_mode_B_vcp = (int) d;
  }
  /* Has BDDS flag */
  if( DEAU_get_values( "site_info.has_bdds", &d, 1 ) < 0 )
  {
    HCI_LE_error( "Unable to read site_info.has_bdds");
    HCI_task_exit( HCI_EXIT_FAIL );
  }
  else
  {
    HCI_site_bdds_flag = (int) d;
  }
  /* Has MLOS flag */
  if( DEAU_get_values( "site_info.has_mlos", &d, 1 ) < 0 )
  {
    HCI_LE_error( "Unable to read site_info.has_mlos");
    HCI_task_exit( HCI_EXIT_FAIL );
  }
  else
  {
    HCI_site_mlos_flag = (int) d;
  }
  /* Has RMS flag */
  if( DEAU_get_values( "site_info.has_rms", &d, 1 ) < 0 )
  {
    HCI_LE_error( "Unable to read site_info.has_rms");
    HCI_task_exit( HCI_EXIT_FAIL );
  }
  else
  {
    HCI_site_rms_flag = (int) d;
  }
  /* ORDA flag */
  if( DEAU_get_values( "site_info.is_orda", &d, 1 ) < 0 )
  {
    HCI_LE_error( "Unable to read site_info.is_orda");
    HCI_task_exit( HCI_EXIT_FAIL );
  }
  else
  {
    HCI_site_orda_flag = (int) d;
  }
  /* RDA Elevation */
  if( DEAU_get_values( "site_info.rda_elev", &d, 1 ) < 0 )
  {
    HCI_LE_error( "Unable to read site_info.rda_elev");
    HCI_task_exit( HCI_EXIT_FAIL );
  }
  else
  {
    HCI_site_rda_elev = (int) d;
  }
  /* RDA Latitude */
  if( DEAU_get_values( "site_info.rda_lat", &d, 1 ) < 0 )
  {
    HCI_LE_error( "Unable to read site_info.rda_lat");
    HCI_task_exit( HCI_EXIT_FAIL );
  }
  else
  {
    HCI_site_rda_lat = (int) d;
  }
  /* RDA Longitude */
  if( DEAU_get_values( "site_info.rda_lon", &d, 1 ) < 0 )
  {
    HCI_LE_error( "Unable to read site_info.rda_lon");
    HCI_task_exit( HCI_EXIT_FAIL );
  }
  else
  {
    HCI_site_rda_lon = (int) d;
  }
  /* RPG ID */
  if( DEAU_get_values( "site_info.rpg_id", &d, 1 ) < 0 )
  {
    HCI_LE_error( "Unable to read site_info.rpg_id");
    HCI_task_exit( HCI_EXIT_FAIL );
  }
  else
  {
    HCI_site_rpg_id = (int) d;
  }
  /* RPG Name */
  if( ( DEAU_get_string_values( "site_info.rpg_name", &c ) < 0 ) ||
      ( c == NULL ) )
  {
    HCI_LE_error( "Unable to read site_info.rpg_name");
    HCI_task_exit( HCI_EXIT_FAIL );
  }
  else
  {
    strcpy( HCI_site_rpg_name, c );
  }
  /* Weather Mode */
  if( DEAU_get_values( "site_info.wx_mode", &d, 1 ) < 0 )
  {
    HCI_LE_error( "Unable to read site_info.wx_mode");
    HCI_task_exit( HCI_EXIT_FAIL );
  }
  else
  {
    HCI_site_def_wx_mode = (int) d;
  }
}

int HCI_default_mode_A_vcp()
{
  return HCI_site_def_mode_A_vcp;
}
int HCI_default_mode_B_vcp()
{
  return HCI_site_def_mode_B_vcp;
}
int HCI_default_wx_mode()
{
  return HCI_site_def_wx_mode;
}
int HCI_has_bdds()
{
  return HCI_site_bdds_flag;
}
int HCI_has_mlos()
{
  return HCI_site_mlos_flag;
}
int HCI_has_rms()
{
  return HCI_site_rms_flag;
}
int HCI_bkg_product_code()
{
  return HCI_site_rms_flag;
}
int HCI_is_orda()
{
  return HCI_site_orda_flag;
}
int HCI_rda_elevation()
{
  return HCI_site_rda_elev;
}
int HCI_rda_latitude()
{
  return HCI_site_rda_lat;
}
int HCI_rda_longitude()
{
  return HCI_site_rda_lon;
}
int HCI_rpg_id()
{
  return HCI_site_rpg_id;
}
int HCI_agency_password( char **pwd_buf )
{
  static char *agency_pwd = NULL;
  int ret;

  ret = DEAU_get_string_values( HCI_AGENCY_PWD_ID, &agency_pwd );
  *pwd_buf = agency_pwd;
  return ret;
}
int HCI_roc_password( char **pwd_buf )
{
  static char *roc_pwd = NULL;
  int ret;

  ret = DEAU_get_string_values( HCI_ROC_PWD_ID, &roc_pwd );
  *pwd_buf = roc_pwd;
  return ret;
}
int HCI_urc_password( char **pwd_buf )
{
  static char *urc_pwd = NULL;
  int ret;

  ret = DEAU_get_string_values( HCI_URC_PWD_ID, &urc_pwd );
  *pwd_buf = urc_pwd;
  return ret;
}
int HCI_set_agency_password( char *passwd )
{
  return DEAU_set_values( HCI_AGENCY_PWD_ID, 1, passwd, 1, 0 );
}
int HCI_set_roc_password( char *passwd )
{
  return DEAU_set_values( HCI_ROC_PWD_ID, 1, passwd, 1, 0 );
}
int HCI_set_urc_password( char *passwd )
{
  return DEAU_set_values( HCI_URC_PWD_ID, 1, passwd, 1, 0 );
}
int HCI_rpg_name( char *icao_buf )
{
  strncpy( icao_buf, HCI_site_rpg_name, HCI_ICAO_LEN );
  return 0;
}
  
/**********************************************************************
    
    Set title of Top_widget.
  
**********************************************************************/
    
static void Set_top_widget_title()
{
  char buf1[16];
  char buf2[16];
  int red_control_status;

  if( Partial_init_flag == HCI_YES_FLAG ){ return; }

  if( Site_info_system == HCI_FAA_SYSTEM )
  {
    red_control_status = ORPGRED_rda_control_status( ORPGRED_MY_CHANNEL );

    if( red_control_status == ORPGRED_RDA_CONTROLLING )
    {
      sprintf( buf1, "Controlling" );
    }
    else if( red_control_status == ORPGRED_RDA_NON_CONTROLLING )
    {
      sprintf( buf1, "Non-controlling" );
    }
    else
    {
      sprintf( buf1, "Unknown" );
    }

    if( ORPGRED_channel_state( ORPGRED_MY_CHANNEL ) == ORPGRED_CHANNEL_ACTIVE )
    {
      sprintf( buf2, "Active" );
    }
    else
    {
      sprintf( buf2, "Inactive" );
    }
    sprintf( Top_widget_title, "%s - (FAA:%d %s/%s)", HCI_task_titles[HCI_task_index], Site_info_channel_number, buf2, buf1 );
  }
  else if( Site_info_system == HCI_NWSR_SYSTEM )
  { 
    Site_info_channel_number = ORPGRDA_channel_num();
    sprintf( Top_widget_title, "%s - (NWS:%d)", HCI_task_titles[HCI_task_index], Site_info_channel_number );
  }
  else
  {
    sprintf( Top_widget_title, HCI_task_titles[HCI_task_index] );
  }

  XtVaSetValues( Top_widget, XmNtitle, Top_widget_title, NULL );
}   
    
/**********************************************************************

    Access function for top widget.
    
**********************************************************************/
    
Widget HCI_get_top_widget()
{   
  return Top_widget; 
}   

/**********************************************************************
    
    Access function for XtAppContext.

**********************************************************************/

XtAppContext HCI_get_appcontext()
{ 
  return App_context;
} 

/**********************************************************************

    Access function for Display.

**********************************************************************/

Display *HCI_get_display()
{
  return HCI_display;
}

/**********************************************************************

    Access function for Window.

**********************************************************************/

Window HCI_get_window()
{
  return XtWindow( Top_widget );
}
/**********************************************************************

    Access function for Screen.

**********************************************************************/

Screen *HCI_get_screen()
{
  return HCI_screen;
}

/**********************************************************************

    Access function for Colormap

**********************************************************************/

Colormap HCI_get_colormap()
{
  return HCI_colormap;
}

/**********************************************************************

    Access function for Depth

**********************************************************************/

Dimension HCI_get_depth()
{
  return HCI_depth;
}

/**********************************************************************

    Access function for system node.

**********************************************************************/

int HCI_get_node()
{
  return Site_info_node;
}

/**********************************************************************

    Access function for system channel number.

**********************************************************************/

int HCI_get_channel_number()
{
  return Site_info_channel_number;
}

/**********************************************************************

    Access function for system type.

**********************************************************************/

int HCI_get_system()
{
  return Site_info_system;
}

/*************************************************************************

    Returns the RPG mrpg machine name. "" indicates local host.

*************************************************************************/

char *HCI_orpg_machine_name()
{
  return( Orpg_machine_name );
}

/**************************************************************************

    Returns TRUE if this app is running in low bandwidth mode.

 **************************************************************************/

int HCI_is_low_bandwidth()
{
  return( Low_bandwidth_flag );
}

/**************************************************************************

    Returns TRUE if this app is running over a satellite connection.

 **************************************************************************/

int HCI_is_satellite_connection()
{
  return( Satellite_connection_flag );
}

/**************************************************************************

    Returns -1 if we are not in simulation mode. Returns a pasitive baud 
    rate otherwise

 **************************************************************************/

int HCI_simulation_speed()
{
  return( Simulation_speed );
}

/**************************************************************************
  
    Returns a string that can be used to start a child HCI application.
    The string ends with & and should be appended to the end of a hci
    child start-up string.

***************************************************************************/

char *HCI_child_options_string()
{
  static char *options_str = NULL;
  char buf[HCI_BUF_256];

  options_str = STR_copy( options_str, " " );

  if( Simulation_speed > 0 )
  {
    sprintf( buf, "-s %d ", Simulation_speed );
    options_str = STR_cat( options_str, buf );
  }

  if( Low_bandwidth_flag ){ options_str = STR_cat( options_str, "-l " ); }
  if( Satellite_connection_flag ){ options_str = STR_cat( options_str, "-S " ); }

  sprintf( buf, "-N %d &", Chan_num_for_syscfg );
  options_str = STR_cat( options_str, buf );

  return options_str;
}

/**********************************************************************

    Set user-defined destroy callback.

**********************************************************************/

void HCI_set_destroy_callback( int (*destroy_cb)() )
{
  HCI_user_destroy_callback = destroy_cb;
}

/**********************************************************************

    Set user-defined event loop timer.

**********************************************************************/

void HCI_set_timer_interval( int timer_loop )
{
  User_time_interval_count = timer_loop / HCI_QUARTER_SECOND;
}

/*************************************************************************

    Register a termination handler function.

*************************************************************************/

int HCI_register_term_hdlr( void (*cleanup_fxn_p)(int exit_code) )
{
  return( ORPGTASK_reg_term_hdlr( cleanup_fxn_p ) );
}

/*************************************************************************

    Callback when X Error occurs.

*************************************************************************/

static int HCI_XError_handler( Display *display, XErrorEvent *xerr )
{
  char msg[HCI_BUF_256];
  XGetErrorText( display, xerr->error_code, msg, HCI_BUF_256 );
  HCI_LE_error( "Serial %d, Error_code %d, Request Code %d, Minor Code %d", (int)xerr->serial, (int)xerr->error_code, (int)xerr->request_code, (int)xerr->minor_code );
  HCI_LE_error( "%s", msg );
  return 0;
}

/*************************************************************************

    Callback when X IO Error occurs.

*************************************************************************/

static int HCI_XIOError_handler( Display *display )
{
  HCI_LE_error( "X I/O error, application will exit" );
  HCI_task_exit(  HCI_EXIT_FAIL );
  return 0;
}

/*************************************************************************

    Set cursor to busy.

*************************************************************************/

void HCI_busy_cursor()
{
  XDefineCursor( HCI_get_display(), HCI_get_window(), Busy_cursor );
}

/*************************************************************************

    Set cursor to selectable.

*************************************************************************/

void HCI_selectable_cursor()
{
  XDefineCursor( HCI_get_display(), HCI_get_window(), Selectable_cursor );
}

/*************************************************************************

    Set cursor to default.

*************************************************************************/

void HCI_default_cursor()
{
  XUndefineCursor( HCI_get_display(), HCI_get_window() );
}

/*************************************************************************

    Write out multiline messages. Variable le_code determines category.

*************************************************************************/

static void HCI_LE_multiline( int le_code, const char *format, va_list *args )
{
  char buf[ LE_MAX_MSG_LENGTH ];
  char *temp_str = NULL;
  char *line_ptr = NULL;
  int line_len = -1;

  temp_str = va_arg( *args, char * );

  while( 1 )
  {
    line_ptr = strstr( temp_str, "\n" );
    if( line_ptr == NULL ){ line_ptr = temp_str + strlen( temp_str ); }
    line_len = line_ptr - temp_str;
    if( line_len > 0 )
    {
      if( line_len >= LE_MAX_MSG_LENGTH ){ line_len = LE_MAX_MSG_LENGTH-1; }
      strncpy( buf, temp_str, line_len );
      buf[line_len] = '\0';
      if( le_code == HCI_LE_STATUS )
      {
        HCI_LE_status( "%s", buf );
      }
      else if( le_code == HCI_LE_STATUS_LOG )
      {
        HCI_LE_status_log( "%s", buf );
      }
      else if( le_code == HCI_LE_ERROR )
      {
        HCI_LE_error( "%s", buf );
      }
      else if( le_code == HCI_LE_ERROR_LOG )
      {
        HCI_LE_error_log( "%s", buf );
      }
      else
      {
        HCI_LE_log( "%s", buf );
      }
    }
    if( *line_ptr == '\0' ){ break; }
    temp_str = line_ptr + 1;
  }
}

/*************************************************************************

    Write message to task log as a LOG message.

*************************************************************************/

void HCI_LE_log( const char *format, ...)
{
  char buf[ LE_MAX_MSG_LENGTH ];
  va_list arg_ptr;

  if( format == NULL ){ return; }

  va_start( arg_ptr, format );

  if( strcmp( format, "%ms" ) == 0 )
  {
    HCI_LE_multiline( HCI_LE_LOG, format, &arg_ptr );
  }
  else
  {
    vsprintf( buf, format, arg_ptr );
    LE_send_msg( HCI_LE_LOG, "   %s", buf );
  }
  va_end( arg_ptr );
}

/*************************************************************************

    Write message to task log as a STATUS message.

*************************************************************************/

void HCI_LE_status( const char *format, ...)
{
  char buf[ LE_MAX_MSG_LENGTH ];
  va_list arg_ptr;

  if( format == NULL ){ return; }

  va_start( arg_ptr, format );

  if( strcmp( format, "%ms" ) == 0 )
  {
    HCI_LE_multiline( HCI_LE_STATUS, format, &arg_ptr );
  }
  else
  {
    vsprintf( buf, format, arg_ptr );
    LE_send_msg( HCI_LE_STATUS, "S: %s", buf );
  }
  va_end( arg_ptr );
}

/*************************************************************************

    Write message to task log as an ERROR message.

*************************************************************************/

void HCI_LE_error( const char *format, ...)
{
  char buf[ LE_MAX_MSG_LENGTH ];
  va_list arg_ptr;

  if( format == NULL ){ return; }

  va_start( arg_ptr, format );

  if( strcmp( format, "%ms" ) == 0 )
  {
    HCI_LE_multiline( HCI_LE_ERROR, format, &arg_ptr );
  }
  else
  {
    vsprintf( buf, format, arg_ptr );
    LE_send_msg( HCI_LE_ERROR, "E:  %s", buf );
  }
  va_end( arg_ptr );
}

/*************************************************************************

    Write message to RPG status log.

*************************************************************************/

void HCI_LE_status_log( const char *format, ...)
{
  char buf[ LE_MAX_MSG_LENGTH ];
  va_list arg_ptr;

  if( format == NULL ){ return; }

  va_start( arg_ptr, format );

  if( strcmp( format, "%ms" ) == 0 )
  {
    HCI_LE_multiline( HCI_LE_STATUS_LOG, format, &arg_ptr );
  }
  else
  {
    vsprintf( buf, format, arg_ptr );
    LE_send_msg( HCI_LE_STATUS_LOG, "%s", buf );
  }
  va_end( arg_ptr );
}

/*************************************************************************

    Write message to RPG error log.

*************************************************************************/

void HCI_LE_error_log( const char *format, ...)
{
  char buf[ LE_MAX_MSG_LENGTH ];
  va_list arg_ptr;

  if( format == NULL ){ return; }

  va_start( arg_ptr, format );

  if( strcmp( format, "%ms" ) == 0 )
  {
    HCI_LE_multiline( HCI_LE_ERROR_LOG, format, &arg_ptr );
  }
  else
  {
    vsprintf( buf, format, arg_ptr );
    LE_send_msg( HCI_LE_ERROR_LOG, "%s", buf );
  }
  va_end( arg_ptr );
}

/*************************************************************************

    Callback for customized command line arguments.

*************************************************************************/

void HCI_set_custom_args( const char *args, int (*cb)(char, char *), void (*help_cb)() )
{
  strncpy( HCI_custom_args, args, 128 );
  HCI_custom_args_callback = cb;
  HCI_custom_args_help_callback = help_cb;
}

/*************************************************************************

    Callback for MapNotify event.

*************************************************************************/

static void Map_event_callback( Widget w, XtPointer client_data,
                                XEvent *evt, Boolean *x )
{
  if( evt->type == MapNotify ){ Top_widget_minimized_flag = HCI_NO_FLAG; }
  else if( evt->type == UnmapNotify ){ Top_widget_minimized_flag = HCI_YES_FLAG; }
  else{ /* Ignore */ }
}

/*************************************************************************

    Return true (1) if HCI minimized, false (0) otherwise.

*************************************************************************/

int HCI_is_minimized()
{
  return Top_widget_minimized_flag;
}

/*************************************************************************

    Post event to display message on Feedback line of HCI.

*************************************************************************/

void HCI_display_feedback( char *msg )
{
  EN_post( ORPGEVT_HCI_COMMAND_ISSUED, msg, strlen(msg)+1, 0 ); 
}

/*************************************************************************

    Access function for months name array.

*************************************************************************/

char **HCI_get_months()
{
  return (char **) &HCI_months[0];
}

/*************************************************************************

    Access function for month name from index.

*************************************************************************/

char *HCI_get_month( int index )
{
  return (char *) HCI_months[index];
}

/*************************************************************************

    Access function for task name.

*************************************************************************/

char *HCI_get_task_name()
{
  if( HCI_task_index < 0 || HCI_task_index >= NUM_HCI_TASKS ){ return NULL; }
  return HCI_task_titles[HCI_task_index];
}

/*************************************************************************

    Initialize shell widget.

*************************************************************************/

void HCI_Shell_init( Widget *shell_widget, char *shell_title )
{
  if( Hci_Shell_count == MAX_NUM_SHELLS )
  {
    fprintf( stderr, "Max number of shells reached (%d)\n", MAX_NUM_SHELLS );
    HCI_task_exit( HCI_EXIT_FAIL );
  }

  /* Initialize the Xt toolkit and create the Popup Shell. */

  *shell_widget = XtVaCreatePopupShell( "", xmDialogShellWidgetClass, Top_widget, NULL );

  if( *shell_widget == NULL )
  {
    fprintf( stderr, "HCI_Shell_init failed to create *shell_widget\n" );
    HCI_task_exit( HCI_EXIT_FAIL );
  }

  Set_shell_title( *shell_widget, shell_title );

  /* Set shell widget attributes.  */

  XtVaSetValues( *shell_widget,
            XmNmwmFunctions, MWM_FUNC_ALL | MWM_FUNC_MAXIMIZE | MWM_FUNC_CLOSE,
            NULL );

  Hci_Shell[Hci_Shell_count].w = *shell_widget;
  strcpy( Hci_Shell[Hci_Shell_count].title, shell_title );
  Hci_Shell_count++;

  return;
}

/*************************************************************************

    Modify title of shell widget.

*************************************************************************/

void Update_shell_titles()
{
  int i;
  for( i = 0; i < Hci_Shell_count; i++ )
  {
    Set_shell_title( Hci_Shell[i].w, Hci_Shell[i].title );
  }
}

/*************************************************************************

    Initial Realize (popup) of Shell widget.

*************************************************************************/

void HCI_Shell_start( Widget shell_widget, int shell_resize_flag )
{
  Dimension width, height;

  /* Check if window size is allowed to change. */

  if( shell_resize_flag == NO_RESIZE_HCI )
  {
    XtVaGetValues( shell_widget,
                   XmNwidth, &width,
                   XmNheight, &height, NULL );

    XtVaSetValues( shell_widget,
                   XmNminHeight, height,
                   XmNminWidth, width,
                   XmNmaxHeight, height,
                   XmNmaxWidth, width, NULL );
  }

  XtPopup( shell_widget, XtGrabNone );

  return;
}

/*************************************************************************

    Realize (popup) Shell widget.

*************************************************************************/

void HCI_Shell_popup( Widget shell_widget )
{
  XtPopup( shell_widget, XtGrabNone );
  HCI_flush_X_events();
}

/*************************************************************************

    Minimize Shell widget.

*************************************************************************/

void HCI_Shell_popdown( Widget shell_widget )
{
  XtPopdown( shell_widget );
  HCI_flush_X_events();
}

/*************************************************************************

    Flush pending X events.

*************************************************************************/

void HCI_flush_X_events()
{
  while( ( XtAppPending( HCI_get_appcontext() ) & XtIMAll ) != 0 )
  {
    XtAppProcessEvent( HCI_get_appcontext(), XtIMAll );
  }
}

/*************************************************************************

    Set title of Shell widget.

*************************************************************************/

static void Set_shell_title( Widget w, char *shell_title )
{
  char buf1[HCI_BUF_32];
  char buf2[HCI_BUF_16];
  char buf3[HCI_BUF_128];
  int red_control_status;

  if( Partial_init_flag == HCI_YES_FLAG )
  {
    sprintf( buf3, shell_title );
  }
  else if( Site_info_system == HCI_FAA_SYSTEM )
  {
    red_control_status = ORPGRED_rda_control_status( ORPGRED_MY_CHANNEL );

    if( red_control_status == ORPGRED_RDA_CONTROLLING )
    {
      sprintf( buf1, "Controlling" );
    }
    else if( red_control_status == ORPGRED_RDA_NON_CONTROLLING )
    {
      sprintf( buf1, "Non-controlling" );
    }
    else
    {
      sprintf( buf1, "Unknown" );
    }

    if( ORPGRED_channel_state( ORPGRED_MY_CHANNEL ) == ORPGRED_CHANNEL_ACTIVE )
    {
      sprintf( buf2, "Active" );
    }
    else
    {
      sprintf( buf2, "Inactive" );
    }
    sprintf( buf3, "%s - (FAA:%d %s/%s)", shell_title, Site_info_channel_number, buf2, buf1 );
  }
  else if( Site_info_system == HCI_NWSR_SYSTEM )
  { 
    Site_info_channel_number = ORPGRDA_channel_num();
    sprintf( buf3, "%s - (NWS:%d)", shell_title, Site_info_channel_number );
  }
  else
  {
    sprintf( buf3, shell_title );
  }

  XtVaSetValues( w, XmNtitle, buf3, NULL );
}

/*************************************************************************

    Callback when redundant status updated.

*************************************************************************/

static void Redundant_status_updated( int fd, LB_id_t msg_id, int msg_info, void *arg )
{
  Redundant_status_updated_flag = HCI_YES_FLAG;
}
