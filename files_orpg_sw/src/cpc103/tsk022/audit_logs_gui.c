/************************************************************************

      The main source file for gui wrapper of "audit_logs_manager" script.

************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/01/25 16:00:18 $
 * $Id: audit_logs_gui.c,v 1.8 2012/01/25 16:00:18 ccalvert Exp $
 * $Revision: 1.8 $
 * $State: Exp $
 */

/* Include files. */

#include <hci.h>
#include <security/pam_appl.h>
#include <security/pam_modules.h>

/* Definitions. */

enum { NO_ACTION, COPY_AUDIT_LOGS };

#define	MAX_PASSWD_LENGTH	15
#define	ROOT_USER		"root"
#define	DATE_STRING_LENGTH	26
#define	UNIX_TIME_LENGTH	32
#define	CMD_BUF			64
#define	OUTPUT_BUF		512

/* Global variables. */

static Widget	Top_widget = ( Widget ) NULL;
static Widget	Copy_to_cd_button = ( Widget ) NULL;
static Widget	Root_dialog = ( Widget ) NULL;
static Widget	Date_label = ( Widget ) NULL;
static char	Root_passwd[MAX_PASSWD_LENGTH+1];
static char	Popup_buf[OUTPUT_BUF];
static int	Root_user_verified = NO;
static int	Action_flag = NO_ACTION;
static int	Coprocess_flag = HCI_CP_NOT_STARTED;
static void	*Cp = NULL;

/* Function prototypes. */

static int	Destroy_callback( Widget, XtPointer, XtPointer );
static void	Close_callback( Widget, XtPointer, XtPointer );
static void	Set_action_flag( Widget, XtPointer, XtPointer );
static void	Verify_root_user();
static void	Cancel_verify( Widget, XtPointer, XtPointer );
static void	Hide_root_input( Widget, XtPointer, XtPointer );
static void	Authenticate_pam( Widget, XtPointer, XtPointer );
static void	Reset_to_initial_state();
static void	Update_copy_date();
static void	Update_copy_date_label();
static void	Start_coprocess();
static void	Manage_coprocess();
static void	Close_coprocess();
static void	Timer_proc();
static int	Get_root_passwd( int, const struct pam_message **,
                             struct pam_response **, void * );

/* Define structs. */

static struct pam_conv conv = { Get_root_passwd, NULL };

/**************************************************************************

    The main function.

**************************************************************************/

