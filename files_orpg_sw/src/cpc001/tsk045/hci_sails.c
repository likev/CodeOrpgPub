/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2015/01/16 19:13:08 $
 * $Id: hci_sails.c,v 1.5 2015/01/16 19:13:08 steves Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

/* Local include files. */

#include <hci.h>

/* Defines and enums. */

enum { HCI_SAILS_DISABLE,
       HCI_SAILS_ONE_CUT,
       HCI_SAILS_TWO_CUTS,
       HCI_SAILS_THREE_CUTS,
       HCI_SAILS_MAX_NUM_CUTS };


/* Global variables. */

static Widget      Top_widget = (Widget) NULL;
static Widget      Close_button = (Widget) NULL;
static Widget      SAILS_status_label = (Widget) NULL;
static Widget      SAILS_cut_buttons[ HCI_SAILS_MAX_NUM_CUTS ];
static Pixel       Base_bg = (Pixel) NULL;
static Pixel       Button_bg = (Pixel) NULL;
static Pixel       Button_fg = (Pixel) NULL;
static Pixel       Text_bg = (Pixel) NULL;
static Pixel       Text_fg = (Pixel) NULL;
static Pixel       White_color = (Pixel) NULL;
static Pixel       Normal_color = (Pixel) NULL;
static Pixel       Warning_color = (Pixel) NULL;
static XmFontList  List_font = (XmFontList) NULL;
static char        Previous_status_buf[HCI_BUF_32];
static char        Msg_buf[HCI_BUF_256];
static int         Max_num_cuts = -1;
static int         Num_cuts_to_use = -1;
static int         Selected_num_cuts = -1;

/* Function prototypes. */

static void Close_button_callback( Widget, XtPointer, XtPointer );
static void SAILS_cut_button_callback( Widget, XtPointer, XtPointer );
static void SAILS_cut_confirm_yes( Widget, XtPointer, XtPointer );
static void SAILS_cut_confirm_no( Widget, XtPointer, XtPointer );
static void Update_SAILS_status();
static void Timer_proc();

/**********************************************************************

    Description: The main function.

***********************************************************************/

