/************************************************************************
 *									*
 *	Module:  save_log_gui.c						*
 *									*
 *	Description:  This module provides a gui wrapper for the saving *
 *		      of log/process/system data to media.		*
 *									*
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/01/25 16:00:19 $
 * $Id: save_log_gui.c,v 1.13 2012/01/25 16:00:19 ccalvert Exp $
 * $Revision: 1.13 $
 * $State: Exp $
 */

/*	Local include file definitions.			*/

#include <hci.h>

/*	Macros.		*/

#define MAX_READ_BUF_SIZE	2048
#define IP_ADDRESS_LENGTH	64
#define	NUM_VISIBLE_ITEMS	20
#define MAX_CMD_LENGTH		256
#define	MAX_FILENAME_LENGTH	256
#define	MAX_MSG_BUF_SIZE	MAX_READ_BUF_SIZE + MAX_CMD_LENGTH
#define MAX_DESCRIPTION_LENGTH	30
#define MAX_DESCRIPTION_DISPLAY	25
#define MAX_NUM_NODES		5
#define MAX_FILES_PER_NODE	30
#define MAX_NUM_FILES		MAX_FILES_PER_NODE * MAX_NUM_NODES
#define MAX_DATE_LENGTH		9
#define MAX_DISPLAY_LINE_LENGTH	64
#define	MAX_SAVE_LOG_DIR_LENGTH	64

typedef struct {
  char	source_node[ 5 ];      /* Node. */
  char	source_nodename[ 10 ]; /* Node name. */
  int	source_channel;        /* Node channel number. */
  char  source_ip[ IP_ADDRESS_LENGTH ]; /* Node IP. */
  int	detected;              /* Node not found flag. */
  int	is_local;              /* Is local node. */
  int	save_log_failed;       /* Successfully created save log. */
  int	num_files;
  char	filenames[ MAX_FILES_PER_NODE ][ MAX_FILENAME_LENGTH ]; 
  char	filename_descriptions[ MAX_FILES_PER_NODE ][ MAX_DESCRIPTION_LENGTH ]; 
} Node_info_t;

typedef struct {
  char	file_desc[ MAX_DESCRIPTION_LENGTH ];
  char	file_date[ MAX_DATE_LENGTH ];
  int	total_file_size;       /* Total size in bytes. */
  int	selected;
} Description_info_t;

/*	Global widget definitions.	*/

static	Widget		Top_widget = ( Widget ) NULL;
static	Widget		Description_box = ( Widget ) NULL;
static	Widget		Start_button = ( Widget ) NULL;
static	Widget		Copy_button = ( Widget ) NULL;
static	Widget		Update_button = ( Widget ) NULL;
static	Widget		File_select_list = ( Widget ) NULL;
static	Widget		Selected_files_size_label = ( Widget ) NULL;

/*	Global variables.	*/

static	int	FAA_flag = NO;               /* Is this FAA system? */
static	int	Num_nodes = -1;              /* Number of nodes. */
static	int	Start_save_log_flag = NO;    /* Start save log? */
static	int	Update_file_list_flag = NO;  /* Update file list? */
static	int	Start_copy_flag = NO;        /* Start save log copy? */
static	char    Save_log_dir[ MAX_SAVE_LOG_DIR_LENGTH ];
static	char	Msg_buf[ MAX_MSG_BUF_SIZE ] = "";
static	char	Description_string[ MAX_DESCRIPTION_LENGTH ] = "";
static	char    File_list_buffer[ MAX_READ_BUF_SIZE ];
static	int	Num_descriptions = 0; 
static	int	Num_selected_descriptions = 0; 
static	int	File_size_sum = 0; 
static	char	*Temp_log_directory = "/tmp/save_log_temp";
static	Description_info_t	File_descriptions[ MAX_NUM_FILES ];
static	Node_info_t		Node_info[ MAX_NUM_NODES ];

/*	Function prototypes.		*/

static int Destroy_callback( Widget, XtPointer, XtPointer );
static void Close_callback( Widget, XtPointer, XtPointer );
static void Verify_description_input( Widget, XtPointer, XtPointer );
static void Start_callback( Widget, XtPointer, XtPointer );
static void Copy_callback( Widget, XtPointer, XtPointer );
static void Success_callback( Widget, XtPointer, XtPointer );
static void Update_callback( Widget, XtPointer, XtPointer );
static void List_callback( Widget, XtPointer, XtPointer );
static int  Rmt_transmit_data_callback( rmt_transfer_event_t * );
static void Initialize_node_info();
static void Save_log_error_popup();
static void Save_log_finished_popup();
static void Start_save_log();
static void Start_save_log_copy();
static void Save_log_local( Node_info_t * );
static void Save_log_remote( Node_info_t * );
static void Get_file_list_local( Node_info_t * );
static void Get_file_list_remote( Node_info_t * );
static void Parse_file_list( Node_info_t * );
static void Add_description( char *, char *, int );
static int  Get_file_record( char *, char *,char *, char *, int * );
static void Reset_gui();
static void Update_file_list();
static void Update_file_size_counter();
static void Test_source_node_connectivity( Node_info_t * );
static int  Get_description();
static void Timer_proc();

/************************************************************************
 *	Description: This is the main function for the Save Log Task.	*
 *									*
 *	Input:  argc - number of commandline arguments			*
 *		argv - pointer to commandline argument data		*
 *	Output: NONE							*
 *	Return: exit_code						*
 ************************************************************************/

