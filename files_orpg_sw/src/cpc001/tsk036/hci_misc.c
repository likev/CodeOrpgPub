/************************************************************************
 *									*
 *	Module:  hci_misc.c						*
 *									*
 *	Description:  This module provides gui wrappers for various 	*
 *		      tasks that run as scripts.			*
 *									*
 ************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/06/19 20:51:15 $
 * $Id: hci_misc.c,v 1.17 2013/06/19 20:51:15 steves Exp $
 * $Revision: 1.17 $
 * $State: Exp $
 */

/*	Local include file definitions.			*/

#include <hci.h>

/*	Global widget definitions.	*/

static	Widget	Top_widget = (Widget) NULL;
static	int	channel_number = -1;

/*	Function prototypes.		*/

void	close_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
void	hci_misc_restore_adapt_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
void	hci_misc_save_adapt_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
void	hci_password_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
void	hci_hardware_config_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
void	hci_view_log_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
void    hci_misc_modelviewer_callback (Widget w, 
		XtPointer client_data, XtPointer call_data);
void    hci_modeldata_viewer_not_available (Widget  w);
int	hci_activate_child( Display *d, Window w, char *cmd,
                            char *proc, char *win, int object_index );
void	timer_proc();

/************************************************************************
 *	Description: This is the main function for the HCI Misc		*
 *		     task.						*
 *									*
 *	Input:  argc - number of commandline arguments			*
 *		argv - pointer to commandline argument data		*
 *	Output: NONE							*
 *	Return: exit_code						*
 ************************************************************************/