int main( int argc, char *argv[] )
{
  Widget form = (Widget) NULL;;
  Widget rowcol = (Widget) NULL;;
  Widget label = (Widget) NULL;;
  Widget label2 = (Widget) NULL;;
  Widget separator_widget = (Widget) NULL;;
  Widget control_frame = (Widget) NULL;;
  Widget control_form = (Widget) NULL;;
  char buf[5];
  int i = 0;

  /* Initialize HCI. */

  HCI_init( argc, argv, HCI_SAILS_TASK );

  Top_widget = HCI_get_top_widget();
  Base_bg = hci_get_read_color( BACKGROUND_COLOR1 );
  Button_bg = hci_get_read_color( BUTTON_BACKGROUND );
  Button_fg = hci_get_read_color( BUTTON_FOREGROUND );
  Text_bg = hci_get_read_color( TEXT_BACKGROUND );
  Text_fg = hci_get_read_color( TEXT_FOREGROUND );
  White_color = hci_get_read_color( WHITE );
  Normal_color = hci_get_read_color( NORMAL_COLOR );
  Warning_color = hci_get_read_color( WARNING_COLOR );
  List_font = hci_get_fontlist( LIST );
  sprintf( Previous_status_buf, " UNKNOWN " );

  HCI_PM( "Initialize Task Information" );

  /* Create a form to arrange all widgets */

  form = XtVaCreateWidget( "form",
        xmFormWidgetClass, Top_widget,
        XmNautoUnmanage, False,
        XmNforeground, Base_bg,
        XmNbackground, Base_bg,
        NULL );

  /* Create a row column widget to put the close and apply buttons */

  rowcol = XtVaCreateManagedWidget( "rowcol",
        xmRowColumnWidgetClass, form,
        XmNbackground, Base_bg,
        XmNorientation, XmHORIZONTAL,
        XmNnumColumns, 1,
        XmNleftOffset, 20,
        XmNtopAttachment, XmATTACH_FORM,
        XmNleftAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_FORM,
        NULL );

  /* Close button and close button callback */

  Close_button = XtVaCreateManagedWidget( "Close",
        xmPushButtonWidgetClass, rowcol,
        XmNbackground, Button_bg,
        XmNforeground, Button_fg,
        XmNfontList, List_font,
        NULL );

    XtAddCallback( Close_button,
        XmNactivateCallback, Close_button_callback, NULL );

  /* Add horizontal line to separate functional areas */

  separator_widget = XtVaCreateManagedWidget( "separator_widget",
        xmSeparatorWidgetClass, form,
        XmNleftAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_FORM,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, rowcol,
        NULL );

  /* Create a row column widget to put the status label */

  rowcol = XtVaCreateManagedWidget( "rowcol",
        xmRowColumnWidgetClass, form,
        XmNbackground, Base_bg,
        XmNorientation, XmHORIZONTAL,
        XmNnumColumns, 1,
        XmNleftOffset, 20,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, separator_widget,
        XmNleftAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_FORM,
        NULL );

  label = XtVaCreateManagedWidget( "SAILS Status: ",
        xmLabelWidgetClass, rowcol,
        XmNforeground, Text_fg,
        XmNbackground, Base_bg,
        XmNfontList, List_font,
        NULL );

  SAILS_status_label = XtVaCreateManagedWidget( "        ",
        xmLabelWidgetClass, rowcol,
        XmNforeground, Text_fg,
        XmNbackground, Base_bg,
        XmNfontList, List_font,
        NULL );

  /* Add horizontal line to separate functional areas */

  separator_widget = XtVaCreateManagedWidget( "separator_widget",
        xmSeparatorWidgetClass, form,
        XmNleftAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_FORM,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, rowcol,
        NULL );

  /* This frame will contain the SAILS control buttons */

  control_frame = XtVaCreateManagedWidget( "control_frame",
        xmFrameWidgetClass, form,
        XmNbackground, Base_bg,
        XmNleftOffset, 20,
        XmNrightOffset, 20,
        XmNtopOffset, 10,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, separator_widget,
        XmNleftAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_FORM,
        NULL );

  label = XtVaCreateManagedWidget( "SAILS Control",
        xmLabelWidgetClass, control_frame,
        XmNchildType, XmFRAME_TITLE_CHILD,
        XmNchildHorizontalAlignment, XmALIGNMENT_CENTER,
        XmNchildVerticalAlignment, XmALIGNMENT_CENTER,
        XmNbackground, Base_bg,
        XmNforeground, Text_fg,
        XmNfontList, List_font,
        NULL );

  /* Form for inside of frame. */

  control_form = XtVaCreateWidget( "control_form",
        xmFormWidgetClass, control_frame,
        XmNautoUnmanage, False,
        XmNforeground, Base_bg,
        XmNbackground, Base_bg,
        NULL );

  /* Add label to tell user what to do. */

  label = XtVaCreateManagedWidget( "   Select number of SAILS cuts",
        xmLabelWidgetClass, control_form,
        XmNforeground, Text_fg,
        XmNbackground, Base_bg,
        XmNfontList, List_font,
        NULL );

  label2 = XtVaCreateManagedWidget( "   for the next volume scan   ",
        xmLabelWidgetClass, control_form,
        XmNforeground, Text_fg,
        XmNbackground, Base_bg,
        XmNfontList, List_font,
        XmNleftAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_FORM,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, label,
        NULL );

  /* Row column widget to control spacing for toggle buttons. */

  rowcol = XtVaCreateManagedWidget( "rowcol",
        xmRowColumnWidgetClass, control_form,
        XmNbackground, Base_bg,
        XmNorientation, XmHORIZONTAL,
        XmNpacking, XmPACK_COLUMN,
        XmNnumColumns, 1,
        XmNmarginWidth, 30,
        XmNspacing, 30,
        XmNleftAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_FORM,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, label2,
        NULL );

  /* Get number of SAILS cuts for this site. */

  Max_num_cuts = hci_sails_get_site_max_num_cuts();
  Num_cuts_to_use = hci_sails_get_req_num_cuts();

  /* Add 1 to num_cuts to account for disable button */

  for( i = 0; i < (Max_num_cuts+1); i++ )
  {
    sprintf( buf, "%d", i );
    SAILS_cut_buttons[i] = XtVaCreateManagedWidget( buf,
        xmToggleButtonWidgetClass, rowcol,
        XmNbackground, Base_bg,
        XmNforeground, Text_fg,
        XmNselectColor, White_color,
        XmNfontList, List_font,
        NULL );

    XtAddCallback( SAILS_cut_buttons[i],
                   XmNvalueChangedCallback, SAILS_cut_button_callback,
                   (XtPointer) i );

    if( i == Num_cuts_to_use )
    {
      XtVaSetValues( SAILS_cut_buttons[i], XmNset, True, NULL );
    }
  }

  /* Add rowcol to organize bottom label(s) and control spacing. */

  rowcol = XtVaCreateManagedWidget( "rowcol",
        xmRowColumnWidgetClass, form,
        XmNbackground, Base_bg,
        XmNorientation, XmHORIZONTAL,
        XmNpacking, XmPACK_TIGHT,
        XmNleftAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_FORM,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, control_frame,
        NULL );

  label = XtVaCreateManagedWidget( "Selecting \"0\" cuts will disable SAILS.\nAll non-zero numbers are additional\nscans of the lowest elevation beyond\nany already defined in the VCP.",
        xmLabelWidgetClass, rowcol,
        XmNbackground, Base_bg,
        XmNforeground, Text_fg,
        XmNfontList, List_font,
        XmNleftAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_FORM,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, control_frame,
        NULL );

  /* Manage widgets. */

  XtManageChild( control_form );
  XtManageChild( form );

  /* Set initial display. */

  Update_SAILS_status();

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

    Description:  Cut buttons callback function.

***********************************************************************/

static void SAILS_cut_button_callback( Widget w, XtPointer x, XtPointer y )
{
  XmToggleButtonCallbackStruct    *state =
                  (XmToggleButtonCallbackStruct *) y;

  Selected_num_cuts = (int) x;

  if( state->set )
  {
    if( Selected_num_cuts == HCI_SAILS_DISABLE )
    {
      sprintf( Msg_buf, "You are about to DISABLE SAILS.\nThe change will take effect in\nthe next volume scan.\nDo you want to continue?" );
    }
    else
    {
      sprintf( Msg_buf, "You are about to change the number\nof SAILS cuts to %d. The change will\ntake effect in the next volume scan.\nDo you want to continue?", Selected_num_cuts );
    }
    hci_confirm_popup( Top_widget, Msg_buf, SAILS_cut_confirm_yes, SAILS_cut_confirm_no );
  }
}

/**********************************************************************

    Description:  Cut buttons callback function.

***********************************************************************/

static void SAILS_cut_confirm_yes( Widget w, XtPointer x, XtPointer y )
{
  int ret = -1;

  if( ( ret = hci_sails_set_num_cuts( Selected_num_cuts ) ) < 0 )
  {
    sprintf( Msg_buf, "hci_sails_set_num_cuts(%d) returned %d", Selected_num_cuts, ret );
    hci_error_popup( Top_widget, Msg_buf, NULL );
  }
  else
  {
    if( Selected_num_cuts == HCI_SAILS_DISABLE )
    {
      sprintf( Msg_buf, "SAILS Disabled" );
    }
    else
    {
      sprintf( Msg_buf, "SAILS set to %d cut(s)", Selected_num_cuts );
    }
    HCI_display_feedback( Msg_buf );
  }
}

/**********************************************************************

    Description:  Cut buttons callback function.

***********************************************************************/

static void SAILS_cut_confirm_no( Widget w, XtPointer x, XtPointer y )
{
}

/**********************************************************************

    Description: Timer callback function.

***********************************************************************/

static void Timer_proc()
{
  Update_SAILS_status();
}

/**********************************************************************

    Description: Updates the SAILS status label.

***********************************************************************/

static void Update_SAILS_status()
{
  int ret = -1;
  int bg = -1;
  int fg = -1;
  int i = -1;
  char status_buf[HCI_BUF_32];
  XmString status_string = NULL;

  /* Get SAILS status. */

  if( ( ret = hci_sails_get_status() ) < 0 )
  {
    HCI_LE_error( "hci_sails_get_status: %d", ret );
    sprintf( status_buf, " UNKNOWN  " );
    bg = Warning_color;
    fg = Text_fg;
  }
  else
  {
    if( ret == HCI_SAILS_STATUS_DISABLED )
    {
      sprintf( status_buf, " DISABLED " );
      bg = Warning_color;
      fg = Text_fg;
    }
    else if( ret == HCI_SAILS_STATUS_ACTIVE )
    {
      sprintf( status_buf, "  ACTIVE  " );
      bg = Normal_color;
      fg = Text_fg;
    }
    else if( ret == HCI_SAILS_STATUS_INACTIVE )
    {
      sprintf( status_buf, " INACTIVE " );
      bg = Normal_color;
      fg = Text_fg;
    }
    else
    {
      sprintf( status_buf, " UNKNOWN  " );
      bg = Warning_color;
      fg = Text_fg;
    }
  }

  /* Test for difference to prevent GUI from blinking during refresh. */

  if( strcmp( status_buf,Previous_status_buf ) != 0 )
  {
    HCI_LE_log( "SAILS: from %s to %s", Previous_status_buf, status_buf );
    strcpy( Previous_status_buf, status_buf );
    status_string = XmStringCreateLocalized( status_buf );
    XtVaSetValues( SAILS_status_label,
                   XmNlabelType, XmSTRING, 
                   XmNlabelString, status_string, 
                   XmNbackground, bg, 
                   XmNforeground, fg, NULL );
  }

  XmStringFree( status_string );

  Num_cuts_to_use = hci_sails_get_req_num_cuts();
  for( i = 0; i < (Max_num_cuts+1); i++ )
  {
    if( i == Num_cuts_to_use )
    {
      XtVaSetValues( SAILS_cut_buttons[i], XmNset, True, NULL );
    }
    else
    {
      XtVaSetValues( SAILS_cut_buttons[i], XmNset, False, NULL );
    }
  }
}



