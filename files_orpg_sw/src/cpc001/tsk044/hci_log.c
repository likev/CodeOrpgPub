/************************************************************************
 *									*
 *	Module:  hci_log.c						*
 *									*
 *	Description:  This module provides a gui wrapper for		*
 *		      viewing log files with lelb_mon			*
 *									*
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2011/04/20 22:21:18 $
 * $Id: hci_log.c,v 1.16 2011/04/20 22:21:18 ccalvert Exp $
 * $Revision: 1.16 $
 * $State: Exp $
 */

/*	Local include file definitions.			*/

#include <hci.h>

/*	Defines.			*/

#define	MAX_NUM_LABELS		100
#define	LS_LOG_HCI_BUF_SIZE	5120
#define	SCROLL_HEIGHT		400
#define	SCROLL_WIDTH		600
#define	NUM_VISIBLE_ITEMS	20
#define	COMBO_BOX_WIDTH		30
#define	DEFAULT_TAG		"Select log file to view"
#define	NO_LOG_MSG		"No log files found"

/*	Global widget definitions.	*/

static	Widget	Top_widget = ( Widget ) NULL;
static	Widget	Tag_combo_box = ( Widget ) NULL;
static	Widget	System_rowcol = ( Widget ) NULL;
static	Widget	Log_labels[ MAX_NUM_LABELS ];
static	Widget	Start_button = ( Widget ) NULL;
static	Widget	Stop_button = ( Widget ) NULL;

/*	Global variables.		*/

static	int	System_flag = HCI_FAA_SYSTEM;
static	int	Node_flag = HCI_RPGA1;
static	int	Previous_node_flag = HCI_RPGA1;
static	int	View_log_flag = HCI_NO_FLAG;
static	char	Source_nodename[ 10 ] = "RPGA"; /* Node name shown on GUI */
static	char	Source_node[ 5 ] = "rpga"; /* Node name for lib calls */
static	int	Source_channel = 1;
static	char	Output_msgs[ MAX_NUM_LABELS ][ HCI_CP_MAX_BUF ];
static	int	Current_msg_pos = 0; /* Index of latest output msg */
static	int	Num_output_msgs = 0; /* Number of output msgs to display */
static	int	Coprocess_flag = HCI_CP_NOT_STARTED;
static	int	User_cancelled_process = HCI_NO_FLAG;
static	int	Discover_host_ip_status = -1;
static	void*	Cp = NULL;
static	char	Logfile_name[ HCI_BUF_256 ];
static	char	Node_ip_address[ HCI_BUF_64 ];
static	char  	Msg_buf[ HCI_CP_MAX_BUF + HCI_BUF_256 ];
static	char	*Le_log_dir = NULL;

/*	Function prototypes.		*/

static int	Destroy_callback();
static void	Close_callback( Widget, XtPointer, XtPointer );
static void	Node_toggle_callback( Widget, XtPointer, XtPointer );
static void	Logfile_callback( Widget, XtPointer, XtPointer );
static void	Start_viewing_callback( Widget, XtPointer, XtPointer );
static void	Stop_viewing_callback( Widget, XtPointer, XtPointer );
static void	Initialize_node_info();
static void	Update_scroll_display();
static void	Update_label( Widget, int );
static void	Populate_dropdown_list();
static void	Start_coprocess();
static void	Manage_coprocess();
static void	Close_coprocess();
static void	Reset_to_initial_state();
static void	Reset_log_display();
static void	Reset_dropdown_list();
static void	Warn_dropdown_list();
static void	Clear_dropdown_list();
static void	Set_dropdown_list_text();
static void	Set_node_ip_address();
static void	Set_log_directory();
static void	Timer_proc();

/************************************************************************
 *	Description: This is the main function for the Log Viewer task	*
 ************************************************************************/