int main( int argc, char *argv [] )
{
  Widget	form;
  Widget	frame;
  Widget	rowcol;
  Widget	control_rowcol;
  Widget	button;
	
  /* Initialize HCI. */

  HCI_init( argc, argv, HCI_MISC_TASK );

  /* Initialize the Xt toolkit and create the top level widget for *
   * the main control panel.                                       */

  Top_widget = HCI_get_top_widget();

  /* Add redundancy information if site FAA redundant */

  channel_number = HCI_get_channel_number();

  /* Define a form widget to be used as the manager for widgets in *
   * the top level window.                                         */

  form = XtVaCreateManagedWidget( "form",
         xmFormWidgetClass, Top_widget,
         XmNbackground,     hci_get_read_color( BACKGROUND_COLOR1 ),
         NULL );

  control_rowcol = XtVaCreateManagedWidget( "control_rowcol",
           xmRowColumnWidgetClass, form,
           XmNbackground,          hci_get_read_color( BACKGROUND_COLOR1 ),
           XmNorientation,         XmHORIZONTAL,
           XmNpacking,             XmPACK_TIGHT,
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

  XtAddCallback( button,
                 XmNactivateCallback, close_callback, NULL );

  frame = XtVaCreateManagedWidget( "frame",
          xmFrameWidgetClass,  form,
          XmNtopAttachment,    XmATTACH_WIDGET,
          XmNtopWidget,        control_rowcol,
          XmNleftAttachment,   XmATTACH_FORM,
          XmNrightAttachment,  XmATTACH_FORM,
          XmNbottomAttachment, XmATTACH_FORM,
          NULL );

  rowcol = XtVaCreateManagedWidget( "rowcol",
           xmRowColumnWidgetClass, frame,
           XmNbackground,          hci_get_read_color( BACKGROUND_COLOR1 ),
           XmNorientation,         XmVERTICAL,
           XmNpacking,             XmPACK_TIGHT,
           XmNnumColumns,          1,
           XmNentryAlignment,      XmALIGNMENT_CENTER,
           NULL );

  button = XtVaCreateManagedWidget( "Restore Adaptation Data",
           xmPushButtonWidgetClass, rowcol,
           XmNforeground,           hci_get_read_color( BUTTON_FOREGROUND ),
           XmNbackground,           hci_get_read_color( BUTTON_BACKGROUND ),
           XmNfontList,             hci_get_fontlist( LIST ),
           NULL );

  XtAddCallback( button,
                 XmNactivateCallback, hci_misc_restore_adapt_callback, NULL );

  button = XtVaCreateManagedWidget( "Save Adaptation Data",
           xmPushButtonWidgetClass, rowcol,
           XmNforeground,           hci_get_read_color( BUTTON_FOREGROUND ),
           XmNbackground,           hci_get_read_color( BUTTON_BACKGROUND ),
           XmNfontList,             hci_get_fontlist( LIST ),
           NULL );

  XtAddCallback( button,
                 XmNactivateCallback, hci_misc_save_adapt_callback, NULL );

  button = XtVaCreateManagedWidget( "HCI Passwords",
           xmPushButtonWidgetClass, rowcol,
           XmNforeground,           hci_get_read_color( BUTTON_FOREGROUND ),
           XmNbackground,           hci_get_read_color( BUTTON_BACKGROUND ),
           XmNfontList,	            hci_get_fontlist( LIST ),
           NULL );

  XtAddCallback( button,
                 XmNactivateCallback, hci_password_callback, NULL );

  button = XtVaCreateManagedWidget( "Hardware Configuration",
           xmPushButtonWidgetClass, rowcol,
           XmNforeground,           hci_get_read_color( BUTTON_FOREGROUND ),
           XmNbackground,           hci_get_read_color( BUTTON_BACKGROUND ),
           XmNfontList,	            hci_get_fontlist( LIST ),
           NULL );

  XtAddCallback( button,
                 XmNactivateCallback, hci_hardware_config_callback, NULL );

  if( HCI_get_node() != HCI_RPGA_NODE )
  {
    XtSetSensitive( button, False );
  }

  button = XtVaCreateManagedWidget( "View Log File",
           xmPushButtonWidgetClass, rowcol,
           XmNforeground,           hci_get_read_color( BUTTON_FOREGROUND ),
           XmNbackground,           hci_get_read_color( BUTTON_BACKGROUND ),
           XmNfontList,	            hci_get_fontlist( LIST ),
           NULL );

  XtAddCallback( button,
                 XmNactivateCallback, hci_view_log_callback, NULL );

  button = XtVaCreateManagedWidget( "Model Data Viewer",
           xmPushButtonWidgetClass, rowcol,
           XmNforeground,           hci_get_read_color( BUTTON_FOREGROUND ),
           XmNbackground,           hci_get_read_color( BUTTON_BACKGROUND ),
           XmNfontList,             hci_get_fontlist( LIST ),
           NULL );

  XtAddCallback( button,
                 XmNactivateCallback, hci_misc_modelviewer_callback, NULL );

  XtRealizeWidget( Top_widget );

  /* Start HCI loop. */

  HCI_start( timer_proc, HCI_ONE_SECOND, NO_RESIZE_HCI );

  return 0;
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Close" button.				*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
close_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
  XtDestroyWidget( Top_widget );
}

/************************************************************************
 *	Description: This function is the timer event callback.		*
 *									*
 *	Input:	NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
timer_proc ()
{
  /* Nothing in timer event as of yet. */
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Restore Adaptation Data" button.		*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_misc_restore_adapt_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
  char task_name [ 128 ];
  char app_name [ 128 ];

  sprintf( app_name, "hci_restore_adapt -A %d", channel_number );
  sprintf( task_name, "hci_restore_adapt -A %d -O %d ",
           channel_number, RESTORE_ADAPT_BUTTON );
  strcat( task_name, " -name \"Restore Adaptation Data\"" );
  strcat( task_name, HCI_child_options_string() );

  HCI_LE_log( "Spawning %s", task_name );

  hci_activate_child( HCI_get_display(),
                      RootWindowOfScreen( HCI_get_screen() ),
                      task_name,
                      app_name,
                      "Restore Adaptation Data",
                      RESTORE_ADAPT_BUTTON );
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Save Adaptation Data" button.			*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_misc_save_adapt_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
  char task_name [ 128 ];
  char app_name [ 128 ];

  sprintf( app_name, "hci_save_adapt -A %d", channel_number );
  sprintf( task_name, "hci_save_adapt -A %d -O %d ",
           channel_number, SAVE_ADAPT_BUTTON );
  strcat( task_name, " -name \"Save Adaptation Data\"" );
  strcat( task_name, HCI_child_options_string() );

  HCI_LE_log( "Spawning %s", task_name );

  hci_activate_child( HCI_get_display(),
                      RootWindowOfScreen( HCI_get_screen() ),
                      task_name,
                      app_name,
                      "Save Adaptation Data",
                      SAVE_ADAPT_BUTTON );
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "HCI Passwords" button.			*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_password_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
  char task_name [ 128 ];
  char app_name [ 128 ];

  sprintf( app_name, "hci_password -A %d", channel_number );
  sprintf( task_name, "hci_password -A %d -O %d ",
           channel_number, HCI_PASSWORD_BUTTON );
  strcat( task_name, " -name \"HCI Passwords\"" );
  strcat( task_name, HCI_child_options_string() );

  HCI_LE_log( "Spawning %s", task_name );

  hci_activate_child( HCI_get_display(),
                      RootWindowOfScreen( HCI_get_screen() ),
                      task_name,
                      app_name,
                      "HCI Passwords",
                      HCI_PASSWORD_BUTTON );
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Hardware Configuration" button.		*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_hardware_config_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
  char task_name [ 128 ];
  char app_name [ 128 ];

  sprintf( app_name, "hci_hardware_config -A %d", channel_number );
  sprintf( task_name, "hci_hardware_config -A %d -O %d ",
           channel_number, HARDWARE_CONFIG_BUTTON );
  strcat( task_name, " -name \"Hardware Configuration\"" );
  strcat( task_name, HCI_child_options_string() );

  HCI_LE_log( "Spawning %s", task_name );

  hci_activate_child( HCI_get_display(),
                      RootWindowOfScreen( HCI_get_screen() ),
                      task_name,
                      app_name,
                      "Hardware Configuration",
                      HARDWARE_CONFIG_BUTTON );
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Model Data Viewer" button.			*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_misc_modelviewer_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
  char task_name [ 128 ];
  char app_name [ 128 ];

  sprintf( app_name, "modelviewer -A %d", channel_number );
  sprintf( task_name, "modelviewer -A %d -O %d ",
           channel_number, MODEL_DATA_VIEWER_BUTTON );
  strcat( task_name, " -name \"Model Data Viewer\"" );
  strcat( task_name, HCI_child_options_string() );

  if (HCI_is_satellite_connection()) 
     hci_modeldata_viewer_not_available(Top_widget);

  else{

     HCI_LE_log( "Spawning %s", task_name );

     hci_activate_child( HCI_get_display(),
                         RootWindowOfScreen( HCI_get_screen() ),
                         task_name,
                         app_name,
                         "Model Data Viewer",
                         MODEL_DATA_VIEWER_BUTTON );

  }

}

