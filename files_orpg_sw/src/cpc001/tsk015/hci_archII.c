/*
 * RCS info
 * $Author: jeffs $
 * $Locker:  $
 * $Date: 2014/03/18 16:55:30 $
 * $Id: hci_archII.c,v 1.12 2014/03/18 16:55:30 jeffs Exp $
 * $Revision: 1.12 $
 * $State: Exp $
 */

/* Local include files. */

#include <hci.h>
#include <archive_II.h>

/* Defines and enums. */

enum { HCI_TRANSMIT_ARCHII_OFF, HCI_TRANSMIT_ARCHII_ON };

/* Global variables. */

static	Widget	Top_widget;
static	Widget	ArchII_Transmit_On_button;
static	Widget	ArchII_Transmit_Off_button;
static	Widget	ArchII_Control_label;
static	Widget	Client_list;
static	int	ArchII_clients_updated = HCI_NO_FLAG;
static	int	ArchII_control_updated = HCI_NO_FLAG;
static	int	Channel_number = 1;
static	int	ArchII_user_cmd = -1;
static	char	Msg_buf[HCI_BUF_256] = "";
static	char	Previous_control_buf[HCI_BUF_32] = "";
static	char	Current_client_count = -1;
static	char	Current_clients[LDM_MAX_N_CLIENTS][LDM_CLIENT_INFO_SIZE];
static	char	Previous_client_count = -1;
static	char	Previous_clients[LDM_MAX_N_CLIENTS][LDM_CLIENT_INFO_SIZE];

/* Function prototypes. */

static void Close_button_callback( Widget, XtPointer, XtPointer );
static void ArchII_Control_button_callback( Widget, XtPointer, XtPointer );
static void ArchII_Control_button_yes( Widget, XtPointer, XtPointer );
static void ArchII_clients_callback( int, LB_id_t, int, void * );
static void ArchII_control_callback( int, LB_id_t, int, void * );
static void Update_clients_display();
static void Update_control_display();
static void No_client_info( char * );
static void Timer_proc();

/**********************************************************************

    Description: The main function.

***********************************************************************/

