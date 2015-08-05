/************************************************************************
 *									*
 *	Module:  network_connect_gui.c					*
 *									*
 *	Description:  This module provides a gui for ldmping/ping.	*
 *									*
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2011/01/14 19:37:24 $
 * $Id: network_connect_gui.c,v 1.1 2011/01/14 19:37:24 ccalvert Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

/*	Local include file definitions.			*/

#include <hci.h>

/*	Defines/Enums.			*/

enum { LDMPING_TAB = 1, PING_TAB };
enum { LDMPING_ACTION = 1, PING_ACTION };

#define	NUM_TABS			2
#define	FIRST_TAB			LDMPING_TAB
#define	HOST_SCROLL_HEIGHT		300
#define	HOST_SCROLL_WIDTH		550
#define	OUTPUT_SCROLL_HEIGHT		100
#define	OUTPUT_SCROLL_WIDTH		550
#define	MAX_NUM_HOSTS			200
#define	MAX_NUM_HOSTS_PER_LINE		10
#define	MAX_HOST_LENGTH			32
#define	MAX_HOST_BUTTON_LENGTH		8
#define	MAX_HOSTS_BUF_LENGTH		MAX_NUM_HOSTS*(MAX_HOST_LENGTH+1)
#define	MAX_NUMBER_LDMPING_LINES	100
#define	MAX_LDMPING_LINE_LENGTH		200
#define	MAX_NUMBER_PING_LINES		100
#define	MAX_PING_LINE_LENGTH		200

/*	Structures.			*/

/* NONE */

/*	Global widget definitions.	*/

static	Widget	Top_widget = ( Widget ) NULL;
static	Widget	Tab_widget = ( Widget ) NULL;
static	Widget	Close_button = ( Widget ) NULL;
static	Widget	Ldmping_button = ( Widget ) NULL;
static	Widget	Ldmping_clear_button = ( Widget ) NULL;
static	Widget	Ldmping_host_box = ( Widget ) NULL;
static	Widget	Ldmping_down_label = ( Widget ) NULL;
static	Widget	Ldmping_up_label = ( Widget ) NULL;
static	Widget	Ldmping_host_buttons[MAX_NUM_HOSTS];
static	Widget	Start_ping_button = ( Widget ) NULL;
static	Widget	Stop_ping_button = ( Widget ) NULL;
static	Widget	Ping_clear_button = ( Widget ) NULL;
static	Widget	Ping_host_box = ( Widget ) NULL;
static	Widget	Ping_down_label = ( Widget ) NULL;
static	Widget	Ping_up_label = ( Widget ) NULL;
static	Widget	Ping_host_buttons[MAX_NUM_HOSTS];
static	Widget	Ping_labels[MAX_NUMBER_PING_LINES];
static	Pixel	Bg_color = ( Pixel ) NULL;
static	Pixel	Button_color = ( Pixel ) NULL;
static	Pixel	Text_color = ( Pixel ) NULL;
static	Pixel	Edit_color = ( Pixel ) NULL;
static	Pixel	White_color = ( Pixel ) NULL;
static	Pixel	Green_color = ( Pixel ) NULL;
static	Pixel	Yellow_color = ( Pixel ) NULL;
static	Pixel	Red_color = ( Pixel ) NULL;
static	XmFontList List_font = ( XmFontList ) NULL;

/*	Global variables.		*/

static	char *Tab_titles[] = { "Skip", "LDMPING", "PING" };
static	char Ping_msgs[MAX_NUMBER_PING_LINES][MAX_PING_LINE_LENGTH+1];
static	char Remote_hosts[MAX_NUM_HOSTS][MAX_HOST_LENGTH+1];
static	int Num_remote_hosts = 0;
static	int Num_ping_msgs = 0;
static	int Current_ping_msg_index = 0;
static	void *Ping_cp = NULL;
static	int Ping_coprocess_flag = HCI_CP_NOT_STARTED;
static	int Ldmping_flag = HCI_NO_FLAG;
static	int Start_ping_flag = HCI_NO_FLAG;
static	char Popup_buf[HCI_BUF_1024];
static	int Waiting_to_close_flag = HCI_NO_FLAG;
static	int Waiting_to_destroy_flag = HCI_NO_FLAG;

/*	Function prototypes.		*/

static int  Destroy_callback();
static void Close_callback( Widget, XtPointer, XtPointer );
static void Get_remote_hosts();
static void Build_ldmping_tab();
static void Build_ping_tab();
static void Timer_proc();
static void Ldmping_callback( Widget, XtPointer, XtPointer );
static void Ldmping_clear_callback( Widget, XtPointer, XtPointer );
static void Ldmping_host_button_callback( Widget, XtPointer, XtPointer );
static void Start_ping_callback( Widget, XtPointer, XtPointer );
static void Stop_ping_callback( Widget, XtPointer, XtPointer );
static void Ping_clear_callback( Widget, XtPointer, XtPointer );
static void Ping_host_button_callback( Widget, XtPointer, XtPointer );
static void Execute_ldmping();
static void Update_ping_display();
static void Reset_ping_display();
static void Update_ping_label( Widget, int );
static void Start_ping_coprocess();
static void Manage_ping_coprocess();
static void Close_ping_coprocess();
static int  Coprocesses_are_running();
static void Coprocesses_terminate();
static void Set_busy_mode();
static void Unset_busy_mode();
static int  Actions_pending();

/************************************************************************
 Description: This is the main function for the task.
 ************************************************************************/