int main( int argc, char *argv [] )
{
  Widget form;
  Widget frame;
  Widget frame_rowcol;
  Widget control_rowcol;
  Widget rowcol;
  Widget button;
  Widget label;

  /* Initialize HCI. */

  HCI_partial_init( argc, argv, -1 );

  Top_widget = HCI_get_top_widget();

  HCI_set_destroy_callback( Destroy_callback );

  XtVaSetValues( Top_widget, XmNtitle, "Save Log", NULL );

  if( HCI_get_system() == HCI_FAA_SYSTEM ){ FAA_flag = YES; }

  /* Ensure ORPGDAT_HCI_DATA has write permission. */

  ORPGDA_write_permission( ORPGDAT_HCI_DATA );

  /* Set save log directory. */

  sprintf( Save_log_dir, "%s/%s", getenv( "HOME" ), "save_logs" );

  /* Initialize node information. */

  Initialize_node_info();

  /* Use a form widget to be used as the manager for
     widgets in the top level window. */

  form = XtVaCreateManagedWidget( "form",
           xmFormWidgetClass,	Top_widget,
           XmNbackground,	hci_get_read_color( BACKGROUND_COLOR1 ),
           NULL );

  /* Use a rowcolumn widget at the top to manage the Close button. */

  control_rowcol = XtVaCreateManagedWidget( "control_rowcol",
       xmRowColumnWidgetClass,	form,
       XmNbackground,		hci_get_read_color( BACKGROUND_COLOR1 ),
       XmNorientation,		XmHORIZONTAL,
       XmNpacking,		XmPACK_TIGHT,
       XmNtopAttachment,	XmATTACH_FORM,
       XmNleftAttachment,	XmATTACH_FORM,
       XmNrightAttachment,	XmATTACH_FORM,
       NULL );

  button = XtVaCreateManagedWidget( "Close",
          xmPushButtonWidgetClass, control_rowcol,
          XmNforeground,	hci_get_read_color( BUTTON_FOREGROUND ),
          XmNbackground,	hci_get_read_color( BUTTON_BACKGROUND ),
          XmNfontList,		hci_get_fontlist( LIST ),
          NULL );

  XtAddCallback( button,
                 XmNactivateCallback, Close_callback, NULL );

  /* Add frame for border. */

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
          XmNbackground,          hci_get_read_color( BACKGROUND_COLOR1 ),                XmNorientation,         XmVERTICAL,
          XmNpacking,             XmPACK_TIGHT,
          XmNnumColumns,          1,
          XmNentryAlignment,      XmALIGNMENT_CENTER,
          NULL );

  /* Build button for user to start save log process. */

  rowcol = XtVaCreateManagedWidget( "submit_rowcol",
        xmRowColumnWidgetClass, frame_rowcol,
        XmNbackground,          hci_get_read_color( BACKGROUND_COLOR1 ),
        XmNorientation,         XmHORIZONTAL,
        XmNpacking,             XmPACK_TIGHT,
        XmNnumColumns,          1,
        XmNentryAlignment,      XmALIGNMENT_CENTER,
        NULL );

  label = XtVaCreateManagedWidget( "To Begin Save Log Process Click",
          xmLabelWidgetClass,	rowcol,
          XmNforeground,	hci_get_read_color( TEXT_FOREGROUND ),
          XmNbackground,	hci_get_read_color( BACKGROUND_COLOR1 ),
          XmNfontList,		hci_get_fontlist( LIST ),
          NULL );

  Start_button = XtVaCreateManagedWidget( "Start",
          xmPushButtonWidgetClass, rowcol,
          XmNforeground,	hci_get_read_color( BUTTON_FOREGROUND ),
          XmNbackground,	hci_get_read_color( BUTTON_BACKGROUND ),
          XmNfontList,		hci_get_fontlist( LIST ),
          NULL );

  XtAddCallback( Start_button,
                 XmNactivateCallback, Start_callback, NULL );

  /* Build text field to hold description of save log file. */

  rowcol = XtVaCreateManagedWidget( "save_log_filename_rowcol",
        xmRowColumnWidgetClass, frame_rowcol,
        XmNbackground,          hci_get_read_color( BACKGROUND_COLOR1 ),
        XmNorientation,         XmHORIZONTAL,
        XmNpacking,             XmPACK_TIGHT,
        XmNnumColumns,          1,
        XmNentryAlignment,      XmALIGNMENT_CENTER,
        NULL );

  label = XtVaCreateManagedWidget( "Description: ",
          xmLabelWidgetClass,	rowcol,
          XmNforeground,	hci_get_read_color( TEXT_FOREGROUND ),
          XmNbackground,	hci_get_read_color( BACKGROUND_COLOR1 ),
          XmNfontList,		hci_get_fontlist( LIST ),
          NULL );

  Description_box = XtVaCreateManagedWidget( "Description_box",
      xmTextFieldWidgetClass,  rowcol,
      XmNvalue,                "",
      XmNbackground,           hci_get_read_color( EDIT_BACKGROUND ),
      XmNforeground,           hci_get_read_color( EDIT_FOREGROUND ),
      XmNcolumns,              MAX_DESCRIPTION_DISPLAY,
      XmNmaxLength,            MAX_DESCRIPTION_DISPLAY,
      XmNfontList,             hci_get_fontlist( LIST ),
      XmNsensitive,            True,
      NULL );

  XtAddCallback( Description_box, XmNmodifyVerifyCallback,
                 Verify_description_input, NULL );

  /* Empty space for buffer. */

  label = XtVaCreateManagedWidget( " ",
          xmLabelWidgetClass,	frame_rowcol,
          XmNforeground,	hci_get_read_color( TEXT_FOREGROUND ),
          XmNbackground,	hci_get_read_color( BACKGROUND_COLOR1 ),
          XmNfontList,		hci_get_fontlist( LIST ),
          NULL );

  /* Add button to update file list. */

  rowcol = XtVaCreateManagedWidget( "update_list_rowcol",
        xmRowColumnWidgetClass, frame_rowcol,
        XmNbackground,          hci_get_read_color( BACKGROUND_COLOR1 ),
        XmNorientation,         XmHORIZONTAL,
        XmNpacking,             XmPACK_TIGHT,
        XmNnumColumns,          1,
        XmNentryAlignment,      XmALIGNMENT_CENTER,
        NULL );

  label = XtVaCreateManagedWidget( "                  ",
          xmLabelWidgetClass,	rowcol,
          XmNforeground,	hci_get_read_color( TEXT_FOREGROUND ),
          XmNbackground,	hci_get_read_color( BACKGROUND_COLOR1 ),
          XmNfontList,		hci_get_fontlist( LIST ),
          NULL );

  Update_button = XtVaCreateManagedWidget( "Update File List",
          xmPushButtonWidgetClass, rowcol,
          XmNforeground,	hci_get_read_color( BUTTON_FOREGROUND ),
          XmNbackground,	hci_get_read_color( BUTTON_BACKGROUND ),
          XmNfontList,		hci_get_fontlist( LIST ),
          NULL );

  XtAddCallback( Update_button,
                 XmNactivateCallback, Update_callback, NULL );

  /* Build selection box for files. */

  label = XtVaCreateManagedWidget( "  Date                Description               Size   ",
          xmLabelWidgetClass,	frame_rowcol,
          XmNforeground,	hci_get_read_color( TEXT_FOREGROUND ),
          XmNbackground,	hci_get_read_color( BACKGROUND_COLOR1 ),
          XmNfontList,		hci_get_fontlist( LIST ),
          NULL );

  File_select_list = XmCreateScrolledList( frame_rowcol, "sel_box", NULL, 0 );

  XtVaSetValues( File_select_list,
          XmNvisibleItemCount, NUM_VISIBLE_ITEMS,
          XmNforeground,       hci_get_read_color( TEXT_FOREGROUND ),
          XmNbackground,       hci_get_read_color( BACKGROUND_COLOR2 ),
          XmNfontList,         hci_get_fontlist( LIST ),
          XmNscrollBarDisplayPolicy, XmAS_NEEDED,
          XmNselectionPolicy,  XmMULTIPLE_SELECT, 
          NULL );

  XtAddCallback( File_select_list,
                 XmNmultipleSelectionCallback, List_callback, NULL );

  XtManageChild( File_select_list );


  /* Build label to show size of selected files to burn to CD. */

  rowcol = XtVaCreateManagedWidget( "file_size_rowcol",
        xmRowColumnWidgetClass, frame_rowcol,
        XmNbackground,          hci_get_read_color( BACKGROUND_COLOR1 ),
        XmNorientation,         XmHORIZONTAL,
        XmNpacking,             XmPACK_TIGHT,
        XmNnumColumns,          1,
        XmNentryAlignment,      XmALIGNMENT_CENTER,
        NULL );

  Selected_files_size_label = XtVaCreateManagedWidget( "Total size of files selected is 0 bytes.",
          xmLabelWidgetClass,   rowcol,
          XmNforeground,        hci_get_read_color( TEXT_FOREGROUND ),
          XmNbackground,        hci_get_read_color( BACKGROUND_COLOR1 ),
          XmNfontList,          hci_get_fontlist( LIST ),
          NULL );

  /* Build button for user to copy log files to CD. */

  rowcol = XtVaCreateManagedWidget( "copy_rowcol",
        xmRowColumnWidgetClass, frame_rowcol,
        XmNbackground,          hci_get_read_color( BACKGROUND_COLOR1 ),
        XmNorientation,         XmHORIZONTAL,
        XmNpacking,             XmPACK_TIGHT,
        XmNnumColumns,          1,
        XmNentryAlignment,      XmALIGNMENT_CENTER,
        NULL );

  label = XtVaCreateManagedWidget( "To Copy Selected Files, Insert CD and Click",
          xmLabelWidgetClass,	rowcol,
          XmNforeground,	hci_get_read_color( TEXT_FOREGROUND ),
          XmNbackground,	hci_get_read_color( BACKGROUND_COLOR1 ),
          XmNfontList,		hci_get_fontlist( LIST ),
          NULL );

  Copy_button = XtVaCreateManagedWidget( "Copy",
          xmPushButtonWidgetClass, rowcol,
          XmNforeground,	hci_get_read_color( BUTTON_FOREGROUND ),
          XmNbackground,	hci_get_read_color( BUTTON_BACKGROUND ),
          XmNfontList,		hci_get_fontlist( LIST ),
          NULL );

  XtAddCallback( Copy_button,
                 XmNactivateCallback, Copy_callback, NULL );

  XtRealizeWidget( Top_widget );

  /* Initialize info to display. */

  Update_file_list();

  HCI_start( Timer_proc, HCI_ONE_AND_HALF_SECOND, NO_RESIZE_HCI );

  if( FAA_flag )
  {
    RMT_listen_for_progress( Rmt_transmit_data_callback, NULL );
  }

  exit( 0 );
}