int main( int argc, char *argv[] )
{
  Widget form;
  Widget close_rc;
  Widget rowcol;
  Widget label;
  Widget archII_control_form;
  Widget archII_clients_form;
  Widget archII_control_frame;
  Widget archII_clients_frame;
  Widget archII_control_form_rc;
  Widget archII_clients_form_rc;
  Widget close_button;
  int background_color;
  int foreground_color;
  int ret;
  int n = 0;
  Arg args[16];

  /* Initialize HCI. */

  HCI_init( argc, argv, HCI_ARCHII_TASK );

  Top_widget = HCI_get_top_widget();

  HCI_PM( "Initialize Task Information" );

  /* Add redundancy information if site is FAA redundant. */

  if( HCI_get_system() == HCI_FAA_SYSTEM )
  {
    Channel_number = ORPGRED_channel_num( ORPGRED_MY_CHANNEL );
  }

  /* Create a form to arrange all widgets */

  form = XtVaCreateWidget( "form",
        xmFormWidgetClass,      Top_widget,
        XmNautoUnmanage,        False,
        XmNforeground,          hci_get_read_color( BACKGROUND_COLOR1 ),
        XmNbackground,          hci_get_read_color( BACKGROUND_COLOR1 ),
        NULL );

  /* Create a row column widget to put the close button */

  close_rc = XtVaCreateManagedWidget( "close_rc",
        xmRowColumnWidgetClass,   form,
        XmNbackground,            hci_get_read_color( BACKGROUND_COLOR1 ),
        XmNorientation,           XmHORIZONTAL,
        XmNnumColumns,            1,
        XmNleftOffset,            20,
        XmNtopAttachment,         XmATTACH_FORM,
        XmNleftAttachment,        XmATTACH_FORM,
        XmNrightAttachment,       XmATTACH_FORM,
        NULL );

  /* Close button and close button callback */

  close_button = XtVaCreateManagedWidget( "Close",
        xmPushButtonWidgetClass,  close_rc,
        XmNbackground,            hci_get_read_color( BUTTON_BACKGROUND ),
        XmNforeground,            hci_get_read_color( BUTTON_FOREGROUND ),
        XmNfontList,              hci_get_fontlist( LIST ),
        NULL );

    XtAddCallback( close_button,
        XmNactivateCallback, Close_button_callback, NULL );

  /* This frame will contain the archiveII control info */

  archII_control_frame = XtVaCreateManagedWidget( "archII_control_frame",
        xmFrameWidgetClass,     form,
        XmNbackground,          hci_get_read_color( BACKGROUND_COLOR1 ),
        XmNleftOffset,          20,
        XmNrightOffset,         20,
        XmNtopOffset,           10,
        XmNtopAttachment,       XmATTACH_WIDGET,
        XmNtopWidget,           close_rc,
        XmNleftAttachment,      XmATTACH_FORM,
        XmNrightAttachment,     XmATTACH_FORM,
        NULL );

  label = XtVaCreateManagedWidget( "Archive II Control",
        xmLabelWidgetClass,       archII_control_frame,
        XmNchildType,             XmFRAME_TITLE_CHILD,
        XmNchildHorizontalAlignment,  XmALIGNMENT_CENTER,
        XmNchildVerticalAlignment,XmALIGNMENT_CENTER,
        XmNbackground,            hci_get_read_color( BACKGROUND_COLOR1 ),
        XmNforeground,            hci_get_read_color( TEXT_FOREGROUND ),
        XmNfontList,              hci_get_fontlist( LIST ),
        NULL );

  /* Form for inside of frame. */

  archII_control_form = XtVaCreateWidget( "archII_control_form",
        xmFormWidgetClass, archII_control_frame,
        XmNautoUnmanage,   False,
        XmNforeground,     hci_get_read_color( BACKGROUND_COLOR1 ),
        XmNbackground,     hci_get_read_color( BACKGROUND_COLOR1 ),
        NULL );

  /* Row column widget for form inside of frame. */

  archII_control_form_rc = XtVaCreateManagedWidget( "archII_control_form_rc",
        xmRowColumnWidgetClass,   archII_control_form,
        XmNbackground,            hci_get_read_color( BACKGROUND_COLOR1 ),
        XmNorientation,           XmVERTICAL,
        XmNpacking,               XmPACK_TIGHT,
        XmNnumColumns,            1,
        XmNmarginWidth,           10,
        XmNmarginHeight,          10,
        XmNleftAttachment,        XmATTACH_FORM,
        XmNrightAttachment,       XmATTACH_FORM,
        NULL );

  /* Row column widget for control. */

  rowcol = XtVaCreateManagedWidget( "rowcol",
        xmRowColumnWidgetClass,   archII_control_form_rc,
        XmNbackground,            hci_get_read_color( BACKGROUND_COLOR1 ),
        XmNorientation,           XmHORIZONTAL,
        XmNnumColumns,            1,
        XmNpacking,               XmPACK_TIGHT,
        XmNleftAttachment,        XmATTACH_FORM,
        XmNrightAttachment,       XmATTACH_FORM,
        NULL );

  label = XtVaCreateManagedWidget( "Transmit Data: ",
        xmLabelWidgetClass,       rowcol,
        XmNbackground,            hci_get_read_color( BACKGROUND_COLOR1 ),
        XmNforeground,            hci_get_read_color( TEXT_FOREGROUND ),
        XmNfontList,              hci_get_fontlist( LIST ),
        XmNalignment,             XmALIGNMENT_BEGINNING,
        NULL );

  ArchII_Control_label = XtVaCreateManagedWidget( " UNKNOWN ", 
        xmLabelWidgetClass,       rowcol,
        XmNbackground,            hci_get_read_color( BACKGROUND_COLOR1 ),
        XmNforeground,            hci_get_read_color( TEXT_FOREGROUND ),
        XmNfontList,              hci_get_fontlist( LIST ),
        NULL );

  background_color = hci_get_read_color( WARNING_COLOR );
  foreground_color = hci_get_read_color( TEXT_FOREGROUND );

  XtVaSetValues( ArchII_Control_label,
                 XmNbackground, background_color,
                 XmNforeground, foreground_color, NULL );

  /* Row column widget for control buttons. */

  rowcol = XtVaCreateManagedWidget( "archII_rc",
        xmRowColumnWidgetClass,   archII_control_form_rc,
        XmNbackground,            hci_get_read_color( BACKGROUND_COLOR1 ),
        XmNorientation,           XmHORIZONTAL,
        XmNnumColumns,            1,
        XmNpacking,               XmPACK_TIGHT,
        XmNleftAttachment,        XmATTACH_FORM,
        XmNrightAttachment,       XmATTACH_FORM,
        NULL );

  label = XtVaCreateManagedWidget( "    ",
        xmLabelWidgetClass,       rowcol,
        XmNbackground,            hci_get_read_color( BACKGROUND_COLOR1 ),
        XmNforeground,            hci_get_read_color( TEXT_FOREGROUND ),
        XmNfontList,              hci_get_fontlist( LIST ),
        XmNalignment,             XmALIGNMENT_BEGINNING,
        NULL );

  ArchII_Transmit_On_button = XtVaCreateManagedWidget( " On ",
        xmPushButtonWidgetClass,  rowcol,
        XmNbackground,            hci_get_read_color( BUTTON_BACKGROUND ),
        XmNforeground,            hci_get_read_color( BUTTON_FOREGROUND ),
        XmNfontList,              hci_get_fontlist( LIST ),
        NULL );

  XtAddCallback( ArchII_Transmit_On_button,
        XmNactivateCallback, ArchII_Control_button_callback, (XtPointer) HCI_TRANSMIT_ARCHII_ON );

  /* Leave some space between two buttons */

  label = XtVaCreateManagedWidget( "  ",
        xmLabelWidgetClass,       rowcol,
        XmNbackground,            hci_get_read_color( BACKGROUND_COLOR1 ),
        XmNforeground,            hci_get_read_color( TEXT_FOREGROUND ),
        XmNfontList,              hci_get_fontlist( LIST ),
        XmNalignment,             XmALIGNMENT_BEGINNING,
        NULL );

  ArchII_Transmit_Off_button = XtVaCreateManagedWidget( " Off ",
        xmPushButtonWidgetClass,  rowcol,
        XmNbackground,            hci_get_read_color( BUTTON_BACKGROUND ),
        XmNforeground,            hci_get_read_color( BUTTON_FOREGROUND ),
        XmNfontList,              hci_get_fontlist( LIST ),
        NULL );

  XtAddCallback( ArchII_Transmit_Off_button,
        XmNactivateCallback, ArchII_Control_button_callback, (XtPointer) HCI_TRANSMIT_ARCHII_OFF );

  /* This frame will contain the archiveII client info */

  archII_clients_frame = XtVaCreateManagedWidget( "archII_clients_frame",
        xmFrameWidgetClass,     form,
        XmNbackground,          hci_get_read_color( BACKGROUND_COLOR1 ),
        XmNleftOffset,          20,
        XmNrightOffset,         20,
        XmNtopOffset,           10,
        XmNtopAttachment,       XmATTACH_WIDGET,
        XmNtopWidget,           archII_control_frame,
        XmNleftAttachment,      XmATTACH_FORM,
        XmNrightAttachment,     XmATTACH_FORM,
        NULL );

  label = XtVaCreateManagedWidget( "Archive II Clients",
        xmLabelWidgetClass,       archII_clients_frame,
        XmNchildType,             XmFRAME_TITLE_CHILD,
        XmNchildHorizontalAlignment,  XmALIGNMENT_CENTER,
        XmNchildVerticalAlignment,XmALIGNMENT_CENTER,
        XmNbackground,            hci_get_read_color( BACKGROUND_COLOR1 ),
        XmNforeground,            hci_get_read_color( TEXT_FOREGROUND ),
        XmNfontList,              hci_get_fontlist( LIST ),
        NULL );

  /* Form for inside of frame. */

  archII_clients_form = XtVaCreateWidget( "archII_clients_form",
        xmFormWidgetClass, archII_clients_frame,
        XmNautoUnmanage,   False,
        XmNforeground,     hci_get_read_color( BACKGROUND_COLOR1 ),
        XmNbackground,     hci_get_read_color( BACKGROUND_COLOR1 ),
        NULL );

  /* Row column widget for form inside of frame. */

  archII_clients_form_rc = XtVaCreateManagedWidget( "archII_clients_form_rc",
        xmRowColumnWidgetClass,   archII_clients_form,
        XmNbackground,            hci_get_read_color( BACKGROUND_COLOR1 ),
        XmNorientation,           XmVERTICAL,
        XmNpacking,               XmPACK_TIGHT,
        XmNnumColumns,            1,
        XmNmarginWidth,           10,
        XmNmarginHeight,          10,
        XmNleftAttachment,        XmATTACH_FORM,
        XmNrightAttachment,       XmATTACH_FORM,
        NULL );

  /* Scroll area to show connected clients, if any. */

  label = XtVaCreateManagedWidget( "    Connected Clients",
        xmLabelWidgetClass,       archII_clients_form_rc,
        XmNbackground,            hci_get_read_color( BACKGROUND_COLOR1 ),
        XmNforeground,            hci_get_read_color( TEXT_FOREGROUND ),
        XmNfontList,              hci_get_fontlist( LIST ),
        XmNalignment,             XmALIGNMENT_BEGINNING,
        NULL );

  n = 0;
  XtSetArg( args[n], XmNforeground, hci_get_read_color( TEXT_FOREGROUND ) );
  n++;
  XtSetArg( args[n], XmNbackground, hci_get_read_color( BACKGROUND_COLOR2 ) );
  n++;
  XtSetArg( args[n], XmNhighlightColor, hci_get_read_color( BACKGROUND_COLOR2 ) );
  n++;
  XtSetArg( args[n], XmNwidth, 300 );
  n++;
  XtSetArg( args[n], XmNheight, 120 );
  n++;
  XtSetArg( args[n], XmNscrollingPolicy, XmAUTOMATIC );
  n++;
  XtSetArg( args[n], XmNfontList, hci_get_fontlist( LIST ) );
  n++;
  Client_list = XmCreateScrolledList( archII_clients_form_rc, "clients_scroll", args, n );
  XtManageChild( Client_list );

  /* Widget to create some empty space at the bottom */

  rowcol = XtVaCreateManagedWidget( "rowcol",
        xmRowColumnWidgetClass,   form,
        XmNbackground,            hci_get_read_color( BACKGROUND_COLOR1 ),
        XmNorientation,           XmHORIZONTAL,
        XmNisAligned,             False,
        XmNtopAttachment,         XmATTACH_WIDGET,
        XmNtopWidget,             archII_clients_frame,
        XmNtopAttachment,         XmATTACH_WIDGET,
        XmNleftAttachment,        XmATTACH_FORM,
        XmNrightAttachment,       XmATTACH_FORM,
        NULL );

  label = XtVaCreateManagedWidget( " ",
        xmLabelWidgetClass,       rowcol,
        XmNbackground,            hci_get_read_color( BACKGROUND_COLOR1 ),
        XmNforeground,            hci_get_read_color( TEXT_FOREGROUND ),
        XmNfontList,              hci_get_fontlist( LIST ),
        XmNalignment,             XmALIGNMENT_BEGINNING,
        NULL );

  /* Manage widgets. */

  XtManageChild( archII_control_form );
  XtManageChild( archII_clients_form );
  XtManageChild( form );

  /* Register appropriate LBs. */

  ORPGDA_write_permission( ORPGDAT_ARCHIVE_II_INFO );

  ret = ORPGDA_UN_register( ORPGDAT_ARCHIVE_II_INFO, ARCHIVE_II_CLIENTS_ID,
                               ArchII_clients_callback );
  if( ret != LB_SUCCESS )
  {
    HCI_LE_error( "Unable to register ARCHIVE_II_CLIENT_ID, (%d)", ret );
  }

  ret = ORPGDA_UN_register( ORPGDAT_ARCHIVE_II_INFO, ARCHIVE_II_FLOW_ID,
                               ArchII_control_callback );
  if( ret != LB_SUCCESS )
  {
    HCI_LE_error( "Unable to register ARCHIVE_II_FLOW_ID, (%d)", ret );
  }

  /* Set initial display. */

  Update_clients_display();
  Update_control_display();

  /* Display widgets. */

  XtRealizeWidget( Top_widget );

  /* Start HCI loop. */

  HCI_start( Timer_proc, HCI_TWO_SECONDS, NO_RESIZE_HCI );

  return 0;
}