int main( int argc, char *argv [] )
{
  Widget	form;
  Widget	frame;
  Widget	label;
  Widget	rowcol;
  Widget	control_rowcol;
  Widget	system_toggle;
  Widget	frame_rowcol = ( Widget ) NULL;
  Widget	info_scroll = ( Widget ) NULL;
  Widget	info_scroll_form;
  Widget	button;
  Widget	clip;
  int		n;
  Arg		args[ 16 ];
  int		loop = -1;

  /* Initialize HCI. */

  HCI_init( argc, argv, HCI_LOG_TASK );

  Top_widget = HCI_get_top_widget();
  HCI_set_destroy_callback( Destroy_callback );

  /* Set system and source channel. */

  System_flag = HCI_get_system();
  Source_channel = HCI_get_channel_number();

  /* Initialize node information. */

  Initialize_node_info();

  /* Set IP address of source node. */

  Set_node_ip_address();

  /* Set path of log directory. */

  Set_log_directory();

  /* Set toggle button parameters. */

  n = 0;
  XtSetArg( args [n], XmNforeground, hci_get_read_color( TEXT_FOREGROUND ));
  n++;
  XtSetArg( args [n], XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ));    n++;
  XtSetArg( args [n], XmNfontList, hci_get_fontlist (LIST));
  n++;
  XtSetArg( args [n], XmNpacking, XmPACK_TIGHT);
  n++;
  XtSetArg( args [n], XmNorientation, XmHORIZONTAL );
  n++;

  /* Define a form widget to be used as the manager for widgets in *
   * the top level window.                                         */

  form = XtVaCreateManagedWidget( "form",
         xmFormWidgetClass, Top_widget,
         XmNbackground,     hci_get_read_color( BACKGROUND_COLOR1 ),
         NULL );

  /* Use a rowcolumn widget at the top to manage the Close button. */

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

  /* Build the widgets for viewing log info. */

  frame = XtVaCreateManagedWidget( "frame",
          xmFrameWidgetClass,     form,
          XmNtopAttachment,       XmATTACH_WIDGET,
          XmNtopWidget,           control_rowcol,
          XmNleftAttachment,      XmATTACH_FORM,
          XmNrightAttachment,     XmATTACH_FORM,
          XmNbottomAttachment,    XmATTACH_FORM,
          NULL );

  frame_rowcol = XtVaCreateManagedWidget( "frame_rowcol",
          xmRowColumnWidgetClass, frame,
          XmNbackground,          hci_get_read_color( BACKGROUND_COLOR1 ),
          XmNorientation,         XmVERTICAL,
          XmNpacking,             XmPACK_TIGHT,
          XmNnumColumns,          1,
          XmNentryAlignment,      XmALIGNMENT_CENTER,
          NULL );

  /* Build toggle buttons to determine what system to view log
     files on. */

  System_rowcol = XtVaCreateManagedWidget( "rowcol",
          xmRowColumnWidgetClass, frame_rowcol,
          XmNbackground,          hci_get_read_color( BACKGROUND_COLOR1 ),
          XmNorientation,         XmHORIZONTAL,
          XmNpacking,             XmPACK_TIGHT,
          XmNnumColumns,          1,
          XmNentryAlignment,      XmALIGNMENT_CENTER,
          NULL );

  label = XtVaCreateManagedWidget( "System: ",
        xmLabelWidgetClass,     System_rowcol,
        XmNforeground,  hci_get_read_color( TEXT_FOREGROUND ),
        XmNbackground,  hci_get_read_color( BACKGROUND_COLOR1 ),
        XmNfontList,            hci_get_fontlist( LIST ),
        NULL );

  system_toggle = XmCreateRadioBox( System_rowcol, "system_toggle", args, n );

  if( System_flag != HCI_FAA_SYSTEM )
  {
    button = XtVaCreateManagedWidget( "RPGA",
      xmToggleButtonWidgetClass, system_toggle,
      XmNselectColor,            hci_get_read_color( WHITE ),
      XmNforeground,             hci_get_read_color( TEXT_FOREGROUND ),
      XmNbackground,             hci_get_read_color( BACKGROUND_COLOR1 ),
      XmNfontList,               hci_get_fontlist( LIST ),
      NULL);

    if( strcmp( Source_nodename, "RPGA" ) == 0 )
    {
      XtVaSetValues( button, XmNset, True, NULL );
    }
    else
    {
      XtVaSetValues( button, XmNset, False, NULL );
    }

    XtAddCallback( button,
                   XmNarmCallback,  Node_toggle_callback,
                   (XtPointer) HCI_RPGA1 );

    button = XtVaCreateManagedWidget( "RPGB",
      xmToggleButtonWidgetClass, system_toggle,
      XmNselectColor,            hci_get_read_color( WHITE ),
      XmNforeground,             hci_get_read_color( TEXT_FOREGROUND ),
      XmNbackground,             hci_get_read_color( BACKGROUND_COLOR1 ),
      XmNfontList,               hci_get_fontlist( LIST ),
      NULL);

    if( strcmp( Source_nodename, "RPGB" ) == 0 )
    {
      XtVaSetValues( button, XmNset, True, NULL );
    }
    else
    {
      XtVaSetValues( button, XmNset, False, NULL );
    }

    XtAddCallback( button,
                   XmNarmCallback,  Node_toggle_callback,
                   (XtPointer) HCI_RPGB1 );

    button = XtVaCreateManagedWidget( "MSCF",
      xmToggleButtonWidgetClass, system_toggle,
      XmNselectColor,            hci_get_read_color( WHITE ),
      XmNforeground,             hci_get_read_color( TEXT_FOREGROUND ),
      XmNbackground,             hci_get_read_color( BACKGROUND_COLOR1 ),
      XmNfontList,               hci_get_fontlist( LIST ),
      NULL);

    if( strcmp( Source_nodename, "MSCF" ) == 0 )
    {
      XtVaSetValues( button, XmNset, True, NULL );
    }
    else
    {
      XtVaSetValues( button, XmNset, False, NULL );
    }

    XtAddCallback( button,
                   XmNarmCallback,  Node_toggle_callback,
                   (XtPointer) HCI_MSCF );

  }
  else
  {
    button = XtVaCreateManagedWidget( "RPGA 1",
      xmToggleButtonWidgetClass, system_toggle,
      XmNselectColor,            hci_get_read_color( WHITE ),
      XmNforeground,             hci_get_read_color( TEXT_FOREGROUND ),
      XmNbackground,             hci_get_read_color( BACKGROUND_COLOR1 ),
      XmNfontList,               hci_get_fontlist( LIST ),
      NULL);

    if( strcmp( Source_nodename, "RPGA 1" ) == 0 )
    {
      XtVaSetValues( button, XmNset, True, NULL );
    }
    else
    {
      XtVaSetValues( button, XmNset, False, NULL );
    }

    XtAddCallback( button,
                   XmNarmCallback,  Node_toggle_callback,
                   (XtPointer) HCI_RPGA1 );

    button = XtVaCreateManagedWidget( "RPGA 2",
      xmToggleButtonWidgetClass, system_toggle,
      XmNselectColor,            hci_get_read_color( WHITE ),
      XmNforeground,             hci_get_read_color( TEXT_FOREGROUND ),
      XmNbackground,             hci_get_read_color( BACKGROUND_COLOR1 ),
      XmNfontList,               hci_get_fontlist( LIST ),
      NULL);

    if( strcmp( Source_nodename, "RPGA 2" ) == 0 )
    {
      XtVaSetValues( button, XmNset, True, NULL );
    }
    else
    {
      XtVaSetValues( button, XmNset, False, NULL );
    }

    XtAddCallback( button,
                   XmNarmCallback,  Node_toggle_callback,
                   (XtPointer) HCI_RPGA2 );

    button = XtVaCreateManagedWidget( "RPGB 1",
      xmToggleButtonWidgetClass, system_toggle,
      XmNselectColor,            hci_get_read_color( WHITE ),
      XmNforeground,             hci_get_read_color( TEXT_FOREGROUND ),
      XmNbackground,             hci_get_read_color( BACKGROUND_COLOR1 ),
      XmNfontList,               hci_get_fontlist( LIST ),
      NULL);

    if( strcmp( Source_nodename, "RPGB 1" ) == 0 )
    {
      XtVaSetValues( button, XmNset, True, NULL );
    }
    else
    {
      XtVaSetValues( button, XmNset, False, NULL );
    }

    XtAddCallback( button,
                   XmNarmCallback,  Node_toggle_callback,
                   (XtPointer) HCI_RPGB1 );

    button = XtVaCreateManagedWidget( "RPGB 2",
      xmToggleButtonWidgetClass, system_toggle,
      XmNselectColor,            hci_get_read_color( WHITE ),
      XmNforeground,             hci_get_read_color( TEXT_FOREGROUND ),
      XmNbackground,             hci_get_read_color( BACKGROUND_COLOR1 ),
      XmNfontList,               hci_get_fontlist( LIST ),
      NULL);

    if( strcmp( Source_nodename, "RPGB 2" ) == 0 )
    {
      XtVaSetValues( button, XmNset, True, NULL );
    }
    else
    {
      XtVaSetValues( button, XmNset, False, NULL );
    }

    XtAddCallback( button,
                   XmNarmCallback,  Node_toggle_callback,
                   (XtPointer) HCI_RPGB2 );

    button = XtVaCreateManagedWidget( "MSCF",
      xmToggleButtonWidgetClass, system_toggle,
      XmNselectColor,            hci_get_read_color( WHITE ),
      XmNforeground,             hci_get_read_color( TEXT_FOREGROUND ),
      XmNbackground,             hci_get_read_color( BACKGROUND_COLOR1 ),
      XmNfontList,               hci_get_fontlist( LIST ),
      NULL);

    if( strcmp( Source_nodename, "MSCF" ) == 0 )
    {
      XtVaSetValues( button, XmNset, True, NULL );
    }
    else
    {
      XtVaSetValues( button, XmNset, False, NULL );
    }

    XtAddCallback( button,
                   XmNarmCallback,  Node_toggle_callback,
                   (XtPointer) HCI_MSCF );
  }

  XtManageChild( system_toggle );

  /* Build combo box to show log files to choose from. */

  Tag_combo_box = XtVaCreateManagedWidget( "tag_combo_box",
      xmComboBoxWidgetClass, frame_rowcol,
      XmNbackground,         hci_get_read_color( BACKGROUND_COLOR1 ),
      XmNforeground,         hci_get_read_color( TEXT_FOREGROUND ),
      XmNfontList,           hci_get_fontlist( LIST ),
      XmNcomboBoxType,       XmDROP_DOWN_LIST,
      XmNcolumns,            COMBO_BOX_WIDTH,
      XmNvisibleItemCount,   NUM_VISIBLE_ITEMS,
      XmNvalue,              DEFAULT_TAG,
      NULL );

  XtAddCallback( Tag_combo_box, XmNselectionCallback, Logfile_callback, NULL );

  /* Create scroll area to display log output. */

  info_scroll = XtVaCreateManagedWidget ("data_scroll",
                xmScrolledWindowWidgetClass,    frame_rowcol,
                XmNheight,              SCROLL_HEIGHT,
                XmNwidth,               SCROLL_WIDTH,
                XmNscrollingPolicy,     XmAUTOMATIC,
                XmNforeground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNtopAttachment,       XmATTACH_WIDGET,
                XmNtopWidget,           System_rowcol,
                XmNleftAttachment,      XmATTACH_FORM,
                XmNrightAttachment,     XmATTACH_FORM,
                NULL);

  XtVaGetValues( info_scroll, XmNclipWindow, &clip, NULL );

  XtVaSetValues( clip,
                 XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
                 NULL );

  info_scroll_form = XtVaCreateManagedWidget( "scroll_form",
       xmFormWidgetClass, info_scroll,
       XmNforeground, hci_get_read_color( BACKGROUND_COLOR1 ),
       XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
       XmNverticalSpacing, 1,
       NULL );

  rowcol = XtVaCreateManagedWidget( "rowcol",
          xmRowColumnWidgetClass, info_scroll_form,
          XmNforeground,          hci_get_read_color( BACKGROUND_COLOR1 ),
          XmNbackground,          hci_get_read_color( BACKGROUND_COLOR1 ),
          XmNorientation,         XmVERTICAL,
          XmNpacking,             XmPACK_COLUMN,
          XmNnumColumns,          1,
          XmNleftAttachment,      XmATTACH_FORM,
          XmNrightAttachment,      XmATTACH_FORM,
          NULL );

  Log_labels[ 0 ] = XtVaCreateManagedWidget (" ",
                        xmLabelWidgetClass, rowcol,
                        XmNfontList,    hci_get_fontlist (LIST),
                        XmNleftAttachment,      XmATTACH_FORM,
                        XmNrightAttachment,     XmATTACH_FORM,
                        XmNtopAttachment,       XmATTACH_FORM,
                        XmNmarginHeight,        0,
                        XmNborderWidth,         0,
                        XmNshadowThickness,     0,
                        XmNforeground,  hci_get_read_color (TEXT_FOREGROUND),
                        XmNbackground,  hci_get_read_color (BACKGROUND_COLOR1),
                        NULL);

  for( loop = 1; loop < MAX_NUM_LABELS; loop++ )
  {
                Log_labels[loop] = XtVaCreateManagedWidget (" ",
                        xmLabelWidgetClass, rowcol,
                        XmNfontList,    hci_get_fontlist (LIST),
                        XmNleftAttachment,      XmATTACH_FORM,
                        XmNrightAttachment,     XmATTACH_FORM,
                        XmNtopAttachment,       XmATTACH_WIDGET,
                        XmNtopWidget,           Log_labels [loop-1],
                        XmNmarginHeight,        0,
                        XmNborderWidth,         0,
                        XmNshadowThickness,     0,
                        XmNforeground,  hci_get_read_color (TEXT_FOREGROUND),
                        XmNbackground,  hci_get_read_color (BACKGROUND_COLOR1),
                        NULL);
  }

  /* Create start/stop buttons for log viewing. */

  rowcol = XtVaCreateManagedWidget( "rowcol",
          xmRowColumnWidgetClass, frame_rowcol,
          XmNbackground,          hci_get_read_color( BACKGROUND_COLOR1 ),
          XmNorientation,         XmHORIZONTAL,
          XmNpacking,             XmPACK_TIGHT,
          XmNnumColumns,          1,
          XmNentryAlignment,      XmALIGNMENT_CENTER,
          XmNtopAttachment,       XmATTACH_WIDGET,
          XmNtopWidget,           info_scroll,
          XmNleftAttachment,      XmATTACH_FORM,
          XmNrightAttachment,     XmATTACH_FORM,
          NULL );

  Start_button = XtVaCreateManagedWidget( "Start",
           xmPushButtonWidgetClass, rowcol,
           XmNforeground,           hci_get_read_color( BUTTON_FOREGROUND ),
           XmNbackground,           hci_get_read_color( BUTTON_BACKGROUND ),
           XmNfontList,             hci_get_fontlist( LIST ),
           XmNsensitive,            False,
           NULL );

  XtAddCallback( Start_button,
                 XmNactivateCallback, Start_viewing_callback, NULL );

  Stop_button = XtVaCreateManagedWidget( "Stop",
           xmPushButtonWidgetClass, rowcol,
           XmNforeground,           hci_get_read_color( BUTTON_FOREGROUND ),
           XmNbackground,           hci_get_read_color( BUTTON_BACKGROUND ),
           XmNfontList,             hci_get_fontlist( LIST ),
           XmNsensitive,            False,
           NULL );

  XtAddCallback( Stop_button,
                 XmNactivateCallback, Stop_viewing_callback, NULL );

  /* Finished creating widgets, realize top-level widget. */

  XtRealizeWidget( Top_widget );

  /* Populate dropdown list with log files to view. */

  Populate_dropdown_list();

  /* Start HCI loop. */

  HCI_start( Timer_proc, HCI_ONE_SECOND, NO_RESIZE_HCI );

  return 0;
}

