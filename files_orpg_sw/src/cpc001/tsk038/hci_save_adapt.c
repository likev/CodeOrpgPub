/************************************************************************
 *									*
 *	Module:  hci_save_adapt.c					*
 *									*
 *	Description:  This module provides a gui wrapper for the saving *
 *		      of adaptation data to media.			*
 *									*
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/13 15:06:01 $
 * $Id: hci_save_adapt.c,v 1.12 2009/05/13 15:06:01 ccalvert Exp $
 * $Revision: 1.12 $
 * $State: Exp $
 */

/*	Local include file definitions.					*/

#include <hci.h>

/*	Global widget definitions.	*/

static	Widget		Top_widget = ( Widget ) NULL;
static	Widget		start_button = ( Widget ) NULL;

/*	Global variables.	*/

static  int	media_flag = HCI_CD_MEDIA_FLAG;  /* Media to write data to. */
static  int	channel_number = 1;    /* Channel number. */
static	char	msg_buf[ HCI_SYSTEM_MAX_BUF + HCI_BUF_256 ];
static  int	start_flag = HCI_NO_FLAG;

/*	Function prototypes.		*/

void save_adapt_close( Widget w, XtPointer client_data, XtPointer call_data );
void media_toggle_callback( Widget w, XtPointer client_data,
                            XtPointer call_data );
void faa_channel_toggle_callback( Widget w, XtPointer client_data,
                                  XtPointer call_data );
void save_adapt_start( Widget w, XtPointer client_data, XtPointer call_data );
void start_save_adapt();
void reset_gui();
void timer_proc();