/**********************************************************************

    Description: Close button callback function.

***********************************************************************/

static void Close_button_callback( Widget w, XtPointer x, XtPointer y )
{
  HCI_LE_log( "Close button selected" );
  XtDestroyWidget( Top_widget );
}

/**********************************************************************

    Description:  Control buttons callback function.

***********************************************************************/

static void ArchII_Control_button_callback( Widget w, XtPointer x, XtPointer y )
{
  ArchII_user_cmd = (int) x;

  if( ArchII_user_cmd == HCI_TRANSMIT_ARCHII_OFF )
  {
    sprintf( Msg_buf, "You are about to stop Archive II data archiving.\nArchive II data flow will terminate at the\ncompletion of the current volume scan.\n\nDo you want to continue?" );
  }
  else if( ArchII_user_cmd == HCI_TRANSMIT_ARCHII_ON )
  {
    sprintf( Msg_buf, "You are about to start Archive II data archiving.\nArchive II data flow will start at the beginning\nof the next volume scan.\n\nDo you want to continue?" );
  }

  hci_confirm_popup( Top_widget, Msg_buf, ArchII_Control_button_yes, NULL );
}

/**********************************************************************

    Description: Timer callback function.

***********************************************************************/

static void Timer_proc()
{
  if( ArchII_clients_updated == HCI_YES_FLAG )
  {
    ArchII_clients_updated = HCI_NO_FLAG;
    Update_clients_display();
  }

  if( ArchII_control_updated == HCI_YES_FLAG )
  {
    ArchII_control_updated = HCI_NO_FLAG;
    Update_control_display();
  }
}

