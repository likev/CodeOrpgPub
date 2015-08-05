/************************************************************************
 *									*
 *	Module:  hci_restore_adapt.c					*
 *									*
 *	Description:  This module provides a gui for the restoration	*
 *		      of adaptation data from media.			*
 *									*
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2010/03/10 18:57:28 $
 * $Id: hci_restore_adapt.c,v 1.39 2010/03/10 18:57:28 ccalvert Exp $
 * $Revision: 1.39 $
 * $State: Exp $
 */

/*	Local include file definitions.			*/

#include <hci.h>

/*	Global widget definitions.	*/

static	Widget	Top_widget = ( Widget ) NULL;
static	Widget	Start_button = ( Widget ) NULL;

/*	Global variables.	*/

static  int	Media_flag = HCI_CD_MEDIA_FLAG;
static  int	Restore_channel_number = 1;
static	char	Msg_buf[ HCI_CP_MAX_BUF + HCI_BUF_256 ];
static	int	Restore_adapt_flag = HCI_NO_FLAG;
static	int	Coprocess_flag = HCI_CP_NOT_STARTED;
static	void	*Cp;

/*	Function prototypes.		*/

static int Destroy_callback();
static void Close_callback( Widget, XtPointer, XtPointer );
static void Media_toggle_callback( Widget, XtPointer, XtPointer );
static void Faa_channel_toggle_callback( Widget, XtPointer, XtPointer );
static void Start_callback( Widget, XtPointer, XtPointer );
static void Start_coprocess();
static void Manage_coprocess();
static void Close_coprocess();
static void Reset_to_initial_state();
static void Timer_proc();


/************************************************************************
 *	Description: This is the main function for the Restore		*
 *		     Adaptation Data task.				*
 *									*
 *	Input:  argc - number of commandline arguments			*
 *		argv - pointer to commandline argument data		*
 *	Output: NONE							*
 *	Return: exit_code						*
 ************************************************************************/