/************************************************************************
 *	Description: This function is activated when the the HCI	*
 *		     Log Viewer window is destroyed.			*
 ************************************************************************/

static int Destroy_callback()
{
  if( Coprocess_flag != HCI_CP_NOT_STARTED )
  {
    sprintf( Msg_buf, "You are not allowed to exit this task until\n" );
    strcat( Msg_buf, "the log viewing process has been stopped.\n" );
    hci_error_popup( Top_widget, Msg_buf, NULL );
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
    sprintf( Msg_buf, "You are not allowed to exit this task until\n" );
    strcat( Msg_buf, "the log viewing process has been stopped.\n" );
    hci_error_popup( Top_widget, Msg_buf, NULL );
  }
  else
  {
    XtDestroyWidget( Top_widget );
  }
}

/************************************************************************
 * Node_toggle_callback: Callback for system_toggle buttons.
 ************************************************************************/

static void Node_toggle_callback( Widget w, XtPointer x, XtPointer y )
{
  if( ( int ) x != Node_flag )
  {
    Node_flag = ( int ) x;
  }
  else
  {
    /* Ignore duplicate selections. */
    return;
  }

  if( Node_flag == HCI_RPGA1 )
  {
    if( System_flag != HCI_FAA_SYSTEM ){ sprintf( Source_nodename, "RPGA" ); }
    else{ sprintf( Source_nodename, "RPGA 1" ); }
    sprintf( Source_node, "rpga" );
    Source_channel = 1;
  }
  else if( Node_flag == HCI_RPGA2 )
  {
    sprintf( Source_nodename, "RPGA 2" );
    sprintf( Source_node, "rpga" );
    Source_channel = 2;
  }
  else if( Node_flag == HCI_MSCF )
  {
    sprintf( Source_nodename, "MSCF" );
    sprintf( Source_node, "mscf" );
    Source_channel = 1;
  }
  else if( Node_flag == HCI_RPGB1 )
  {
    if( System_flag != HCI_FAA_SYSTEM ){ sprintf( Source_nodename, "RPGB" ); }
    else{ sprintf( Source_nodename, "RPGB 1" ); }
    sprintf( Source_node, "rpgb" );
    Source_channel = 1;
  }
  else if( Node_flag == HCI_RPGB2 )
  {
    sprintf( Source_nodename, "RPGB 2" );
    sprintf( Source_node, "rpgb" );
    Source_channel = 2;
  }
}

