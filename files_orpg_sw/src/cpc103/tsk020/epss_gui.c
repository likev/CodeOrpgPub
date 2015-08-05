/************************************************************************

      The main source file for gui to run EPSS software.

************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/01/25 16:00:18 $
 * $Id: epss_gui.c,v 1.9 2012/01/25 16:00:18 ccalvert Exp $
 * $Revision: 1.9 $
 * $State: Exp $
 */

/* Include files. */

#include <hci.h>

/* Definitions. */

#define	TEMP_BUF		256
#define	NUM_LE_LOG_MESSAGES	500
#define	EPSS_HOMEPAGE_FILE	"epss_selection.htm"
#define	EPSS_DIR_UNDEFINED	-9999

/* Global variables. */

static Widget	Top_widget = ( Widget ) NULL;
static Widget	Ok_button = ( Widget ) NULL;

/* Function prototypes. */

static int Destroy_callback( Widget, XtPointer, XtPointer );
static void Close_callback( Widget w, XtPointer, XtPointer );
static int EPSS_is_installed();
static int Launch_EPSS();
static void Timer_proc();

/**************************************************************************

    The main function.

**************************************************************************/

int main( int argc, char **argv )
{
  Widget	form = ( Widget ) NULL;
  Widget	form_rowcol = ( Widget ) NULL;
  Widget	close_rowcol = ( Widget ) NULL;
  Widget	button = ( Widget ) NULL;
  Widget	label = ( Widget ) NULL;
  int		EPSS_is_installed_flag = -1;
  int		ret_code = -1;
  char		label_buf[ TEMP_BUF ];
  char		space_buf[ TEMP_BUF ];

  /* Initialize HCI. */

  HCI_partial_init( argc, argv, -1 );

  Top_widget = HCI_get_top_widget();

  HCI_set_destroy_callback( Destroy_callback );

  XtVaSetValues( Top_widget, XmNtitle, "EPSS", NULL );

  /* Ensure write permission for ORPGDAT_HCI_DATA. */

  ORPGDA_write_permission( ORPGDAT_HCI_DATA );

  /* Define form to hold the widgets. */

  form = XtVaCreateManagedWidget( "mode_list_form",
                 xmFormWidgetClass, Top_widget,
                 NULL);

  form_rowcol = XtVaCreateManagedWidget( "form_rowcol",
                        xmRowColumnWidgetClass, form,
                        XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
                        XmNorientation, XmVERTICAL,
                        XmNpacking, XmPACK_TIGHT,
                        XmNnumColumns, 1,
                        NULL);

  /* Check to see if EPSS has been installed. If it has, launch
     EPSS software in web browser. If EPSS has not been installed,
     tell user. */

  EPSS_is_installed_flag = EPSS_is_installed();

  HCI_LE_log( "Install flag = %d", EPSS_is_installed_flag );

  if( EPSS_is_installed_flag == YES )
  {
    HCI_LE_log( "EPSS is installed...launching firefox/EPSS" );
    ret_code = Launch_EPSS();

    /* If launch was successful, our job is done, so exit. */
    if( ret_code > -1 )
    {
      HCI_LE_log( "Firefox/EPSS launch succeeded...close GUI" );
      Close_callback( NULL, NULL, NULL );
    }
    else
    {
      /* Launch was not successful. Turn GUI into a giant warning
         popup letting user know bad things happened. */ 

      label = XtVaCreateManagedWidget( " ",
                 xmLabelWidgetClass, form_rowcol,
                 XmNforeground, hci_get_read_color( TEXT_FOREGROUND ),
                 XmNbackground, hci_get_read_color( WARNING_COLOR ),
                 XmNfontList, hci_get_fontlist( LIST ),
                 NULL);

      if( ret_code == EPSS_DIR_UNDEFINED )
      {
        HCI_LE_error( "Firefox/EPSS launch failed: EPSS_DIR is undefined" );
        sprintf( label_buf, "Environmental variable EPSS_DIR undefined" );
        sprintf( space_buf, "                 " );
      }
      else
      {
        HCI_LE_error( "Firefox/EPSS launch failed: (%d)", ret_code );
        sprintf( label_buf, "Failure to launch firefox/EPSS (%d)", ret_code );
        sprintf( space_buf, "              " );
      }

      label = XtVaCreateManagedWidget( label_buf,
                 xmLabelWidgetClass, form_rowcol,
                 XmNforeground, hci_get_read_color( TEXT_FOREGROUND ),
                 XmNbackground, hci_get_read_color( WARNING_COLOR ),
                 XmNfontList, hci_get_fontlist( LIST ),
                 NULL);

      close_rowcol = XtVaCreateManagedWidget( "close_rowcol",
                         xmRowColumnWidgetClass, form_rowcol,
                         XmNbackground, hci_get_read_color( WARNING_COLOR ),
                         XmNorientation, XmHORIZONTAL,
                         XmNnumColumns, 1,
                         XmNtopAttachment, XmATTACH_FORM,
                         XmNleftAttachment, XmATTACH_FORM,
                         XmNrightAttachment, XmATTACH_FORM,
                         NULL);

      label = XtVaCreateManagedWidget( space_buf,
                 xmLabelWidgetClass, close_rowcol,
                 XmNforeground, hci_get_read_color( TEXT_FOREGROUND ),
                 XmNbackground, hci_get_read_color( WARNING_COLOR ),
                 XmNfontList, hci_get_fontlist( LIST ),
                 NULL);

      button = XtVaCreateManagedWidget( " OK ",
                   xmPushButtonWidgetClass, close_rowcol,
                   XmNforeground, hci_get_read_color( BUTTON_FOREGROUND ),
                   XmNbackground, hci_get_read_color( BUTTON_BACKGROUND ),
                   XmNfontList, hci_get_fontlist( LIST ),
                   NULL);

      XtAddCallback( button, XmNactivateCallback, Close_callback, NULL );

      XtVaSetValues( form_rowcol,
                 XmNbackground, hci_get_read_color( WARNING_COLOR ),
                 NULL );

      XtVaSetValues( Top_widget,
                 XmNtitle, "Error Popup",
                 NULL );
    }
  }
  else if( EPSS_is_installed_flag == NO )
  {
    HCI_LE_error( "EPSS is not installed" );

    label = XtVaCreateManagedWidget( " ",
               xmLabelWidgetClass, form_rowcol,
               XmNforeground, hci_get_read_color( TEXT_FOREGROUND ),
               XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
               XmNfontList, hci_get_fontlist( LIST ),
               NULL);

    sprintf( label_buf, "EPSS is not installed." );

    label = XtVaCreateManagedWidget( label_buf,
               xmLabelWidgetClass, form_rowcol,
               XmNforeground, hci_get_read_color( TEXT_FOREGROUND ),
               XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
               XmNfontList, hci_get_fontlist( LIST ),
               NULL);

    close_rowcol = XtVaCreateManagedWidget( "close_rowcol",
                       xmRowColumnWidgetClass, form_rowcol,
                       XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
                       XmNpacking, XmPACK_TIGHT,
                       XmNorientation, XmHORIZONTAL,
                       XmNnumColumns, 1,
                       XmNtopAttachment, XmATTACH_FORM,
                       XmNleftAttachment, XmATTACH_FORM,
                       XmNrightAttachment, XmATTACH_FORM,
                       NULL);

    label = XtVaCreateManagedWidget( "       ",
               xmLabelWidgetClass, close_rowcol,
               XmNforeground, hci_get_read_color( TEXT_FOREGROUND ),
               XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
               XmNfontList, hci_get_fontlist( LIST ),
               NULL);

    Ok_button = XtVaCreateManagedWidget( " OK ",
                 xmPushButtonWidgetClass, close_rowcol,
                 XmNforeground, hci_get_read_color( BUTTON_FOREGROUND ),
                 XmNbackground, hci_get_read_color( BUTTON_BACKGROUND ),
                 XmNfontList, hci_get_fontlist( LIST ),
                 NULL);

    XtAddCallback( Ok_button, XmNactivateCallback, Close_callback, NULL );
  }
  else
  {
    HCI_LE_error( "Unable to determine if EPSS is installed" );

    label = XtVaCreateManagedWidget( " ",
               xmLabelWidgetClass, form_rowcol,
               XmNforeground, hci_get_read_color( TEXT_FOREGROUND ),
               XmNbackground, hci_get_read_color( WARNING_COLOR ),
               XmNfontList, hci_get_fontlist( LIST ),
               NULL);

    sprintf( label_buf, "Unable to determine if EPSS is\ninstalled (error code: %d).\nUnable to continue.", EPSS_is_installed_flag );

    label = XtVaCreateManagedWidget( label_buf,
               xmLabelWidgetClass, form_rowcol,
               XmNforeground, hci_get_read_color( TEXT_FOREGROUND ),
               XmNbackground, hci_get_read_color( WARNING_COLOR ),
               XmNfontList, hci_get_fontlist( LIST ),
               NULL);

    close_rowcol = XtVaCreateManagedWidget( "close_rowcol",
                       xmRowColumnWidgetClass, form_rowcol,
                       XmNbackground, hci_get_read_color( WARNING_COLOR ),
                       XmNorientation, XmHORIZONTAL,
                       XmNnumColumns, 1,
                       XmNtopAttachment, XmATTACH_FORM,
                       XmNleftAttachment, XmATTACH_FORM,
                       XmNrightAttachment, XmATTACH_FORM,
                       NULL);

      label = XtVaCreateManagedWidget( "            ",
                 xmLabelWidgetClass, close_rowcol,
                 XmNforeground, hci_get_read_color( TEXT_FOREGROUND ),
                 XmNbackground, hci_get_read_color( WARNING_COLOR ),
                 XmNfontList, hci_get_fontlist( LIST ),
                 NULL);

      button = XtVaCreateManagedWidget( " OK ",
                   xmPushButtonWidgetClass, close_rowcol,
                   XmNforeground, hci_get_read_color( BUTTON_FOREGROUND ),
                   XmNbackground, hci_get_read_color( BUTTON_BACKGROUND ),
                   XmNfontList, hci_get_fontlist( LIST ),
                   NULL);

      XtAddCallback( button, XmNactivateCallback, Close_callback, NULL );

      XtVaSetValues( form_rowcol,
                 XmNbackground, hci_get_read_color( WARNING_COLOR ),
                 NULL );

      XtVaSetValues( Top_widget,
                 XmNtitle, "Error Popup",
                 NULL );
  }

  /* Display gui to screen. */

  XtRealizeWidget (Top_widget);

  /* Start HCI loop. */
  
  HCI_start( Timer_proc, HCI_ONE_SECOND, NO_RESIZE_HCI );

  return 0;
}

