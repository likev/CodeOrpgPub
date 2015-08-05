/************************************************************************
 *									*
 *	Module:  hci_configure_HUB_rtr.c				*
 *									*
 *	Description:  This module provides a gui wrapper for		*
 *		      configuring various MSCF hardware devices.	*
 *									*
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2010/03/16 14:37:47 $
 * $Id: hci_configure_HUB_rtr.c,v 1.7 2010/03/16 14:37:47 ccalvert Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */

/*	Include file definitions.			*/

#include <hci.h>

/*	Defines.			*/

#define	MAX_NUM_LABELS		20
#define	MAX_PASSWD_LENGTH	15
#define	DATE_STRING_LENGTH	26
#define	NO_DEVICE		99

/*	Global widget definitions.	*/

static	Widget	Top_widget = ( Widget ) NULL;
static	Widget	Config_date_label = ( Widget ) NULL;
static	Widget	Device_button = ( Widget ) NULL;
static	Widget	Info_scroll = ( Widget ) NULL;
static	Widget	Info_scroll_form = ( Widget ) NULL;

/*	Global variables.		*/

static	char	Output_msgs[ MAX_NUM_LABELS ][ HCI_CP_MAX_BUF ];
static	int	Output_msgs_code[ MAX_NUM_LABELS ];
static	int	Current_msg_pos = 0; /* Index of latest output msg */
static	int	Num_output_msgs = 0; /* Number of output msgs to display */
static	int	Coprocess_flag = HCI_CP_NOT_STARTED;
static	void*	Cp = NULL;
static	int	Configure_flag = HCI_NO_FLAG;
static	int	User_cancelled_flag = HCI_NO_FLAG;
static	char  	Msg_buf[ HCI_CP_MAX_BUF + HCI_BUF_256 ];
static	char  	Password_input[ MAX_PASSWD_LENGTH ];
static	char*	Device_label = " Frame Relay HUB Router ";
static	char*	Device_cmd = "config_device -N -f hub";
static	char*	Device_tag = FR_ROUTER_MSCF_DEVICE_TIME_TAG;

/*	Function prototypes.		*/

static int	Destroy_callback();
static void	Close_callback( Widget, XtPointer, XtPointer );
static void	Hub_callback( Widget, XtPointer, XtPointer );
static void	Hide_password_input( Widget, XtPointer, XtPointer );
static void	Submit_password( Widget, XtPointer, XtPointer );
static void	Cancel_password( Widget, XtPointer, XtPointer );
static void	Update_scroll_display( char *, int );
static void	Update_label( Widget, int );
static void	Update_date_display();
static void	Start_coprocess();
static void	Manage_coprocess();
static void	Close_coprocess();
static void	Update_configuration_date();
static void	Password_prompt( char * );
static void	Reset_to_initial_state();
static time_t	Get_device_config_time();
static void	Timer_proc();

/************************************************************************
 *	Description: This is the main function for the Hardware		*
 *		     Configuration task.				*
 ************************************************************************/