/************************************************************************
 * Timer_proc: Callback for event timer.
 ************************************************************************/

static void Timer_proc()
{
  if( Node_flag != Previous_node_flag )
  {
    XtSetSensitive( Start_button, False );
    Previous_node_flag = Node_flag;
    /* Call function to clear log display. */
    Reset_log_display();
    /* Call function to clear dropdown list. */
    Reset_dropdown_list();
    /* Get new IP address. */
    Set_node_ip_address();
    /* Call function to populate dropdown list with log files to view. */
    Populate_dropdown_list();
  }
  else if( View_log_flag )
  {
    View_log_flag = HCI_NO_FLAG;
    XtSetSensitive( System_rowcol, False );
    XtSetSensitive( Tag_combo_box, False );
    XtSetSensitive( Start_button, False );
    XtSetSensitive( Stop_button, True );
    HCI_busy_cursor();
    /* Call function to start co-process to view log. */
    Start_coprocess();
  }
  else if( Coprocess_flag == HCI_CP_STARTED )
  {
    /* Call function to manage co-process. */
    Manage_coprocess();
  }
  else if( Coprocess_flag == HCI_CP_FINISHED )
  {
    Coprocess_flag = HCI_CP_NOT_STARTED;
    /* Call function to reset gui to original state. */
    Reset_to_initial_state();
  }
}