int main( int argc, char *argv [] )
{
  Widget form;
  Widget control_rowcol;
  int i;

  /* Initialize HCI. */

  HCI_partial_init( argc, argv, -1 );

  Top_widget = HCI_get_top_widget();

  HCI_set_destroy_callback( Destroy_callback );

  XtVaSetValues( Top_widget, XmNtitle, "Verify Network Connectivity", NULL );

  /* Initialize miscellaneous variables. */

  Bg_color = hci_get_read_color( BACKGROUND_COLOR1 );
  Text_color = hci_get_read_color( TEXT_FOREGROUND );
  Edit_color = hci_get_read_color( EDIT_BACKGROUND );
  Button_color = hci_get_read_color( BUTTON_BACKGROUND );
  White_color = hci_get_read_color( WHITE );
  Green_color = hci_get_read_color( GREEN );
  Yellow_color = hci_get_read_color( YELLOW );
  Red_color = hci_get_read_color( RED );
  List_font = hci_get_fontlist( LIST );

  /* Define a form widget to be used as the manager for widgets in *
   * the top level window.                                         */

  form = XtVaCreateManagedWidget( "form",
         xmFormWidgetClass, Top_widget,
         XmNbackground,     Bg_color,
         NULL );

  /* Use a rowcolumn widget at the top to manage the Close button. */

  control_rowcol = XtVaCreateManagedWidget( "control_rowcol",
           xmRowColumnWidgetClass, form,
           XmNbackground,          Bg_color,
           XmNorientation,         XmHORIZONTAL,
           XmNtopAttachment,       XmATTACH_FORM,
           XmNleftAttachment,      XmATTACH_FORM,
           XmNrightAttachment,     XmATTACH_FORM,
           NULL );

  Close_button = XtVaCreateManagedWidget( "Close",
           xmPushButtonWidgetClass, control_rowcol,
           XmNforeground,           White_color,
           XmNbackground,           Button_color,
           XmNfontList,             List_font,
           NULL );

  XtAddCallback( Close_button,
                 XmNactivateCallback, Close_callback, NULL );

  /* Use notebook widget to manage tabs. Each tab has a page,
     and each page is its own GUI. */

  Tab_widget = XtVaCreateWidget( "tab_widget",
           xmNotebookWidgetClass,   form,
           XmNtopAttachment,        XmATTACH_WIDGET,
           XmNtopWidget,            control_rowcol,
           XmNorientation,          XmVERTICAL,
           XmNbindingType,          XmNONE,
           XmNbackPagePlacement,    XmTOP_RIGHT,
           XmNbackPageNumber,       0,
           XmNbackPageSize,         0,
           XmNforeground,           Text_color,
           XmNbackground,           Bg_color,
           XmNfontList,             List_font,
           NULL );

  /* Get list of remote hosts and IPs. */

  Get_remote_hosts();

  /* Create pages. */

  for( i = FIRST_TAB; i < FIRST_TAB + NUM_TABS; i++ )
  {
    /* For each tab, call function to create page/GUI. */
    if( i == LDMPING_TAB ){ Build_ldmping_tab(); }
    else{ Build_ping_tab(); }

    /* Create tabs to pages above. */

    XtVaCreateManagedWidget( Tab_titles[i],
           xmPushButtonWidgetClass, Tab_widget,
           XmNnotebookChildType,    XmMAJOR_TAB,
           XmNforeground,           Text_color,
           XmNbackground,           Bg_color,
           XmNfontList,             List_font,
           NULL );
  }

  /* Remove page scroller widget. */

  XtUnmanageChild( XtNameToWidget( Tab_widget, "PageScroller" ) );

  /* Manage and display tab widget. */

  XtManageChild( Tab_widget );

  /* Finished creating widgets, realize top-level widget. */

  XtRealizeWidget( Top_widget );

  /* Start HCI loop. */

  HCI_start( Timer_proc, HCI_ONE_SECOND, NO_RESIZE_HCI );

  return 0;
}

/************************************************************************
 Description: This function is activated when the the window is destroyed.
 ************************************************************************/

static int Destroy_callback()
{
  HCI_LE_log( "Destroy callback" );

  /* Terminate coprocesses before exiting. Otherwise, zombie
     processes may occur. */

  if( Coprocesses_are_running() == HCI_YES_FLAG &&
      Waiting_to_destroy_flag == HCI_NO_FLAG )
  {
    HCI_LE_log( "Coprocesses running...try to terminate before exit" );
    Waiting_to_destroy_flag = HCI_YES_FLAG;
    Set_busy_mode();
    Coprocesses_terminate();
    return HCI_NOT_OK_TO_EXIT;
  }

  HCI_LE_log( "Destroy --- exit" );
  return HCI_OK_TO_EXIT;
}

/************************************************************************
 Description: This function is activated when the user selects
              the "Close" button.
 ************************************************************************/

static void Close_callback( Widget w, XtPointer y, XtPointer z )
{
  HCI_LE_log( "Close callback" );

  /* Terminate coprocesses before exiting. Otherwise, zombie
     processes may occur. */

  if( Coprocesses_are_running() == HCI_YES_FLAG &&
      Waiting_to_close_flag == HCI_NO_FLAG )
  {
    HCI_LE_log( "Coprocesses running...try to terminate before close" );
    Waiting_to_close_flag = HCI_YES_FLAG;
    Set_busy_mode();
    Coprocesses_terminate();
  }
  else
  {
    HCI_LE_log( "Close --- destroy" );
    XtDestroyWidget( Top_widget );
  }
}

/************************************************************************
 Description: Callback for timer event.
 ************************************************************************/

static void Timer_proc()
{
  /* Check state of various flags and take action accordingly. Setting
     flags and checking in the timer callback ensures widgets are updated
     in a consistent manner. */

  if( Ldmping_flag == HCI_YES_FLAG )
  {
    Ldmping_flag = HCI_NO_FLAG;
    Execute_ldmping();
  }

  if( Start_ping_flag == HCI_YES_FLAG )
  {
    Start_ping_flag = HCI_NO_FLAG;
    if( Ping_coprocess_flag == HCI_CP_NOT_STARTED &&
        Waiting_to_close_flag == HCI_NO_FLAG &&
        Waiting_to_destroy_flag == HCI_NO_FLAG )
    {
      Start_ping_coprocess();
    }
  }
  else if( Ping_coprocess_flag == HCI_CP_STARTED )
  {
    Manage_ping_coprocess();
  }
  else if( Ping_coprocess_flag == HCI_CP_FINISHED )
  {
    Ping_coprocess_flag = HCI_CP_NOT_STARTED;
  }

  /* Check flag to indicate GUI tried to close with coprocesses
     running. When coprocesses are terminated, retry. */

  if( Waiting_to_close_flag == HCI_YES_FLAG )
  {
    if( Coprocesses_are_running() == HCI_NO_FLAG )
    {
      Waiting_to_close_flag = HCI_NO_FLAG;
      Close_callback( NULL, NULL, NULL );
    }
  }

  /* Check flag to indicate GUI tried to end task with coprocesses
     running. When coprocesses are terminated, retry. */

  if( Waiting_to_destroy_flag == HCI_YES_FLAG )
  {
    if( Coprocesses_are_running() == HCI_NO_FLAG )
    {
      Waiting_to_destroy_flag = HCI_NO_FLAG;
      Destroy_callback();
    }
  }

  /* If no actions are pending, then get out of busy mode. */

  if( ! Actions_pending() ){ Unset_busy_mode(); }
}

