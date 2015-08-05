/************************************************************************

      The main source file of the tool "rpg_admin_user".

************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2011/10/03 21:49:39 $
 * $Id: rpg_admin_user_gui.c,v 1.18 2011/10/03 21:49:39 ccalvert Exp $
 * $Revision: 1.18 $
 * $State: Exp $
 */

/* Include files. */

#include <hci.h>
#include <crack.h>
#include <security/pam_appl.h>
#include <security/pam_modules.h>

/* Definitions. */

#define	RPG_LOW_UID		19000
#define	RPG_HIGH_UID		20000
#define	RPG_GID			101
#define	ADD_USER		1
#define	REMOVE_USER		0
#define	TEXT_NUM_COLS		12 /* Width of text username/passwd boxes. */
#define	SCROLL_AREA_WIDTH	32 /* Width of scroll area containing users. */
#define	MAX_NUM_RPG_USERS	1000
#define	MAX_TEXT_OUTPUT_SIZE	2048
#define	MIN_NAME_LENGTH		6
#define	MAX_NAME_LENGTH		15
#define	MIN_PASSWD_LENGTH	12
#define	MAX_PASSWD_LENGTH	20
#define	MAX_LINE_LENGTH		250
#define	PASSWORD_FILE		"/etc/passwd"
#define	TEMP_FILE		"/tmp/rpg_admin.txt"
#define	OP_RPG_FILE		"/etc/operate_rpg.conf"
#define	ROOT_USER		"root"
#define	CRACKLIB_PATH		"/usr/lib/cracklib_dict"
#define	NO			0
#define	YES			1
#define	NUM_LE_LOG_MESSAGES	200


/* Global variables. */

static Widget		Top_widget = ( Widget ) NULL;
static int		Task_flag = ADD_USER;
static int		Number_of_users = 0;
static Widget		Password_label = ( Widget ) NULL;
static Widget		Retype_password_label = ( Widget ) NULL;
static Widget		Username_input = ( Widget ) NULL;
static Widget		Password_input = ( Widget ) NULL;
static Widget		Retype_password_input = ( Widget ) NULL;
static Widget		User_list_scroll = ( Widget ) NULL;
static Widget		User_list_scroll_form = ( Widget ) NULL;
static Widget		User_list_label[ MAX_NUM_RPG_USERS ];
static char		User_list_entry[ MAX_NUM_RPG_USERS ][ MAX_NAME_LENGTH ];
static char		Root_passwd[ MAX_PASSWD_LENGTH + 1 ];
static char		New_user_passwd[ MAX_PASSWD_LENGTH + 1 ];
static char		Retype_new_user_passwd[ MAX_PASSWD_LENGTH + 1 ];
static char		Salt_key[ 13 ];

/* Function prototypes. */

static int  Destroy_callback( Widget, XtPointer, XtPointer );
static void Close_callback( Widget, XtPointer, XtPointer );
static void Add_user_callback( Widget, XtPointer, XtPointer );
static void Del_user_callback( Widget, XtPointer, XtPointer );
static void Verify_root_user( Widget, XtPointer, XtPointer );
static void Label_callback( Widget, XtPointer, XtPointer );
static void Hide_root_input( Widget, XtPointer, XtPointer );
static void Authenticate_pam( Widget, XtPointer, XtPointer );
static void Display_user_list();
static void Build_user_list();
static void Error_popup( char * );
static void Add_rpg_user();
static void Remove_rpg_user();
static void Add_user_to_operate_rpg_file( char * );
static void Remove_user_from_operate_rpg_file( char * );
static void Submit_user_action();
static void New_salt_key();
static int  Get_root_passwd( int, const struct pam_message **,
                      struct pam_response **, void * );
static int User_sort( const void *, const void * );
static void Timer_proc();

/* Define structs. */

static struct pam_conv conv = { Get_root_passwd, NULL };

/**************************************************************************

    The main function.

**************************************************************************/