/************************************************************************
 * Destroy_callback: Destroys Top_widget.
 ************************************************************************/

static int Destroy_callback( Widget w, XtPointer x, XtPointer y )
{
  HCI_LE_log( "In destroy callback" );
  return HCI_OK_TO_EXIT;
}

/************************************************************************
 * Close_callback: Callback for "Close" button.
 ************************************************************************/

static void Close_callback( Widget w, XtPointer x, XtPointer y )
{
  HCI_LE_log( "In close callback" );
  XtDestroyWidget( Top_widget );
}

/************************************************************************
 * Start_callback: Callback for "Start" button.
 ************************************************************************/

static void Start_callback( Widget w, XtPointer x, XtPointer y )
{
  HCI_LE_log( "In start callback" );

  /* Set cursor to "busy". */

  HCI_busy_cursor();

  /* Desensitize button. */

  XtVaSetValues( Start_button, XmNsensitive, False, NULL );

  /* Set flag. */

  Start_save_log_flag = YES;
}

/************************************************************************
 * Copy_callback: Callback for "Copy" button.
 ************************************************************************/

static void Copy_callback( Widget w, XtPointer x, XtPointer y )
{
  HCI_LE_log( "In copy callback" );

  /* Set cursor to "busy". */

  HCI_busy_cursor();

  /* Desensitize button. */

  XtVaSetValues( Copy_button, XmNsensitive, False, NULL );

  /* Set flag. */

  Start_copy_flag = YES;
}

/************************************************************************
 * Update_callback: Callback for "Update File List" button.
 ************************************************************************/

static void Update_callback( Widget w, XtPointer x, XtPointer y )
{
  HCI_LE_log( "In update callback" );

  /* Set cursor to "busy". */

  HCI_busy_cursor();

  /* Desensitize button. */

  XtVaSetValues( Update_button, XmNsensitive, False, NULL );

  /* Set flag. */

  Update_file_list_flag = YES;
}

/************************************************************************
 * Timer_proc: Callback for event timer.
 ************************************************************************/

static void Timer_proc()
{
  int loop = -1;

  /* Use flag values to take appropriate action. */

  if( Start_save_log_flag )
  {
    /* Unset flag. */
    Start_save_log_flag = NO;
    /* Start save log process. */
    Start_save_log();
    /* Start process to build/display save log list. */
    Update_file_list();
    /* Reset GUI to default settings. */
    Reset_gui();
  }
  else if( Start_copy_flag )
  {
    /* Unset flag. */
    Start_copy_flag = NO;
    /* Start process to copy selected files to CD. */
    Start_save_log_copy();
    /* Reset GUI to default settings. */
    Reset_gui();
  }
  else if( Update_file_list_flag )
  {
    /* Unset flag. */
    Update_file_list_flag = NO;
    /* Test connectivity of nodes. */
    for( loop = 0; loop < Num_nodes; loop++ )
    {
      Test_source_node_connectivity( &Node_info[ loop ] );
    }
    /* Start process to build/display save log list. */
    Update_file_list();
    /* Reset GUI to default settings. */
    Reset_gui();
  }
}

/************************************************************************
 * Start_save_log: Start save log process.
 ************************************************************************/

static void Start_save_log()
{
  int loop = -1;

  /* Make user user entered a description before continuing. */

  if( Get_description() < 0 ){ return; }

  /* Loop through nodes. */
  for( loop = 0; loop < Num_nodes; loop++ )
  {
    /* Find detected nodes. */
    if( Node_info[ loop ].detected )
    {
      /* Local/remote nodes require different logic (and function calls). */
      if( Node_info[ loop ].is_local )
      {
        Save_log_local( &Node_info[ loop ] );
      }
      else
      {
        Save_log_remote( &Node_info[ loop ] );
      } 
    }
  }

  /* Build message for popup when finished. */

  sprintf( Msg_buf, "Save log results:\n\n" );
  for( loop = 0; loop < Num_nodes; loop++ )
  {
    if( Node_info[ loop ].detected )
    {
      if( Node_info[ loop ].save_log_failed == NO )
      {
        /* Save log succeeded on the node. */
        strcat( Msg_buf, Node_info[ loop ].source_nodename );
        strcat( Msg_buf, ": SUCCESS\n" );
      }
      else
      {
        /* Save log failed on the node. */
        strcat( Msg_buf, Node_info[ loop ].source_nodename );
        strcat( Msg_buf, ": FAILED\n" );
      }
    }
    else
    {
      /* Node not detected. */
      strcat( Msg_buf, Node_info[ loop ].source_nodename );
      strcat( Msg_buf, ": NODE NOT FOUND\n" );
    }
  }
  strcat( Msg_buf, "\n" );

  /* Popup up message when finished. */

  Save_log_finished_popup();
}