/************************************************************************
 Description: Build ldmping widget.
 ************************************************************************/

static void Build_ldmping_tab()
{
  Widget form;
  Widget button_rowcol;
  Widget host_frame;
  Widget host_scroll;
  Widget host_scroll_form;
  Widget host_scroll_rowcol;
  Widget host_row_rowcol = (Widget) NULL;
  Widget host_clip;
  int i = -1;
  int j = -1;
  int start_index = -1;
  int len = -1;
  char buf[MAX_HOST_LENGTH+1];
  char *tok = NULL;

  form = XtVaCreateWidget( "form",
           xmFormWidgetClass,       Tab_widget,
           XmNnotebookChildType,    XmPAGE,
           XmNpageNumber,           LDMPING_TAB,
           XmNforeground,           Text_color,
           XmNbackground,           Bg_color,
           NULL );

  button_rowcol = XtVaCreateManagedWidget( "button_rowcol",
           xmRowColumnWidgetClass, form,
           XmNbackground,          Bg_color,
           XmNorientation,         XmHORIZONTAL,
           XmNtopAttachment,       XmATTACH_FORM,
           XmNleftAttachment,      XmATTACH_FORM,
           XmNrightAttachment,     XmATTACH_FORM,
           NULL );

  Ldmping_button = XtVaCreateManagedWidget( "Ldmping",
           xmPushButtonWidgetClass, button_rowcol,
           XmNforeground,           White_color,
           XmNbackground,           Button_color,
           XmNfontList,             List_font,
           NULL );

  XtAddCallback( Ldmping_button,
                 XmNactivateCallback, Ldmping_callback, NULL );

  XtVaCreateManagedWidget( "    Host:",
           xmLabelWidgetClass, button_rowcol,
           XmNforeground, Text_color,
           XmNbackground, Bg_color,
           XmNfontList, List_font,
           NULL );

  Ldmping_host_box = XtVaCreateManagedWidget( "ldmping_host_box",
           xmTextFieldWidgetClass, button_rowcol,
           XmNforeground, Text_color,
           XmNbackground, Edit_color,
           XmNfontList, List_font,
           XmNeditable, True,
           XmNvalue, "",
           XmNcolumns, MAX_HOST_LENGTH,
           XmNmaxLength, MAX_HOST_LENGTH,
           NULL);

  Ldmping_clear_button = XtVaCreateManagedWidget( "Clear",
           xmPushButtonWidgetClass, button_rowcol,
           XmNforeground,           White_color,
           XmNbackground,           Button_color,
           XmNfontList,             List_font,
           NULL );

  XtAddCallback( Ldmping_clear_button,
                 XmNactivateCallback, Ldmping_clear_callback, NULL );

  XtVaCreateManagedWidget( "    Status:",
           xmLabelWidgetClass, button_rowcol,
           XmNforeground, Text_color,
           XmNbackground, Bg_color,
           XmNfontList, List_font,
           NULL );

  Ldmping_down_label = XtVaCreateManagedWidget( " DOWN ",
           xmLabelWidgetClass, button_rowcol,
           XmNforeground, Text_color,
           XmNbackground, Bg_color,
           XmNfontList, List_font,
           NULL);

  Ldmping_up_label = XtVaCreateManagedWidget( "  UP  ",
           xmLabelWidgetClass, button_rowcol,
           XmNforeground, Text_color,
           XmNbackground, Bg_color,
           XmNfontList, List_font,
           NULL);

  host_frame = XtVaCreateManagedWidget( "host_frame",
           xmFrameWidgetClass,      form,
           XmNforeground,           Text_color,
           XmNbackground,           Bg_color,
           XmNtopAttachment,        XmATTACH_WIDGET,
           XmNtopWidget,            button_rowcol,
           XmNleftAttachment,       XmATTACH_FORM,
           XmNrightAttachment,      XmATTACH_FORM,
           XmNbottomAttachment,     XmATTACH_FORM,
           NULL );

  XtVaCreateManagedWidget( " SELECT REMOTE HOST ",
           xmLabelWidgetClass,      host_frame,
           XmNchildType,            XmFRAME_TITLE_CHILD,
           XmNchildHorizontalAlignment, XmALIGNMENT_CENTER,
           XmNchildVerticalAlignment, XmALIGNMENT_CENTER,
           XmNforeground,           Text_color,
           XmNbackground,           Bg_color,
           XmNfontList,             List_font,
           NULL );

  host_scroll = XtVaCreateManagedWidget( "host_scroll",
           xmScrolledWindowWidgetClass, host_frame,
           XmNwidth,                HOST_SCROLL_WIDTH,
           XmNscrollingPolicy,      XmAUTOMATIC,
           XmNforeground,           Text_color,
           XmNbackground,           Bg_color,
           NULL );

  XtVaGetValues( host_scroll, XmNclipWindow, &host_clip, NULL );
  XtVaSetValues( host_clip,
                 XmNbackground, Bg_color,
                 NULL );

  host_scroll_form = XtVaCreateWidget( "ldmping_scroll_form",
           xmFormWidgetClass,       host_scroll,
           XmNforeground,           Text_color,
           XmNbackground,           Bg_color,
           XmNverticalSpacing,      1,
           NULL );

  host_scroll_rowcol = XtVaCreateManagedWidget( "host_scroll_rowcol",
           xmRowColumnWidgetClass,  host_scroll_form,
           XmNforeground,           Text_color,
           XmNbackground,           Bg_color,
           XmNorientation,          XmVERTICAL,
           XmNpacking,              XmPACK_COLUMN,
           XmNnumColumns,           1,
           XmNleftAttachment,       XmATTACH_FORM,
           XmNrightAttachment,      XmATTACH_FORM,
           NULL );

  i = 0;
  while( i < Num_remote_hosts )
  {
    if( i % MAX_NUM_HOSTS_PER_LINE == 0 )
    {
      host_row_rowcol = XtVaCreateManagedWidget( "host_row_rowcol",
           xmRowColumnWidgetClass,  host_scroll_rowcol,
           XmNforeground,           Text_color,
           XmNbackground,           Bg_color,
           XmNorientation,          XmHORIZONTAL,
           XmNpacking,              XmPACK_COLUMN,
           XmNnumColumns,           1,
           XmNleftAttachment,       XmATTACH_FORM,
           XmNrightAttachment,      XmATTACH_FORM,
           NULL );
    }

    strcpy( buf, Remote_hosts[i] );
    if( ( tok = strtok( buf, "." ) ) != NULL ){ len = strlen( tok ); }
    else{ len = strlen( buf ); }
    if( len > MAX_HOST_BUTTON_LENGTH ){ len = MAX_HOST_BUTTON_LENGTH; }
    if( ( start_index = ( MAX_HOST_BUTTON_LENGTH - len ) / 2 ) <= 0 )
    {
      start_index = 0;
    }
    for( j = 0; j < start_index; j++ ){ buf[j] = ' '; }
    for( ; j < ( start_index + len ); j++ )
    {
      buf[j] = Remote_hosts[i][j-start_index];
    }
    for( ; j < MAX_HOST_BUTTON_LENGTH; j++ )
    {
      buf[j] = ' ';
    }
    buf[MAX_HOST_BUTTON_LENGTH] = '\0';

    Ldmping_host_buttons[i] = XtVaCreateManagedWidget( buf,
           xmPushButtonWidgetClass, host_row_rowcol,
           XmNforeground,           White_color,
           XmNbackground,           Button_color,
           XmNfontList,             List_font,
           NULL );

    XtAddCallback( Ldmping_host_buttons[i],
           XmNactivateCallback, Ldmping_host_button_callback, (XtPointer) i );

    i++;
  }

  XtManageChild( host_scroll_form );
  XtManageChild( form );
}