int main( int argc, char **argv )
{
  char		temp_char[MAX_LINE_LENGTH] = "";
  Widget	form;
  Widget	form_rowcol;
  Widget	close_rowcol;
  Widget	button;
  Widget	frame;
  Widget	toggle_rowcol;
  Widget	rpg_user_toggle;
  Widget	username_rowcol;
  Widget	password_rowcol;
  Widget	retype_password_rowcol;
  Widget	submit_rowcol;
  Widget	label_rowcol;
  Widget	label;
  Widget	scroll_clip;
  int		n;
  Arg		args[16];

  /* Initialize HCI. */

  HCI_partial_init( argc, argv, -1 );

  Top_widget = HCI_get_top_widget();

  HCI_set_destroy_callback( Destroy_callback );

  XtVaSetValues( Top_widget, XmNtitle, "User Account Manager", NULL );

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

  toggle_rowcol = XtVaCreateManagedWidget( "toggle_rowcol",
                         xmRowColumnWidgetClass, frame,
                         XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
                         XmNorientation, XmHORIZONTAL,
                         XmNnumColumns, 1,
                         XmNleftAttachment, XmATTACH_FORM,
                         XmNrightAttachment, XmATTACH_FORM,
                         NULL);

  label = XtVaCreateManagedWidget( "RPG User: ",
                  xmLabelWidgetClass, toggle_rowcol,
                  XmNforeground, hci_get_read_color( TEXT_FOREGROUND ),
                  XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
                  XmNfontList, hci_get_fontlist( LIST ),
                  NULL);

  n = 0;

  XtSetArg( args [n], XmNforeground, hci_get_read_color( TEXT_FOREGROUND ));
  n++;
  XtSetArg( args [n], XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ) );
  n++;
  XtSetArg( args [n], XmNfontList, hci_get_fontlist( LIST ) );
  n++;
  XtSetArg( args [n], XmNpacking, XmPACK_TIGHT );
  n++;
  XtSetArg( args [n], XmNorientation, XmHORIZONTAL );
  n++;

  rpg_user_toggle = XmCreateRadioBox( toggle_rowcol, "rpg_user", args, n );

  button = XtVaCreateManagedWidget( "Add",
                   xmToggleButtonWidgetClass, rpg_user_toggle,
                   XmNselectColor, hci_get_read_color( WHITE ),
                   XmNforeground, hci_get_read_color( TEXT_FOREGROUND ),
                   XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
                   XmNfontList, hci_get_fontlist( LIST ),
                   XmNset, True,
                   NULL);

  XtAddCallback( button,
                 XmNarmCallback, Add_user_callback,
                 NULL );

  button = XtVaCreateManagedWidget( "Remove (select from list)",
                   xmToggleButtonWidgetClass, rpg_user_toggle,
                   XmNselectColor, hci_get_read_color( WHITE ),
                   XmNforeground, hci_get_read_color( TEXT_FOREGROUND ),
                   XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
                   XmNfontList, hci_get_fontlist( LIST ),
                   XmNset, False,
                   NULL);

  XtAddCallback( button,
                 XmNarmCallback, Del_user_callback,
                 NULL );

  XtManageChild( rpg_user_toggle );

  /* Add text widget for input of user name. */

  username_rowcol = XtVaCreateManagedWidget( "username_rowcol",
                         xmRowColumnWidgetClass, form_rowcol,
                         XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
                         XmNorientation, XmHORIZONTAL,
                         XmNnumColumns, 1,
                         XmNtopAttachment, XmATTACH_WIDGET,
                         XmNtopWidget, frame,
                         XmNleftAttachment, XmATTACH_FORM,
                         XmNrightAttachment, XmATTACH_FORM,
                         NULL);

  sprintf( temp_char, "User\'s Name (%d-%d characters):",
           MIN_NAME_LENGTH, MAX_NAME_LENGTH );

  label = XtVaCreateManagedWidget( temp_char,
                  xmLabelWidgetClass, username_rowcol,
                  XmNforeground, hci_get_read_color( TEXT_FOREGROUND ),
                  XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
                  XmNfontList, hci_get_fontlist( LIST ),
                  NULL);

  Username_input = XtVaCreateManagedWidget( "username_input",
                           xmTextFieldWidgetClass, username_rowcol,
                           XmNforeground, hci_get_read_color( TEXT_FOREGROUND ),
                           XmNbackground, hci_get_read_color( EDIT_BACKGROUND ),
                           XmNfontList, hci_get_fontlist( LIST ),
                           XmNcolumns, TEXT_NUM_COLS,
                           XmNmaxLength, MAX_NAME_LENGTH,
                           NULL);

  /* Add text widget for input of user password. */

  password_rowcol = XtVaCreateManagedWidget( "password_rowcol",
                         xmRowColumnWidgetClass, form_rowcol,
                         XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
                         XmNorientation, XmHORIZONTAL,
                         XmNnumColumns, 1,
                         XmNtopAttachment, XmATTACH_WIDGET,
                         XmNtopWidget, username_rowcol,
                         XmNleftAttachment, XmATTACH_FORM,
                         XmNrightAttachment, XmATTACH_FORM,
                         NULL);

  sprintf( temp_char, "  Password (%d-%d characters):",
           MIN_PASSWD_LENGTH, MAX_PASSWD_LENGTH );

  Password_label = XtVaCreateManagedWidget( temp_char,
                  xmLabelWidgetClass, password_rowcol,
                  XmNforeground, hci_get_read_color( TEXT_FOREGROUND ),
                  XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
                  XmNfontList, hci_get_fontlist( LIST ),
                  NULL);

  Password_input = XtVaCreateManagedWidget( "password_input",
                           xmTextFieldWidgetClass, password_rowcol,
                           XmNforeground, hci_get_read_color( TEXT_FOREGROUND ),
                           XmNbackground, hci_get_read_color( EDIT_BACKGROUND ),
                           XmNfontList, hci_get_fontlist( LIST ),
                           XmNcolumns, TEXT_NUM_COLS,
                           XmNmaxLength, MAX_PASSWD_LENGTH,
                           NULL);

  XtAddCallback( Password_input, XmNmodifyVerifyCallback,
                 Hide_root_input, ( XtPointer ) New_user_passwd );

  /* Add text widget for input of user password again. */

  retype_password_rowcol = XtVaCreateManagedWidget( "retype_password_rowcol",
                         xmRowColumnWidgetClass, form_rowcol,
                         XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
                         XmNorientation, XmHORIZONTAL,
                         XmNnumColumns, 1,
                         XmNtopAttachment, XmATTACH_WIDGET,
                         XmNtopWidget, password_rowcol,
                         XmNleftAttachment, XmATTACH_FORM,
                         XmNrightAttachment, XmATTACH_FORM,
                         NULL);

  Retype_password_label = XtVaCreateManagedWidget( "              Retype Password:",
                  xmLabelWidgetClass, retype_password_rowcol,
                  XmNforeground, hci_get_read_color( TEXT_FOREGROUND ),
                  XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
                  XmNfontList, hci_get_fontlist( LIST ),
                  NULL);

  Retype_password_input = XtVaCreateManagedWidget( "retype_password_input",
                           xmTextFieldWidgetClass, retype_password_rowcol,
                           XmNforeground, hci_get_read_color( TEXT_FOREGROUND ),
                           XmNbackground, hci_get_read_color( EDIT_BACKGROUND ),
                           XmNfontList, hci_get_fontlist( LIST ),
                           XmNcolumns, TEXT_NUM_COLS,
                           XmNmaxLength, MAX_PASSWD_LENGTH,
                           NULL);

  XtAddCallback( Retype_password_input, XmNmodifyVerifyCallback,
                 Hide_root_input, ( XtPointer ) Retype_new_user_passwd );

  /* Add submit button to initiate action. */

  submit_rowcol = XtVaCreateManagedWidget( "submit_rowcol",
                         xmRowColumnWidgetClass, form_rowcol,
                         XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
                         XmNorientation, XmHORIZONTAL,
                         XmNnumColumns, 1,
                         XmNtopAttachment, XmATTACH_WIDGET,
                         XmNtopWidget, password_rowcol,
                         XmNleftAttachment, XmATTACH_FORM,
                         XmNrightAttachment, XmATTACH_FORM,
                         NULL);

  label = XtVaCreateManagedWidget( "                ",
                  xmLabelWidgetClass, submit_rowcol,
                  XmNforeground, hci_get_read_color( BACKGROUND_COLOR1 ),
                  XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
                  XmNfontList, hci_get_fontlist( LIST ),
                  NULL);

  button = XtVaCreateManagedWidget( "Submit",
                   xmPushButtonWidgetClass, submit_rowcol,
                   XmNforeground, hci_get_read_color( BUTTON_FOREGROUND ),
                   XmNbackground, hci_get_read_color( BUTTON_BACKGROUND ),
                   XmNfontList, hci_get_fontlist( LIST ),
                   NULL);

  XtAddCallback( button, XmNactivateCallback, Verify_root_user, NULL );

  /* Add empty label to provide vertical spacing. */

  label = XtVaCreateManagedWidget( " ",
                 xmLabelWidgetClass, form_rowcol,
                 XmNforeground, hci_get_read_color( BACKGROUND_COLOR1 ),
                 XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
                 XmNfontList, hci_get_fontlist( LIST ),
                 NULL);

  /* Add label. */

  label_rowcol = XtVaCreateManagedWidget( "label_rowcol",
                         xmRowColumnWidgetClass, form_rowcol,
                         XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
                         XmNorientation, XmHORIZONTAL,
                         XmNnumColumns, 1,
                         XmNtopAttachment, XmATTACH_WIDGET,
                         XmNtopWidget, submit_rowcol,
                         XmNleftAttachment, XmATTACH_FORM,
                         XmNrightAttachment, XmATTACH_FORM,
                         NULL);

  label = XtVaCreateManagedWidget( "         List of current RPG Users",
                  xmLabelWidgetClass, label_rowcol,
                  XmNforeground, hci_get_read_color( TEXT_FOREGROUND ),
                  XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
                  XmNfontList, hci_get_fontlist( LIST ),
                  NULL);

  /* Add text/scroll area to show current list of RPG users. */

  User_list_scroll = XtVaCreateManagedWidget( "user_list_scroll",
                         xmScrolledWindowWidgetClass, form_rowcol,
                         XmNheight, 200,
                         XmNscrollingPolicy, XmAUTOMATIC,
                         XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
                         NULL);

  XtVaGetValues( User_list_scroll,
                 XmNclipWindow, &scroll_clip,
                 NULL );
  XtVaSetValues( scroll_clip,
                 XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
                 NULL );

  /* Initialize list/labels of RPG users. */

  Display_user_list();

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
  HCI_LE_log( "In admin_destroy" );
  return HCI_OK_TO_EXIT;
}