/************************************************************************
 * Save_log_error_popup: Popup in event of error.
 ************************************************************************/

static void Save_log_error_popup()
{
  Widget dialog;
  Widget temp_widget;
  Widget ok_button;
  XmString ok_string;
  XmString msg;

  ok_string = XmStringCreateLocalized( "OK" );

  msg = XmStringCreateLtoR( Msg_buf, XmFONTLIST_DEFAULT_TAG );

  dialog = XmCreateInformationDialog( Top_widget, "error", NULL, 0 );

  /* Get rid of Cancel and Help buttons on popup. */

  temp_widget = XmMessageBoxGetChild( dialog, XmDIALOG_CANCEL_BUTTON );
  XtUnmanageChild( temp_widget );

  temp_widget = XmMessageBoxGetChild( dialog, XmDIALOG_HELP_BUTTON );
  XtUnmanageChild( temp_widget );

  /* Set properties of message label. */

  temp_widget = XmMessageBoxGetChild( dialog, XmDIALOG_MESSAGE_LABEL );
  XtVaSetValues( temp_widget,
          XmNforeground,  hci_get_read_color( TEXT_FOREGROUND ),
          XmNbackground,  hci_get_read_color( WARNING_COLOR ),
          XmNfontList,    hci_get_fontlist( LIST ),
          NULL );

  /* Set properties of OK button. */

  ok_button = XmMessageBoxGetChild( dialog, XmDIALOG_OK_BUTTON );

  XtVaSetValues( ok_button,
          XmNforeground,  hci_get_read_color( BUTTON_FOREGROUND ),
          XmNbackground,  hci_get_read_color( BUTTON_BACKGROUND ),
          XmNfontList,    hci_get_fontlist( LIST ),
          NULL );

  /* Set properties of popup. */

  XtVaSetValues (dialog,
          XmNmessageString,       msg,
          XmNokLabelString,       ok_string,
          XmNbackground,          hci_get_read_color( WARNING_COLOR ),
          XmNforeground,          hci_get_read_color( TEXT_FOREGROUND ),
          XmNdialogStyle,         XmDIALOG_PRIMARY_APPLICATION_MODAL,
          XmNdeleteResponse,      XmDESTROY,
          NULL);

  /* Free allocated space. */

  XmStringFree( ok_string );
  XmStringFree( msg );

  /* Do this to make popup appear. */

  XtManageChild( dialog );
  XtPopup( Top_widget, XtGrabNone );
}

/************************************************************************
 * Save_log_finished_popup: Popup when finished with save log.
 ************************************************************************/

static void Save_log_finished_popup()
{
  Widget dialog;
  Widget temp_widget;
  Widget ok_button;
  XmString ok;
  XmString msg;

  ok = XmStringCreateLocalized( "OK" );

  msg = XmStringCreateLtoR( Msg_buf, XmFONTLIST_DEFAULT_TAG );

  dialog = XmCreateInformationDialog( Top_widget, "info", NULL, 0 );

  /* Get rid of Cancel and Help buttons on popup. */

  temp_widget = XmMessageBoxGetChild( dialog, XmDIALOG_CANCEL_BUTTON );
  XtUnmanageChild( temp_widget );

  temp_widget = XmMessageBoxGetChild( dialog, XmDIALOG_HELP_BUTTON );
  XtUnmanageChild( temp_widget );

  /* Set properties of message label. */

  temp_widget = XmMessageBoxGetChild( dialog, XmDIALOG_MESSAGE_LABEL );
  XtVaSetValues( temp_widget,
          XmNforeground,  hci_get_read_color( TEXT_FOREGROUND ),
          XmNbackground,  hci_get_read_color( WARNING_COLOR ),
          XmNfontList,    hci_get_fontlist( LIST ),
          NULL );

  /* Set properties of OK button. */

  ok_button = XmMessageBoxGetChild( dialog, XmDIALOG_OK_BUTTON );

  XtVaSetValues( ok_button,
          XmNforeground,  hci_get_read_color( BUTTON_FOREGROUND ),
          XmNbackground,  hci_get_read_color( BUTTON_BACKGROUND ),
          XmNfontList,    hci_get_fontlist( LIST ),
          NULL );

  XtAddCallback( ok_button,
                 XmNactivateCallback, Success_callback, NULL );

  /* Set properties of popup. */

  XtVaSetValues (dialog,
          XmNmessageString,       msg,
          XmNokLabelString,       ok,
          XmNbackground,          hci_get_read_color( WARNING_COLOR ),
          XmNforeground,          hci_get_read_color( TEXT_FOREGROUND ),
          XmNdialogStyle,         XmDIALOG_PRIMARY_APPLICATION_MODAL,
          XmNdeleteResponse,      XmDESTROY,
          NULL);

  /* Free allocated space. */

  XmStringFree( ok );
  XmStringFree( msg );

  /* Do this to make popup appear. */

  XtManageChild( dialog );
  XtPopup( Top_widget, XtGrabNone );
}

/************************************************************************
 * Success_callback: User acknowledges popup.
 ************************************************************************/

static void Success_callback( Widget w, XtPointer x, XtPointer y )
{
  /* Clear description box widget when save log finished. */
  XtVaSetValues( Description_box, XmNvalue, "", NULL );
}

/************************************************************************
 * Save_log_local: Save log for source node that is local.
 ************************************************************************/

static void Save_log_local( Node_info_t *obj )
{
  int ret = -1;
  int n_bytes = -1;
  char output_buffer[ MAX_READ_BUF_SIZE ];
  char cmd[ MAX_CMD_LENGTH ];

  /* Build command string. */

  sprintf( cmd, "save_log" );

  if( strlen( Description_string ) != 0 )
  {
    strcat( cmd, " -a " );
    strcat( cmd, Description_string );
  }

  if( ! ORPGMISC_is_operational() )
  {
    strcat( cmd, " -s" );
  }

  /* Run command. */

  ret = MISC_system_to_buffer( cmd, output_buffer,
                               MAX_READ_BUF_SIZE, &n_bytes );
  ret = ret >> 8;

  if( ret != 0 )
  {
    /* Save log failed. Popup message. */
    HCI_LE_error( "Command: %s Failed (%d) for Node: %s", cmd, ret, obj->source_nodename );
    HCI_LE_error( "%ms", output_buffer );
    sprintf( Msg_buf, "Command: %s Failed (%d) for Node: %s.\n\n%s", cmd, ret, obj->source_nodename, output_buffer );
    Save_log_error_popup();
    obj->save_log_failed = YES;
  }
  else
  {
    obj->save_log_failed = NO;
  }
}