/************************************************************************
 Description: Build ping widget.
 ************************************************************************/

static void Build_ping_tab()
{
  Widget form;
  Widget button_rowcol;
  Widget host_frame;
  Widget host_scroll;
  Widget host_scroll_form;
  Widget host_scroll_rowcol;
  Widget host_row_rowcol = (Widget) NULL;
  Widget host_clip;
  Widget ping_frame;
  Widget ping_scroll;
  Widget ping_scroll_form;
  Widget ping_scroll_rowcol;
  Widget ping_clip;
  int i = -1;
  int j = -1;
  int start_index = -1;
  int len = -1;
  char buf[MAX_HOST_LENGTH+1];
  char *tok = NULL;

  form = XtVaCreateWidget( "form",
           xmFormWidgetClass,       Tab_widget,
           XmNnotebookChildType,    XmPAGE,
           XmNpageNumber,           PING_TAB,
           XmNforeground,           Text_color,
           XmNbackground,           Bg_color,
           NULL );

  button_rowcol = XtVaCreateManagedWidget( "button_rowcol",
           xmRowColumnWidgetClass, form,
           XmNbackground,          Bg_color,
           XmNorientation,         XmHORIZONTAL,
           XmNtopAttachment,       XmATTACH_FORM,
           XmNleftAttachment,      XmATTACH_FORM,
           XmNrightAttachment,     XmATTACH_FORM,
           NULL );

  Start_ping_button = XtVaCreateManagedWidget( "Start Ping",
           xmPushButtonWidgetClass, button_rowcol,
           XmNforeground,           White_color,
           XmNbackground,           Button_color,
           XmNfontList,             List_font,
           NULL );

  XtAddCallback( Start_ping_button,
                 XmNactivateCallback, Start_ping_callback, NULL );

  Stop_ping_button = XtVaCreateManagedWidget( "Stop Ping",
           xmPushButtonWidgetClass, button_rowcol,
           XmNforeground,           White_color,
           XmNbackground,           Button_color,
           XmNfontList,             List_font,
           XmNsensitive,            False,
           NULL );

  XtAddCallback( Stop_ping_button,
                 XmNactivateCallback, Stop_ping_callback, NULL );

  XtVaCreateManagedWidget( "    Host:",
           xmLabelWidgetClass, button_rowcol,
           XmNforeground, Text_color,
           XmNbackground, Bg_color,
           XmNfontList, List_font,
           NULL );

  Ping_host_box = XtVaCreateManagedWidget( "ping_host_box",
           xmTextFieldWidgetClass, button_rowcol,
           XmNforeground, Text_color,
           XmNbackground, Edit_color,
           XmNfontList, List_font,
           XmNeditable, True,
           XmNvalue, "",
           XmNcolumns, MAX_HOST_LENGTH,
           XmNmaxLength, MAX_HOST_LENGTH,
           NULL);

  Ping_clear_button = XtVaCreateManagedWidget( "Clear",
           xmPushButtonWidgetClass, button_rowcol,
           XmNforeground,           White_color,
           XmNbackground,           Button_color,
           XmNfontList,             List_font,
           NULL );

  XtAddCallback( Ping_clear_button,
                 XmNactivateCallback, Ping_clear_callback, NULL );

  XtVaCreateManagedWidget( "    Status:",
           xmLabelWidgetClass, button_rowcol,
           XmNforeground, Text_color,
           XmNbackground, Bg_color,
           XmNfontList, List_font,
           NULL );

  Ping_down_label = XtVaCreateManagedWidget( " DOWN ",
           xmLabelWidgetClass, button_rowcol,
           XmNforeground, Text_color,
           XmNbackground, Bg_color,
           XmNfontList, List_font,
           NULL);

  Ping_up_label = XtVaCreateManagedWidget( "  UP  ",
           xmLabelWidgetClass, button_rowcol,
           XmNforeground, Text_color,
           XmNbackground, Bg_color,
           XmNfontList, List_font,
           NULL);

  host_frame = XtVaCreateManagedWidget( "host_frame",
           xmFrameWidgetClass,      form,
           XmNforeground,           Text_color,
           XmNbackground,           Bg_color,
           XmNtopAttachment,        XmATTACH_WIDGET,
           XmNtopWidget,            button_rowcol,
           XmNleftAttachment,       XmATTACH_FORM,
           XmNrightAttachment,      XmATTACH_FORM,
           NULL );

  XtVaCreateManagedWidget( " SELECT REMOTE HOST ",
           xmLabelWidgetClass,      host_frame,
           XmNchildType,            XmFRAME_TITLE_CHILD,
           XmNchildHorizontalAlignment, XmALIGNMENT_CENTER,
           XmNchildVerticalAlignment, XmALIGNMENT_CENTER,
           XmNforeground,           Text_color,
           XmNbackground,           Bg_color,
           XmNfontList,             List_font,
           NULL );

  host_scroll = XtVaCreateManagedWidget( "host_scroll",
           xmScrolledWindowWidgetClass, host_frame,
           XmNheight,               HOST_SCROLL_HEIGHT,
           XmNwidth,                HOST_SCROLL_WIDTH,
           XmNscrollingPolicy,      XmAUTOMATIC,
           XmNforeground,           Text_color,
           XmNbackground,           Bg_color,
           NULL );

  XtVaGetValues( host_scroll, XmNclipWindow, &host_clip, NULL );
  XtVaSetValues( host_clip,
                 XmNbackground, Bg_color,
                 NULL );

  host_scroll_form = XtVaCreateWidget( "host_scroll_form",
           xmFormWidgetClass,       host_scroll,
           XmNforeground,           Text_color,
           XmNbackground,           Bg_color,
           XmNverticalSpacing,      1,
           NULL );

  host_scroll_rowcol = XtVaCreateManagedWidget( "host_scroll_rowcol",
           xmRowColumnWidgetClass,  host_scroll_form,
           XmNforeground,           Text_color,
           XmNbackground,           Bg_color,
           XmNorientation,          XmVERTICAL,
           XmNpacking,              XmPACK_COLUMN,
           XmNnumColumns,           1,
           XmNleftAttachment,       XmATTACH_FORM,
           XmNrightAttachment,      XmATTACH_FORM,
           NULL );

  i = 0;
  while( i < Num_remote_hosts )
  {
    if( i % MAX_NUM_HOSTS_PER_LINE == 0 )
    {
      host_row_rowcol = XtVaCreateManagedWidget( "host_row_rowcol",
           xmRowColumnWidgetClass,  host_scroll_rowcol,
           XmNforeground,           Text_color,
           XmNbackground,           Bg_color,
           XmNorientation,          XmHORIZONTAL,
           XmNpacking,              XmPACK_COLUMN,
           XmNnumColumns,           1,
           XmNleftAttachment,       XmATTACH_FORM,
           XmNrightAttachment,      XmATTACH_FORM,
           NULL );
    }

    strcpy( buf, Remote_hosts[i] );
    if( ( tok = strtok( buf, "." ) ) != NULL ){ len = strlen( tok ); }
    else{ len = strlen( buf ); }
    if( len > MAX_HOST_BUTTON_LENGTH ){ len = MAX_HOST_BUTTON_LENGTH; }
    if( ( start_index = ( MAX_HOST_BUTTON_LENGTH - len ) / 2 ) <= 0 )
    {
      start_index = 0;
    }
    for( j = 0; j < start_index; j++ ){ buf[j] = ' '; }
    for( ; j < ( start_index + len ); j++ )
    {
      buf[j] = Remote_hosts[i][j-start_index];
    }
    for( ; j < MAX_HOST_BUTTON_LENGTH; j++ )
    {
      buf[j] = ' ';
    }
    buf[MAX_HOST_BUTTON_LENGTH] = '\0';

    Ping_host_buttons[i] = XtVaCreateManagedWidget( buf,
           xmPushButtonWidgetClass, host_row_rowcol,
           XmNforeground,           White_color,
           XmNbackground,           Button_color,
           XmNfontList,             List_font,
           NULL );

    XtAddCallback( Ping_host_buttons[i],
           XmNactivateCallback, Ping_host_button_callback, (XtPointer) i );

    i++;
  }

  XtManageChild( host_scroll_form );

  ping_frame = XtVaCreateManagedWidget( "ping_frame",
           xmFrameWidgetClass,      form,
           XmNforeground,           Text_color,
           XmNbackground,           Bg_color,
           XmNtopAttachment,        XmATTACH_WIDGET,
           XmNtopWidget,            host_frame,
           XmNleftAttachment,       XmATTACH_FORM,
           XmNrightAttachment,      XmATTACH_FORM,
           NULL );

  XtVaCreateManagedWidget( " PING OUTPUT ",
           xmLabelWidgetClass,      ping_frame,
           XmNchildType,            XmFRAME_TITLE_CHILD,
           XmNchildHorizontalAlignment, XmALIGNMENT_CENTER,
           XmNchildVerticalAlignment, XmALIGNMENT_CENTER,
           XmNforeground,           Text_color,
           XmNbackground,           Bg_color,
           XmNfontList,             List_font,
           NULL );

  ping_scroll = XtVaCreateManagedWidget( "ping_scroll",
           xmScrolledWindowWidgetClass, ping_frame,
           XmNheight,               OUTPUT_SCROLL_HEIGHT,
           XmNwidth,                OUTPUT_SCROLL_WIDTH,
           XmNscrollingPolicy,      XmAUTOMATIC,
           XmNforeground,           Text_color,
           XmNbackground,           Bg_color,
           NULL );

  XtVaGetValues( ping_scroll, XmNclipWindow, &ping_clip, NULL );
  XtVaSetValues( ping_clip,
                 XmNbackground, Bg_color,
                 NULL );

  ping_scroll_form = XtVaCreateWidget( "ping_scroll_form",
           xmFormWidgetClass,       ping_scroll,
           XmNforeground,           Text_color,
           XmNbackground,           Bg_color,
           XmNverticalSpacing,      1,
           NULL );

  /* This rowcol ensures all labels are of equal width. */

  ping_scroll_rowcol = XtVaCreateManagedWidget( "ping_scroll_rowcol",
           xmRowColumnWidgetClass,  ping_scroll_form,
           XmNforeground,           Text_color,
           XmNbackground,           Bg_color,
           XmNorientation,          XmVERTICAL,
           XmNpacking,              XmPACK_COLUMN,
           XmNnumColumns,           1,
           XmNleftAttachment,       XmATTACH_FORM,
           XmNrightAttachment,      XmATTACH_FORM,
           NULL );

  /* Pre-define space for ping messages by creating label
     widgets. Label widgets will be used as a circular
     array. */

  Ping_labels[0] = XtVaCreateManagedWidget (" ",
           xmLabelWidgetClass,      ping_scroll_rowcol,
           XmNfontList,             List_font,
           XmNleftAttachment,       XmATTACH_FORM,
           XmNrightAttachment,      XmATTACH_FORM,
           XmNtopAttachment,        XmATTACH_FORM,
           XmNmarginHeight,         0,
           XmNborderWidth,          0,
           XmNshadowThickness,      0,
           XmNforeground,           Text_color,
           XmNbackground,           Bg_color,
           NULL);

  for( i = 1; i < MAX_NUMBER_PING_LINES; i++ )
  {
           Ping_labels[i] = XtVaCreateManagedWidget (" ",
           xmLabelWidgetClass,      ping_scroll_rowcol,
           XmNfontList,             List_font,
           XmNleftAttachment,       XmATTACH_FORM,
           XmNrightAttachment,      XmATTACH_FORM,
           XmNtopAttachment,        XmATTACH_WIDGET,
           XmNtopWidget,            Ping_labels[i-1],
           XmNmarginHeight,         0,
           XmNborderWidth,          0,
           XmNshadowThickness,      0,
           XmNforeground,           Text_color,
           XmNbackground,           Bg_color,
           NULL);
  }

  XtManageChild( ping_scroll_form );
  XtManageChild( form );
}