/************************************************************************
 *	Description: This function determines the log file directory.
 ************************************************************************/

static void Set_log_directory()
{
  char *temp_ptr = getenv( "LE_DIR_EVENT" );

  if( temp_ptr == NULL )
  {
    HCI_LE_error( "Env variable LE_DIR_EVENT not defined" );
    HCI_task_exit( HCI_EXIT_FAIL );
  }

  /* Strip off event number. */

  Le_log_dir = strtok( temp_ptr, ":" );
  HCI_LE_log( "Log directory: %s", Le_log_dir );
}

/************************************************************************
 *	Description: This function determines the source node's IP address.
 ************************************************************************/

static void Set_node_ip_address()
{
  EN_cntl_unblock();
  Discover_host_ip_status = ORPGMGR_discover_host_ip( Source_node,
			Source_channel, Node_ip_address, HCI_BUF_64 );
  EN_cntl_block();

  if( Discover_host_ip_status < 0 )
  {
    HCI_LE_error( "ORPGMGR_discover_host_ip failed (%d) for %s:%d",
             Discover_host_ip_status, Source_node, Source_channel );
  }
  else if( Discover_host_ip_status == 0 )
  {
    HCI_LE_error( "System %s:%d not found.", Source_node, Source_channel );
  }
}

/************************************************************************
 *	Description: This function populates the dropdown list with the
 *                   log files available on the source node.
 ************************************************************************/