/***************************************************************************/
/* CLOSE_CALLBACK(): Callback for "Close" button. Destroys main widget.    */
/***************************************************************************/

static void Close_callback( Widget w, XtPointer x, XtPointer y )
{
  HCI_LE_log( "In admin_close" );
  XtDestroyWidget( Top_widget );
}

/***************************************************************************/
/* TIMER_PROC(): Timer callback.                                           */
/***************************************************************************/

static void Timer_proc()
{
  /* Do nothing */
}

/***************************************************************************/
/* ADD_CALLBACK(): Callback for "Add" toggle button.                       */
/***************************************************************************/

static void Add_user_callback( Widget w, XtPointer x, XtPointer y )
{
  HCI_LE_log( "Add button selected" );

  Task_flag = ADD_USER;

  /* If adding a user, sensitize password-related fields. */

  XtVaSetValues( Password_label,
                 XmNsensitive, True,
                 NULL );
  XtVaSetValues( Retype_password_label,
                 XmNsensitive, True,
                 NULL );
  XtVaSetValues( Password_input,
                 XmNsensitive, True,
                 XmNbackground, hci_get_read_color( EDIT_BACKGROUND ),
                 XmNvalue, "",
                 NULL );
  XtVaSetValues( Retype_password_input,
                 XmNsensitive, True,
                 XmNbackground, hci_get_read_color( EDIT_BACKGROUND ),
                 XmNvalue, "",
                 NULL );
  XtVaSetValues( Username_input,
                 XmNeditable, True,
                 XmNvalue, "",
                 NULL );
}