/************************************************************************
 *      Description: The following function creates a popup window      *
 *                   to inform the user that the Model Data Viewer      *
 *                   is unavailable for current bandwidth.              *
 *                                                                      *
 *      Input:  NONE                                                    *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

void
hci_modeldata_viewer_not_available (
Widget  w
)
{
        char buf[HCI_BUF_128];

        if (HCI_is_satellite_connection())
        {
          sprintf( buf, "The Model Data Viewer Display window is not\navailable over a satellite connection." );
        }
        else
        {
          sprintf( buf, "The Model Data Viewer Display\nwindow is not available." );
        }
        hci_warning_popup( Top_widget, buf, NULL );
}


/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "View Log File" button.			*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_view_log_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
  char task_name [ 128 ];
  char app_name [ 128 ];

  sprintf( app_name, "hci_log -A %d", channel_number );
  sprintf( task_name, "hci_log -A %d -O %d ",
           channel_number, VIEW_LOG_BUTTON );
  strcat( task_name, " -name \"Log Viewer\"" );
  strcat( task_name, HCI_child_options_string() );

  HCI_LE_log( "Spawning %s", task_name );

  hci_activate_child( HCI_get_display(),
                      RootWindowOfScreen( HCI_get_screen() ),
                      task_name,
                      app_name,
                      "Log Viewer",
                      VIEW_LOG_BUTTON );
}