int main( int argc, char **argv )
{
  Widget	form;
  Widget	form_rowcol;
  Widget	close_rowcol;
  Widget	button;
  Widget	frame;
  Widget	frame_rowcol;
  Widget	label;
  XmString	initial_label;

  /* Initialize HCI. */

  HCI_partial_init( argc, argv, -1 );

  Top_widget = HCI_get_top_widget();

  HCI_set_destroy_callback( Destroy_callback );

  /* Need to reset the GUI title. */

  XtVaSetValues( Top_widget, XmNtitle, "Copy Audit Logs", NULL );
 
  /* Initialize date labels so width is correct. */

  initial_label = XmStringCreateLocalized( "                         " );

  /* Define form to hold the widgets. */

  form = XtVaCreateManagedWidget( "audit_log_form",
                 xmFormWidgetClass, Top_widget,
                 NULL);

  form_rowcol = XtVaCreateManagedWidget( "form_rowcol",
                        xmRowColumnWidgetClass, form,
                        XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
                        XmNorientation, XmVERTICAL,
                        XmNpacking, XmPACK_TIGHT,
                        XmNnumColumns, 1,
                        NULL);

  /* Add close button. */

  close_rowcol = XtVaCreateManagedWidget( "close_rowcol",
                         xmRowColumnWidgetClass, form_rowcol,
                         XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
                         XmNorientation, XmHORIZONTAL,
                         XmNnumColumns, 1,
                         XmNtopAttachment, XmATTACH_FORM,
                         XmNleftAttachment, XmATTACH_FORM,
                         XmNrightAttachment, XmATTACH_FORM,
                         NULL);

  button = XtVaCreateManagedWidget( "Close",
                   xmPushButtonWidgetClass, close_rowcol,
                   XmNforeground, hci_get_read_color( BUTTON_FOREGROUND ),
                   XmNbackground, hci_get_read_color( BUTTON_BACKGROUND ),
                   XmNfontList, hci_get_fontlist( LIST ),
                   NULL);

  XtAddCallback( button, XmNactivateCallback, Close_callback, NULL );

  /* Add toggle buttons to either add or remove RPG users. */

  frame = XtVaCreateManagedWidget( "frame",
                         xmFrameWidgetClass, form_rowcol,
                         XmNtopAttachment,       XmATTACH_WIDGET,
                         XmNtopWidget,           close_rowcol,
                         XmNleftAttachment,      XmATTACH_FORM,
                         XmNrightAttachment,     XmATTACH_FORM,
                         NULL);

  frame_rowcol = XtVaCreateManagedWidget( "frame_rowcol",
                         xmRowColumnWidgetClass, frame,
                         XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
                         XmNorientation, XmHORIZONTAL,
                         XmNnumColumns, 1,
                         NULL);

  Copy_to_cd_button = XtVaCreateManagedWidget( "Copy Logs",
                   xmPushButtonWidgetClass, frame_rowcol,
                   XmNforeground, hci_get_read_color( BUTTON_FOREGROUND ),
                   XmNbackground, hci_get_read_color( BUTTON_BACKGROUND ),
                   XmNfontList, hci_get_fontlist( LIST ),
                   NULL);

  XtAddCallback( Copy_to_cd_button,
                 XmNactivateCallback, Set_action_flag,
                 ( XtPointer ) COPY_AUDIT_LOGS );

  XtVaCreateManagedWidget( "  ",
                   xmLabelWidgetClass, frame_rowcol,
                   XmNforeground, hci_get_read_color( BACKGROUND_COLOR1 ),
                   XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
                   NULL);

  label = XtVaCreateManagedWidget( " Last Copied ",
          xmLabelWidgetClass,     frame_rowcol,
          XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
          XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
          XmNfontList,            hci_get_fontlist (LIST),
          NULL);

  Date_label = XtVaCreateManagedWidget( "",
          xmLabelWidgetClass,     frame_rowcol,
          XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
          XmNbackground,          hci_get_read_color (BACKGROUND_COLOR2),
          XmNfontList,            hci_get_fontlist (LIST),
          XmNlabelString,         initial_label,
          NULL);

  XmStringFree( initial_label );

  /* Update date label. */

  Update_copy_date_label();
 
  /* Display gui to screen. */

  XtRealizeWidget( Top_widget );

  /* Start HCI loop. */

  HCI_start( Timer_proc, HCI_ONE_SECOND, NO_RESIZE_HCI );

  return 0;
}

/************************************************************************
 Description: Callback for main widget when it is destroyed.
************************************************************************/

static int Destroy_callback( Widget w, XtPointer x, XtPointer y )
{
  if( Coprocess_flag != HCI_CP_NOT_STARTED )
  {
    sprintf( Popup_buf, "You are not allowed to exit\n" );
    strcat( Popup_buf, "until the task has completed." );
    hci_error_popup( Top_widget, Popup_buf, NULL );
    return HCI_NOT_OK_TO_EXIT;
  }

  return HCI_OK_TO_EXIT;
}

/************************************************************************
 Description: Callback for "Close" button. Destroys main widget.
************************************************************************/

static void Close_callback( Widget w, XtPointer x, XtPointer y )
{
  if( Coprocess_flag != HCI_CP_NOT_STARTED )
  {
    sprintf( Popup_buf, "You are not allowed to exit\n" );
    strcat( Popup_buf, "until the task has completed." );
    hci_error_popup( Top_widget, Popup_buf, NULL );
  }
  else
  {
    XtDestroyWidget( Top_widget );
  }
}

/************************************************************************
 Description: Callback for any of the action buttons.
************************************************************************/

static void Set_action_flag( Widget w, XtPointer x, XtPointer y )
{
  /* Set action flag. */

  Action_flag = ( int ) x;

  /* Set cursor to "busy". */

  HCI_busy_cursor();

  /* De-sensitize action buttons. */

  XtSetSensitive( Copy_to_cd_button, False );
}