/************************************************************************
 Description: Callback to set flag to ldmping.
 ************************************************************************/

static void Ldmping_callback( Widget w, XtPointer y, XtPointer z )
{
  Ldmping_flag = HCI_YES_FLAG;
  XtVaSetValues( Ldmping_down_label, XmNbackground, Bg_color, XmNforeground, Text_color, NULL );
  XtVaSetValues( Ldmping_up_label, XmNbackground, Bg_color, XmNforeground, Text_color, NULL );
  Set_busy_mode();
}

/************************************************************************
 Description: Callback to set flag to start ping.
 ************************************************************************/

static void Start_ping_callback( Widget w, XtPointer y, XtPointer z )
{
  Start_ping_flag = HCI_YES_FLAG;
  Set_busy_mode();
  Reset_ping_display();
}

/************************************************************************
 Description: Callback to set flag to stop ping.
 ************************************************************************/

static void Stop_ping_callback( Widget w, XtPointer y, XtPointer z )
{
  if( Ping_coprocess_flag == HCI_CP_STARTED )
  {
    Close_ping_coprocess();
  }
}

/************************************************************************
 Description: Execute ldmping.
 ************************************************************************/

static void Execute_ldmping()
{
  int ret = -1;
  int len = -1;
  char cmd[HCI_BUF_256];
  char *host_buf = NULL;
  static int verify_ldmping_path = HCI_NO_FLAG;

  /* Make sure ldmping is in the path. */

  if( verify_ldmping_path == HCI_NO_FLAG )
  {
    verify_ldmping_path = HCI_YES_FLAG;
    sprintf( cmd, "which ldmping > /dev/null 2>&1" );
    ret = MISC_system_to_buffer( cmd, NULL, -1, &len );
    ret = ret >> 8;

    if( ret != 0 )
    {
      sprintf( Popup_buf, "Unable to find ldmping binary" );
      hci_error_popup( Top_widget, Popup_buf, NULL );
      HCI_LE_error( Popup_buf );
      return;
    }
  }

  /* Get host/ip to ldmping from input box. */

  host_buf = XmTextGetString( Ldmping_host_box );
  if( strlen( host_buf ) == 0 )
  {
    sprintf( Popup_buf, "No remote host input or selected" );
    hci_error_popup( Top_widget, Popup_buf, NULL );
    HCI_LE_error( Popup_buf );
    return;
  }
 
  /* Start co-process. */
  
  sprintf( cmd, "ldmping -t 5 -i 0 %s", host_buf );
  XtFree( host_buf );
 
  ret = MISC_system_to_buffer( cmd, NULL, -1, &len );
  ret = ret >> 8;

  if( ret == 0 )
  {
    XtVaSetValues( Ldmping_up_label,
                   XmNbackground, Green_color,
                   XmNforeground, Text_color, NULL );
  }
  else if( ret == 1 )
  {
    XtVaSetValues( Ldmping_down_label,
                   XmNbackground, Red_color,
                   XmNforeground, White_color,  NULL );
  }
  else
  {
    sprintf( Popup_buf, "ldmping failed (%d)", ret );
    hci_error_popup( Top_widget, Popup_buf, NULL );
    HCI_LE_error( Popup_buf );
  }
}