static void Populate_dropdown_list()
{
  int ret = -1, sys_ret = -1;
  int n_bytes = -1;
  char func[ HCI_BUF_256 ];
  char cmd[ HCI_BUF_256 ];
  char buf[ HCI_BUF_256 ];
  char output_buffer[ LS_LOG_HCI_BUF_SIZE ];
  char base_name[ HCI_BUF_256 ];
  char *temp_ptr, *char_ptr;
  int int_offset = -1;
  Widget combo_list = ( Widget ) NULL;
  XmString str;

  if( Discover_host_ip_status < 0 )
  {
    sprintf( Msg_buf, "ORPGMGR_discover_host_ip failed (%d) for %s.\n\n",
             ret, Source_nodename );
    strcat( Msg_buf, "Unable to display log files." );
    hci_error_popup( Top_widget, Msg_buf, NULL );
    Warn_dropdown_list();
    return;
  }
  else if( Discover_host_ip_status == 0 )
  {
    sprintf( Msg_buf, "System %s not found.\n\n", Source_nodename );
    strcat( Msg_buf, "Unable to display log files." );
    hci_error_popup( Top_widget, Msg_buf, NULL );
    Warn_dropdown_list();
    return;
  }

  /* Build function to call remotely. Don't let user see hardware
     configuration logs, since they may contain unencrypted passwords. */

  sprintf( func, "%s:MISC_system_to_buffer", Node_ip_address );
  sprintf( cmd, "sh -c ( ls %s | egrep -v '^config_device' )", Le_log_dir );

  ret = RSS_rpc( func, "i-r s-i ba-%d-io i ia-io", LS_LOG_HCI_BUF_SIZE,
             &sys_ret, cmd, output_buffer, LS_LOG_HCI_BUF_SIZE, &n_bytes );
  sys_ret = sys_ret >> 8;

  if( ret != 0 )
  {
    sprintf( Msg_buf, "RSS_rpc failed (%d).\n\n", ret );
    strcat( Msg_buf, "Unable to display log files." );
    hci_error_popup( Top_widget, Msg_buf, NULL );
    Warn_dropdown_list();
  }
  else if( sys_ret != 0 )
  {
    sprintf( Msg_buf, "MISC_system_to_buffer failed (%d).\n\n", sys_ret );
    strcat( Msg_buf, "Unable to display log files." );
    hci_error_popup( Top_widget, Msg_buf, NULL );
    Warn_dropdown_list();
  }
  else
  {
    /* Get reference to combo list and empty it. */
    XtVaGetValues( Tag_combo_box, XmNlist, &combo_list, NULL );
    XmListDeleteAllItems( combo_list );

    /* Parse output for log file names. */
    temp_ptr = output_buffer;
    while( ( int_offset = MISC_get_token( temp_ptr, "S\n", 0, buf, HCI_BUF_256 ) ) > 0 )
    {
      /* We only care about files that end in ".log" */
      if( strstr( buf, ".log" ) != NULL )
      {
        /* Remove ".log" from file name. */
        char_ptr = strtok( buf, "." );
        sprintf( base_name, char_ptr );
        while( ( char_ptr = strtok( NULL, "." ) ) != NULL )
        {
          if( strcmp( char_ptr, "log" ) != 0 )
          {
            strcat( base_name, "." );
            strcat( base_name, char_ptr );
          }
        }
        str = XmStringCreateLocalized( base_name );
        XmListAddItemUnselected( combo_list, str, 0 );
        XmStringFree( str );
      }
      temp_ptr+=int_offset;
    }
  }
}

/************************************************************************
 *	Description: This function is called when a new logfile is
 *                   chosen from the dropdown list.
 ************************************************************************/

static void Logfile_callback( Widget w, XtPointer x, XtPointer y )
{
  char *text_item;

  /* Extract name from Widget and set logfile name. */

  XmComboBoxCallbackStruct *cbs = ( XmComboBoxCallbackStruct * ) y;
  XmStringGetLtoR( cbs->item_or_text, XmFONTLIST_DEFAULT_TAG, &text_item );
  sprintf( Logfile_name, "%s", text_item );
  XtFree( text_item );

  /* If user selects "no log message", return and do nothing. */

  if( strcmp( Logfile_name, NO_LOG_MSG ) == 0 ){ Warn_dropdown_list();return; }

  /* Set flag to start log viewing process. */

  if( strstr( Logfile_name, DEFAULT_TAG  ) == NULL )
  {
    Reset_log_display();
    View_log_flag = HCI_YES_FLAG;
  }
}

/************************************************************************
 *	Description: This function is called to stop logfile viewing.
 ************************************************************************/

void Stop_viewing_callback( Widget w, XtPointer x, XtPointer y )
{
  User_cancelled_process = HCI_YES_FLAG;
  XtSetSensitive( Stop_button, False );
  Close_coprocess();
}

