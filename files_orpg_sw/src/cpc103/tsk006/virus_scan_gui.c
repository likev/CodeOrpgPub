/************************************************************************
 *      Module:  virus_scan_gui.c                                       *
 *                                                                      *
 *      Description:  This module is a gui wrapper for the virus	*
 *                    scanning software.				*
 *                                                                      *
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/07/26 17:16:01 $
 * $Id: virus_scan_gui.c,v 1.7 2012/07/26 17:16:01 ccalvert Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */

/* Include files. */

#include <hci.h>

enum { NO_SCAN, SCAN_CD };
enum { NO_MEDIA_MOUNTED, CD_MOUNTED };

/* Macros/definitions. */

#define	COPROCESS_BUF		512
#define	MAX_LINE_BUF		512
#define	BUF_SIZE		128
#define	ERROR_BUF_SIZE		COPROCESS_BUF + BUF_SIZE
#define	MAX_NUM_LABELS		30
#define	SCROLL_WIDTH		550
#define	SCROLL_HEIGHT		400

/*      Global widget definitions. */

static Widget       Top_widget         = (Widget) NULL;
static Widget       Scan_cd_button     = (Widget) NULL;

/* Global variables. */

static void	*Cp = NULL;
static int	Coprocess_flag = HCI_CP_NOT_STARTED;
static int	Scan_flag = NO_SCAN;
static int	Media_mounted_flag = NO_MEDIA_MOUNTED;
static char	Popup_buf[ERROR_BUF_SIZE] = "";
static char	Media_mount_point[BUF_SIZE] = "";

static int	Destroy_callback( Widget, XtPointer, XtPointer );
static void	Close_callback( Widget, XtPointer, XtPointer );
static void	Scan_cd_callback( Widget, XtPointer, XtPointer );
static int 	Mount_media();
static void	Unmount_media();
static void	Start_coprocess();
static void	Manage_coprocess();
static void	Close_coprocess();
static void	Set_display();
static void	Reset_display();
static void	Timer_proc();

/************************************************************************
 *      Description: This is the main function for the Virus Scan task. *
 ************************************************************************/