/************************************************************************
 Description: Start ping coprocess.
 ************************************************************************/

static void Start_ping_coprocess()
{
  int ret = -1;
  char cmd[HCI_BUF_256];
  char *host_buf = NULL;

  /* Get host/ip to ldmping from input box. */

  host_buf = XmTextGetString( Ping_host_box );
  if( strlen( host_buf ) == 0 )
  {
    sprintf( Popup_buf, "No remote host input or selected" );
    hci_error_popup( Top_widget, Popup_buf, NULL );
    HCI_LE_error( Popup_buf );
    return;
  }
 
  /* Reset scroll display. */
  
  Num_ping_msgs = 0;
  Current_ping_msg_index = -1;
  
  /* Start co-process. */
  
  sprintf( cmd, "ping -i 3 -W 3 %s", host_buf );
  XtFree( host_buf );
  
  ret = MISC_cp_open( cmd, HCI_CP_MANAGE, &Ping_cp );

  if( ret != 0 )
  {
    Ping_coprocess_flag = HCI_CP_FINISHED;
    sprintf( Popup_buf, "Unable to start ping process (%d)", ret );
    hci_error_popup( Top_widget, Popup_buf, NULL );
    HCI_LE_error( Popup_buf );
  }
  else
  {
    Ping_coprocess_flag = HCI_CP_STARTED;
  }
}

/************************************************************************
 Description: Retrieve latest output from ping coprocess.
 ************************************************************************/

static void Manage_ping_coprocess()
{
  int ret = -1;
  char buf[HCI_BUF_512];
  int new_msgs = HCI_NO_FLAG;

  /* Read coprocess to see if any new data is available. */

  while( Ping_cp != NULL &&
         ( ret = MISC_cp_read_from_cp( Ping_cp, buf, HCI_BUF_512 ) ) != 0 )
  {
    /* Strip trailing newline. */
    if( buf[strlen( buf )-1] == '\n' ){ buf[strlen(buf)-1] = '\0'; }

    /* Take action according to value of ret. */
    if( ret == HCI_CP_DOWN )
    {
      HCI_LE_log( "Ping coprocess DOWN" );
      if( ( ret = ( MISC_cp_get_status( Ping_cp ) >> 8 ) ) != 0 )
      {
        sprintf( Popup_buf, "Ping coprocess terminated with error (%d)", ret );
        /*hci_error_popup( Top_widget, Popup_buf, NULL );*/
        HCI_LE_error( Popup_buf );
      }
      Close_ping_coprocess();
      break;
    }
    else if( ret == HCI_CP_STDERR || ret == HCI_CP_STDOUT )
    {
      /* Set flag to indicate new ping messages. */
      new_msgs = HCI_YES_FLAG;
      /* Update starting label position (circular array). */
      Current_ping_msg_index++;
      Current_ping_msg_index%=MAX_NUMBER_LDMPING_LINES;
      /* Update number of labels (not to exceed maximum number allowed). */
      Num_ping_msgs++;
      if( Num_ping_msgs > MAX_NUMBER_LDMPING_LINES ){ Num_ping_msgs--; }
      /* This line needs to be displayed in ping scroll box. */
      strncpy( Ping_msgs[Current_ping_msg_index], buf, MAX_LDMPING_LINE_LENGTH );
      if( strlen( buf ) > MAX_LDMPING_LINE_LENGTH )
      {
        Ping_msgs[Current_ping_msg_index][MAX_LDMPING_LINE_LENGTH] = '\0';
      }
    }
    else
    {
      /* No new data available, do nothing... */
    }
  }

  /* If new messages available, update scroll with latest info. */

  if( new_msgs )
  {
    Update_ping_display();
  }
}