/************************************************************************
 Description: Callback for event timer.
************************************************************************/

static void Timer_proc()
{
  if( Action_flag == COPY_AUDIT_LOGS )
  {
    if( Root_user_verified == NO )
    {
      Verify_root_user();
    }
    else if( Coprocess_flag == HCI_CP_NOT_STARTED )
    {
      Start_coprocess();
    }
    else if( Coprocess_flag == HCI_CP_STARTED )
    {
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
 Description: Start coprocess to copy logs.
************************************************************************/

static void Start_coprocess()
{
  char cmd[CMD_BUF];
  int ret;

  /* Start co-process. */

  strcpy( cmd, "audit_logs_manager -N" );

  if( ( ret = MISC_cp_open( cmd, MISC_CP_MANAGE, &Cp ) >> 8 ) != 0 )
  {
    sprintf( Popup_buf, "MISC_cp_open failed (%d)\n\n", ret );
    strcat( Popup_buf, "Unable to continue." );
    hci_error_popup( Top_widget, Popup_buf, NULL );
    Coprocess_flag = HCI_CP_FINISHED;
    return;
  }

  Coprocess_flag = HCI_CP_STARTED;
}

/************************************************************************
 Description: Manage co-process.
************************************************************************/

static void Manage_coprocess()
{
  int ret;
  char cp_buf[OUTPUT_BUF];

  while( Cp != NULL &&
         ( ret = MISC_cp_read_from_cp( Cp, cp_buf, OUTPUT_BUF ) ) != 0 )
  {
    if( cp_buf[strlen( cp_buf )-1] == '\n' )
    {
      cp_buf[strlen( cp_buf )-1] = '\0';
    }

    if( ret == HCI_CP_DOWN )
    {
      /* Check exit status and take appropriate action. */
      if( ( ret = MISC_cp_get_status( Cp ) >> 8 ) != 0 )
      {
        /* Exit status indicates an error. */
        sprintf( Popup_buf, "Command audit_logs_manager failed (%d).", ret );
        hci_error_popup( Top_widget, Popup_buf, NULL );
      }
      else
      {
        Update_copy_date();
        strcpy( Popup_buf, "Successfully copied audit logs" );
        hci_info_popup( Top_widget, Popup_buf, NULL );
      }
      Close_coprocess();
      break;
    }
    else if( ret == HCI_CP_STDOUT )
    {
      if( strlen( cp_buf ) > 0 )
      {
        HCI_LE_log( cp_buf );
      }
    }
    else if( ret == HCI_CP_STDERR )
    {
      sprintf( Popup_buf, "Error running audit_logs_manager.\n\n" );
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
 Description: Close coprocess.
************************************************************************/

static void Close_coprocess()
{
  MISC_cp_close( Cp );
  Cp = NULL;
  Coprocess_flag = HCI_CP_FINISHED;
}

/************************************************************************
 Description: Verify user is root before adding/removing users.
************************************************************************/

static void Verify_root_user()
{
  Widget temp_widget;
  Widget ok_button;
  Widget cancel_button;
  Widget text_field;
  XmString ok_button_string;
  XmString cancel_button_string;
  XmString msg;

  /* Get current user id. If user is already root, skip verification. */

  if( getuid() == ( int ) 0 )
  {
    Root_user_verified = YES;
    return;
  }

  /* If Root_dialog isn't null and isn't managed (visible), then
     password popup has already been displayed but user entered
     incorrect root password. Re-manage the root dialog so the
     user can re-enter the root password. */

  if( Root_dialog != ( Widget ) NULL &&
      XtIsManaged( Root_dialog ) == False )
  {
    XtManageChild( Root_dialog );
    return;
  }

  /* If Root_dialog isn't null, then the popup is currently visible. */

  if( Root_dialog != ( Widget ) NULL ){ return; }

  /* User isn't root, so create popup for entering root password. */

  Root_dialog = XmCreatePromptDialog( Top_widget, "password", NULL, 0 );

  /* Set various string labels. */
   
  ok_button_string = XmStringCreateLocalized( "OK" );
  cancel_button_string = XmStringCreateLocalized( "Cancel" );
  msg = XmStringCreateLtoR( "Enter root password:", XmFONTLIST_DEFAULT_TAG );

  /* Get rid of Help button on popup. */

  temp_widget = XmSelectionBoxGetChild( Root_dialog, XmDIALOG_HELP_BUTTON );
  XtUnmanageChild( temp_widget );

  /* Set properties of OK button. */

  ok_button = XmSelectionBoxGetChild( Root_dialog, XmDIALOG_OK_BUTTON );

  XtVaSetValues( ok_button,
          XmNforeground, hci_get_read_color( BUTTON_FOREGROUND ),
          XmNbackground, hci_get_read_color( BUTTON_BACKGROUND ),
          XmNfontList, hci_get_fontlist( LIST ),
          NULL );

  XtAddCallback( ok_button, XmNactivateCallback, Authenticate_pam, NULL );

  /* Set properties of Cancel button. */

  cancel_button = XmSelectionBoxGetChild( Root_dialog, XmDIALOG_CANCEL_BUTTON );

  XtVaSetValues( cancel_button,
          XmNforeground, hci_get_read_color( BUTTON_FOREGROUND ),
          XmNbackground, hci_get_read_color( BUTTON_BACKGROUND ),
          XmNfontList, hci_get_fontlist( LIST ),
          NULL );

  XtAddCallback( cancel_button, XmNactivateCallback, Cancel_verify, NULL );

  /* Set properties of text field. */

  text_field = XmSelectionBoxGetChild( Root_dialog, XmDIALOG_TEXT );

  XtVaSetValues( text_field,
          XmNforeground, hci_get_read_color( TEXT_FOREGROUND ),
          XmNbackground, hci_get_read_color( EDIT_BACKGROUND ),
          XmNfontList, hci_get_fontlist( LIST ),
          XmNcolumns, MAX_PASSWD_LENGTH,
          XmNmaxLength, MAX_PASSWD_LENGTH,
          NULL );

  XtAddCallback( text_field, XmNmodifyVerifyCallback,
                 Hide_root_input, ( XtPointer ) Root_passwd );

  /* Set properties of popup. */

  XtVaSetValues (Root_dialog,
          XmNselectionLabelString, msg,
          XmNokLabelString, ok_button_string,
          XmNcancelLabelString, cancel_button_string,
          XmNbackground, hci_get_read_color( WARNING_COLOR ),
          XmNforeground, hci_get_read_color( TEXT_FOREGROUND ),
          XmNfontList, hci_get_fontlist( LIST ),
          XmNdialogType, XmDIALOG_PROMPT,
          XmNdeleteResponse, XmDESTROY,
          NULL);

  /* Free allocated space. */

  XmStringFree( ok_button_string );
  XmStringFree( cancel_button_string );
  XmStringFree( msg );

  /* Do this to make popup appear. */

  XtManageChild( Root_dialog );
  XtPopup( Top_widget, XtGrabNone );
}

/************************************************************************
 Description: Cancel root password verification.
************************************************************************/

static void Cancel_verify( Widget w, XtPointer x, XtPointer y )
{
  if( Root_dialog != ( Widget ) NULL )
  {
    XtDestroyWidget( Root_dialog );
    Root_dialog = ( Widget ) NULL;
  }
  Reset_to_initial_state();
}

/************************************************************************
 Description: Prompt user for root password. This function is called by
              the Linux-PAM module.
************************************************************************/

static int Get_root_passwd( int num_msg, const struct pam_message **msg,
                     struct pam_response **resp, void *appdata_ptr )
{
  int count;
  struct pam_response *r;
  char *passwd = ( char * ) malloc( MAX_PASSWD_LENGTH * sizeof( char ) );

  r = (struct pam_response *) calloc( num_msg, sizeof( struct pam_response ) );

  for( count = 0; count < num_msg; count++ )
  {
    switch( msg[count] -> msg_style )
    {
      case PAM_PROMPT_ECHO_OFF:
        strcpy( passwd, Root_passwd );
        break;
      default:
        strcpy( passwd, "bad password" );
    }
    r[count].resp_retcode = 0;
    r[count].resp = passwd;
  }
  *resp = r;
  return PAM_SUCCESS;
}

/************************************************************************
 Description: Hide password by replacing letters with "*".
************************************************************************/

static void Hide_root_input( Widget w, XtPointer x, XtPointer y )
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
    passwd_array[cbs->startPos] = '\0';
    return;
  }

  /* Don't allow cutting and pasting. */
  if( cbs->text->length > 1 )
  {
    cbs->doit = False;
    return;
  }

  passwd_array[cbs->currInsert] = cbs->text->ptr[0];
  passwd_array[cbs->currInsert+1] = '\0';

  cbs->text->ptr[0] = '*';
}

/************************************************************************
 Description: Use Linux-PAM module to verify root password.
************************************************************************/

static void Authenticate_pam( Widget w, XtPointer x, XtPointer y )
{
  pam_handle_t *pamh = NULL;
  int retval;
  XmString msg;

  retval = pam_start( "passwd", ROOT_USER, &conv, &pamh ); 

  if( retval == PAM_SUCCESS )
  {
    retval = pam_authenticate( pamh, 0 );
    pam_end( pamh, 0 );

    if( retval == PAM_SUCCESS )
    {
      if( setuid( 0 ) < 0 )
      {
        strcpy( Popup_buf, "Cannot setuid root" );
        hci_error_popup( Top_widget, Popup_buf, NULL );
        Reset_to_initial_state();
        return;
      }
      Root_user_verified = YES;
    }
    else
    {
      msg = XmStringCreateLtoR( "Invalid password.\nRe-enter root password:",
                                XmFONTLIST_DEFAULT_TAG );
      XtVaSetValues (Root_dialog, XmNselectionLabelString, msg, NULL );
      XmStringFree( msg );
    }
  }
  else
  {
    strcpy( Popup_buf, "AUTHENTICATION FAILED: Pam module error" );
    hci_error_popup( Top_widget, Popup_buf, NULL );
    Coprocess_flag = HCI_CP_FINISHED;
  }
}

/************************************************************************
 Description: Reset gui/variables when action has finished.
************************************************************************/

static void Reset_to_initial_state()
{
  Action_flag = NO_ACTION;
  HCI_default_cursor();
  XtSetSensitive( Copy_to_cd_button, True );
}

/************************************************************************
 Description: This function updates the date of audit log copy.
************************************************************************/

static void Update_copy_date()
{
  int ret = -1;
  char buf[UNIX_TIME_LENGTH];

  sprintf( buf, "%li", time( NULL ) );
  ret = hci_set_install_info( COPY_AUDIT_LOGS_TIME_TAG, buf );

  if( ret < 0 )
  {
    HCI_LE_error( "Unable to update copy date %s", COPY_AUDIT_LOGS_TIME_TAG );
  }
  Update_copy_date_label();
}

/************************************************************************
 Description: This function updates copy date widget to its latest value.
************************************************************************/

static void Update_copy_date_label()
{
  time_t config_time = -1;
  char label_buf[DATE_STRING_LENGTH];
  XmString label_string;
  char value_buf[UNIX_TIME_LENGTH];

  if( hci_get_install_info( COPY_AUDIT_LOGS_TIME_TAG, value_buf, UNIX_TIME_LENGTH ) < 0 )
  {
    sprintf( label_buf, "           NA            " );
    XtVaSetValues( Date_label,
                   XmNbackground, hci_get_read_color( BACKGROUND_COLOR2 ),
                   NULL );
  }
  else
  {
    sscanf( value_buf, "%li", &config_time );
    strftime( label_buf, DATE_STRING_LENGTH, " %c ",
              gmtime( &config_time ) );
    XtVaSetValues( Date_label,
                   XmNbackground, hci_get_read_color( WHITE ),
                   NULL );
  }
  label_string = XmStringCreateLocalized( label_buf );
  XtVaSetValues( Date_label, XmNlabelString,
                 label_string, NULL );
  XmStringFree( label_string );
}