/************************************************************************
 *	Description: This function is called to start logfile viewing.
 ************************************************************************/

void Start_viewing_callback( Widget w, XtPointer x, XtPointer y )
{
  View_log_flag = HCI_YES_FLAG;
}

/************************************************************************
 *	Description: Start the coprocess to view the log.
 ************************************************************************/

static void Start_coprocess()
{
  int ret = -1;
  char cmd[ HCI_BUF_256 ];

  /* Set flag in case of error. */

  /* Reset scroll display. */

  Num_output_msgs = 0;
  Current_msg_pos = -1;

  /* Start co-process. */

  if( strcmp( Source_node, "mscf" ) == 0 )
  {
    sprintf( cmd, "lem %s:%s", Source_node, Logfile_name );
  }
  else
  {
    sprintf( cmd, "lem %s%d:%s", Source_node, Source_channel, Logfile_name );
  }

  if( ( ret = MISC_cp_open( cmd, HCI_CP_MANAGE, &Cp ) ) != 0 )
  {
    /* Co-process failed. */
    sprintf( Msg_buf, "MISC_cp_open failed (%d).\n\n", ret );
    strcat( Msg_buf, "Unable to view log file." );
    hci_error_popup( Top_widget, Msg_buf, NULL );
    Coprocess_flag = HCI_CP_FINISHED;
  }
  else
  {
    Coprocess_flag = HCI_CP_STARTED;
  }
}

/************************************************************************
 *	Description: This function manages the coprocess that is
 *                   watching the selected log file.
 ************************************************************************/

static void Manage_coprocess()
{
  int ret = -1;
  char buf[ HCI_CP_MAX_BUF ];
  int new_msgs = HCI_NO_FLAG;

  /* Read coprocess to see if any new data is available. */

  while( Cp != NULL &&
         ( ret = MISC_cp_read_from_cp( Cp, buf, HCI_CP_MAX_BUF ) ) != 0 )
  {
    /* Strip trailing newline. */
    if( buf[ strlen( buf ) - 1 ] == '\n' )
    {
      buf[ strlen( buf ) - 1 ] = '\0';
    }

    /* Take action according to value of ret. */
    if( ret == HCI_CP_DOWN )
    {
      if( ( ret = ( MISC_cp_get_status( Cp ) >> 8 ) ) != 0 )
      { 
        /* If the log process exited with error, and the error isn't caused
           by the user cancelling the process, pop up an error message. */
        if( User_cancelled_process == HCI_NO_FLAG )
        {
          sprintf( Msg_buf, "Error reading log file (%d).", ret );
          hci_error_popup( Top_widget, Msg_buf, NULL );
        }
      }
      User_cancelled_process = HCI_NO_FLAG;
      Close_coprocess();
      break;
    }
    else if( ret == HCI_CP_STDERR )
    {
      /* Ignore error from lem task. */
    }
    else if( ret == HCI_CP_STDOUT )
    {
      /* Set flag to indicate new log messages. */
      new_msgs = HCI_YES_FLAG;
      /* Update starting label position (circular array). */
      Current_msg_pos++;
      Current_msg_pos%=MAX_NUM_LABELS;
      /* Update number of labels (not to exceed maximum number allowed). */
      Num_output_msgs++;
      if( Num_output_msgs > MAX_NUM_LABELS ){ Num_output_msgs--; }
      /* This line is just a message that needs to
         be displayed in the message scroll box. */
      sprintf( Output_msgs[ Current_msg_pos ], "%s", buf );
    }
    else
    {
      /* No new data available, do nothing... */
    }
  }

  /* If new messages available, update scroll with latest info. */
  if( new_msgs )
  {
    Update_scroll_display();
  }
}

/************************************************************************
 *	Description: Close coprocess.
 ************************************************************************/

static void Close_coprocess()
{
  MISC_cp_close( Cp );
  Cp = NULL;
  Coprocess_flag = HCI_CP_FINISHED;
}

/************************************************************************
 *	Description: This function updates the scroll with the latest	*
 *		     output from the log viewer task.
 ************************************************************************/

static void Update_scroll_display()
{
  int loop = -1;
  int temp_int = -1;

  /* The latest message is located at position Current_msg_pos.
     Start there and work down to index 0. */

  temp_int = 0;
  for( loop = Current_msg_pos; loop >= 0; loop-- )
  {
    Update_label( Log_labels[ loop ], temp_int );
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
      Update_label( Log_labels[ loop ], temp_int );
      temp_int++;
    }
  }
}

/************************************************************************
 *	Description: This function updates a label with the appropriate	*
 *		     value and color.					*
 ************************************************************************/

static void Update_label( Widget w, int index )
{
  XmString msg;

  /* Convert c-buf to XmString variable. */

  msg = XmStringCreateLtoR( Output_msgs[ index ], XmFONTLIST_DEFAULT_TAG );

  /* Modify appropriate output label. */

  XtVaSetValues( w,
                 XmNlabelString, msg,
                 XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
                 NULL );

  /* Free allocated space. */

  XmStringFree( msg );
}