/************************************************************************
 Description: Close ping coprocess.
 ************************************************************************/

static void Close_ping_coprocess()
{
  MISC_cp_close( Ping_cp );
  Ping_cp = NULL;
  Ping_coprocess_flag = HCI_CP_FINISHED;
}

/************************************************************************
 Description: Update labels on ping scroll with latest info.
 ************************************************************************/

static void Update_ping_display()
{
  int i = -1;
  int temp_int = 0;

  /* The latest message is located at position current_msg_pos.
     Start there and work down to index 0. */

  for( i = Current_ping_msg_index; i >= 0; i-- )
  {
    Update_ping_label( Ping_labels[i], temp_int );
    temp_int++;
  }

  /* If the number of output messages has reached the maximum,
     start at the last array position and work down to the
     index just above the current_msg_pos index. This is done
     to allow the array to be treated as circular. */

  if( Num_ping_msgs == MAX_NUMBER_PING_LINES )
  {
    for( i = MAX_NUMBER_PING_LINES - 1; i > Current_ping_msg_index; i-- )
    {
      Update_ping_label( Ping_labels[i], temp_int );
      temp_int++;
    }
  }
}

/************************************************************************
 Description: Reset ping display widgets.
 ************************************************************************/

static void Reset_ping_display()
{
  int i = -1;
  XmString msg;

  XtVaSetValues( Ping_down_label, XmNbackground, Bg_color, XmNforeground, Text_color, NULL );
  XtVaSetValues( Ping_up_label, XmNbackground, Bg_color, XmNforeground, Text_color, NULL );

  for( i = 0; i < MAX_NUMBER_PING_LINES; i++ )
  {
    msg = XmStringCreateLtoR( " ", XmFONTLIST_DEFAULT_TAG );
    XtVaSetValues( Ping_labels[i],
               XmNlabelString, msg,
               XmNforeground, Bg_color,
               XmNbackground, Bg_color,
               NULL );
    XmStringFree( msg );
  }
}

/************************************************************************
 Description: Update label widget. Highlight start/end of volumes.
 ************************************************************************/

static void Update_ping_label( Widget w, int index )
{
  int bg_color;
  int fg_color;

  /* Convert c-buf to XmString variable. */

  XmString msg = XmStringCreateLtoR( Ping_msgs[index], XmFONTLIST_DEFAULT_TAG );

  /* Determine color. */

  if( strstr( Ping_msgs[index], " bytes from " ) != NULL )
  {
    fg_color = Text_color;
    bg_color = Green_color;
    XtVaSetValues( Ping_up_label,
                   XmNbackground, Green_color,
                   XmNforeground, Text_color, NULL );
    XtVaSetValues( Ping_down_label,
                   XmNbackground, Bg_color,
                   XmNforeground, Text_color, NULL );
  }
  else if( strstr( Ping_msgs[index], "Destination Host Unreachable" ) != NULL )
  {
    fg_color = White_color;
    bg_color = Red_color;
    XtVaSetValues( Ping_down_label,
                   XmNbackground, Red_color,
                   XmNforeground, White_color, NULL );
    XtVaSetValues( Ping_up_label,
                   XmNbackground, Bg_color,
                   XmNforeground, Text_color, NULL );
  }
  else if( strstr( Ping_msgs[index], "ping: unknown host" ) != NULL )
  {
    fg_color = White_color;
    bg_color = Red_color;
    XtVaSetValues( Ping_down_label,
                   XmNbackground, Red_color,
                   XmNforeground, White_color, NULL );
    XtVaSetValues( Ping_up_label,
                   XmNbackground, Bg_color,
                   XmNforeground, Text_color, NULL );
  }
  else
  {
    fg_color = Text_color;
    bg_color = Bg_color;
  }

  /* Modify appropriate output label. */

  XtVaSetValues( w,
                 XmNlabelString, msg,
                 XmNforeground, fg_color,
                 XmNbackground, bg_color,
                 NULL );

  /* Free allocated space. */

  XmStringFree( msg );
}

/************************************************************************
 Description: Check if any coprocesses are currently running.
 ************************************************************************/

static int Coprocesses_are_running()
{
  if(  Ping_coprocess_flag != HCI_CP_NOT_STARTED )
  {
    return HCI_YES_FLAG;
  }

  return HCI_NO_FLAG;
}

/************************************************************************
 Description: Terminate all currently running coprocess.
 ************************************************************************/

static void Coprocesses_terminate()
{
  if( Ping_coprocess_flag != HCI_CP_NOT_STARTED )
  {
    Close_ping_coprocess();
  }
}

/************************************************************************
 Description: Go into busy mode by setting cursor to busy and 
              de-sensitizing widgets.
 ************************************************************************/

static void Set_busy_mode()
{
  int i;
  HCI_busy_cursor();
  XtSetSensitive( Close_button, False );
  XtSetSensitive( Ldmping_button, False );
  XtSetSensitive( Ldmping_clear_button, False );
  XtSetSensitive( Ldmping_host_box, False );
  XtSetSensitive( Ldmping_down_label, False );
  XtSetSensitive( Ldmping_up_label, False );
  XtSetSensitive( Start_ping_button, False );
  XtSetSensitive( Stop_ping_button, True );
  XtSetSensitive( Ping_clear_button, False );
  XtSetSensitive( Ping_host_box, False );
  for( i = 0; i < Num_remote_hosts; i++ )
  {
    XtSetSensitive( Ldmping_host_buttons[i], False );
  }
  for( i = 0; i < Num_remote_hosts; i++ )
  {
    XtSetSensitive( Ping_host_buttons[i], False );
  }
}