/************************************************************************
 *	Description: This is the main function for the Save		*
 *		     Adaptation Data task.				*
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
  Widget faa_button1;
  Widget faa_button2;
  Widget label;
  Widget media_toggle;
  Widget faa_channel_toggle;
  Arg args[ 16 ];
  int n;

  /* Initialization HCI. */

  HCI_init( argc, argv, HCI_SAVE_ADAPT_TASK );

  Top_widget = HCI_get_top_widget();

  /* Add redundancy information if site FAA redundant */

  if( HCI_get_system() == HCI_FAA_SYSTEM )
  {
    channel_number = ORPGRED_channel_num( ORPGRED_MY_CHANNEL );
  }

  /* Set args for all toggle buttons. */

  n = 0;
  XtSetArg( args [n], XmNforeground, hci_get_read_color( TEXT_FOREGROUND ));
  n++;
  XtSetArg( args [n], XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ));
  n++;
  XtSetArg( args [n], XmNfontList, hci_get_fontlist (LIST));
  n++;
  XtSetArg( args [n], XmNpacking, XmPACK_TIGHT);
  n++;
  XtSetArg( args [n], XmNorientation, XmHORIZONTAL );
  n++;

  /* Use a form widget to be used as the manager for widgets *
   * in the top level window.                                */

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
                 XmNactivateCallback, save_adapt_close, NULL );

  /* Build the widgets for saving adaptation data. */

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

  /* If this is an FAA system, build toggle button to let *
   * user choose channel one or channel two.              */

  if( HCI_get_system() == HCI_FAA_SYSTEM )
  {
    rowcol = XtVaCreateManagedWidget( "faa_rowcol",
          xmRowColumnWidgetClass, frame_rowcol,
          XmNbackground,          hci_get_read_color( BACKGROUND_COLOR1 ),
          XmNorientation,         XmHORIZONTAL,
          XmNpacking,             XmPACK_TIGHT,
          XmNnumColumns,          1,
          XmNentryAlignment,      XmALIGNMENT_CENTER,
          NULL );

    label = XtVaCreateManagedWidget( "FAA Channel: ",
          xmLabelWidgetClass,	rowcol,
          XmNforeground,	hci_get_read_color( TEXT_FOREGROUND ),
          XmNbackground,	hci_get_read_color( BACKGROUND_COLOR1 ),
          XmNfontList,		hci_get_fontlist( LIST ),
          NULL );

    faa_channel_toggle = XmCreateRadioBox( rowcol, "faa_channel", args, n );

    faa_button1 = XtVaCreateManagedWidget( "RPGA 1",
        xmToggleButtonWidgetClass,	faa_channel_toggle,
        XmNselectColor,			hci_get_read_color( WHITE ),
        XmNforeground,			hci_get_read_color( TEXT_FOREGROUND ),
        XmNbackground,			hci_get_read_color( BACKGROUND_COLOR1 ),
        XmNfontList,			hci_get_fontlist( LIST ),
        XmNset,				True,
        NULL);

    XtAddCallback( faa_button1,
                   XmNarmCallback,  faa_channel_toggle_callback,
                   (XtPointer) 1 );

    faa_button2 = XtVaCreateManagedWidget( "RPGA 2",
        xmToggleButtonWidgetClass,	faa_channel_toggle,
        XmNselectColor,			hci_get_read_color( WHITE ),
        XmNforeground,			hci_get_read_color( TEXT_FOREGROUND ),
        XmNbackground,			hci_get_read_color( BACKGROUND_COLOR1 ),
        XmNfontList,			hci_get_fontlist( LIST ),
        XmNset,				False,
        NULL);

    XtAddCallback( faa_button2,
                   XmNarmCallback,  faa_channel_toggle_callback,
                   (XtPointer) 2 );

    if( channel_number == 1 )
    {
      XtVaSetValues( faa_button1, XmNset, True, NULL );
      XtVaSetValues( faa_button2, XmNset, False, NULL );
    }
    else if( channel_number == 2 )
    {
      XtVaSetValues( faa_button1, XmNset, False, NULL );
      XtVaSetValues( faa_button2, XmNset, True, NULL );
    }

    XtManageChild( faa_channel_toggle );
  }

  if( ! ORPGMISC_is_operational() )
  {
    /* Build the toggle for selecting media type. */

    rowcol = XtVaCreateManagedWidget( "media_rowcol",
            xmRowColumnWidgetClass, frame_rowcol,
            XmNbackground,          hci_get_read_color( BACKGROUND_COLOR1 ),
            XmNorientation,         XmHORIZONTAL,
            XmNpacking,             XmPACK_TIGHT,
            XmNnumColumns,          1,
            XmNentryAlignment,      XmALIGNMENT_CENTER,
            NULL );

    label = XtVaCreateManagedWidget( "Select Media:   ",
            xmLabelWidgetClass,   rowcol,
            XmNforeground,        hci_get_read_color( TEXT_FOREGROUND ),
            XmNbackground,        hci_get_read_color( BACKGROUND_COLOR1 ),
            XmNfontList,          hci_get_fontlist( LIST ),
            NULL );

    media_toggle = XmCreateRadioBox( rowcol, "media_toggle", args, n );
  
    button = XtVaCreateManagedWidget( "Floppy",
        xmToggleButtonWidgetClass,      media_toggle,
        XmNselectColor,                 hci_get_read_color( WHITE ),
        XmNforeground,                  hci_get_read_color( TEXT_FOREGROUND ),
        XmNbackground,                  hci_get_read_color( BACKGROUND_COLOR1 ),
        XmNfontList,                    hci_get_fontlist( LIST ),
        XmNset,                         False,
        NULL);

    XtAddCallback( button,
                   XmNarmCallback,  media_toggle_callback,
                   (XtPointer) HCI_FLOPPY_MEDIA_FLAG );

    button = XtVaCreateManagedWidget( "CD",
        xmToggleButtonWidgetClass,      media_toggle,
        XmNselectColor,                 hci_get_read_color( WHITE ),
        XmNforeground,                  hci_get_read_color( TEXT_FOREGROUND ),
        XmNbackground,                  hci_get_read_color( BACKGROUND_COLOR1 ),
        XmNfontList,                    hci_get_fontlist( LIST ),
        XmNset,                         True,
      NULL);

    XtAddCallback( button,
                   XmNarmCallback,  media_toggle_callback,
                   (XtPointer) HCI_CD_MEDIA_FLAG );

    XtManageChild( media_toggle );
  }

  /* Build "Start" button to actually start the process. */

  rowcol = XtVaCreateManagedWidget( "start_rowcol",
        xmRowColumnWidgetClass, frame_rowcol,
        XmNbackground,          hci_get_read_color( BACKGROUND_COLOR1 ),
        XmNorientation,         XmHORIZONTAL,
        XmNpacking,             XmPACK_TIGHT,
        XmNnumColumns,          1,
        XmNentryAlignment,      XmALIGNMENT_CENTER,
        NULL );

  label = XtVaCreateManagedWidget( "Insert Media into Media Drive and Click",
          xmLabelWidgetClass,	rowcol,
          XmNforeground,	hci_get_read_color( TEXT_FOREGROUND ),
          XmNbackground,	hci_get_read_color( BACKGROUND_COLOR1 ),
          XmNfontList,		hci_get_fontlist( LIST ),
          NULL );

  start_button = XtVaCreateManagedWidget( "Start",
          xmPushButtonWidgetClass, rowcol,
          XmNforeground,	hci_get_read_color( BUTTON_FOREGROUND ),
          XmNbackground,	hci_get_read_color( BUTTON_BACKGROUND ),
          XmNfontList,		hci_get_fontlist( LIST ),
          NULL );

  XtAddCallback( start_button,
                 XmNactivateCallback, save_adapt_start, NULL );

  XtRealizeWidget( Top_widget );

  /* Start HCI loop. */

  HCI_start( timer_proc, HCI_ONE_AND_HALF_SECOND, NO_RESIZE_HCI );

  return 0;
}

/************************************************************************
 * save_adapt_close: Callback for "Close" button.
 ************************************************************************/

void save_adapt_close( Widget w, XtPointer client_data, XtPointer call_data )
{
  XtDestroyWidget( Top_widget );
}

/************************************************************************
 * media_toggle_callback: Callback for media_toggle buttons.
 ************************************************************************/