/***************************************************************************/
/* REMOVE_CALLBACK(): Callback for "Remove" toggle button.                 */
/***************************************************************************/

static void Del_user_callback( Widget w, XtPointer x, XtPointer y )
{
  HCI_LE_log( "Remove button selected" );

  Task_flag = REMOVE_USER;

  /* If removing a user, desensitize password-related fields. */

  XtVaSetValues( Password_label,
                 XmNsensitive, False,
                 NULL );
  XtVaSetValues( Retype_password_label,
                 XmNsensitive, False,
                 NULL );
  XtVaSetValues( Password_input,
                 XmNsensitive, False,
                 XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
                 XmNvalue, "",
                 NULL );
  XtVaSetValues( Retype_password_input,
                 XmNsensitive, False,
                 XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
                 XmNvalue, "",
                 NULL );
  XtVaSetValues( Username_input,
                 XmNeditable, False,
                 XmNvalue, "select user",
                 NULL );
}

/***************************************************************************/
/* SUBMIT_USER_ACTION(): Callback for "Submit" button.                     */
/***************************************************************************/

static void Submit_user_action()
{
  HCI_LE_log( "Submit button selected" );

  if( Task_flag == ADD_USER )
  {
    Add_rpg_user();
  }
  else
  {
    Remove_rpg_user();
  }

  /* Update list of RPG users. */

  Display_user_list();
}

/***************************************************************************/
/* DISPLAY_USER_LIST(): Updates gui with list of RPG users.                */
/***************************************************************************/

static void Display_user_list()
{
  int i = -1;

  /* Unmanage previous labels so new ones can be created. */

  for( i = 0; i < Number_of_users; i++ )
  {
    XtUnmanageChild( User_list_label[ i ] );
  }

  /* Read password file and populate array with RPG users. */

  Build_user_list();

  /* Build array of labels that correspond to list of RPG users. */

  if( User_list_scroll_form == ( Widget ) NULL )
  {
    User_list_scroll_form = XtVaCreateManagedWidget( "user_list_scroll_form",
                         xmFormWidgetClass, User_list_scroll,
                         XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
                         NULL );
  }

  if( Number_of_users > 0 )
  {
    User_list_label[ 0 ] = XtVaCreateManagedWidget( "user_list_label",
                         xmTextFieldWidgetClass, User_list_scroll_form,
                         XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
                         XmNforeground, hci_get_read_color( TEXT_FOREGROUND ),
                         XmNfontList, hci_get_fontlist( LIST ),
                         XmNcolumns, SCROLL_AREA_WIDTH,
                         XmNtopAttachment, XmATTACH_FORM,
                         XmNleftAttachment, XmATTACH_FORM,
                         XmNrightAttachment, XmATTACH_FORM,
                         NULL );

    XmTextSetString( User_list_label[ 0 ], User_list_entry[ 0 ] );
    XtAddCallback( User_list_label[ 0 ], XmNfocusCallback,
                   Label_callback, NULL );

    for( i = 1; i < Number_of_users; i++ )
    {
      User_list_label[ i ] = XtVaCreateManagedWidget( "user_list_label",
                         xmTextFieldWidgetClass,  User_list_scroll_form,
                         XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
                         XmNforeground, hci_get_read_color( TEXT_FOREGROUND ),
                         XmNfontList, hci_get_fontlist( LIST ),
                         XmNcolumns, MAX_NAME_LENGTH,
                         XmNeditable, False,
                         XmNtopAttachment, XmATTACH_WIDGET,
                         XmNtopWidget, User_list_label[i-1],
                         XmNleftAttachment, XmATTACH_FORM,
                         XmNrightAttachment, XmATTACH_FORM,
                         NULL );

      XmTextSetString( User_list_label[ i ], User_list_entry[ i ] );
      XtAddCallback( User_list_label[ i ], XmNfocusCallback,
                     Label_callback, NULL );
    }
  }
  else
  {
    User_list_label[ 0 ] = XtVaCreateManagedWidget( "None",
                         xmLabelWidgetClass, User_list_scroll_form,
                         XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
                         XmNforeground, hci_get_read_color( TEXT_FOREGROUND ),
                         XmNfontList, hci_get_fontlist( LIST ),
                         XmNtopAttachment, XmATTACH_FORM,
                         XmNleftAttachment, XmATTACH_FORM,
                         XmNrightAttachment, XmATTACH_FORM,
                         NULL );
  }
}

/***************************************************************************/
/* BUILD_USER_LIST(): Read password file and create list of RPG users.     */
/***************************************************************************/