/************************************************************************
 * Save_log_remote: Save log for source node that is remote.
 ************************************************************************/

static void Save_log_remote( Node_info_t *obj )
{
  int ret = -1;
  int sys_ret = -1;
  int n_bytes = -1;
  char output_buffer[ MAX_READ_BUF_SIZE ];
  char cmd[ MAX_CMD_LENGTH ];
  char func[ MAX_CMD_LENGTH ];

  /* Build function to call remotely. */

  sprintf( func, "%s:MISC_system_to_buffer", obj->source_ip );
  sprintf( cmd, "save_log" );

  if( strlen( Description_string ) != 0 )
  {
    strcat( cmd, " -a" );
    strcat( cmd, Description_string );
  }

  /*  Unblock EN events so RSS function works properly. */

  (void) EN_cntl_unblock();

  ret = RSS_rpc( func, "i-r s-i ba-%d-io i ia-io", MAX_READ_BUF_SIZE,
                 &sys_ret, cmd, output_buffer, MAX_READ_BUF_SIZE, &n_bytes );
  sys_ret = sys_ret >> 8;

  /*  Continue to block EN events. */

  (void) EN_cntl_block();

  if( ret != 0 )
  {
    /* RSS failed. Popup message. */
    HCI_LE_error( "RSS_rpc failed (%d) for Node: %s", ret, obj->source_nodename );
    sprintf( Msg_buf, "RSS_rpc failed (%d) for Node: %s", ret, obj->source_nodename );
    Save_log_error_popup();
  }
  else if( sys_ret != 0 )
  {
    /* Save log failed. Popup message. */
    HCI_LE_error( "MISC_system_to_buffer failed (%d) for Node: %s", sys_ret, obj->source_nodename );
    HCI_LE_error( "%ms", output_buffer );
    sprintf( Msg_buf, "MISC_system_to_buffer failed (%d) for Node: %s.\n\n%s", sys_ret, obj->source_nodename, output_buffer );
    Save_log_error_popup();
    obj->save_log_failed = YES;
  }
  else
  {
    obj->save_log_failed = NO;
  }
}

/************************************************************************
 * Get_description: Get description string from description widget.
 ************************************************************************/

static int Get_description()
{
  char *text_string = NULL;
  int ret = -1;

  /* Check the filename field */

  text_string = XmTextGetString( Description_box );

  if( strlen( text_string ) != 0 )
  {
    /* Description box is non-empty. Return success. */
    ret = 0;
    strcpy( Description_string, text_string );
  }
  else
  {
    /* Description box is empty. Popup message. */
    sprintf( Msg_buf, "You must enter description." );
    Save_log_error_popup();
  }

  XtFree( text_string );
  return ret;
}

/************************************************************************
 * Verify_description_input: Make sure input for filename is valid. This
 *     callback is called every time a character is input into the
 *     filename text box.
 ************************************************************************/

static void Verify_description_input( Widget w, XtPointer x, XtPointer y )
{
  char char_input;

  XmTextVerifyCallbackStruct *cbs = ( XmTextVerifyCallbackStruct * ) y;

  /* Don't worry about backspaces. */

  if( cbs->text->ptr == NULL ){ return; }

  /* Don't allow cutting and pasting. */

  if( cbs->text->length > 1 )
  {
    cbs->doit = False;
    return;
  }

  /* Only allow alphanumeric and '_' characters in the filename. */

  char_input = *( cbs->text->ptr );

  if( isalnum( char_input ) == NO && char_input != '_' )
  {
    cbs->doit = False;
    return;
  }
}

/************************************************************************
 * Start_save_log_copy: Copy save log file(s) to CD.
 ************************************************************************/