int main( int argc, char *argv[] )
{
  Widget form;
  Widget frame;
  Widget frame_rowcol;
  Widget control_rowcol;
  Widget rowcol;
  Widget faa_rowcol;
  Widget button;
  Widget faa_button1;
  Widget faa_button2;
  Widget label;
  Widget faa_channel_toggle;
  Widget media_toggle;
  Arg args[ 16 ];
  int n;
  int channel_number = 1;


  /* Initialize HCI. */

  HCI_init( argc, argv, HCI_RESTORE_ADAPT_TASK );

  Top_widget = HCI_get_top_widget();

  HCI_set_destroy_callback( Destroy_callback );

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

  XtAddCallback( button, XmNactivateCallback, Close_callback, NULL );

  /* Build the widgets for restoring adaptation data. */

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

  /* If this is an FAA system, build toggle buttons so *
   * user can choose channel one or channel two.       */

  if( HCI_get_system() == HCI_FAA_SYSTEM )
  {
    faa_rowcol = XtVaCreateManagedWidget( "faa_rowcol",
          xmRowColumnWidgetClass, frame_rowcol,
          XmNbackground,          hci_get_read_color( BACKGROUND_COLOR1 ),
          XmNorientation,         XmHORIZONTAL,
          XmNpacking,             XmPACK_TIGHT,
          XmNnumColumns,          1,
          XmNentryAlignment,      XmALIGNMENT_CENTER,
          NULL );

    label = XtVaCreateManagedWidget( "FAA Channel: ",
          xmLabelWidgetClass,	faa_rowcol,
          XmNforeground,	hci_get_read_color( TEXT_FOREGROUND ),
          XmNbackground,	hci_get_read_color( BACKGROUND_COLOR1 ),
          XmNfontList,		hci_get_fontlist( LIST ),
          NULL );

    faa_channel_toggle = XmCreateRadioBox( faa_rowcol, "faa_channel", args, n );

    faa_button1 = XtVaCreateManagedWidget( "RPGA 1",
        xmToggleButtonWidgetClass,	faa_channel_toggle,
        XmNselectColor,			hci_get_read_color( WHITE ),
        XmNforeground,			hci_get_read_color( TEXT_FOREGROUND ),
        XmNbackground,			hci_get_read_color( BACKGROUND_COLOR1 ),
        XmNfontList,			hci_get_fontlist( LIST ),
        NULL);

    XtAddCallback( faa_button1,
                   XmNarmCallback,  Faa_channel_toggle_callback,
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
                   XmNarmCallback,  Faa_channel_toggle_callback,
                   (XtPointer) 2 );

    if( channel_number == 1 )
    {
      XtVaSetValues( faa_button1, XmNset, True, NULL );
      XtVaSetValues( faa_button2, XmNset, False, NULL );
      Restore_channel_number = 1;
    }
    else if( channel_number == 2 )
    {
      XtVaSetValues( faa_button1, XmNset, False, NULL );
      XtVaSetValues( faa_button2, XmNset, True, NULL );
      Restore_channel_number = 2;
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
            xmLabelWidgetClass,	rowcol,
            XmNforeground,	hci_get_read_color( TEXT_FOREGROUND ),
            XmNbackground,	hci_get_read_color( BACKGROUND_COLOR1 ),
            XmNfontList,		hci_get_fontlist( LIST ),
            NULL );

    media_toggle = XmCreateRadioBox( rowcol, "media_toggle", args, n );

    button = XtVaCreateManagedWidget( "Floppy",
        xmToggleButtonWidgetClass,	media_toggle,
        XmNselectColor,			hci_get_read_color( WHITE ),
        XmNforeground,			hci_get_read_color( TEXT_FOREGROUND ),
        XmNbackground,			hci_get_read_color( BACKGROUND_COLOR1 ),
        XmNfontList,			hci_get_fontlist( LIST ),
        XmNset,				False,
        NULL);

    XtAddCallback( button,
                   XmNarmCallback,  Media_toggle_callback,
                   (XtPointer) HCI_FLOPPY_MEDIA_FLAG );

    button = XtVaCreateManagedWidget( "CD",
        xmToggleButtonWidgetClass,	media_toggle,
        XmNselectColor,			hci_get_read_color( WHITE ),
        XmNforeground,			hci_get_read_color( TEXT_FOREGROUND ),
        XmNbackground,			hci_get_read_color( BACKGROUND_COLOR1 ),
        XmNfontList,			hci_get_fontlist( LIST ),
        XmNset,				True,
        NULL);

    XtAddCallback( button,
                   XmNarmCallback,  Media_toggle_callback,
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

  label = XtVaCreateManagedWidget( "Insert Adaptation Media into Media Drive and Click",
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

  XtAddCallback( Start_button, XmNactivateCallback, Start_callback, NULL );

  XtRealizeWidget( Top_widget );

  /* Start HCI loop. */

  HCI_start( Timer_proc, HCI_ONE_AND_HALF_SECOND, NO_RESIZE_HCI );

  return 0;
}

/************************************************************************
 * Destroy_callback: Exit task.
 ************************************************************************/

static int Destroy_callback()
{
  if( Coprocess_flag != HCI_CP_NOT_STARTED )
  {
    sprintf( Msg_buf, "You are not allowed to exit this task\n" );
    strcat( Msg_buf, "until the restore process has been completed.\n" );
    hci_error_popup( Top_widget, Msg_buf, NULL );
    return HCI_NOT_OK_TO_EXIT;
  }

  return HCI_OK_TO_EXIT;
}

/************************************************************************
 * Close_callback: Callback for "Close" button.
 ************************************************************************/

static void Close_callback( Widget w, XtPointer x, XtPointer y )
{
  if( Coprocess_flag != HCI_CP_NOT_STARTED )
  {
    sprintf( Msg_buf, "You are not allowed to exit this task\n" );
    strcat( Msg_buf, "until the restore process has been completed.\n" );
    hci_error_popup( Top_widget, Msg_buf, NULL );
  }
  else
  {
    XtDestroyWidget( Top_widget );
  }
}

/************************************************************************
 * Media_toggle_callback: Callback for media_toggle buttons.
 ************************************************************************/

static void Media_toggle_callback( Widget w, XtPointer x, XtPointer y )
{
  Media_flag = ( int ) x;
}

/************************************************************************
 * Faa_channel_toggle_callback: Callback for faa_channel_toggle buttons.
 ************************************************************************/

static void Faa_channel_toggle_callback( Widget w, XtPointer x, XtPointer y )
{
  Restore_channel_number = ( int ) x;
}

/************************************************************************
 * Start_callback: Callback for "Start" button.
 ************************************************************************/

static void Start_callback( Widget w, XtPointer x, XtPointer y )
{
  /* Set cursor to "busy". */

  HCI_busy_cursor();

  /* De-sensitize "Start" button. */

  XtVaSetValues( Start_button, XmNsensitive, False, NULL );

  /* Set flag indicating user wants to start restore process. */

  Restore_adapt_flag = HCI_YES_FLAG;
}

/************************************************************************
 * Timer_proc: Callback for event timer.
 ************************************************************************/

static void Timer_proc()
{
  /* Call various functions depending on flag values. */

  if( Restore_adapt_flag )
  {
    Restore_adapt_flag = HCI_NO_FLAG;
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
    /* Reset gui to initial state. */
    Reset_to_initial_state();
  }
}

/************************************************************************
 * Manage_coprocess: Manage coprocess.
 ************************************************************************/

static void Manage_coprocess()
{
  int ret;
  char cp_buf[HCI_CP_MAX_BUF];

  while( Cp != NULL &&
         ( ret = MISC_cp_read_from_cp( Cp, cp_buf, HCI_CP_MAX_BUF ) ) != 0 )
  {
    if( ret == HCI_CP_DOWN )
    {
      /* Coprocess finished, check exit status. */
      if( ( ret = ( MISC_cp_get_status( Cp ) >> 8 ) ) != 0 )
      {
        /* Exit status indicates an error. */
        sprintf( Msg_buf, "Task restore_adapt failed (%d).\n", ret );

        switch( ret )
        {
          case MEDCP_DEVICE_NOT_FOUND :
            strcat( Msg_buf, "Medcp failed to find the media device." );
            break;
          case MEDCP_CHECK_MOUNT_FAILED :
            strcat( Msg_buf, "Check for mounted media failed." );
            break;
          case MEDCP_DEVICE_MOUNT_FAILED :
            strcat( Msg_buf, "Unable to mount media device." );
            break;
          case MEDCP_DEVICE_UNMOUNT_FAILED :
            strcat( Msg_buf, "Unable to unmount media device." );
            break;
          case MEDCP_DEVICE_MISMATCH :
            strcat( Msg_buf, "Mismatch detected between media devices." );
            break;
          case MEDCP_MEDIA_NOT_DETECTED :
            strcat( Msg_buf, "Media not detected." );
            break;
          case MEDCP_MEDIA_NOT_MOUNTED :
            strcat( Msg_buf, "Media not mounted." );
            break;
          case MEDCP_MEDIA_NOT_BLANK :
            strcat( Msg_buf, "Media not blank." );
            break;
          case MEDCP_MEDIA_NOT_WRITABLE :
            strcat( Msg_buf, "Media not writable." );
            break;
          case MEDCP_MEDIA_NOT_REWRITABLE :
            strcat( Msg_buf, "Media not rewritable." );
            break;
          case MEDCP_BLANKING_MEDIA_FAILED :
            strcat( Msg_buf, "Unable to blank media." );
            break;
          case MEDCP_MEDIA_WRITE_FAILED :
            strcat( Msg_buf, "Writing to media failed." );
            break;
          case MEDCP_MEDIA_VERIFY_FAILED :
            strcat( Msg_buf, "Media verification failed." );
            break;
          case MEDCP_MEDIA_TOO_SMALL :
            strcat( Msg_buf, "File(s) too large for media." );
            break;
          case MEDCP_CREATE_CD_IMAGE_FAILED :
            strcat( Msg_buf, "Creation of ISO image failed." );
            break;
          default :
            strcat( Msg_buf, "Unknown error." );
            break;
        }
        hci_error_popup( Top_widget, Msg_buf, NULL );
      }
      else
      {
        /* Let user know restore was a success. */
        sprintf( Msg_buf, "Successfully restored adaptation data" );
        hci_info_popup( Top_widget, Msg_buf, NULL );
      }
      Close_coprocess();
      break;
    }
    else if( ret == HCI_CP_STDOUT )
    {
      /* Do nothing. No output expected. */
    }
    else if( ret == HCI_CP_STDERR )
    {
      sprintf( Msg_buf, "Error restoring adaptation data.\n\n" );
      strcat( Msg_buf, cp_buf );
      hci_error_popup( Top_widget, Msg_buf, NULL );
    }
    else
    {
      /* Do nothing. No new data available. */
    }
  }
}

/************************************************************************
 * Close_coprocess: Close coprocess.
 ************************************************************************/

static void Close_coprocess()
{
  MISC_cp_close( Cp );
  Cp = NULL;
  Coprocess_flag = HCI_CP_FINISHED;
}

/************************************************************************
 * Start_coprocess: Start coprocess to restore adaptation data.
 ************************************************************************/

static void Start_coprocess()
{
  int ret;
  char cmd[ HCI_BUF_256 ];

  /* Build command line/argument list to pass to restore_adapt task. */

  sprintf( cmd, "restore_adapt -n" );

  /* Is the adaptation data on floppy or CD? */

  if( Media_flag == HCI_FLOPPY_MEDIA_FLAG )
  {
    strcat( cmd, " -f" );
  }
  else
  {
    strcat( cmd, " -c" );
  }

  /* Which channel to restore? */

  if( Restore_channel_number == 1 )
  {
    strcat( cmd, " -o rpg1" );
  }
  else if( Restore_channel_number == 2 )
  {
    strcat( cmd, " -o rpg2" );
  }

  /* Start co-process using restore_adapt command. */

  if( ( ret = MISC_cp_open( cmd, HCI_CP_MANAGE, &Cp ) ) != 0 )
  {
    /* Co-process failed. */
    sprintf( Msg_buf, "MISC_cp_open failed (%d)\n\n", ret );
    strcat( Msg_buf, "Unable to Continue." );
    hci_error_popup( Top_widget, Msg_buf, NULL );
    Coprocess_flag = HCI_CP_FINISHED;
  }

  /* Co-process was successfully started. */

  Coprocess_flag = HCI_CP_STARTED;
}

/***************************************************************************
 * Reset_to_initial_state: Reset flags, buttons, cursors, etc. to initial
 *    states.
 ***************************************************************************/

static void Reset_to_initial_state()
{
  XtVaSetValues( Start_button, XmNsensitive, True, NULL );
  HCI_default_cursor();
}