static void Build_user_list()
{
  FILE *fp = NULL;
  char line_buf[ MAX_LINE_LENGTH ];
  char *token;
  int  uid = -1;
  int  gid = -1;

  Number_of_users = 0;

  /* Open/read password file. If there's an error, display popup. */

  fp = fopen( PASSWORD_FILE, "r" );

  if( fp == NULL )
  {
    HCI_LE_error( "Unable to open file: %s", PASSWORD_FILE );
    sprintf( line_buf, "Unable to open file: %s\n", PASSWORD_FILE );
    Error_popup( line_buf );
  }
  else
  {
    while( fgets( line_buf, MAX_LINE_LENGTH, fp ) != NULL )
    {
      /* First token is username. */
      token = strtok( line_buf, ":" );
      if( token != NULL )
      {
        /* Skip second token. */
        strtok( NULL, ":" );
        /* Third token is uid. */
        sscanf( strtok( NULL, ":" ), "%d", &uid );
        /* Fourth token is gid. */
        sscanf( strtok( NULL, ":" ), "%d", &gid );
        if( uid >= RPG_LOW_UID && uid <= RPG_HIGH_UID && gid == RPG_GID )
        {
          sprintf( User_list_entry[ Number_of_users ], token );
          Number_of_users++;
        }
      }
    }
    fclose( fp );
    qsort( User_list_entry, Number_of_users, MAX_NAME_LENGTH, User_sort ); 
  }
}

/***************************************************************************/
/* LABEL_CALLBACK(): Callback for text field containing RPG user.          */
/***************************************************************************/

static void Label_callback( Widget w, XtPointer x, XtPointer y )
{
  /* If user clicks on label, change value of "Username" text field. */

  char *text_string = XmTextGetString( w );
  XmTextSetString( Username_input, text_string );
  XtFree( text_string );
}

/***************************************************************************/
/* ERROR_POPUP(): Build/display pop-up with error message.                 */
/***************************************************************************/

static void Error_popup( char *msg_buf )
{
  Widget dialog;
  Widget temp_widget;
  Widget ok_button;
  XmString ok;
  XmString msg;

  ok = XmStringCreateLocalized( "OK" );

  msg = XmStringCreateLtoR( msg_buf, XmFONTLIST_DEFAULT_TAG );

  dialog = XmCreateInformationDialog( Top_widget, "error", NULL, 0 );

  /* Get rid of Cancel and Help buttons on popup. */

  temp_widget = XmMessageBoxGetChild( dialog, XmDIALOG_CANCEL_BUTTON );
  XtUnmanageChild( temp_widget );

  temp_widget = XmMessageBoxGetChild( dialog, XmDIALOG_HELP_BUTTON );
  XtUnmanageChild( temp_widget );

  /* Set properties of OK button. */

  ok_button = XmMessageBoxGetChild( dialog, XmDIALOG_OK_BUTTON );

  XtVaSetValues( ok_button,
          XmNforeground, hci_get_read_color( BUTTON_FOREGROUND ),
          XmNbackground, hci_get_read_color( BUTTON_BACKGROUND ),
          NULL );

  /* Set properties of popup. */

  XtVaSetValues (dialog,
          XmNmessageString, msg,
          XmNokLabelString, ok,
          XmNbackground, hci_get_read_color( WARNING_COLOR ),
          XmNforeground, hci_get_read_color( TEXT_FOREGROUND ),
          XmNdialogStyle, XmDIALOG_PRIMARY_APPLICATION_MODAL,
          XmNdeleteResponse, XmDESTROY,
          NULL);

  /* Free allocated space. */

  XmStringFree( ok );
  XmStringFree( msg );

  /* Do this to make popup appear. */

  XtManageChild( dialog );
  XtPopup( Top_widget, XtGrabNone );
}

/***************************************************************************/
/* ADD_RPG_USERS(): Add RPG user to system.                                */
/***************************************************************************/