int main( int argc, char *argv [] )
{
  Widget	form;
  Widget	frame;
  Widget	body_frame;
  Widget	label;
  Widget	rowcol;
  Widget	item_rowcol;
  Widget	row_rowcol;
  Widget	control_rowcol;
  Widget	clip;
  Widget	button;
  XmString	initial_label;
  XmString	label_string;

  /* Initialize HCI. */

  HCI_partial_init( argc, argv, HCI_HUB_RTR_LOAD_TASK );

  Top_widget = HCI_get_top_widget();

  HCI_set_destroy_callback( Destroy_callback );

  XtVaSetValues( Top_widget, XmNtitle, HCI_get_task_name( HCI_HUB_RTR_LOAD_TASK ), NULL );

  /* Initialize date labels so width is correct. */

  initial_label = XmStringCreateLocalized( "                         " );

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

  XtAddCallback( button, XmNactivateCallback, Close_callback, NULL );

  body_frame = XtVaCreateManagedWidget( "body_frame",
          xmFrameWidgetClass,  form,
          XmNtopAttachment,    XmATTACH_WIDGET,
          XmNtopWidget,        control_rowcol,
          XmNleftAttachment,   XmATTACH_FORM,
          XmNrightAttachment,  XmATTACH_FORM,
          NULL );

  item_rowcol = XtVaCreateManagedWidget( "item_rowcol",
           xmRowColumnWidgetClass, body_frame,
           XmNbackground,          hci_get_read_color( BACKGROUND_COLOR1 ),
           XmNorientation,         XmVERTICAL,
           XmNnumColumns,          1,
           NULL );

  row_rowcol = XtVaCreateManagedWidget( "row_rowcol",
           xmRowColumnWidgetClass, item_rowcol,
           XmNbackground,          hci_get_read_color( BACKGROUND_COLOR1 ),
           XmNorientation,         XmHORIZONTAL,
           XmNnumColumns,          1,
           NULL );

  frame = XtVaCreateManagedWidget( "frame",
          xmFrameWidgetClass,  row_rowcol,
          XmNleftAttachment,   XmATTACH_FORM,
          NULL );

  label_string = XmStringCreateLocalized( Device_label );

  label = XtVaCreateManagedWidget( "",
          xmLabelWidgetClass,     frame,
          XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
          XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
          XmNfontList,            hci_get_fontlist (LIST),
          XmNlabelString,         label_string,
          NULL);

  XmStringFree( label_string );

  frame = XtVaCreateManagedWidget( "frame",
          xmFrameWidgetClass,  row_rowcol,
          NULL );

  rowcol = XtVaCreateManagedWidget( "row_rowcol",
           xmRowColumnWidgetClass, frame,
           XmNbackground,          hci_get_read_color( BACKGROUND_COLOR1 ),
           XmNorientation,         XmHORIZONTAL,
           XmNnumColumns,          1,
           NULL );

  label = XtVaCreateManagedWidget( " Last Configured ",
          xmLabelWidgetClass,     rowcol,
          XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
          XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
          XmNfontList,            hci_get_fontlist (LIST),
          NULL);

  Config_date_label = XtVaCreateManagedWidget( "",
          xmLabelWidgetClass,     rowcol,
          XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
          XmNbackground,          hci_get_read_color (BACKGROUND_COLOR2),
          XmNfontList,            hci_get_fontlist (LIST),
          XmNlabelString,         initial_label,
          NULL);

  Device_button = XtVaCreateManagedWidget( "Configure",
           xmPushButtonWidgetClass, row_rowcol,
           XmNforeground,           hci_get_read_color( BUTTON_FOREGROUND ),
           XmNbackground,           hci_get_read_color( BUTTON_BACKGROUND ),
           XmNfontList,             hci_get_fontlist( LIST ),
           XmNrightAttachment,      XmATTACH_FORM,
           NULL );

  XtAddCallback( Device_button, XmNactivateCallback, Hub_callback, NULL );

  /* Initialize configuration date. */

  Update_date_display();

  XmStringFree( initial_label );

  /* Create scroll area to display configuration output. */

  Info_scroll = XtVaCreateManagedWidget ("data_scroll",
                xmScrolledWindowWidgetClass,    form,
                XmNheight,              200,
                XmNscrollingPolicy,     XmAUTOMATIC,
                XmNforeground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNtopAttachment,       XmATTACH_WIDGET,
                XmNtopWidget,           body_frame,
                XmNbottomAttachment,    XmATTACH_FORM,
                XmNleftAttachment,      XmATTACH_FORM,
                XmNrightAttachment,     XmATTACH_FORM,
                NULL);

  XtVaGetValues( Info_scroll, XmNclipWindow, &clip, NULL );

  XtVaSetValues( clip,
                 XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
                 NULL );

  XtRealizeWidget( Top_widget );

  /* Start HCI loop. */

  HCI_start( Timer_proc, HCI_ONE_AND_HALF_SECOND, NO_RESIZE_HCI );

  return 0;
}

/************************************************************************
 *	Description: This function is activated when the the HCI	*
 *		     Hardware Configuration window is destroyed.	*
 ************************************************************************/