static void Start_save_log_copy()
{
  int ret = -1;
  int loop1 = -1;
  int loop2 = -1;
  int loop3 = -1;
  int n_bytes = -1;
  char output_buffer[ MAX_READ_BUF_SIZE ];
  char cmd[ MAX_CMD_LENGTH + ( MAX_NUM_FILES * MAX_FILENAME_LENGTH ) ];
  char src_filename[ MAX_FILENAME_LENGTH ];
  char tmp_filenames[ MAX_NUM_FILES ][ MAX_FILENAME_LENGTH ];
  int pos = 0;

  /* If user hasn't selected any descriptions, do not continue. */

  if( Num_selected_descriptions < 1 )
  {
    sprintf( Msg_buf, "No descriptions selected." );
    Save_log_error_popup();
    return;
  }

  /* Start with a clean slate by removing temporary log directory
     in case it was previously created but not removed. Don't worry
     about checking return codes. */

  sprintf( cmd, "rm -rf %s", Temp_log_directory );
  ret = MISC_system_to_buffer( cmd, output_buffer,
                               MAX_READ_BUF_SIZE, &n_bytes );

  /* Create directory to temporarily hold files. Instead of passing
     each filename to medcp (which can lead to a command line large
     enough to exceed the limit of argv), the directory name will be
     passed instead. If creation fails, alert user. */

  sprintf( cmd, "mkdir -p %s", Temp_log_directory );
  ret = MISC_system_to_buffer( cmd, output_buffer,
                               MAX_READ_BUF_SIZE, &n_bytes );
  ret = ret >> 8;

  if( ret != 0 )
  {
    /* Command mkdir failed. Popup message. */
    HCI_LE_error( "Command %s failed (%d) on local node", cmd, ret );
    sprintf( Msg_buf, "Command %s failed (%d) on local node.\n\n",
             cmd, ret );
    strcat( Msg_buf, output_buffer );
    Save_log_error_popup();
    return;
  }

  /* Loop through all displayed descriptions. */
  for( loop1 = 0; loop1 < Num_descriptions; loop1++ )
  {
    /* Find a selected description. */
    if( File_descriptions[ loop1 ].selected )
    {
      /* Loop through all nodes. */
      for( loop2 = 0; loop2 < Num_nodes; loop2++ )
      {
        /* Find a detected node. */
        if( Node_info[ loop2 ].detected )
        {
          /* Loop through all descriptions on the detected node. */
          for( loop3 = 0; loop3 < Node_info[ loop2 ].num_files; loop3++ )
          {
            /* Find description on node that matches selected description.
               Copy file to temp directory until burned to CD. */
            if( strcmp( File_descriptions[ loop1 ].file_desc, Node_info[ loop2 ].filename_descriptions[ loop3 ] ) == 0 )
            {
              if( Node_info[ loop2 ].is_local )
              {
                /* Local node requires a simple "cp". */
                sprintf( tmp_filenames[ pos ], "%s/%s", Temp_log_directory, Node_info[ loop2 ].filenames[ loop3 ] );
                sprintf( cmd, "cp -f %s/%s %s", Save_log_dir, Node_info[ loop2 ].filenames[ loop3 ], tmp_filenames[ pos ] );
                ret = MISC_system_to_buffer( cmd, output_buffer,
                                 MAX_READ_BUF_SIZE, &n_bytes );
                ret = ret >> 8;
                if( ret != 0 )
                {
                  /* Simple "cp" failed. Popup message. */
                  HCI_LE_error( "Command %s failed (%d) on local node", cmd, ret );
                  sprintf( Msg_buf, "Command\n%s\nfailed (%d) on local node.\n\n",
                         cmd, ret );
                  strcat( Msg_buf, output_buffer );
                  Save_log_error_popup();
                }
                else
                {
                  /* Successful. Increment number of files to burn to CD. */
                  pos++;
                }
              }
              else
              {
                /* Remote node requires a more complex RSS copy. */
                sprintf( src_filename, "%s:%s/%s", Node_info[ loop2 ].source_ip, Save_log_dir, Node_info[ loop2 ].filenames[ loop3 ] );
                sprintf( tmp_filenames[ pos ], "%s/%s", Temp_log_directory, Node_info[ loop2 ].filenames[ loop3 ] );
                (void) EN_cntl_unblock();
                ret = RSS_copy( src_filename, tmp_filenames[ pos ] );
                (void) EN_cntl_block();
                if( ret != RSS_SUCCESS )
                {
                  /* RSS copy failed. Popup message. */
                  HCI_LE_error( "RSS_copy failed (%d)for file %s:%s", ret, Node_info[ loop2 ].source_nodename, Node_info[ loop2 ].filenames[ loop3 ] );
                  sprintf( Msg_buf, "RSS_copy failed (%d)for file\n", ret );
                  strcat( Msg_buf, Node_info[ loop2 ].filenames[ loop3 ] );
                  strcat( Msg_buf, "\non node " );
                  strcat( Msg_buf, Node_info[ loop2 ].source_nodename );
                  strcat( Msg_buf, "\n" );
                  Save_log_error_popup();
                }
                else
                {
                  /* Successful. Increment number of files to burn to CD. */
                  pos++;
                }
              }
              break;
            }
          }
        }
      }
    }
  }

  /* Use medcp utility to copy save log file(s) to CD. */

  sprintf( cmd, "medcp -ce %s cd", Temp_log_directory );
  ret = MISC_system_to_buffer( cmd, output_buffer,
                               MAX_READ_BUF_SIZE, &n_bytes );
  ret = ret >> 8;

  if( ret != 0 && ret != MEDCP_EJECT_T_FAILED )
  {
    /* Command failed. Popup message. */
    HCI_LE_error( "Error copying save log files to CD (%d)", ret );
    sprintf( Msg_buf, "Error copying save log files to CD (%d).\n\n", ret );
    strcat( Msg_buf, output_buffer );
    Save_log_error_popup();
  }
  else
  {
    /* Popup up message when finished. */
    strcpy( Msg_buf, "Save logs successfully written to CD.\n" );
    Save_log_finished_popup();
  }

  /* Remove save log temp directory. */

  sprintf( cmd, "rm -rf %s", Temp_log_directory );
  ret = MISC_system_to_buffer( cmd, output_buffer,
                               MAX_READ_BUF_SIZE, &n_bytes );
  ret = ret >> 8;

  /* Don't check return values. If it isn't successful,
     it isn't worth telling the user. */
}

/************************************************************************
 * Rmt_transmit_data_callback: Callback for data transfer from RMT library.
 ************************************************************************/

static int  Rmt_transmit_data_callback( rmt_transfer_event_t *event )
{
  static time_t	previous_transmit_time = -1;
  static int init_flag = NO;
  time_t current_transmit_time = -1;
  int num_second_delay = -1;

  if( init_flag == NO )
  {
    init_flag = YES;
    previous_transmit_time = time( NULL );
    return 0;
  }
  
  current_transmit_time = time( NULL );
  num_second_delay = ( current_transmit_time - previous_transmit_time ) / 1000;
  previous_transmit_time = current_transmit_time;

  if( num_second_delay > 0 ){ sleep( num_second_delay ); }

  return 0;
}

/************************************************************************
 * Reset_gui: Reset gui components to default values.
 ************************************************************************/

static void Reset_gui()
{
  /* Unset "busy" cursor. */

  HCI_default_cursor();

  /* Sensitize buttons. */

  XtVaSetValues( Start_button, XmNsensitive, True, NULL );
  XtVaSetValues( Copy_button, XmNsensitive, True, NULL );
  XtVaSetValues( Update_button, XmNsensitive, True, NULL );
}

/************************************************************************
 * Initialize_node_info: Initialize node information.
 ************************************************************************/

static void Initialize_node_info()
{
  if( FAA_flag )
  {
    Num_nodes = MAX_NUM_NODES;

    strcpy( Node_info[ 0 ].source_node, "RPGA" );
    strcpy( Node_info[ 0 ].source_nodename, "RPGA 1" );
    Node_info[ 0 ].source_channel = 1;
    Node_info[ 0 ].save_log_failed = NO;
    Node_info[ 0 ].num_files = 0;
    Test_source_node_connectivity( &Node_info[ 0 ] );

    strcpy( Node_info[ 1 ].source_node, "RPGA" );
    strcpy( Node_info[ 1 ].source_nodename, "RPGA 2" );
    Node_info[ 1 ].source_channel = 2;
    Node_info[ 1 ].save_log_failed = NO;
    Node_info[ 1 ].num_files = 0;
    Test_source_node_connectivity( &Node_info[ 1 ] );

    strcpy( Node_info[ 2 ].source_node, "RPGB" );
    strcpy( Node_info[ 2 ].source_nodename, "RPGB 1" );
    Node_info[ 2 ].source_channel = 1;
    Node_info[ 2 ].save_log_failed = NO;
    Node_info[ 2 ].num_files = 0;
    Test_source_node_connectivity( &Node_info[ 2 ] );

    strcpy( Node_info[ 3 ].source_node, "RPGB" );
    strcpy( Node_info[ 3 ].source_nodename, "RPGB 2" );
    Node_info[ 3 ].source_channel = 2;
    Node_info[ 3 ].save_log_failed = NO;
    Node_info[ 3 ].num_files = 0;
    Test_source_node_connectivity( &Node_info[ 3 ] );

    strcpy( Node_info[ 4 ].source_node, "MSCF" );
    strcpy( Node_info[ 4 ].source_nodename, "MSCF" );
    Node_info[ 4 ].source_channel = 1;
    Node_info[ 4 ].save_log_failed = NO;
    Node_info[ 4 ].num_files = 0;
    Test_source_node_connectivity( &Node_info[ 4 ] );
  }
  else
  {
    Num_nodes = 3;

    strcpy( Node_info[ 0 ].source_node, "RPGA" );
    strcpy( Node_info[ 0 ].source_nodename, "RPGA" );
    Node_info[ 0 ].source_channel = 1;
    Node_info[ 0 ].save_log_failed = NO;
    Node_info[ 0 ].num_files = 0;
    Test_source_node_connectivity( &Node_info[ 0 ] );

    strcpy( Node_info[ 1 ].source_node, "RPGB" );
    strcpy( Node_info[ 1 ].source_nodename, "RPGB" );
    Node_info[ 1 ].source_channel = 1;
    Node_info[ 1 ].save_log_failed = NO;
    Node_info[ 1 ].num_files = 0;
    Test_source_node_connectivity( &Node_info[ 1 ] );

    strcpy( Node_info[ 2 ].source_node, "MSCF" );
    strcpy( Node_info[ 2 ].source_nodename, "MSCF" );
    Node_info[ 2 ].source_channel = 1;
    Node_info[ 2 ].save_log_failed = NO;
    Node_info[ 2 ].num_files = 0;
    Test_source_node_connectivity( &Node_info[ 2 ] );
  }
}