/**********************************************************************

    Description: Updates the client list.

***********************************************************************/

static void Update_clients_display()
{
  int archII_rtn = -1;
  int i = -1, j = -1;
  int bufptr = -1;
  int ip_len = -1;
  int found = HCI_NO_FLAG;
  int update_client_info = HCI_NO_FLAG;
  XmString s[LDM_MAX_N_CLIENTS];
  ArchII_clients_t ArchII_clients;

  /* Get Archive II clients. */

  archII_rtn = ORPGDA_read( ORPGDAT_ARCHIVE_II_INFO, (char *) &ArchII_clients, 
                            sizeof( ArchII_clients_t ), ARCHIVE_II_CLIENTS_ID );

  if( archII_rtn <= 0 )
  {
    /* Error reading message. */
    HCI_LE_error( "ORPGDA_read(ARCHIVE_II_CLIENTS_ID) Failed: %d", archII_rtn );
    /* Print error message to screen. */
    No_client_info( "Cannot retrieve clients" );
  }
  else
  {
    /* Set connected client area. */

    if( ArchII_clients.num_clients == 0 )
    {
      /* No clients connected. */
      No_client_info( "None" );
    }
    else
    {
      /* Clients connected. */
      bufptr = 0;
      Current_client_count = 0;
      for( i = 0; i < ArchII_clients.num_clients; i++ )
      {
        ip_len = strlen( &ArchII_clients.buf[bufptr] );

        if( ip_len == 0 )
        {
          /* If blank, assume no others to read. */
          break;
        }
        else if( ( bufptr + ip_len ) < MAX_LDM_CLIENT_INFO_SIZE )
        {
          if( Current_client_count < LDM_MAX_N_CLIENTS )
          {
            strcpy( Current_clients[i], &ArchII_clients.buf[bufptr] );
            bufptr += ( ip_len + 1 );
            Current_client_count++;
          }
          else
          {
            HCI_LE_log( "Number of clients exceeds maximum" );
            No_client_info( "Too many clients" );
          }
        }
        else
        {
          HCI_LE_log( "Maximum client buffer size exceeded" );
          No_client_info( "Client buffer error" );
        }
      }

      if( Current_client_count == 0 )
      {
        /* No clients connected. */
        No_client_info( "None" );
      }
    }
  }

  /* Test for difference to prevent GUI from blinking during refresh. */

  if( Current_client_count != Previous_client_count )
  {
    update_client_info = HCI_YES_FLAG;
  }
  else
  {
    for( i = 0; i < Current_client_count; i++ )
    {
      found = HCI_NO_FLAG;
      for( j = 0; j < Current_client_count; j++ )
      {
        if( strcmp( Current_clients[i], Previous_clients[j] ) == 0 )
        {
          found = HCI_YES_FLAG;
          break;
        }
      }
      if( found == HCI_NO_FLAG ){ update_client_info = HCI_YES_FLAG; }
    }
  }

  if( update_client_info == HCI_YES_FLAG )
  {
    Previous_client_count = Current_client_count;
    for( i = 0; i < Current_client_count; i++ )
    {
      strcpy( Previous_clients[i], Current_clients[i] );
      s[i] = XmStringCreateLocalized( Current_clients[i] );
    }
    XtVaSetValues( Client_list, XmNitemCount, Current_client_count, XmNitems, s, NULL );
    XmListUpdateSelectedList( Client_list );
    for( i = 0; i < Current_client_count; i++ )
    {
      XmStringFree( s[i] );
    }
  }
}