int main( int argc, char *argv[] )
{
  Widget          form;
  Widget          form_rowcol;
  Widget          rowcol;
  Widget          frame;
  Widget          close_rowcol;
  Widget          close_button;

  /* Initialize HCI. */

  HCI_partial_init( argc, argv, -1 );

  Top_widget = HCI_get_top_widget();

  HCI_set_destroy_callback( Destroy_callback );

  /* Need to reset the GUI title. */

  XtVaSetValues( Top_widget, XmNtitle, "Virus Scan Control", NULL );

  /* Use a form widget and rowcol to organize the various menu widgets. */

  form = XtVaCreateManagedWidget( "form",
                xmFormWidgetClass, Top_widget,
                XmNautoUnmanage,   False,
                XmNbackground,     hci_get_read_color (BACKGROUND_COLOR1),
                NULL );

  form_rowcol = XtVaCreateManagedWidget( "form_rowcol",
                xmRowColumnWidgetClass, form,
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNorientation,         XmVERTICAL,
                XmNpacking,             XmPACK_TIGHT,
                XmNnumColumns,          1,
                NULL);

  /* Add close button. */

  close_rowcol = XtVaCreateManagedWidget( "close_rowcol",
                         xmRowColumnWidgetClass, form_rowcol,
                         XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),                         XmNorientation, XmHORIZONTAL,
                         XmNnumColumns, 1,
                         XmNtopAttachment, XmATTACH_FORM,
                         XmNleftAttachment, XmATTACH_FORM,
                         XmNrightAttachment, XmATTACH_FORM,
                         NULL);

  close_button = XtVaCreateManagedWidget( "Close",
                   xmPushButtonWidgetClass, close_rowcol,
                   XmNforeground, hci_get_read_color( BUTTON_FOREGROUND ),
                   XmNbackground, hci_get_read_color( BUTTON_BACKGROUND ),
                   XmNfontList, hci_get_fontlist( LIST ),
                   NULL);

  XtAddCallback( close_button, XmNactivateCallback, Close_callback, NULL );

  /* Outline buttons in frame. */

  frame = XtVaCreateManagedWidget( "frame",
                         xmFrameWidgetClass, form_rowcol,
                         XmNtopAttachment,       XmATTACH_WIDGET,
                         XmNtopWidget,           close_rowcol,
                         XmNleftAttachment,      XmATTACH_FORM,
                         XmNrightAttachment,     XmATTACH_FORM,
                         NULL);

  /* Define rowcol buttons. */

  rowcol = XtVaCreateManagedWidget( "rowcol",
                xmRowColumnWidgetClass, frame,
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNorientation,         XmHORIZONTAL,
                XmNpacking,             XmPACK_TIGHT,
                XmNnumColumns,          1,
                XmNisAligned,           False,
                XmNtopAttachment,       XmATTACH_FORM,
                XmNleftAttachment,      XmATTACH_FORM,
                XmNrightAttachment,     XmATTACH_FORM,
                NULL );

  XtVaCreateManagedWidget( "Put media in media drive and click ",
                xmLabelWidgetClass,     rowcol,
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

  Scan_cd_button = XtVaCreateManagedWidget (" Scan ",
                xmPushButtonWidgetClass,rowcol,
                XmNforeground,          hci_get_read_color (BUTTON_FOREGROUND),
                XmNbackground,          hci_get_read_color (BUTTON_BACKGROUND),
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

  XtAddCallback( Scan_cd_button, XmNactivateCallback, Scan_cd_callback, NULL );

  /* Display gui to screen. */

  XtRealizeWidget( Top_widget );

  /* Start HCI loop. */

  HCI_start( Timer_proc, HCI_ONE_SECOND, NO_RESIZE_HCI );

  return 0;
}

/************************************************************************
 *      Description: This function is activated when the Virus Scan     *
 *                   window is destroyed.                               *
 ************************************************************************/

static int Destroy_callback( Widget w, XtPointer x, XtPointer y )
{
  if( Coprocess_flag != HCI_CP_NOT_STARTED ||
      Media_mounted_flag != NO_MEDIA_MOUNTED )
  {
    sprintf( Popup_buf, "You are not allowed to exit until\n" );
    strcat( Popup_buf, "the virus scan has completed." );
    hci_error_popup( Top_widget, Popup_buf, NULL );
    return HCI_NOT_OK_TO_EXIT;
  }

  return HCI_OK_TO_EXIT;
}

/************************************************************************
 *      Description: This function is activated when the user selects   *
 *                   the "Close" button in the Virus Scan window.       *
 ************************************************************************/

static void Close_callback( Widget w, XtPointer x, XtPointer y )
{
  if( Coprocess_flag != HCI_CP_NOT_STARTED ||
      Media_mounted_flag != NO_MEDIA_MOUNTED )
  {
    sprintf( Popup_buf, "You are not allowed to exit until\n" );
    strcat( Popup_buf, "the virus scan has completed." );
    hci_error_popup( Top_widget, Popup_buf, NULL );
  }
  else
  {
    XtDestroyWidget( Top_widget );
  }
}

/************************************************************************
 *      Description: This is the timer function for the Virus Scan task *
 ************************************************************************/

static void Timer_proc()
{
  /* Do we need to start any processes? */

  if( Scan_flag != NO_SCAN && !Media_mounted_flag )
  {
    Media_mounted_flag = Mount_media();
    if( Media_mounted_flag == NO_MEDIA_MOUNTED )
    {
      Scan_flag = NO_SCAN;
      Reset_display();
    }
  }
  else if( Scan_flag != NO_SCAN && Media_mounted_flag )
  {
    Scan_flag = NO_SCAN;
    Start_coprocess();
  }
  else if( Coprocess_flag == HCI_CP_STARTED )
  {
    Manage_coprocess();
  } 
  else if( Coprocess_flag == HCI_CP_FINISHED )
  {
    Coprocess_flag = HCI_CP_NOT_STARTED;
    Unmount_media();
    Reset_display();
  } 
}

/************************************************************************
 *      Description: This function is activated when the user selects   *
 *                   the "Scan CD" button.
 ************************************************************************/

static void Scan_cd_callback( Widget w, XtPointer x, XtPointer y )
{
  Scan_flag = SCAN_CD;
  Set_display();
}

/************************************************************************
 *      Description: Mount media to be scanned.                         *
 ************************************************************************/

static int Mount_media()
{
  char cmd[BUF_SIZE];
  char buf[BUF_SIZE];
  int ret = -1;
  int n = -1;

  if( Scan_flag == SCAN_CD )
  {
    sprintf( cmd, "medcp -mp cd" );
  }
  else
  {
    return NO_MEDIA_MOUNTED;
  }

  if( ( ret = ( MISC_system_to_buffer( cmd, buf, BUF_SIZE, &n ) >> 8 ) ) < 0 )
  {
    sprintf( Popup_buf, "MISC_system_to_buffer failed (%d) during mount", ret );
    hci_error_popup( Top_widget, Popup_buf, NULL );
    return NO_MEDIA_MOUNTED;
  }
  else if( ret > 0 )
  {
    sprintf( Popup_buf, "Command \"%s\" failed (%d)", cmd, ret );
    hci_error_popup( Top_widget, Popup_buf, NULL );
    return NO_MEDIA_MOUNTED;
  }
  else
  {
    if( n )
    {
      strcpy( Media_mount_point, buf );
      if( Media_mount_point[strlen( Media_mount_point )-1] == '\n' )
      {
        Media_mount_point[strlen( Media_mount_point )-1] = '\0';
      }
    }
  }

  return CD_MOUNTED;
}

/************************************************************************
 *      Description: Unmount media to be scanned.                       *
 ************************************************************************/

static void Unmount_media()
{
  char cmd[BUF_SIZE];
  int ret = -1;

  if( Media_mounted_flag == CD_MOUNTED )
  {
    sprintf( cmd, "medcp -u cd" );
  }
  else
  {
    return;
  }

  if( ( ret = ( MISC_system_to_buffer( cmd, NULL, -1, NULL ) >> 8 ) ) < 0 )
  {
    sprintf( Popup_buf, "MISC_system_to_buffer unmount failed (%d)", ret );
    hci_error_popup( Top_widget, Popup_buf, NULL );
  }
  else if( ret > 0 )
  {
    sprintf( Popup_buf, "Command \"%s\" failed (%d)", cmd, ret );
    hci_error_popup( Top_widget, Popup_buf, NULL );
  }
}

/************************************************************************
 *      Description: This function starts the Virus Scan coprocess.     *
 ************************************************************************/

static void Start_coprocess()
{
  char cmd[BUF_SIZE];
  int ret = -1;

  /* Build function to call. */

  sprintf( cmd, "uvscan -r --secure --ignore-links %s/*", Media_mount_point );

  /* Start coprocess. */

  if( ( ret = MISC_cp_open( cmd, MISC_CP_MANAGE, &Cp ) ) != 0 )
  {
    sprintf( Popup_buf, "MISC_cp_open failed (%d)\n", ret );
    strcat( Popup_buf, "Unable to continue." );
    hci_error_popup( Top_widget, Popup_buf, NULL );
    Coprocess_flag = HCI_CP_FINISHED;
  }

  Coprocess_flag = HCI_CP_STARTED;
}

/************************************************************************
 *      Description: Manage output of virus scan coprocess.             *
 ************************************************************************/

static void Manage_coprocess()
{
  int ret = -1;
  char cp_buf[COPROCESS_BUF];

  while( Cp != NULL &&
         ( ret = MISC_cp_read_from_cp( Cp, cp_buf, COPROCESS_BUF ) ) != 0 )
  {
    /* Strip trailing newline. */

    if( cp_buf[strlen( cp_buf )-1] == '\n' )
    {
      cp_buf[strlen( cp_buf )-1] = '\0';
    }

    /* MISC function succeeded. Take appropriate action. */

    if( ret == MISC_CP_DOWN )
    {
      /* Check exit status and take appropriate action. */
      if( ( ret = MISC_cp_get_status( Cp ) >> 8 ) != 0 )
      {
        /* Exit status indicates an error. */
        sprintf( Popup_buf, "Virus Scan process failed (%d).", ret );
        strcat( Popup_buf, "\nDo not use media." );
        hci_error_popup( Top_widget, Popup_buf, NULL );
      }
      else
      {
        hci_info_popup( Top_widget, "Scan successful", NULL );
      }
      Close_coprocess();
      break;
    }
    else if( ret == MISC_CP_STDERR )
    {
      sprintf( Popup_buf, "Error running uvscan.\n\n" );
      strcat( Popup_buf, cp_buf );
      hci_error_popup( Top_widget, Popup_buf, NULL );
    }
    else
    {
      /* Do nothing. */
    }
  }
}

/************************************************************************
 *      Description: Close coproces.
 ************************************************************************/

static void Close_coprocess()
{
  MISC_cp_close( Cp );
  Cp = NULL;
  Coprocess_flag = HCI_CP_FINISHED;
}

/************************************************************************
 *      Description: This function resets the display to default.       *
 ************************************************************************/

static void Set_display()
{
  HCI_busy_cursor();
  XtSetSensitive( Scan_cd_button, False );
}

/************************************************************************
 *      Description: This function resets the display to default.       *
 ************************************************************************/

static void Reset_display()
{
  Scan_flag = NO_SCAN;
  Media_mounted_flag = NO_MEDIA_MOUNTED;
  sprintf( Media_mount_point, "%s", "" );
  HCI_default_cursor();
  XtSetSensitive( Scan_cd_button, True );
}