/************************************************************************
 * Test_source_node_connectivity: Test if nodes are detectable over network.
 ************************************************************************/

static void Test_source_node_connectivity( Node_info_t *obj )
{
  int ret = -1;
  
  /*  Unblock EN events so ORPGMGR function works properly. */

  (void) EN_cntl_unblock();

  /* Test for source node. */

  ret = ORPGMGR_discover_host_ip( obj->source_node, obj->source_channel,
                                  obj->source_ip, IP_ADDRESS_LENGTH);

  /*  Continue to block EN events. */

  (void) EN_cntl_block();

  if( ret > 0 )
  {
    obj->detected = YES; /* Node is detectable. */
    if( strlen( obj->source_ip ) == 0 ){ obj->is_local = YES; } /* Local. */
    else{ obj->is_local = NO; } /* Not local. */
  }
  else
  {
    /* Node is not detectable, set values accordingly. */
    obj->detected = NO;
    obj->source_ip[0] = '\0';
    obj->is_local = NO;

    if( ret < 0 )
    {
      /* Command failed. Popup message. */
      HCI_LE_error( "ORPGMGR_discover_host_ip failed (%d) for %s",
                   ret, obj->source_nodename );
    }
    else if( ret == 0 )
    {
      /* Command succeeded, but node not found on network. Popup message. */
      HCI_LE_error( "System %s not found",
                   obj->source_nodename );
    }
  }
}

/************************************************************************
 * Update_file_list: Update file list with latest save log files.
 ************************************************************************/

static void Update_file_list()
{
  int loop = -1;
  int loop2 = -1;
  int offset = -1;
  XmString str;
  char buf[ MAX_DISPLAY_LINE_LENGTH ];
  char temp_desc[ MAX_DESCRIPTION_LENGTH ];

  /* Reset counter. */

  Num_descriptions = 0; 

  /* Loop through nodes. */
  for( loop = 0; loop < Num_nodes; loop++ )
  {
    /* Reset counter. */
    Node_info[ loop ].num_files = 0;
    /* Find detected nodes. */
    if( Node_info[ loop ].detected )
    {
      /* Local/remote nodes require different logic (and function calls). */
      if( Node_info[ loop ].is_local )
      {
        Get_file_list_local( &Node_info[ loop ] );
      }
      else
      {
        Get_file_list_remote( &Node_info[ loop ] );
      }
      /* Parse list of files to build display list. */
      Parse_file_list( &Node_info[ loop ] );
    }
  }

  /* Build display list widget. */

  Num_selected_descriptions = 0;
  XmListDeleteAllItems( File_select_list );
  for( loop = 0; loop < Num_descriptions; loop++ )
  {
    /* Determine number of preceding spaces to center description. */
    offset = MAX_DESCRIPTION_LENGTH - strlen( File_descriptions[ loop ].file_desc );
    if( offset < 0 ){ offset = 0; }
    else{ offset /= 2; }
    for( loop2 = 0; loop2 < offset; loop2++ )
    {
      temp_desc[ loop2 ] = ' ';
    }
    /* Use temporary variable to center. Left justify description. */
    strcpy( &temp_desc[ loop2 ], File_descriptions[ loop ].file_desc );
    sprintf( buf, "%8s    %-30s    %-8d", File_descriptions[ loop ].file_date, temp_desc, File_descriptions[ loop ].total_file_size);
    str=XmStringCreateLocalized( buf );
    XmListAddItemUnselected( File_select_list, str, 0 );
    XmStringFree(str);
    File_descriptions[ loop ].selected = NO;
  }

  /* Since no items are selected, make sure files sizes selected is zero. */

  File_size_sum = 0;
  Update_file_size_counter();
}

/************************************************************************
 * Get_file_list_local: Get list of save log files on the local node.
 ************************************************************************/

static void Get_file_list_local( Node_info_t *obj )
{
  int ret = -1;
  int n_bytes = -1;
  char cmd[ MAX_CMD_LENGTH ];

  /* Build command string. */

  sprintf( cmd, "save_log -l" );

  /* Run command. */

  File_list_buffer[ 0 ] = '\0';

  ret = MISC_system_to_buffer( cmd, File_list_buffer,
                               MAX_READ_BUF_SIZE, &n_bytes );
  ret = ret >> 8;

  if( ret != 0 )
  {
    /* Command failed. Popup message. */
    HCI_LE_error( "Command: %s Failed (%d) for Node: %s.", cmd, ret, obj->source_nodename );
    sprintf( Msg_buf, "Command: %s Failed (%d) for Node: %s.", cmd, ret, obj->source_nodename );
    Save_log_error_popup();
  }
}

/************************************************************************
 * Get_file_list_remote: Get list of save log files on a remote node.
 ************************************************************************/

static void Get_file_list_remote( Node_info_t *obj )
{
  int ret = -1;
  int sys_ret = -1;
  int n_bytes = -1;
  char cmd[ MAX_CMD_LENGTH ];
  char func[ MAX_CMD_LENGTH ];

  /* Build function to call remotely. */

  sprintf( func, "%s:MISC_system_to_buffer", obj->source_ip );
  sprintf( cmd, "save_log -l" );

  /*  Unblock EN events so RSS function works properly. */

  (void) EN_cntl_unblock();

  ret = RSS_rpc( func, "i-r s-i ba-%d-io i ia-io", MAX_READ_BUF_SIZE,
              &sys_ret, cmd, File_list_buffer, MAX_READ_BUF_SIZE, &n_bytes );
  sys_ret = sys_ret >> 8;

  /*  Continue to block EN events. */

  (void) EN_cntl_block();

  if( ret != 0 )
  {
    /* RSS failed. Popup message. */
    HCI_LE_error( "RSS_rpc failed (%d) for Node: %s", ret, obj->source_nodename );
    sprintf( Msg_buf, "RSS_rpc failed (%d) for Node: %s", ret, obj->source_nodename );
    Save_log_error_popup();
  }
  else if( sys_ret != 0 )
  {
    /* Command failed. Popup message. */
    HCI_LE_error( "MISC_system_to_buffer failed (%d) for Node: %s.", sys_ret, obj->source_nodename );
    sprintf( Msg_buf, "MISC_system_to_buffer failed (%d) for Node: %s.", sys_ret, obj->source_nodename );
    Save_log_error_popup();
  }
}