/***************************************************************************/
/* DESTROY_CALLBACK(): Callback for main widget when it is destroyed.      */
/***************************************************************************/

static int Destroy_callback( Widget w, XtPointer x, XtPointer y )
{
  HCI_LE_log( "In Destroy_callback" );
  return HCI_OK_TO_EXIT;
}

/***************************************************************************/
/* CLOSE_CALLBACK(): Callback for "Close" button. Destroys main widget.    */
/***************************************************************************/

static void Close_callback( Widget w, XtPointer x, XtPointer y )
{
  HCI_LE_log( "In Close_callback" );
  XtDestroyWidget( Top_widget );
}

/***************************************************************************/
/* TIMER_PROC(): Callback for event timer.                                 */
/***************************************************************************/

static void Timer_proc()
{
  /* Do nothing */
}

/***************************************************************************/
/* EPSS_IS_INSTALLED: Has EPSS been installed?				   */
/***************************************************************************/

static int EPSS_is_installed()
{
  char cmd[ TEMP_BUF ];
  sprintf( cmd, "install_epss -N -c" );
  return MISC_system_to_buffer( cmd, NULL, -1, NULL ) >> 8;
}

/***************************************************************************/
/* LAUNCH_EPSS: Launch EPSS software in web browser.			   */
/***************************************************************************/

static int Launch_EPSS()
{
  char cmd[ TEMP_BUF ];

  if( getenv( "EPSS_DIR" ) == NULL )
  {
    return EPSS_DIR_UNDEFINED;
  }

  sprintf( cmd, "firefox file://%s/%s &", getenv( "EPSS_DIR" ), EPSS_HOMEPAGE_FILE );
  return MISC_system_to_buffer( cmd, NULL, -1, NULL ) >> 8;
}