static void Add_rpg_user()
{
  char *username_string = NULL;
  const char *pw_flag = NULL;
  char buf[ 250 ] = "";
  char output_buffer[ MAX_TEXT_OUTPUT_SIZE ] = "";
  int return_code = -1;
  int n_bytes = -1;

  HCI_LE_log( "Adding RPG user" );

  /* Fetch username and password from appropriate widgets. */

  username_string = XmTextGetString( Username_input );

  /* If username or password is of length zero, then warn user. */

  if( strlen( username_string ) == 0 )
  {
    HCI_LE_error( "User name to add is empty" );
    Error_popup( "No \"User's Name\" specified to add." );
  }
  else if( strlen( username_string ) < MIN_NAME_LENGTH || strlen( username_string ) > MAX_NAME_LENGTH )
  {
    HCI_LE_error( "Invaild Username length" );
    Error_popup( "Invalid Username length." );
  }
  else if( strlen( New_user_passwd ) == 0 )
  {
    HCI_LE_error( "No password specified" );
    Error_popup( "No \"Password\" specified." );
  }
  else if( strlen( New_user_passwd ) < MIN_PASSWD_LENGTH || strlen( New_user_passwd ) > MAX_PASSWD_LENGTH )
  {
    HCI_LE_error( "Invalid password length" );
    Error_popup( "Invalid Password length." );
    XtVaSetValues( Password_input, XmNvalue, "", NULL );
    XtVaSetValues( Retype_password_input, XmNvalue, "", NULL );
  }
  else if( strcmp( New_user_passwd, Retype_new_user_passwd) != 0 )
  {
    HCI_LE_error( "Passwords do not match" );
    Error_popup( "Passwords do not match." );
    XtVaSetValues( Password_input, XmNvalue, "", NULL );
    XtVaSetValues( Retype_password_input, XmNvalue, "", NULL );
  }
  else
  {
    /* Remove /tmp/gconfd-username if it exists. If the directory exists,
       the user has been added before. If the ownership info doesn't
       match (i.e. the user now has a different uid), then problems may
       arise when the user logs on. Since passing -f flag to 'rm', no
       check of the 'rm' command's return value is necessary (always 0). */   

    sprintf( buf, "/bin/rm -rf /tmp/gconfd-%s", username_string );
    return_code = MISC_system_to_buffer( buf, output_buffer,
                                         MAX_TEXT_OUTPUT_SIZE, &n_bytes );
    return_code = return_code >> 8;

    /* Make sure password is valid. */

    pw_flag = FascistCheck( New_user_passwd, CRACKLIB_PATH );

    if( pw_flag != NULL )
    {
      HCI_LE_error( "Password validation failed: %s", pw_flag );
      sprintf( output_buffer, "BAD PASSWORD: %s", pw_flag );
      Error_popup( output_buffer );
      XtVaSetValues( Password_input, XmNvalue, "", NULL );
      XtVaSetValues( Retype_password_input, XmNvalue, "", NULL );
      return;
    }

    /* Add RPG user to system. */

    New_salt_key();
    sprintf( buf, "/usr/sbin/useradd -n -p %s -G uucp,lock,floppy,disk,wheel %s",
             crypt( New_user_passwd, Salt_key ), username_string );
    return_code = MISC_system_to_buffer( buf, output_buffer,
                                         MAX_TEXT_OUTPUT_SIZE, &n_bytes );
    return_code = return_code >> 8;

    if( return_code != 0 )
    {
      HCI_LE_error( "useradd command failed: %d", return_code );
      sprintf( buf, "\nError adding RPG User %s.\n", username_string );
      strcat( output_buffer, buf );
      Error_popup( output_buffer );
    }
    else
    {
      sprintf( buf, "chage -d 1 %s", username_string );
      return_code = MISC_system_to_buffer( buf, output_buffer,
                                         MAX_TEXT_OUTPUT_SIZE, &n_bytes );
      return_code = return_code >> 8;

      if( return_code != 0 )
      {
        HCI_LE_error( "chage command failed: %d", return_code );
        sprintf( buf, "\nUser successfully added, but password not expired.\nTell RPG User to change password at login.\n" );
        strcat( output_buffer, buf );
        Error_popup( output_buffer );
      }

      sprintf( buf, "chmod 750 /home/%s", username_string );
      return_code = MISC_system_to_buffer( buf, output_buffer,
                                         MAX_TEXT_OUTPUT_SIZE, &n_bytes );
      return_code = return_code >> 8;

      if( return_code != 0 )
      {
        HCI_LE_error( "chmod command failed: %d", return_code );
        sprintf( buf, "\nUser successfully added, but unable to set\npermission of /home/%s.\n", username_string );
        strcat( output_buffer, buf );
        Error_popup( output_buffer );
      }

      Add_user_to_operate_rpg_file( username_string );
    }
    XtVaSetValues( Password_input, XmNvalue, "", NULL );
    XtVaSetValues( Retype_password_input, XmNvalue, "", NULL );
    XtVaSetValues( Username_input, XmNvalue, "", NULL );
  }

  XtFree( username_string );
}

/***************************************************************************/
/* REMOVE_RPG_USERS(): Remove RPG user to system.                          */
/***************************************************************************/

static void Remove_rpg_user()
{
  char *username_string = NULL;
  char buf[ 250 ] = "";
  char output_buffer[ MAX_TEXT_OUTPUT_SIZE ] = "";
  int return_code = -1;
  int n_bytes = -1;

  HCI_LE_log( "Removing RPG user" );

  /* Fetch username from appropriate widget. */

  username_string = XmTextGetString( Username_input );

  /* If username is of length zero, then warn user. */

  if( strlen( username_string ) != 0 )
  {
    /* Delete RPG user from system. */

    sprintf( buf, "/usr/sbin/userdel -r %s", username_string );
    return_code = MISC_system_to_buffer( buf, output_buffer,
                                         MAX_TEXT_OUTPUT_SIZE, &n_bytes );
    return_code = return_code >> 8;

    if( return_code != 0 )
    {
      HCI_LE_error( "userdel command failed: %d", return_code );
      sprintf( buf, "\nError removing RPG User %s.\n",
               username_string );
      strcat( output_buffer, buf );
      Error_popup( output_buffer );
    }
    else
    {
      XtVaSetValues( Password_input, XmNvalue, "", NULL );
      XtVaSetValues( Retype_password_input, XmNvalue, "", NULL );
      XtVaSetValues( Username_input, XmNvalue, "select user", NULL );
    }

    Remove_user_from_operate_rpg_file( username_string );
  }
  else
  {
    HCI_LE_error( "User name to remove is empty" );
    Error_popup( "No \"User's Name\" specified to remove." );
  }

  XtFree( username_string );
}

/***************************************************************************/
/* ADD_USER_TO_OPERATE_RPG_FILE(): Add RPG user to operate_rpg.conf.       */
/***************************************************************************/

