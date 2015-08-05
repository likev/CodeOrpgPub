/************************************************************************
 *							                *
 *	Module:  hci_force_adapt_load.c					*
 *							                *
 *	Description:  This module is used to force the user to load	*
 *		      adaptation data.					*
 *					                        	*
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2010/03/16 14:37:47 $
 * $Id: hci_force_adapt_load.c,v 1.18 2010/03/16 14:37:47 ccalvert Exp $
 * $Revision: 1.18 $
 * $State: Exp $
 */

/*	Local include file definitions.					*/

#include <hci_control_panel.h>

/* Global widget definitions. */

static	Widget		Top_widget = ( Widget ) NULL;

/* Function prototypes. */

static void adapt_load_close( Widget, XtPointer, XtPointer );

/***************************************************************************/
/* HCI_FORCE_ADAPT_LOAD: Inform user that adaptation data is not installed.*/
/***************************************************************************/

void hci_force_adapt_load( int argc, char *argv[] )
{
  Widget form;
  Widget form_rowcol;
  Widget rowcol;
  Widget button;
  Widget label;
  char temp_buf[ HCI_BUF_256 ];

  /* Let user know this gui tried to launch. */

  HCI_LE_log( "Force Adaptation Data Load Main HCI screen running" );

  /* Initialize HCI. */

  HCI_partial_init( argc, argv, HCI_TASK );

  /* Get reference to top-level widget. */

  Top_widget = HCI_get_top_widget();

  /* Set title, since only a partial init. */

  XtVaSetValues( Top_widget, XmNtitle, HCI_get_task_name( HCI_TASK ), NULL );

  /* Use a form widget to be used as the manager for widgets *
   * in the top level window.                                */

  form = XtVaCreateManagedWidget( "form",
           xmFormWidgetClass,   Top_widget,
           XmNbackground,       hci_get_read_color( BACKGROUND_COLOR1 ),
           NULL );

  /* Build the widgets. */

  form_rowcol = XtVaCreateManagedWidget( "form_rowcol",
          xmRowColumnWidgetClass, form,
          XmNbackground,          hci_get_read_color( BACKGROUND_COLOR1 ),
          XmNorientation,         XmVERTICAL,
          XmNpacking,             XmPACK_TIGHT,
          XmNnumColumns,          1,
          NULL );

  /* Build warning/info label. */

  if( HCI_get_node() == HCI_RPGA_NODE )
  {
    sprintf( temp_buf, "Adaptation data must be loaded on this system\n" );
    strcat( temp_buf, "before the HCI will launch." );
  }
  else
  {
    sprintf( temp_buf, "Adaptation data must be loaded on RPGA before\n" );
    strcat( temp_buf, "the HCI will launch.\n");
  }

  label = XtVaCreateManagedWidget( temp_buf,
          xmLabelWidgetClass,   form_rowcol,
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
       XmNtopWidget,            label,
       XmNleftAttachment,       XmATTACH_FORM,
       XmNrightAttachment,      XmATTACH_FORM,
       NULL );

  /* Build button to close task. */

  label = XtVaCreateManagedWidget( "                  ",
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
                 XmNactivateCallback, adapt_load_close, NULL );

  XtRealizeWidget( Top_widget );

  /* Start HCI loop. */

  HCI_start( NULL, -1, NO_RESIZE_HCI );
}

/***************************************************************************/
/* ADAPT_LOAD_CLOSE: Closes this task.                                     */
/***************************************************************************/

static void adapt_load_close( Widget w, XtPointer client_data, XtPointer call_data )
{
  XtDestroyWidget( Top_widget );
}