/************************************************************************
 * Parse_file_list: Parse File_list_buffer to get file info.
 ************************************************************************/

static void Parse_file_list( Node_info_t *obj )
{
  char *tok;
  char file_date[ MAX_DATE_LENGTH ];
  char file_name[ MAX_FILENAME_LENGTH ];
  char file_description[ MAX_DESCRIPTION_LENGTH ];
  int file_size = 0;
  int pos = 0;

  /* If buffer is empty, do not continue. */

  if( strlen( File_list_buffer ) < 1 ){ return; }

  /* Take it one line at a time. */
  tok = strtok( File_list_buffer, "\n" );
  if( Get_file_record( tok, file_name, file_description, file_date, &file_size ) )
  {
    /* Record was successfully parsed. Fill in values. */
    strcpy( obj->filenames[ pos ], file_name );
    strcpy( obj->filename_descriptions[ pos ], file_description );
    /* Increment/set counter showing number of files for this node. */
    pos++;
    obj->num_files = pos;
    /* Add to global description list. */
    Add_description( file_description, file_date, file_size );
  }
  /* Continue processing one line at a time. */
  while( ( tok = strtok( NULL, "\n" ) ) && pos < MAX_FILES_PER_NODE )
  {
    if( Get_file_record( tok, file_name, file_description, file_date, &file_size ) )
    {
      /* Record was successfully parsed. Fill in values. */
      strcpy( obj->filenames[ pos ], file_name );
      strcpy( obj->filename_descriptions[ pos ], file_description );
      /* Increment/set counter showing number of files for this node. */
      pos++;
      obj->num_files = pos;
      /* Add to global description list. */
      Add_description( file_description, file_date, file_size );
    }
  }
}

/************************************************************************
 * Get_file_record: Parse file record to get various file info.
 ************************************************************************/

static int Get_file_record( char *file_record, char *fname, char *fdesc, char *fdate, int *fsize )
{
  char *p = NULL;
  char *q = NULL;
  int token_number = 0;
  int pos = 0;
  int num_tokens = 1;

  /* Find number of tokens in file record (use whitespace as separator). */

  q = strdup( file_record );
  p = q;
  p = strstr( p, " " );
  if( p != NULL ){ p++; num_tokens++; }
  while( p != NULL )
  {
    p = strstr( p, " " );
    if( p != NULL ){ p++; num_tokens++; }
  }
  free( q );

  /* Sanity check. */

  if( num_tokens < 2 ){ return 0; }

  sscanf( file_record, "%s %d", fname, fsize );

  /* Sanity check. */

  if( fname == NULL || strlen( fname ) < 1 ){ return 0; }
  if( *fsize < 1 ){ return 0; }

  /* Parse filename for info. First token is date. */

  pos = 0;
  q = strdup( file_record );
  p = q;
  while( *p != '.' && pos < MAX_DATE_LENGTH - 1 )
  {
    fdate[ pos ] = *p;
    p++;
    pos++;
  }
  fdate[ pos ] = '\0';

  /* Skip to 6th token (description). */
  while( token_number < 4 && p != NULL )
  {
    p++; 
    p = strstr( p, "." );
    token_number++;
  }

  /* If p is NULL, then filename is of unexpected format. */

  if( p != NULL && *p == '.' ){ p++; }
  else{ free( q ); return 0; }

  /* Make sure this isn't the last token. */

  if( strstr( p, "." ) != NULL )
  {
    pos = 0;
    /* Copy by letter until end of token. */
    while( *p != '.' && pos < MAX_DESCRIPTION_LENGTH - 1 )
    {
      fdesc[ pos ] = *p;
      p++;
      pos++;
    }
    fdesc[ pos ] = '\0';
  }
  else
  {
    free( q );
    return 0;
  }

  free( q );
  return 1;
}

/************************************************************************
 * Add_description: Add description to global list if not already present.
 ************************************************************************/

static void Add_description( char *f_desc, char *f_date, int file_size )
{
  int found = NO;
  int loop1;

  found = NO;
  /* Loop through all descriptions. */
  for( loop1 = 0; loop1 < Num_descriptions; loop1++ )
  {
    /* If description previously added, add file size to total size. */
    if( strcmp( f_desc, File_descriptions[ loop1 ].file_desc ) == 0 )
    {
      found = YES;
      File_descriptions[ loop1 ].total_file_size += file_size;
      break;
    }
  }
  /* Description not previously added. Add now. */
  if( found == NO )
  {
    strcpy( File_descriptions[ Num_descriptions ].file_desc, f_desc );
    strcpy( File_descriptions[ Num_descriptions ].file_date, f_date );
    File_descriptions[ Num_descriptions ].total_file_size = file_size;
    Num_descriptions++;
  }
}

/************************************************************************
 * List_callback: Callback when item is selected in list.
 ************************************************************************/

static void List_callback( Widget w, XtPointer x, XtPointer y )
{
  XmListCallbackStruct *cbs = (XmListCallbackStruct *) y;

  if( cbs->selected_item_count > Num_selected_descriptions )
  {
    /* Selected. */
    File_descriptions[ cbs->item_position - 1 ].selected = YES;
    File_size_sum += File_descriptions[ cbs->item_position-1 ].total_file_size;
  }
  else if( cbs->selected_item_count < Num_selected_descriptions )
  {
    /* De-selected. */
    File_descriptions[ cbs->item_position - 1 ].selected = NO;
    File_size_sum -= File_descriptions[ cbs->item_position-1 ].total_file_size;
  }
  Num_selected_descriptions = cbs->selected_item_count;
  Update_file_size_counter();
}

/************************************************************************
 * Update_file_size_counter: Update label showing total size of files
 *    selected.
 ************************************************************************/

static void Update_file_size_counter()
{
  char buf[ 128 ];
  XmString str;

  sprintf( buf, "Total size of files selected is %d bytes.", File_size_sum );
  str = XmStringCreateLtoR( buf, XmFONTLIST_DEFAULT_TAG );
  XtVaSetValues( Selected_files_size_label, XmNlabelString, str, NULL );
  XmStringFree( str );
}