static void Add_user_to_operate_rpg_file( char *username )
{
  char buf[ 250 ] = "";
  char output_buffer[ MAX_TEXT_OUTPUT_SIZE ] = "";
  int return_code = -1;
  int n_bytes = -1;
  int  uid = -1;

  HCI_LE_log( "Adding user to operate_rpg.conf" );

  /* Get user id (uid) of RPG user just added. */

  sprintf( buf, "id -u %s", username );
  return_code = MISC_system_to_buffer( buf, output_buffer,
                                       MAX_TEXT_OUTPUT_SIZE, &n_bytes );
  return_code = return_code >> 8;

  if( return_code != 0 )
  {
    HCI_LE_error( "id -u command failed: %d", return_code );
    sprintf( buf, "\nError using 'id -u' command for RPG User %s.", username );
    strcat( buf, " Add RPG User to operate_rpg.conf file manually.\n" );
    strcat( output_buffer, buf );
    Error_popup( output_buffer );
  }
  else
  {
    /* Append entry for new RPG user to operate_rpg.conf file. */

    sscanf( output_buffer, "%d", &uid );
    sprintf( buf, "/bin/sh -c \"echo 'Operator_uid %d # %s' >> %s\"",
             uid, username, OP_RPG_FILE );
    return_code = MISC_system_to_buffer( buf, output_buffer,
                                         MAX_TEXT_OUTPUT_SIZE, &n_bytes );
    return_code = return_code >> 8;

    if( return_code != 0 )
    {
      HCI_LE_error( "Error modifying %s: %d", OP_RPG_FILE, return_code );
      sprintf( buf, "\nUser successfully added, but unable\nto add entry to operate_rpg.conf file.\nAdd entry manually." );
      strcat( output_buffer, buf );
      Error_popup( output_buffer );
    }
  }
}

/***************************************************************************/
/* REMOVE_USER_FROM_OPERATE_RPG_FILE(): Remove user from operate_rpg.conf. */
/***************************************************************************/

static void Remove_user_from_operate_rpg_file( char *username )
{
  char buf[ 250 ] = "";
  char output_buffer[ MAX_TEXT_OUTPUT_SIZE ] = "";
  int return_code = -1;
  int n_bytes = -1;

  HCI_LE_log( "Removing user from operate_rpg.conf" );

  /* Copy contents of operate_rpg.conf file to a temporary file, */
  /* ignoring the entry for the username just deleted.           */

  sprintf( buf, "/bin/sh -c \"cat %s | grep -v ' %s$' > %s\"",
           OP_RPG_FILE, username, TEMP_FILE);
  return_code = MISC_system_to_buffer( buf, output_buffer,
                                       MAX_TEXT_OUTPUT_SIZE, &n_bytes );
  return_code = return_code >> 8;

  if( return_code != 0 )
  {
    HCI_LE_error( "cat/grep command failed: %d", return_code );
    sprintf( buf, "\nError removing RPG User %s from operate_rpg.conf file.",
             username );
    strcat( buf, " Remove RPG User from file manually.\n" );
    strcat( output_buffer, buf );
    Error_popup( output_buffer );
  }
  else
  {
    /* Overwrite operate_rpg.conf with temporary file. */

    sprintf( buf, "cp %s %s", TEMP_FILE, OP_RPG_FILE );
    return_code = MISC_system_to_buffer( buf, output_buffer,
                                         MAX_TEXT_OUTPUT_SIZE, &n_bytes );
    return_code = return_code >> 8;

    if( return_code != 0 )
    {
      HCI_LE_error( "Error modifying %s: %d", OP_RPG_FILE, return_code );
      sprintf( buf, "\nError overwriting operate_rpg.conf file. Remove %s",
               username );
      strcat( buf, " from file manually.\n" );
      strcat( output_buffer, buf );
      Error_popup( output_buffer );
    }
  }
}

/***************************************************************************/
/* VERIFY_ROOT_USER(): Verify user is root before adding/removing users.   */
/***************************************************************************/