static int Destroy_callback()
{
  if( Coprocess_flag != HCI_CP_NOT_STARTED )
  {
    sprintf( Msg_buf, "You are not allowed to exit this task\n" );
    strcat( Msg_buf, "until the configuration process has been\n" );
    strcat( Msg_buf, "completed.\n" );
    hci_error_popup( Top_widget, Msg_buf, NULL );
    return HCI_NOT_OK_TO_EXIT;
  }

  HCI_LE_log( "hci_config_HUB_rtr HCI_task_exit(HCI_EXIT_SUCCESS)" );
  return HCI_OK_TO_EXIT;
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Close" button.				*
 ************************************************************************/

static void Close_callback( Widget w, XtPointer x, XtPointer y )
{
  if( Coprocess_flag != HCI_CP_NOT_STARTED )
  {
    sprintf( Msg_buf, "You are not allowed to close this task\n" );
    strcat( Msg_buf, "until the configuration process has been\n" );
    strcat( Msg_buf, "completed.\n" );
    hci_error_popup( Top_widget, Msg_buf, NULL );
  }
  else
  {
    XtDestroyWidget( Top_widget );
  }
}

/************************************************************************
 *      Description: This function is activated when the user selects   *
 *                   the "Frame Relay Router" button.                   *
 ************************************************************************/

static void Hub_callback( Widget w, XtPointer x, XtPointer y )
{
  Configure_flag = HCI_YES_FLAG;
  XtVaSetValues( Device_button, XmNsensitive, False, NULL );
  HCI_busy_cursor();
}

/************************************************************************
 * Timer_proc: Callback for event timer.
 ************************************************************************/

static void Timer_proc()
{
  if( Configure_flag == HCI_YES_FLAG )
  {
    if( Coprocess_flag == HCI_CP_NOT_STARTED )
    {
      /* Call function to start co-process to configure device. */
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
      Reset_to_initial_state();
    }
  }
}

/************************************************************************
 *	Description: This function updates a date widget to its		*
 *		     latest value.					*
 ************************************************************************/

static void Update_date_display()
{
  time_t config_time = -1;
  char label_buf[ DATE_STRING_LENGTH ];
  XmString label_string;

  config_time = Get_device_config_time();

  if( config_time <= 0 )
  {
    sprintf( label_buf, "           NA            " );
    XtVaSetValues( Config_date_label,
                   XmNbackground, hci_get_read_color( BACKGROUND_COLOR2 ),
                   NULL );
  }
  else
  {
    strftime( label_buf, DATE_STRING_LENGTH, " %c ",
              gmtime( &config_time ) );
    XtVaSetValues( Config_date_label,
                   XmNbackground, hci_get_read_color( WHITE ),
                   NULL );
  }
  label_string = XmStringCreateLocalized( label_buf );
  XtVaSetValues( Config_date_label, XmNlabelString,
                 label_string, NULL );
  XmStringFree( label_string );
}

/************************************************************************
 *	Description: This function executes the script to configure the	*
 *		     device.						*
 ************************************************************************/

static void Start_coprocess()
{
  int ret = -1;

  /* Reset scroll display. */

  Num_output_msgs = 0;
  Current_msg_pos = 0;
  if( Info_scroll_form != NULL ){ XtUnmanageChild( Info_scroll_form ); }

  /* Build function to call remotely. */

  if( ( ret = MISC_cp_open( Device_cmd, HCI_CP_MANAGE, &Cp ) ) != 0 )
  {
    HCI_LE_error( "MISC_cp_open failed: %d", ret );
    sprintf( Msg_buf, "MISC_cp_open failed (%d)\n\n", ret );
    strcat( Msg_buf, "Unable to configure device." );
    hci_error_popup( Top_widget, Msg_buf, NULL );
    Coprocess_flag = HCI_CP_FINISHED;
  }
  else
  {
    Coprocess_flag = HCI_CP_STARTED;
  }
}

/************************************************************************
 *	Description: This function manages the coprocess that is
 *                   configuring the selected device.
 ************************************************************************/

static void Manage_coprocess()
{
  int ret = -1;
  char cp_buf[ HCI_CP_MAX_BUF ];


  /* Read coprocess to see if any new data is available. */

  while( Cp != NULL &&
         ( ret = MISC_cp_read_from_cp( Cp, cp_buf, HCI_CP_MAX_BUF ) ) != 0 )
  {
    /* Strip trailing newline. */

    if( cp_buf[ strlen( cp_buf ) - 1 ] == '\n' )
    {
      cp_buf[ strlen( cp_buf ) - 1 ] = '\0';
    }

    /* Take action depending on value of ret variable. */

    if( ret == HCI_CP_DOWN )
    {
      /* Co-process has been terminated. If the user cancelled
         the coprocess (for example, cancelling at the password
         prompt), then start over. */
      if( User_cancelled_flag == HCI_YES_FLAG )
      {
        User_cancelled_flag = HCI_NO_FLAG;
        return; 
      }

      /*Retrieve exit status. */
      if( ( ret = ( MISC_cp_get_status( Cp ) >> 8 ) ) != 0 )
      {
        /* Exit status indicates error. */
        HCI_LE_error( "Cp exit code error: %d", ret );
        sprintf( Msg_buf, "Configuration error (%d)", ret );
        hci_error_popup( Top_widget, Msg_buf, NULL );
      }
      else
      {
        /* Exit status indicates success. Update date labels. */
        Update_configuration_date();
        HCI_LE_log( "HUB rtr configuration successful" );
        hci_info_popup( Top_widget, "HUB rtr configuration successful", NULL );
      }
      Close_coprocess();
      break;
    }
    else if( ret == HCI_CP_STDERR )
    {
      /* Display message in scroll box. */
      Update_scroll_display( cp_buf, HCI_CP_STDERR );
    }
    else if( ret == HCI_CP_STDOUT )
    {
      /* If coprocess buffer contains the token "Password:", then
         the user needs to be prompted for a password to enter. */
      if( strstr( cp_buf, "Password:" ) != NULL )
      {
        /* This line has the "Password:" token. Create
           password prompt popup. */
        Password_prompt( cp_buf );
      }
      else
      {
        /* This line is just a message that needs to
           be displayed in the message scroll box. */
        Update_scroll_display( cp_buf, HCI_CP_STDOUT );
      }
    }
    else
    {
      /* No new data available, do nothing... */
    }
  }
}

/************************************************************************
 *	Description: This function closes the coprocess.
 ************************************************************************/

static void Close_coprocess()
{
  MISC_cp_close( Cp );
  Cp = NULL;
  Coprocess_flag = HCI_CP_FINISHED;
}

/************************************************************************
 *	Description: This function updates the date information after	*
 *		     a successful configuration.			*
 ************************************************************************/

static void Update_configuration_date()
{
  int ret = -1;
  char buf[ HCI_BUF_32 ];

  sprintf( buf, "%li", time( NULL ) );
  if( ( ret = ORPGMISC_set_install_info( Device_tag, buf ) ) < 0 )
  {
    HCI_LE_error( "Unable to update config time for header %s", Device_tag );
    sprintf( Msg_buf, "Unable to update config time for header %s",
             Device_tag );
    Update_scroll_display( Msg_buf, HCI_CP_STDERR );
  }
  Update_date_display();
}

/************************************************************************
 *	Description: This function updates the scroll with the latest	*
 *		     output from the configuration task.		*
 ************************************************************************/

static void Update_scroll_display( char *Msg_buf, int msg_code )
{
  Widget label = ( Widget ) NULL;
  Widget rowcol = ( Widget ) NULL;
  int loop = -1;

  /* Modify appropriate msg in array. */

  sprintf( Output_msgs[ Current_msg_pos ], "%s", Msg_buf );
  Output_msgs_code[ Current_msg_pos ] = msg_code;

  /* Create widgets to display labels. */

  if( Info_scroll_form != ( Widget ) NULL )
  {
    XtDestroyWidget( Info_scroll_form );
  }

  Info_scroll_form = XtVaCreateWidget( "scroll_form",
       xmFormWidgetClass, Info_scroll,
       XmNforeground, hci_get_read_color( BACKGROUND_COLOR1 ),
       XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
       XmNverticalSpacing, 1,
       NULL );

  /* This rowcol ensures all labels are of equal width. */

  rowcol = XtVaCreateManagedWidget( "rc",
       xmRowColumnWidgetClass, Info_scroll_form,
       XmNforeground, hci_get_read_color( BACKGROUND_COLOR1 ),
       XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
       XmNorientation, XmVERTICAL,
       XmNpacking, XmPACK_COLUMN,
       XmNnumColumns,1,
       XmNleftAttachment, XmATTACH_FORM,
       XmNrightAttachment, XmATTACH_FORM,
       NULL );

  /* This long label sets the initial width. This ensures any
     colored labels stretch across the entire scrolling area. */

  label = XtVaCreateManagedWidget( "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
       xmLabelWidgetClass, rowcol,
       XmNforeground, hci_get_read_color( BACKGROUND_COLOR1 ),
       XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
       XmNfontList, hci_get_fontlist( LIST ),
       XmNleftAttachment, XmATTACH_FORM,
       XmNrightAttachment, XmATTACH_FORM,
       XmNalignment, XmALIGNMENT_BEGINNING,
       NULL );

  /* The latest message is located at position Current_msg_pos.
     Start there and work down to index 0. */

  for( loop = Current_msg_pos; loop >= 0; loop-- )
  {
    label = XtVaCreateManagedWidget( "label",
              xmLabelWidgetClass, Info_scroll_form,
              XmNforeground, hci_get_read_color( TEXT_FOREGROUND ),
              XmNfontList, hci_get_fontlist( LIST ),
              XmNtopAttachment, XmATTACH_WIDGET,
              XmNtopWidget, label,
              XmNleftAttachment, XmATTACH_FORM,
              XmNrightAttachment, XmATTACH_FORM,
              XmNalignment, XmALIGNMENT_BEGINNING,
              NULL );
    Update_label( label, loop );
  }

  /* If the number of output messages has reached the maximum
     allowed amount, then start at the last array position and
     work down to the index just above the Current_msg_pos index.
     This is done to allow the array to be treated as circular. */

  if( Num_output_msgs == MAX_NUM_LABELS )
  {
    for( loop = MAX_NUM_LABELS - 1; loop > Current_msg_pos; loop-- )
    {
      label = XtVaCreateManagedWidget( "label",
              xmLabelWidgetClass, Info_scroll_form,
              XmNforeground, hci_get_read_color( TEXT_FOREGROUND ),
              XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
              XmNfontList, hci_get_fontlist( LIST ),
              XmNtopAttachment, XmATTACH_WIDGET,
              XmNtopWidget, label,
              XmNleftAttachment, XmATTACH_FORM,
              XmNrightAttachment, XmATTACH_FORM,
              XmNalignment, XmALIGNMENT_BEGINNING,
              NULL );
      Update_label( label, loop );
    }
  }

  XtManageChild( Info_scroll_form );

  /* Update starting label position (circular array). */

  Current_msg_pos++;
  Current_msg_pos%=MAX_NUM_LABELS;

  /* Update number of labels (not to exceed maximum number allowed). */

  Num_output_msgs++;
  if( Num_output_msgs > MAX_NUM_LABELS ){ Num_output_msgs--; }
}

/************************************************************************
 *	Description: This function updates a label with the appropriate	*
 *		     value and color.					*
 ************************************************************************/

static void Update_label( Widget w, int index )
{
  XmString msg;

  /* Convert c-buf to XmString variable. */

  msg = XmStringCreateLtoR( Output_msgs[ index ], XmFONTLIST_DEFAULT_TAG );

  /* Modify appropriate output label. */

  if( Output_msgs_code[ index ] == HCI_CP_STDERR )
  {
    XtVaSetValues( w,
                   XmNlabelString, msg,
                   XmNbackground, hci_get_read_color( WARNING_COLOR ),
                   NULL );
  }
  else
  {
    XtVaSetValues( w,
                   XmNlabelString, msg,
                   XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
                   NULL );
  }

  /* Free allocated space. */

  XmStringFree( msg );
}

/**************************************************************************
 * Get_device_config_time: Print information about custom options.
**************************************************************************/

static time_t Get_device_config_time()
{
  char value_buf[ HCI_BUF_32 ];
  long time_value;

  if( ORPGMISC_get_install_info( Device_tag, value_buf, HCI_BUF_32 ) < 0 )
  {
    return 0;
  }

  sscanf( value_buf, "%li", &time_value );
  return ( time_t ) time_value;
}

/**************************************************************************
 * Password_prompt: Allow user to enter password for configuration.
**************************************************************************/

static void Password_prompt( char *cp_buf )
{
  Widget dialog;
  Widget temp_widget;
  Widget ok_button;
  Widget cancel_button;
  Widget text_field;
  XmString ok_button_string;
  XmString cancel_button_string;
  XmString msg;

  /* Create popup for entering password. */

  dialog = XmCreatePromptDialog( Top_widget, "password", NULL, 0 );

  /* Set various string labels. */

  ok_button_string = XmStringCreateLocalized( "OK" );
  msg = XmStringCreateLtoR( cp_buf, XmFONTLIST_DEFAULT_TAG );

  cancel_button_string = XmStringCreateLocalized( "Cancel" );
  msg = XmStringCreateLtoR( cp_buf, XmFONTLIST_DEFAULT_TAG );

  /* Get rid of Help button on popup. */

  temp_widget = XmSelectionBoxGetChild( dialog, XmDIALOG_HELP_BUTTON );
  XtUnmanageChild( temp_widget );

  /* Set properties of OK button. */

  ok_button = XmSelectionBoxGetChild( dialog, XmDIALOG_OK_BUTTON );

  XtVaSetValues( ok_button,
          XmNforeground, hci_get_read_color( BUTTON_FOREGROUND ),
          XmNbackground, hci_get_read_color( BUTTON_BACKGROUND ),
          XmNfontList,   hci_get_fontlist( LIST ),
          NULL );

  XtAddCallback( ok_button, XmNactivateCallback, Submit_password, NULL );

  /* Set properties of CANCEL button. */

  cancel_button = XmSelectionBoxGetChild( dialog, XmDIALOG_CANCEL_BUTTON );

  XtVaSetValues( cancel_button,
          XmNforeground, hci_get_read_color( BUTTON_FOREGROUND ),
          XmNbackground, hci_get_read_color( BUTTON_BACKGROUND ),
          XmNfontList,   hci_get_fontlist( LIST ),
          NULL );

  XtAddCallback( cancel_button, XmNactivateCallback, Cancel_password, NULL );

  /* Set properties of text field. */

  text_field = XmSelectionBoxGetChild( dialog, XmDIALOG_TEXT );

  XtVaSetValues( text_field,
          XmNforeground, hci_get_read_color( TEXT_FOREGROUND ),
          XmNbackground, hci_get_read_color( EDIT_BACKGROUND ),
          XmNfontList,   hci_get_fontlist( LIST ),
          XmNcolumns, MAX_PASSWD_LENGTH,
          XmNmaxLength, MAX_PASSWD_LENGTH,
          NULL );

  XtAddCallback( text_field, XmNmodifyVerifyCallback,
                 Hide_password_input, ( XtPointer ) Password_input );

  /* Set properties of popup. */

  XtVaSetValues (dialog,
          XmNselectionLabelString, msg,
          XmNokLabelString, ok_button_string,
          XmNcancelLabelString, cancel_button_string,
          XmNbackground, hci_get_read_color( WARNING_COLOR ),
          XmNforeground, hci_get_read_color( TEXT_FOREGROUND ),
          XmNfontList,   hci_get_fontlist( LIST ),
          XmNdialogType, XmDIALOG_PROMPT,
          XmNdeleteResponse, XmDESTROY,
          NULL);

  /* Free allocated space. */

  XmStringFree( ok_button_string );
  XmStringFree( cancel_button_string );
  XmStringFree( msg );

  /* Do this to make popup appear. */

  XtManageChild( dialog );
  XtPopup( Top_widget, XtGrabNone );
}

/***************************************************************************
 * Hide_password_input: Hide password by replacing letters with "*".
 ***************************************************************************/

static void Hide_password_input( Widget w, XtPointer x, XtPointer y )
{
  char *passwd_array;
  XmTextVerifyCallbackStruct *cbs;

  cbs = ( XmTextVerifyCallbackStruct * ) y;
  passwd_array = ( char * ) x;

  /* If text field is empty, don't worry about it. */
  if( cbs->text->length == 0 && cbs->currInsert == 0 ){ return; }

  /* Don't worry about backspaces. */
  if( cbs->startPos < cbs->currInsert )
  {
    passwd_array[ cbs->startPos ] = '\0';
    return;
  }

  /* Don't allow cutting and pasting. */
  if( cbs->text->length > 1 )
  {
    cbs->doit = False;
    return;
  }

  passwd_array[ cbs->currInsert ] = cbs->text->ptr[ 0 ];
  passwd_array[ cbs->currInsert + 1 ] = '\0';

  cbs->text->ptr[ 0 ] = '*';
}

/***************************************************************************
 * Submit_password: Submit password to configuration task.
 ***************************************************************************/

static void Submit_password( Widget w, XtPointer x, XtPointer y )
{
  char temp_password[ MAX_PASSWD_LENGTH ];
  /* Add newline to end of password. */
  sprintf( temp_password, "%s\n", Password_input );
  if( Cp != NULL )
  {
    MISC_cp_write_to_cp( Cp, temp_password );
  }
  else
  {
    sprintf( Msg_buf, "Coprocess terminated, unable to submit password" );
    hci_error_popup( Top_widget, Msg_buf, NULL );
  }
}

/***************************************************************************
 * Cancel_password: Cancel password submission.
 ***************************************************************************/

static void Cancel_password( Widget w, XtPointer x, XtPointer y )
{
  HCI_LE_log( "Cancel root password entry" );
  Close_coprocess();
  User_cancelled_flag = HCI_YES_FLAG;
}

/***************************************************************************
 * Reset_to_initial_state: Reset flags, buttons, cursors, etc. to initial
 *    states.
 ***************************************************************************/

void Reset_to_initial_state()
{
  HCI_LE_log( "Reset GUI" );
  Configure_flag = HCI_NO_FLAG;
  XtVaSetValues( Device_button, XmNsensitive, True, NULL );
  HCI_default_cursor();
}