/**********************************************************************

    Description: Updates the control label.

***********************************************************************/

static void No_client_info( char *msg )
{
  strcpy( Current_clients[0], msg );
  Current_client_count = 1;
}

/**********************************************************************

    Description: Updates the control label.

***********************************************************************/

static void Update_control_display()
{
  int archII_rtn = -1;
  int bg = -1;
  int fg = -1;
  char control_buf[HCI_BUF_32];
  XmString archII_control_string = NULL;
  ArchII_transmit_status_t ArchII_transmit_status;

  /* Get Archive II status. */

  archII_rtn = ORPGDA_read( ORPGDAT_ARCHIVE_II_INFO,
                            (char *) &ArchII_transmit_status, 
                            sizeof( ArchII_transmit_status_t ),
                            ARCHIVE_II_FLOW_ID );

  if( archII_rtn <= 0 )
  {
    /* Error reading message. */
    HCI_LE_error( "ORPGDA_read(ARCHIVE_II_FLOW_ID) Failed: %d", archII_rtn );
    /* Set status label to UNKNOWN. */
    sprintf( control_buf, " UNKNOWN " );
    bg = hci_get_read_color( WARNING_COLOR );
    fg = hci_get_read_color( TEXT_FOREGROUND );
    XtSetSensitive( ArchII_Transmit_On_button, False );
    XtSetSensitive( ArchII_Transmit_Off_button, False );
  }
  else
  {
    /* Set Archive II label.  */
    if( ArchII_transmit_status.status == ARCHIVE_II_TRANSMIT_ON )
    {
      sprintf( control_buf, "   ON    " );
      bg = hci_get_read_color( NORMAL_COLOR );
      fg = hci_get_read_color( TEXT_FOREGROUND );
      XtSetSensitive( ArchII_Transmit_On_button, False );
      XtSetSensitive( ArchII_Transmit_Off_button, True );
    }
    else if( ArchII_transmit_status.status == ARCHIVE_II_TRANSMIT_OFF )
    {
      sprintf( control_buf, "   OFF   " );
      bg = hci_get_read_color( WARNING_COLOR );
      fg = hci_get_read_color( TEXT_FOREGROUND );
      XtSetSensitive( ArchII_Transmit_On_button, True );
      XtSetSensitive( ArchII_Transmit_Off_button, False );
    }
    else
    {
      sprintf( control_buf, " UNKNOWN " );
      bg = hci_get_read_color( WARNING_COLOR );
      fg = hci_get_read_color( TEXT_FOREGROUND );
      XtSetSensitive( ArchII_Transmit_On_button, False );
      XtSetSensitive( ArchII_Transmit_Off_button, False );
    }
  }

  /* Test for difference to prevent GUI from blinking during refresh. */

  if( strcmp( control_buf,Previous_control_buf ) != 0 )
  {
    strcpy( Previous_control_buf, control_buf );
    archII_control_string = XmStringCreateLocalized( control_buf );
    XtVaSetValues( ArchII_Control_label, 
                   XmNlabelType, XmSTRING, 
                   XmNlabelString, archII_control_string, 
                   XmNbackground, bg, 
                   XmNforeground, fg, NULL );
  }

  XmStringFree( archII_control_string );
}

