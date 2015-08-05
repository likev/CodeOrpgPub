/************************************************************************
 *							                *
 *	Module:  hci_force_dev_configure.c				*
 *							                *
 *	Description:  This module is used to force the user to 		*
 *		      configure hardware devices.			*
 *					                        	*
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2010/03/16 14:37:47 $
 * $Id: hci_force_dev_configure.c,v 1.11 2010/03/16 14:37:47 ccalvert Exp $
 * $Revision: 1.11 $
 * $State: Exp $
 */

/*	Local include file definitions.					*/

#include <hci_control_panel.h>

/* Global widget definitions. */

static	Widget		Top_widget = ( Widget ) NULL;

/* Global variable definitions. */

static	char	msg_buf[ HCI_BUF_512 ];

/* Function prototypes. */

void dev_config_close( Widget w, XtPointer client_data, XtPointer call_data );
void dev_configure( Widget w, XtPointer client_data, XtPointer call_data );
int  hci_activate_child( Display *d, Window w, char *cmd,
                         char *proc, char *win, int object_index );

/************************************************************************
 *	Description: This module is responsible for defining/drawing	*
 *		     the gui to the screen.				*
 *									*
 *	Input:  argc - number of items in command line string		*
 *		argv - pointer to command line string			*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_force_dev_configure( int argc, char *argv[] )
{
  Widget form;
  Widget frame;
  Widget frame_rowcol;
  Widget rowcol;
  Widget button;
  Widget label;
  int node_flag = -1;

  /* Let user know this gui tried to launch. */

  HCI_LE_log( "Force Hardware Configuration Main HCI screen running" );

  /* Initialize HCI. */

  HCI_partial_init( argc, argv, HCI_TASK );

  /* Get reference to top-level widget. */

  Top_widget = HCI_get_top_widget();

  /* Set title, since only a partial init. */

  XtVaSetValues( Top_widget, XmNtitle, HCI_get_task_name( HCI_TASK ), NULL );

  /* What node is this running on? */

  node_flag = HCI_get_node();

  /* Use a form widget to be used as the manager for widgets *
   * in the top level window.                                */

  form = XtVaCreateManagedWidget( "form",
           xmFormWidgetClass,   Top_widget,
           XmNbackground,       hci_get_read_color( BACKGROUND_COLOR1 ),
           NULL );

  /* Build the widgets. */

  frame = XtVaCreateManagedWidget( "frame",
          xmFrameWidgetClass,     form,
          XmNtopAttachment,       XmATTACH_FORM,
          XmNleftAttachment,      XmATTACH_FORM,
          XmNrightAttachment,     XmATTACH_FORM,
          NULL );

  frame_rowcol = XtVaCreateManagedWidget( "frame_rowcol",
          xmRowColumnWidgetClass, frame,
          XmNbackground,          hci_get_read_color( BACKGROUND_COLOR1 ),
          XmNorientation,         XmVERTICAL,
          XmNpacking,             XmPACK_TIGHT,
          XmNnumColumns,          1,
          NULL );

  /* Build warning/info label. */

  if( node_flag == HCI_RPGA_NODE )
  {
    sprintf( msg_buf, "Hardware devices must be configured before\n" );
    strcat( msg_buf, "the HCI will launch. To configure hardware\n" );
    strcat( msg_buf, "devices, click the \"Configure\" button.\n" );
  }
  else
  {
    sprintf( msg_buf, "Hardware devices must be configured from RPGA\n" );
    strcat( msg_buf, "before the HCI will launch.\n" );
  }

  label = XtVaCreateManagedWidget( msg_buf,
          xmLabelWidgetClass,   frame_rowcol,
          XmNforeground,        hci_get_read_color( TEXT_FOREGROUND ),
          XmNbackground,        hci_get_read_color( BACKGROUND_COLOR1 ),
          XmNfontList,          hci_get_fontlist( LIST ),
          NULL );

  rowcol = XtVaCreateManagedWidget( "rowcol",
       xmRowColumnWidgetClass,  form,
       XmNbackground,           hci_get_read_color( BACKGROUND_COLOR1 ),
       XmNorientation,          XmHORIZONTAL,
       XmNpacking,              XmPACK_TIGHT,
       XmNnumColumns,           1,
       XmNentryAlignment,       XmALIGNMENT_CENTER,
       XmNtopAttachment,        XmATTACH_WIDGET,
       XmNtopWidget,            frame,
       XmNleftAttachment,       XmATTACH_FORM,
       XmNrightAttachment,      XmATTACH_FORM,
       NULL );

  if( node_flag == HCI_RPGA_NODE )
  {
    /* Build button to launch restore task. */

    button = XtVaCreateManagedWidget( "Configure",
            xmPushButtonWidgetClass, rowcol,
            XmNforeground,        hci_get_read_color( BUTTON_FOREGROUND ),
            XmNbackground,        hci_get_read_color( BUTTON_BACKGROUND ),
            XmNfontList,          hci_get_fontlist( LIST ),
            NULL );

    XtAddCallback( button,
                   XmNactivateCallback, dev_configure, NULL );

    label = XtVaCreateManagedWidget( "                         ",
            xmLabelWidgetClass,   rowcol,
            XmNforeground,        hci_get_read_color( TEXT_FOREGROUND ),
            XmNbackground,        hci_get_read_color( BACKGROUND_COLOR1 ),
            XmNfontList,          hci_get_fontlist( LIST ),
            NULL );

    /* Build button to close this task. */

    button = XtVaCreateManagedWidget( "Close",
            xmPushButtonWidgetClass, rowcol,
            XmNforeground,        hci_get_read_color( BUTTON_FOREGROUND ),
            XmNbackground,        hci_get_read_color( BUTTON_BACKGROUND ),
            XmNfontList,          hci_get_fontlist( LIST ),
            NULL );

    XtAddCallback( button,
                   XmNactivateCallback, dev_config_close, NULL );
  }
  else
  {
    /* Build button to close task. */

    label = XtVaCreateManagedWidget( "                    ",
            xmLabelWidgetClass,   rowcol,
            XmNforeground,        hci_get_read_color( TEXT_FOREGROUND ),
            XmNbackground,        hci_get_read_color( BACKGROUND_COLOR1 ),
            XmNfontList,          hci_get_fontlist( LIST ),
            NULL );

    button = XtVaCreateManagedWidget( "Close",
            xmPushButtonWidgetClass, rowcol,
            XmNforeground,        hci_get_read_color( BUTTON_FOREGROUND ),
            XmNbackground,        hci_get_read_color( BUTTON_BACKGROUND ),
            XmNfontList,          hci_get_fontlist( LIST ),
            NULL );

    XtAddCallback( button,
                   XmNactivateCallback, dev_config_close, NULL );
  }

  XtRealizeWidget( Top_widget );

  /* Start HCI loop. */

  HCI_start( NULL, -1, NO_RESIZE_HCI );
}

/************************************************************************
 * dev_config_close: Closes this task.
 ************************************************************************/

void dev_config_close( Widget w, XtPointer client_data, XtPointer call_data )
{
  XtDestroyWidget( Top_widget );
}

/************************************************************************
 * dev_configure: Callback for "Configure" button.
 ************************************************************************/

void dev_configure( Widget w, XtPointer client_data, XtPointer call_data )
{
  char task_name[ HCI_BUF_256 ];
  char app_name[ HCI_BUF_256 ];
  int channel_number = -1;

  /* Get channel info. */

  channel_number = HCI_get_channel_number();

  /* Set app/task name. */

  sprintf( app_name, "hci_hardware_config -A %d", channel_number );
  sprintf( task_name, "hci_hardware_config -A %d -O %d",
           channel_number, HARDWARE_CONFIG_BUTTON );
  strcat( task_name, " -name \"Hardware Configuration\"" );
  strcat( task_name, HCI_child_options_string() );

  /* Call function to launch task. */

  hci_activate_child( NULL, ( Window ) NULL, task_name, app_name,
      "Hardware Configuration", RESTORE_ADAPT_BUTTON );

  dev_config_close( NULL, NULL, NULL );
}