/************************************************************************
 Description: Go out of busy mode by setting cursor to normal and 
              re-sensitizing widgets.
 ************************************************************************/

static void Unset_busy_mode()
{
  int i;
  HCI_default_cursor();
  XtSetSensitive( Close_button, True );
  XtSetSensitive( Ldmping_button, True );
  XtSetSensitive( Ldmping_clear_button, True );
  XtSetSensitive( Ldmping_host_box, True );
  XtSetSensitive( Ldmping_down_label, True );
  XtSetSensitive( Ldmping_up_label, True );
  XtSetSensitive( Start_ping_button, True );
  XtSetSensitive( Stop_ping_button, False );
  XtSetSensitive( Ping_clear_button, True );
  XtSetSensitive( Ping_host_box, True );
  for( i = 0; i < Num_remote_hosts; i++ )
  {
    XtSetSensitive( Ldmping_host_buttons[i], True );
  }
  for( i = 0; i < Num_remote_hosts; i++ )
  {
    XtSetSensitive( Ping_host_buttons[i], True );
  }
}

/************************************************************************
 Description: Return HCI_YES_FLAG if any actions are pending, HCI_NO_FLAG
              otherwise.
 ************************************************************************/

static int Actions_pending()
{
  if( Ldmping_flag == HCI_YES_FLAG ||
      Start_ping_flag == HCI_YES_FLAG ||
      Coprocesses_are_running() ||
      Waiting_to_close_flag == HCI_YES_FLAG ||
      Waiting_to_destroy_flag == HCI_YES_FLAG )
  {
    return HCI_YES_FLAG;
  }

  return HCI_NO_FLAG;
}

/************************************************************************
 Description: Get list of remote hosts and IPs to ldmping/ping.
 ************************************************************************/

static void Get_remote_hosts()
{
  int ret = -1;
  int len = -1;
  int i = -1;
  char *tok = NULL;
  char buf[MAX_HOSTS_BUF_LENGTH];
  char *cmd = "sh -c (egrep '^[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+[[:space:]].*' /etc/hosts | awk '{print $2};')";

  /* Reset number of remote hosts. */

  Num_remote_hosts = 0;

  /* Run command to get list of remote hosts from /etc/hosts. */

  ret = MISC_system_to_buffer( cmd, buf, MAX_HOSTS_BUF_LENGTH, &len );
  ret = ret >> 8;

  if( ret != 0 )
  {
    sprintf( Popup_buf, "MISC_system_to_buffer failed (%d)", ret );
    hci_error_popup( Top_widget, Popup_buf, NULL );
    HCI_LE_error( Popup_buf );
    return;
  }
  else if( len < 1 )
  {
    sprintf( Popup_buf, "Command (%s) returned no data", cmd );
    hci_error_popup( Top_widget, Popup_buf, NULL );
    HCI_LE_error( Popup_buf );
    return;
  }

  /* Parse command output to get remote hosts. */

  if( ( tok = strtok( buf, " \n" ) ) != NULL && strlen( tok ) > 0 )
  {
    if( strlen( tok ) >= MAX_HOST_LENGTH )
    {
      strncpy( Remote_hosts[Num_remote_hosts], tok, MAX_HOST_LENGTH );
      Remote_hosts[Num_remote_hosts][MAX_HOST_LENGTH] = '\0';
    }
    else
    {
      strcpy( Remote_hosts[Num_remote_hosts], tok );
    }
    for( i = 0; i < strlen( Remote_hosts[Num_remote_hosts] ); i++ )
    {
      Remote_hosts[Num_remote_hosts][i] = toupper( Remote_hosts[Num_remote_hosts][i] );
    }
    Num_remote_hosts++;

    while( ( tok = strtok( NULL, " \n" ) ) != NULL && strlen( tok ) > 0 )
    {
      if( strlen( tok ) >= MAX_HOST_LENGTH )
      {
        strncpy( Remote_hosts[Num_remote_hosts], tok, MAX_HOST_LENGTH );
        Remote_hosts[Num_remote_hosts][MAX_HOST_LENGTH] = '\0';
      }
      else
      {
        strcpy( Remote_hosts[Num_remote_hosts], tok );
      }
      for( i = 0; i < strlen( Remote_hosts[Num_remote_hosts] ); i++ )
      {
        Remote_hosts[Num_remote_hosts][i] = toupper( Remote_hosts[Num_remote_hosts][i] );
      }
      Num_remote_hosts++;
      if( Num_remote_hosts > MAX_NUM_HOSTS )
      {
        HCI_LE_log( "Max number of remote hosts reached" );
      }
    }
  }

  if( Num_remote_hosts == 0 )
  {
    sprintf( Popup_buf, "No remote hosts found in /etc/hosts" );
    hci_error_popup( Top_widget, Popup_buf, NULL );
    HCI_LE_error( Popup_buf );
  }
}

/************************************************************************
 Description: Callback for ldmping host button.
 ************************************************************************/

static void Ldmping_host_button_callback( Widget w, XtPointer y, XtPointer z )
{
  int index = (int) y;

  XmTextSetString( Ldmping_host_box, Remote_hosts[index] );
  XtVaSetValues( Ldmping_down_label, XmNbackground, Bg_color, XmNforeground, Text_color, NULL );
  XtVaSetValues( Ldmping_up_label, XmNbackground, Bg_color, XmNforeground, Text_color, NULL );
}

/************************************************************************
 Description: Callback for ping host button.
 ************************************************************************/

static void Ping_host_button_callback( Widget w, XtPointer y, XtPointer z )
{
  int index = (int) y;

  XmTextSetString( Ping_host_box, Remote_hosts[index] );
  Reset_ping_display();
}

/************************************************************************
 Description: Callback for ldmping Clear button.
 ************************************************************************/

static void Ldmping_clear_callback( Widget w, XtPointer y, XtPointer z )
{
  XmTextSetString( Ldmping_host_box, "" );
  XtVaSetValues( Ldmping_down_label, XmNbackground, Bg_color, XmNforeground, Text_color, NULL );
  XtVaSetValues( Ldmping_up_label, XmNbackground, Bg_color, XmNforeground, Text_color, NULL );
}

/************************************************************************
 Description: Callback for ping Clear button.
 ************************************************************************/

static void Ping_clear_callback( Widget w, XtPointer y, XtPointer z )
{
  XmTextSetString( Ping_host_box, "" );
  Reset_ping_display();
}