void media_toggle_callback( Widget w, XtPointer client_data,
                            XtPointer call_data )
{
  media_flag = ( int ) client_data;
}

/************************************************************************
 * faa_channel_toggle_callback: Callback for faa_channel_toggle buttons.
 ************************************************************************/

void faa_channel_toggle_callback( Widget w, XtPointer client_data,
                                  XtPointer call_data )
{
  channel_number = ( int ) client_data;
}

/************************************************************************
 * save_adapt_start: Callback for "Start" button.
 ************************************************************************/

void save_adapt_start( Widget w, XtPointer client_data, XtPointer call_data )
{
  /* Set cursor to "busy". */

  HCI_busy_cursor();

  /* Desensitize button. */

  XtVaSetValues( start_button, XmNsensitive, False, NULL );

  /* Set start flag. */

  start_flag = HCI_YES_FLAG;
}

/************************************************************************
 * timer_proc: Callback for event timer.
 ************************************************************************/

void timer_proc()
{
  /* Call various functions depending on flag values. */

  if( start_flag )
  {
    /* Unset start flag. */
    start_flag = HCI_NO_FLAG;
    start_save_adapt();
  }
}

/************************************************************************
 * start_save_adapt: Start save adapt process.
 ************************************************************************/

void start_save_adapt( Widget w, XtPointer client_data, XtPointer call_data )
{
  int ret;
  int n_bytes;
  char output_buffer[ HCI_SYSTEM_MAX_BUF ];
  char cmd[ HCI_BUF_256 ];

  /* Build command line/argument list to pass to save_adapt task. */

  sprintf( cmd, "save_adapt -q" );

  /* Write the adaptation data to media or CD? */

  if( media_flag == HCI_FLOPPY_MEDIA_FLAG )
  {
    strcat( cmd, " -f" );
  }
  else
  {
    strcat( cmd, " -c" );
  }

  /* Set channel number. */

  if( channel_number == 1 )
  {
    strcat( cmd, " -o rpg1" );
  }
  else if( channel_number == 2 )
  {
    strcat( cmd, " -o rpg2" );
  }

  /* Call task that actually does the saving. */

  ret = MISC_system_to_buffer( cmd, output_buffer,
                               HCI_SYSTEM_MAX_BUF, &n_bytes );
  ret = ret >> 8;

  if( ret != 0 )
  {
    /* Exit status indicates an error. */
    sprintf( msg_buf, "Task save_adapt failed (%d).\n", ret );

    switch( ret )
    {
      case MEDCP_DEVICE_NOT_FOUND :
        strcat( msg_buf, "Medcp failed to find the media device." );
        break;
      case MEDCP_CHECK_MOUNT_FAILED :
        strcat( msg_buf, "Check for mounted media failed." );
        break;
      case MEDCP_DEVICE_MOUNT_FAILED :
        strcat( msg_buf, "Unable to mount media device." );
        break;
      case MEDCP_DEVICE_UNMOUNT_FAILED :
        strcat( msg_buf, "Unable to unmount media device." );
        break;
      case MEDCP_DEVICE_MISMATCH :
        strcat( msg_buf, "Mismatch detected between media devices." );
        break;
      case MEDCP_MEDIA_NOT_DETECTED :
        strcat( msg_buf, "Media not detected." );
        break;
      case MEDCP_MEDIA_NOT_MOUNTED :
        strcat( msg_buf, "Media not mounted." );
        break;
      case MEDCP_MEDIA_NOT_BLANK :
        strcat( msg_buf, "Media not blank." );
        break;
      case MEDCP_MEDIA_NOT_WRITABLE :
        strcat( msg_buf, "Media not writable." );
        break;
      case MEDCP_MEDIA_NOT_REWRITABLE :
        strcat( msg_buf, "Media not rewritable." );
        break;
      case MEDCP_BLANKING_MEDIA_FAILED :
        strcat( msg_buf, "Unable to blank media." );
        break;
      case MEDCP_MEDIA_WRITE_FAILED :
        strcat( msg_buf, "Writing to media failed." );
        break;
      case MEDCP_MEDIA_VERIFY_FAILED :
        strcat( msg_buf, "Media verification failed." );
        break;
      case MEDCP_MEDIA_TOO_SMALL :
        strcat( msg_buf, "File(s) too large for media." );
        break;
      case MEDCP_CREATE_CD_IMAGE_FAILED :
        strcat( msg_buf, "Creation of ISO image failed." );
        break;
      default :
        strcat( msg_buf, "\n" );
        strcat( msg_buf, output_buffer );
        break;
    }

    hci_error_popup( Top_widget, msg_buf, reset_gui );
  }
  else
  {
    hci_info_popup( Top_widget, "Successfully saved adaptation data.", reset_gui );
  }
}

/************************************************************************
 * reset_gui: Reset gui components to default values.
 ************************************************************************/

void reset_gui()
{
  /* Unset "busy" cursor. */

  HCI_default_cursor();

  /* Sensitize "Start" button. */

  XtVaSetValues( start_button, XmNsensitive, True, NULL );
}