/**********************************************************************

    Description: Archive II Control button "Yes" confirmation callback
                 function.

***********************************************************************/

static void ArchII_Control_button_yes( Widget w, XtPointer x, XtPointer y )
{
  int ret = -1;
  ArchII_command_t ArchII_command;

  /* Set the command update time. */

  ArchII_command.ctime = time( NULL );

  if( ArchII_user_cmd == HCI_TRANSMIT_ARCHII_OFF )
  {
    ArchII_command.command = ARCHIVE_II_NEED_TO_STOP;
    ret = ORPGDA_write( ORPGDAT_ARCHIVE_II_INFO, (char *) &ArchII_command, 
                           sizeof( ArchII_command_t ), ARCHIVE_II_COMMAND_ID );

    if( ret < 0 )
    {
      HCI_LE_error( "Error Writing To ARCHIVE II INFO Buffer" );
      HCI_LE_error( "Archive II Data ERROR" );
      sprintf( Msg_buf, "Unable to stop Archive II (%d)", ret );
      hci_error_popup( Top_widget, Msg_buf, NULL );
    }
    else
    {
      HCI_LE_log( "Archive II Data Commanded to STOP" );
    }
  }
  else if( ArchII_user_cmd == HCI_TRANSMIT_ARCHII_ON )
  {
    ArchII_command.command = ARCHIVE_II_NEED_TO_START;
    ret = ORPGDA_write( ORPGDAT_ARCHIVE_II_INFO, (char *) &ArchII_command, 
                           sizeof( ArchII_command_t ), ARCHIVE_II_COMMAND_ID );

    if( ret < 0 )
    {
      HCI_LE_error( "Error Writing To ARCHIVE II INFO Buffer" );
      HCI_LE_error( "Archive II Data ERROR" );
      sprintf( Msg_buf, "Unable to start Archive II (%d)", ret );
      hci_error_popup( Top_widget, Msg_buf, NULL );
    }
    else
    {
      HCI_LE_log( "Archive II Data Commanded to START" );
    }
  }
}

/************************************************************************

    Description: This function sets the flag indicating the Archive II
                 clients message has been updated.

 ************************************************************************/

void ArchII_clients_callback( int fd, LB_id_t msg_id, int msg_info, void *arg )
{
  ArchII_clients_updated = HCI_YES_FLAG;
}

/************************************************************************

    Description: This function sets the flag indicating the Archive II
                 transmit message has been updated.

 ************************************************************************/

void ArchII_control_callback( int fd, LB_id_t msg_id, int msg_info, void *arg )
{
  ArchII_control_updated = HCI_YES_FLAG;
}