/***************************************************************************
 * Reset_to_initial_state: Reset flags, buttons, cursors, etc. to initial
 *    states.
 ***************************************************************************/

static void Reset_to_initial_state()
{
  XtSetSensitive( System_rowcol, True );
  XtSetSensitive( Tag_combo_box, True );
  XtSetSensitive( Start_button, True );
  XtSetSensitive( Stop_button, False );
  HCI_default_cursor();
}

/***************************************************************************
 * Reset_log_display: Clear log display.
 ***************************************************************************/

static void Reset_log_display()
{
  int loop = -1;
  char *blank_string = "";

  for( loop = 0; loop < MAX_NUM_LABELS; loop++ )
  {
    sprintf( Output_msgs[ loop ], blank_string );
    Update_label( Log_labels[ loop ], loop );
  }
}

/***************************************************************************
 * Reset_dropdown_list: Reset dropdown list.
 ***************************************************************************/

static void Reset_dropdown_list()
{
  /* Clear dropdown list. */
  Clear_dropdown_list();
  /* Set text field to default message. */
  Set_dropdown_list_text( DEFAULT_TAG );
}

/***************************************************************************
 * Warn_dropdown_list: Warn user of no log files being found.
 ***************************************************************************/

static void Warn_dropdown_list()
{
  Widget combo_list = ( Widget ) NULL;
  XmString str;

  /* Clear dropdown list. */
  Clear_dropdown_list();

  /* Get reference to combo list and empty it. */
  XtVaGetValues( Tag_combo_box, XmNlist, &combo_list, NULL );

  /* Add warning item. */
  str = XmStringCreateLocalized( NO_LOG_MSG );
  XmListAddItemUnselected( combo_list, str, 0 );
  XmStringFree( str );
  /* Set text field to warning message. */
  Set_dropdown_list_text( DEFAULT_TAG );
}

/***************************************************************************
 * Clear_dropdown_list: Clear dropdown list.
 ***************************************************************************/

static void Clear_dropdown_list()
{
  Widget combo_list = ( Widget ) NULL;

  /* Get reference to combo list and empty it. */
  XtVaGetValues( Tag_combo_box, XmNlist, &combo_list, NULL );
  XmListDeleteAllItems( combo_list );
}

/***************************************************************************
 * Set_dropdown_list_text: Set message of dropdown list text field.
 ***************************************************************************/

static void Set_dropdown_list_text( char *msg )
{
  Widget tfield = ( Widget ) NULL;

  /* Get reference to TextField and set text. */
  XtVaGetValues( Tag_combo_box, XmNtextField, &tfield, NULL );
  XtVaSetValues( tfield, XmNvalue, msg, NULL );
}

/***************************************************************************
 * Initialize_node_info: Initialize variables based on node info.
 ***************************************************************************/

static void Initialize_node_info()
{
  char *host_buf = NULL;

  /* Determine node this is running on. */

  if( ( host_buf = ORPGMISC_get_site_name( "type" ) ) == NULL )
  {
    HCI_LE_error( "ORPGMISC_get_site_name(\"type\") is NULL" );
    HCI_task_exit( HCI_EXIT_FAIL );
  }
  else if( strcmp( host_buf, "rpga" ) == 0 )
  {
    Node_flag = HCI_RPGA1;
    Previous_node_flag = HCI_RPGA1;
    if( System_flag != HCI_FAA_SYSTEM )
    {
      sprintf( Source_nodename, "RPGA" );
      Source_channel = 1;
    }
    else
    {
      sprintf( Source_nodename, "RPGA %d", Source_channel );
      if( Source_channel == 2 )
      {
        Node_flag = HCI_RPGA2;
        Previous_node_flag = HCI_RPGA2;
      }
    }
    sprintf( Source_node, "rpga" );
  }
  else if( strcmp( host_buf, "rpgb" ) == 0 )
  {
    Node_flag = HCI_RPGB1;
    Previous_node_flag = HCI_RPGB1;
    if( System_flag != HCI_FAA_SYSTEM )
    {
      sprintf( Source_nodename, "RPGB" );
      Source_channel = 1;
    }
    else
    {
      sprintf( Source_nodename, "RPGB %d", Source_channel );
      if( Source_channel == 2 )
      {
        Node_flag = HCI_RPGB2;
        Previous_node_flag = HCI_RPGB2;
      }
    }
    sprintf( Source_node, "rpgb" );
  }
  else if( strcmp( host_buf, "mscf" ) == 0 )
  {
    Node_flag = HCI_MSCF;
    Previous_node_flag = HCI_MSCF;
    sprintf( Source_nodename, "MSCF" );
    sprintf( Source_node, "mscf" );
    Source_channel = 1;
  }
  else
  {
    HCI_LE_error( "Unexpected node type: %s", host_buf );
    HCI_task_exit( HCI_EXIT_FAIL );
  }
}