static void Verify_root_user( Widget w, XtPointer x, XtPointer y )
{
  Widget dialog;
  Widget temp_widget;
  Widget ok_button;
  Widget cancel_button;
  Widget text_field;
  XmString ok_button_string;
  XmString cancel_button_string;
  XmString msg;

  HCI_LE_log( "Verify root privilige" );

  /* Get current user id. If user is root, then submit action. */

  if( getuid() == ( int ) 0 )
  {
    HCI_LE_log( "User is root, skip root password prompt" );
    Submit_user_action();
    return;
  }

  /* User isn't root, so create popup for entering root password. */

  dialog = XmCreatePromptDialog( Top_widget, "password", NULL, 0 );

  /* Set various string labels. */
   
  ok_button_string = XmStringCreateLocalized( "OK" );
  cancel_button_string = XmStringCreateLocalized( "Cancel" );
  msg = XmStringCreateLtoR( "   Enter root password:   ", XmFONTLIST_DEFAULT_TAG );

  /* Get rid of Help button on popup. */

  temp_widget = XmSelectionBoxGetChild( dialog, XmDIALOG_HELP_BUTTON );
  XtUnmanageChild( temp_widget );

  /* Set properties of OK button. */

  ok_button = XmSelectionBoxGetChild( dialog, XmDIALOG_OK_BUTTON );

  XtVaSetValues( ok_button,
          XmNforeground, hci_get_read_color( BUTTON_FOREGROUND ),
          XmNbackground, hci_get_read_color( BUTTON_BACKGROUND ),
          XmNfontList, hci_get_fontlist( LIST ),
          NULL );

  XtAddCallback( ok_button, XmNactivateCallback, Authenticate_pam, NULL );

  /* Set properties of Cancel button. */

  cancel_button = XmSelectionBoxGetChild( dialog, XmDIALOG_CANCEL_BUTTON );

  XtVaSetValues( cancel_button,
          XmNforeground, hci_get_read_color( BUTTON_FOREGROUND ),
          XmNbackground, hci_get_read_color( BUTTON_BACKGROUND ),
          XmNfontList, hci_get_fontlist( LIST ),
          NULL );

  /* Set properties of text field. */

  text_field = XmSelectionBoxGetChild( dialog, XmDIALOG_TEXT );

  XtVaSetValues( text_field,
          XmNforeground, hci_get_read_color( TEXT_FOREGROUND ),
          XmNbackground, hci_get_read_color( EDIT_BACKGROUND ),
          XmNfontList, hci_get_fontlist( LIST ),
          XmNcolumns, TEXT_NUM_COLS,
          XmNmaxLength, MAX_PASSWD_LENGTH,
          NULL );

  XtAddCallback( text_field, XmNmodifyVerifyCallback,
                 Hide_root_input, ( XtPointer ) Root_passwd );

  /* Set properties of popup. */

  XtVaSetValues (dialog,
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

  XtManageChild( dialog );
  XtPopup( Top_widget, XtGrabNone );
}

/***************************************************************************/
/* GET_ROOT_PASSWD(): Prompt user for root password. This function is      */
/*                    called by the Linux-PAM module.                      */
/***************************************************************************/

static int Get_root_passwd( int num_msg, const struct pam_message **msg,
                     struct pam_response **resp, void *appdata_ptr )
{
  int count;
  struct pam_response *r;
  char *passwd = ( char * ) malloc( MAX_PASSWD_LENGTH * sizeof( char ) );

  r = (struct pam_response *) calloc( num_msg, sizeof( struct pam_response ) );

  for( count = 0; count < num_msg; count++ )
  {
    switch( msg[ count ] -> msg_style )
    {
      case PAM_PROMPT_ECHO_OFF:
        strcpy( passwd, Root_passwd );
        break;
      default:
        strcpy( passwd, "bad password" );
    }
    r[ count ].resp_retcode = 0;
    r[ count ].resp = passwd;
  }
  *resp = r;
  return PAM_SUCCESS;
}

/***************************************************************************/
/* HIDE_INPUT(): Hide password by replacing letters with "*".              */
/***************************************************************************/

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

/***************************************************************************/
/* AUTHENTICATE_ROOT_USER(): Use Linux-PAM module to verify root password. */
/***************************************************************************/

static void Authenticate_pam( Widget w, XtPointer x, XtPointer y )
{
  pam_handle_t *pamh = NULL;
  int retval;

  retval = pam_start( "passwd", ROOT_USER, &conv, &pamh ); 

  if( retval == PAM_SUCCESS )
  {
    retval = pam_authenticate( pamh, 0 );
    pam_end( pamh, 0 );

    if( retval == PAM_SUCCESS )
    {
      if( ( retval = setuid( 0 ) ) < 0 )
      {
        HCI_LE_error( "Cannot setuid root: %d", retval );
        Error_popup( "Cannot setuid root\n" );
        return;
      }
      Submit_user_action();
    }
    else
    {
      HCI_LE_error( "PAM authentication failure: %d", retval );
      Error_popup( "AUTHENTICATION FAILED: Invalid password\n" );
      XtVaSetValues( Password_input, XmNvalue, "", NULL );
      XtVaSetValues( Retype_password_input, XmNvalue, "", NULL );
      XtVaSetValues( Username_input, XmNvalue, "", NULL );
    }
  }
  else
  {
    HCI_LE_error( "PAM module failure: %d", retval );
    Error_popup( "AUTHENTICATION FAILED: Pam module error\n" );
    XtVaSetValues( Password_input, XmNvalue, "", NULL );
    XtVaSetValues( Retype_password_input, XmNvalue, "", NULL );
    XtVaSetValues( Username_input, XmNvalue, "", NULL );
  }
}

/***************************************************************************/
/* NEW_SALT_KEY(): Generate new and random salt key for use with crypt().  */
/***************************************************************************/

static void New_salt_key()
{
  const char *seedchars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  int len = strlen( seedchars ); 

  srand( time( NULL ) );

  Salt_key[0]='$';
  Salt_key[1]='1';
  Salt_key[2]='$';
  Salt_key[3]=seedchars[rand()%len];
  Salt_key[4]=seedchars[rand()%len];
  Salt_key[5]=seedchars[rand()%len];
  Salt_key[6]=seedchars[rand()%len];
  Salt_key[7]=seedchars[rand()%len];
  Salt_key[8]=seedchars[rand()%len];
  Salt_key[9]=seedchars[rand()%len];
  Salt_key[10]=seedchars[rand()%len];
  Salt_key[11]='$';
  Salt_key[12]='\0';
}

/************************************************************************
 * User_sort: Sorting function for user name array.
 ************************************************************************/

static int User_sort( const void *input1, const void *input2 )
{
  const char *in1 = input1;
  const char *in2 = input2;

  return strcmp( in1, in2 );
}


