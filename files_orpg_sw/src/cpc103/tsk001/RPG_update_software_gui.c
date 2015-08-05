/************************************************************************

      The main source file for GUI wrapper of "RPG_update_software" script.

************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/04/21 20:52:43 $
 * $Id: RPG_update_software_gui.c,v 1.16 2014/04/21 20:52:43 steves Exp $
 * $Revision: 1.16 $
 * $State: Exp $
 */

/* RPG include files. */

#include <hci.h>
#include <security/pam_appl.h>

/* Definitions. */

enum { NO_ACTION, DISPLAY_BUILD_NUMBER, UPDATE_SOFTWARE };
enum { UNKNOWN_CD = 20, SITE_SPECIFIC_CD, EPSS_CD, RDA_HELP_CD, RDA_JAR_CD, PATCH_CD, TOOLS_CD, VIRUS_DEFINITION_CD, SECURITY_SCAN_CD, TEST_ECP_CD };

#define	SCROLL_AREA_HEIGHT	300
#define	SCROLL_AREA_WIDTH	500
#define	MAX_PASSWD_LENGTH	20
#define	UPDATE_CMD_LEN		128
#define	MAX_OBUF		2048
#define	MAX_NUM_PROMPT_LINES	15
#define	ROOT_USER		"root"
#define UPDATE_SCRIPT		"RPG_update_software -N "
#define MAX_NUM_LABELS		30
#define SUDO_PASSWD_PROMPT	"Root Password:"

/* Global variables. */

static Widget		Top_widget = ( Widget ) NULL;
static Widget		Update_button = ( Widget ) NULL;
static Widget		Build_button = ( Widget ) NULL;
static Widget		Msg_dialog = ( Widget ) NULL;
static Widget		Root_dialog = ( Widget ) NULL;
static Widget		Info_scroll_form = ( Widget ) NULL;
static Widget		Info_scroll = ( Widget ) NULL;
static char		Root_passwd[MAX_PASSWD_LENGTH+1];
static char		Popup_buf[MAX_OBUF];
static char		Output_msgs[MAX_NUM_LABELS][MAX_OBUF];
static int		Output_msgs_code[MAX_NUM_LABELS];
static int		Current_msg_pos = 0;
static int		Num_output_msgs = 0;
static char		Update_cmd[UPDATE_CMD_LEN];
static int		Root_user_verified = NO;
static int		Reverify_passwd_flag = NO;
static int		Initial_prompt_for_CD = NO;
static int		User_inserted_CD = NO;
static int		Found_cd_type = NO;
static int		Cd_type = UNKNOWN_CD;
static int		Action_flag = NO_ACTION;
static int		Coprocess_flag = HCI_CP_NOT_STARTED;
static void		*Cp = NULL;
static void		( *Coprocess_has_stdout )( char * ) = NULL;
static void		( *Coprocess_has_stderr )( char * ) = NULL;
static void		( *Manage_coprocess )( void ) = NULL;

/* Function prototypes. */

static int  Destroy_callback( Widget, XtPointer, XtPointer );
static void Close_callback( Widget, XtPointer, XtPointer );
static void Set_action_flag( Widget, XtPointer, XtPointer );
static void Display_build_number();
static int  Determine_cd_type();
static void Verify_root_user();
static void Cancel_verify( Widget, XtPointer, XtPointer );
static void Hide_root_input( Widget, XtPointer, XtPointer );
static void Authenticate_pam( Widget, XtPointer, XtPointer );
static void Reset_to_initial_state();
static void Start_coprocess();
static void Manage_coprocess_generic();
static void Manage_coprocess_sudo();
static void Coprocess_has_stdout_generic( char * );
static void Coprocess_has_stdout_site_specific( char * );
static void Coprocess_has_stderr_generic( char * );
static void Coprocess_has_stderr_sudo( char * );
static void Close_coprocess();
static void Trim_coprocess_buf( char * );
static void Update_scroll_display( char *, int );
static void Update_label( Widget , int );
static void User_adapt_prompt_popup( char *, int );
static void Initial_CD_ok_callback( Widget, XtPointer, XtPointer );
static void Initial_CD_cancel_callback( Widget, XtPointer, XtPointer );
static void Cancel_merge_callback( Widget, XtPointer, XtPointer );
static void Ok_popup_ok_callback( Widget w, XtPointer x, XtPointer y );
static void Timer_proc();
static int  Get_root_passwd( int, const struct pam_message **,
                      struct pam_response **, void * );

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
  Widget	scroll_clip;

  /* Initialize HCI. */

  HCI_partial_init( argc, argv, -1 );

  Top_widget = HCI_get_top_widget();

  HCI_set_destroy_callback( Destroy_callback );

  XtVaSetValues( Top_widget, XmNtitle, "Update RPG Software", NULL );

  /* Define top-level form and rowcol to hold all widgets. */

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

  /* Add buttons to update software or display build information. */

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

  XtVaCreateManagedWidget( "  ",
                   xmLabelWidgetClass, frame_rowcol,
                   XmNforeground, hci_get_read_color( BACKGROUND_COLOR1 ),
                   XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
                   NULL);
 
  Update_button = XtVaCreateManagedWidget( "Update Software",
                   xmPushButtonWidgetClass, frame_rowcol,
                   XmNforeground, hci_get_read_color( BUTTON_FOREGROUND ),
                   XmNbackground, hci_get_read_color( BUTTON_BACKGROUND ),
                   XmNfontList, hci_get_fontlist( LIST ),
                   NULL);

  XtAddCallback( Update_button,
                 XmNactivateCallback, Set_action_flag,
                 ( XtPointer ) UPDATE_SOFTWARE );

  XtVaCreateManagedWidget( "   ",
                   xmLabelWidgetClass, frame_rowcol,
                   XmNforeground, hci_get_read_color( BACKGROUND_COLOR1 ),
                   XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
                   NULL);
 
  Build_button = XtVaCreateManagedWidget( "Build Number",
                   xmPushButtonWidgetClass, frame_rowcol,
                   XmNforeground, hci_get_read_color( BUTTON_FOREGROUND ),
                   XmNbackground, hci_get_read_color( BUTTON_BACKGROUND ),
                   XmNfontList, hci_get_fontlist( LIST ),
                   NULL);

  XtAddCallback( Build_button,
                 XmNactivateCallback, Set_action_flag,
                 ( XtPointer ) DISPLAY_BUILD_NUMBER );

  /* Add empty label to provide vertical spacing. */

  label = XtVaCreateManagedWidget( " ",
                 xmLabelWidgetClass, form_rowcol,
                 XmNforeground, hci_get_read_color( BACKGROUND_COLOR1 ),
                 XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
                 XmNfontList, hci_get_fontlist( LIST ),
                 NULL);

  /* Create scroll area to display output. */

  Info_scroll = XtVaCreateManagedWidget ("data_scroll",
                xmScrolledWindowWidgetClass, form_rowcol,
                XmNheight,              SCROLL_AREA_HEIGHT,
                XmNwidth,               SCROLL_AREA_WIDTH,
                XmNscrollingPolicy,     XmAUTOMATIC,
                XmNforeground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNtopAttachment,       XmATTACH_WIDGET,
                XmNtopWidget,           label,
                XmNbottomAttachment,    XmATTACH_FORM,
                XmNleftAttachment,      XmATTACH_FORM,
                XmNrightAttachment,     XmATTACH_FORM,
                NULL);

  XtVaGetValues( Info_scroll, XmNclipWindow, &scroll_clip, NULL );

  XtVaSetValues( scroll_clip,
                 XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
                 NULL );

  /* Display GUI to screen. */

  XtRealizeWidget( Top_widget );

  /* Start HCI loop. */

  HCI_start( Timer_proc, HCI_ONE_SECOND, NO_RESIZE_HCI );

  return 0;
}

/**************************************************************************
 DESTROY_CALLBACK: Callback for main widget when it is destroyed.
**************************************************************************/

static int Destroy_callback( Widget w, XtPointer x, XtPointer y )
{
  HCI_LE_log( 0, "In destroy callback" );

  if( Coprocess_flag != HCI_CP_NOT_STARTED )
  {
    /* Don't let user exit/close if coprocess is running. */
    HCI_LE_log( "Cannot destroy widget with running coprocess" );
    sprintf( Popup_buf, "You are not allowed to exit this task until\n" );
    strcat( Popup_buf,  "the update process has been completed.\n" );
    hci_error_popup( Top_widget, Popup_buf, NULL );
    return HCI_NOT_OK_TO_EXIT;
  }

  return HCI_OK_TO_EXIT;
}

/**************************************************************************
 CLOSE_CALLBACK: Callback for "Close" button. Destroys main widget.
**************************************************************************/

static void Close_callback( Widget w, XtPointer x, XtPointer y )
{
  HCI_LE_log( "In Close callback." );

  if( Coprocess_flag != HCI_CP_NOT_STARTED )
  {
    /* Don't let user exit/close if coprocess is running. */
    HCI_LE_log( "Cannot close widget with running coprocess" );
    sprintf( Popup_buf, "You are not allowed to exit this task until\n" );
    strcat( Popup_buf,  "the update process has been completed.\n" );
    hci_error_popup( Top_widget, Popup_buf, NULL );
  }
  else
  {
    HCI_LE_log( "Calling XtDestroyWidget( Top_widget )" );
    XtDestroyWidget( Top_widget );
  }
}

/**************************************************************************
 SET_ACTION_FLAG: Callback for any of the action buttons.
**************************************************************************/

static void Set_action_flag( Widget w, XtPointer x, XtPointer y )
{
  /* Set action flag. */

  Action_flag = ( int ) x;

  HCI_LE_log( "User selected action flag: %d", Action_flag );

  /* Set cursor to "busy". */

  HCI_busy_cursor();

  /* De-sensitize action buttons. */

  XtVaSetValues( Build_button, XmNsensitive, False, NULL );
  XtVaSetValues( Update_button, XmNsensitive, False, NULL );
}

/**************************************************************************
 TIMER_PROC: Callback for event timer.
**************************************************************************/

static void Timer_proc()
{
  /* Take action according to value of Action_flag. Order is important.
     Displaying build information should be first. If user wants to
     update the software, make sure the CD is a recongized format.
     Next, if the action requires root privilige, authenticate the user
     for root access. Next, start the appropriate coprocess. Next,
     manage the appropriate coprocess. Once the coprocess is finished,
     reset the GUI to the initial state. */

  if( Action_flag == DISPLAY_BUILD_NUMBER )
  {
    Display_build_number();
    Reset_to_initial_state();
  }
  else if( Action_flag == UPDATE_SOFTWARE )
  {
    if( Initial_prompt_for_CD == NO )
    {
      Initial_prompt_for_CD = YES;
      sprintf( Popup_buf, "To begin, insert media into\nmedia tray and click \"OK\"" );
      hci_custom_confirm_popup( Top_widget, Popup_buf, "OK", Initial_CD_ok_callback, "Cancel", Initial_CD_cancel_callback );
    }
    else if( User_inserted_CD == NO )
    {
      /* Waiting for user to insert CD and click "OK" on popup. */
    }
    else if( Found_cd_type == NO )
    {
      if( Determine_cd_type() == UNKNOWN_CD ){ Reset_to_initial_state(); }
    }
    else if( Root_user_verified == NO &&
             ( Cd_type == PATCH_CD ||
               Cd_type == SITE_SPECIFIC_CD ||
               Cd_type == VIRUS_DEFINITION_CD ||
               Cd_type == SECURITY_SCAN_CD ||
               Cd_type == TEST_ECP_CD ) )
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

/**************************************************************************
 DISPLAY_BUILD_NUMBER: Callback for "Display Build Number" button.
**************************************************************************/

static void Display_build_number()
{
  char buf[32];
  char obuf[MAX_OBUF];
  int n;
  int ret;

  HCI_LE_log( "Attempting to display build number" );

  /* Build command to run on local system. Display build information
     in scroll area if successful. Popup error message otherwise. */

  sprintf( buf, "%s -b", UPDATE_SCRIPT );
  if( ( ret = ( MISC_system_to_buffer( buf, obuf, MAX_OBUF, &n ) >> 8 ) ) != 0 )
  {
    HCI_LE_error( "Command: %s failed (%d)", buf, ret );
    hci_error_popup( Top_widget, obuf, NULL );
  }
  else
  {
    HCI_LE_log( "Display build version:" );
    HCI_LE_log( "%ms", obuf );
    Update_scroll_display( obuf, HCI_CP_STDOUT );
  }
}

/**************************************************************************
 DETERMINE_CD_TYPE: Determine type of CD being used for the update.
**************************************************************************/

static int Determine_cd_type()
{
  char buf[64];
  char obuf[MAX_OBUF];
  int n;
  int ret;

  HCI_LE_log( "Attempting to determine type of CD being used for update" );

  /* Build command to run on local system. */

  sprintf( buf, "%s -c", UPDATE_SCRIPT );
  if( ( ret = ( MISC_system_to_buffer( buf, obuf, MAX_OBUF, &n ) >> 8 ) ) < 0 )
  {
    HCI_LE_error( "Command: %s failed (%d)", buf, ret );
    sprintf( Popup_buf, "Command: %s failed (%d)\n", buf, ret );
    strcpy( Popup_buf, obuf );
    hci_error_popup( Top_widget, Popup_buf, NULL );
    return UNKNOWN_CD;
  }
  else if( ret < UNKNOWN_CD )
  {
    HCI_LE_error( "CD missing or unable to determine type (%d)", ret );
    sprintf( Popup_buf, "CD missing or unable to determine type (%d)\n", ret );
    strcpy( Popup_buf, obuf );
    hci_error_popup( Top_widget, Popup_buf, NULL );
    return UNKNOWN_CD;
  }
  else if( ret == UNKNOWN_CD )
  {
    strcpy( obuf, "The CD is of an unknown type." );
    HCI_LE_error( obuf );
    hci_error_popup( Top_widget, obuf, NULL );
    return UNKNOWN_CD;
  }

  Found_cd_type = YES;

  if( ret == PATCH_CD )
  {
    HCI_LE_log( "CD is a patch CD" );
    Cd_type = PATCH_CD;
    Coprocess_has_stdout = Coprocess_has_stdout_generic;
    Coprocess_has_stderr = Coprocess_has_stderr_sudo;
    Manage_coprocess = Manage_coprocess_sudo;
    sprintf( Update_cmd, "%s -P", UPDATE_SCRIPT );
  }
  else if( ret == SITE_SPECIFIC_CD )
  {
    HCI_LE_log( "CD is a site-specific CD" );
    Cd_type = SITE_SPECIFIC_CD;
    Coprocess_has_stdout = Coprocess_has_stdout_site_specific;
    Coprocess_has_stderr = Coprocess_has_stderr_sudo;
    Manage_coprocess = Manage_coprocess_sudo;
    sprintf( Update_cmd, "%s -S", UPDATE_SCRIPT );
  }
  else if( ret == RDA_HELP_CD )
  {
    HCI_LE_log( "CD is a RDA Help File CD" );
    Cd_type = RDA_HELP_CD;
    Coprocess_has_stdout = Coprocess_has_stdout_generic;
    Coprocess_has_stderr = Coprocess_has_stderr_generic;
    Manage_coprocess = Manage_coprocess_generic;
    sprintf( Update_cmd, "%s -H", UPDATE_SCRIPT );
  }
  else if( ret == RDA_JAR_CD )
  {
    HCI_LE_log( "CD is a RDA JAR File CD" );
    Cd_type = RDA_JAR_CD;
    Coprocess_has_stdout = Coprocess_has_stdout_generic;
    Coprocess_has_stderr = Coprocess_has_stderr_generic;
    Manage_coprocess = Manage_coprocess_generic;
    sprintf( Update_cmd, "%s -J", UPDATE_SCRIPT );
  }
  else if( ret == EPSS_CD )
  {
    HCI_LE_log( "CD is an EPSS CD" );
    Cd_type = EPSS_CD;
    Coprocess_has_stdout = Coprocess_has_stdout_generic;
    Coprocess_has_stderr = Coprocess_has_stderr_generic;
    Manage_coprocess = Manage_coprocess_generic;
    sprintf( Update_cmd, "%s -E", UPDATE_SCRIPT );
  }
  else if( ret == TOOLS_CD )
  {
    HCI_LE_log( "CD is a Tools CD" );
    Cd_type = TOOLS_CD;
    Coprocess_has_stdout = Coprocess_has_stdout_generic;
    Coprocess_has_stderr = Coprocess_has_stderr_generic;
    Manage_coprocess = Manage_coprocess_generic;
    sprintf( Update_cmd, "%s -T", UPDATE_SCRIPT );
  }
  else if( ret == VIRUS_DEFINITION_CD )
  {
    HCI_LE_log( "CD is an Antivirus DAT update CD" );
    Cd_type = VIRUS_DEFINITION_CD;
    Coprocess_has_stdout = Coprocess_has_stdout_generic;
    Coprocess_has_stderr = Coprocess_has_stderr_sudo;
    Manage_coprocess = Manage_coprocess_sudo;
    sprintf( Update_cmd, "%s -A", UPDATE_SCRIPT );
  }
  else if( ret == SECURITY_SCAN_CD )
  {
    HCI_LE_log( "CD is a Security Scan CD" );
    Cd_type = SECURITY_SCAN_CD;
    Coprocess_has_stdout = Coprocess_has_stdout_generic;
    Coprocess_has_stderr = Coprocess_has_stderr_sudo;
    Manage_coprocess = Manage_coprocess_sudo;
    sprintf( Update_cmd, "%s -B", UPDATE_SCRIPT );
  }
  else if( ret == TEST_ECP_CD )
  {
    HCI_LE_log( "CD is a Test ECP CD" );
    Cd_type = TEST_ECP_CD;
    Coprocess_has_stdout = Coprocess_has_stdout_generic;
    Coprocess_has_stderr = Coprocess_has_stderr_sudo;
    Manage_coprocess = Manage_coprocess_sudo;
    sprintf( Update_cmd, "%s -C", UPDATE_SCRIPT );
  }

  return ret;
}

/**************************************************************************
 START_COPROCESS: Start coprocess to update from CD.
**************************************************************************/

static void Start_coprocess()
{
  int ret = -1;

  HCI_LE_log( "Starting coprocess: %s", Update_cmd );

  if( ( ret = MISC_cp_open( Update_cmd, HCI_CP_MANAGE, &Cp ) ) != 0 )
  {
    sprintf( Popup_buf, "MISC_cp_open failed (%d)", ret );
    HCI_LE_error( Popup_buf );
    hci_error_popup( Top_widget, Popup_buf, NULL );
    Coprocess_flag = HCI_CP_FINISHED;
  }
  else
  {
    HCI_LE_log( "Coprocess successfully started" );
    Coprocess_flag = HCI_CP_STARTED;
  }
}

/**************************************************************************
 MANAGE_COPROCESS_GENERIC: Generic manage coprocess.
**************************************************************************/

static void Manage_coprocess_generic()
{
  int ret = -1;
  char cp_buf[MAX_OBUF];

  /* Read coprocess to see if any new data is available. If so, take
     action depending on coprocess status. */

  while( Cp != NULL &&
         ( ret = MISC_cp_read_from_cp( Cp, cp_buf, MAX_OBUF ) ) != 0 )
  {
    Trim_coprocess_buf( cp_buf );

    if( ret == HCI_CP_DOWN )
    {
      if( ( ret = ( MISC_cp_get_status( Cp ) >> 8 ) ) != 0 )
      {
        HCI_LE_error( "Coprocess exited with error (%d)", ret );
        sprintf( Popup_buf, "Update unsuccessful (%d)", ret );
        hci_error_popup( Top_widget, Popup_buf, NULL );
      }
      else
      {
        HCI_LE_log( "Coprocess exited with no error" );
        hci_info_popup( Top_widget, "Update successful", NULL );
      }
      Close_coprocess();
      break;
    }
    else if( ret == HCI_CP_STDERR )
    {
      Coprocess_has_stderr( cp_buf );
    }
    else if( ret == HCI_CP_STDOUT )
    {
      Coprocess_has_stdout( cp_buf );
    }
    else
    {
      /* No new data available, do nothing... */
    }
  }
}

/**************************************************************************
 MANAGE_COPROCESS_SUDO: Manage coprocess when using sudo.
**************************************************************************/

static void Manage_coprocess_sudo()
{
  int ret = -1;
  char cp_buf[MAX_OBUF];

  /* Watch for sudo password prompt. This has to be done since the prompt
     does not end with a newline. */

  strcpy( cp_buf, SUDO_PASSWD_PROMPT );

  /* Read coprocess to see if any new data is available. If so, take
     action depending on coprocess status. */

  while( Cp != NULL &&
         ( ret = MISC_cp_read_from_cp( Cp, cp_buf, MAX_OBUF | MISC_CP_MATCH_STR) ) != 0 )
  {
    /* Don't remove trailing newline for site-specific CD, since they
       may be needed in a popup. */
    if( Cd_type != SITE_SPECIFIC_CD ){ Trim_coprocess_buf( cp_buf ); }
    if( ret == HCI_CP_DOWN )
    {
      if( ( ret = ( MISC_cp_get_status( Cp ) >> 8 ) ) != 0 )
      {
        HCI_LE_error( "Coprocess exited with error (%d)", ret );
        sprintf( Popup_buf, "Update unsuccessful (%d)", ret );
        hci_error_popup( Top_widget, Popup_buf, NULL );
      }
      else
      {
        HCI_LE_log( "Coprocess exited with no error" );
        hci_info_popup( Top_widget, "Update successful", NULL );
      }
      Close_coprocess();
      break;
    }
    else if( ret == HCI_CP_STDERR )
    {
      Coprocess_has_stderr( cp_buf );
    }
    else if( ret == HCI_CP_STDOUT )
    {
      Coprocess_has_stdout( cp_buf );
    }
    else
    {
      /* No new data available, do nothing... */
    }
  }
}

/**************************************************************************
 COPROCESS_HAS_STDOUT_GENERIC: Handle stdout in a generic way.
**************************************************************************/

static void Coprocess_has_stdout_generic( char *cp_buf )
{
  HCI_LE_log( "Coprocess STDOUT:" );
  HCI_LE_log( "%ms", cp_buf );
  Update_scroll_display( cp_buf, HCI_CP_STDOUT );
}

/**************************************************************************
 COPROCESS_HAS_STDOUT_SITE_SPECIFIC: Handle stdout for Site-Specific CD
**************************************************************************/

static void Coprocess_has_stdout_site_specific( char *cp_buf )
{
  static char Prompt_msg_buf[MAX_OBUF*MAX_NUM_PROMPT_LINES];
  static int user_adapt_prompt = 0;
  static int ok_prompt = 0;
  static int num_adapt_buttons = 0;
  static int error_reboot_prompt = 0;

  /* Search stdout from coprocess for key phrases that will be used
     to prompt for user input. Since the prompt may encompass
     multiple lines, collect all lines before creating the popup. */

  if( strstr( cp_buf, "The CD contains default adaptation" ) != NULL )
  {
    /* First part of key phrase. Set flag to indicate more lines
       need to be collected. */
    sprintf( Prompt_msg_buf, cp_buf );
    user_adapt_prompt = 1;
  }
  else if( user_adapt_prompt && strstr( cp_buf, "Enter choice (1-" ) != NULL )
  {
    /* Last part of multi-line key phrase. Set flag to indicate all
       lines have been collected and create the popup. */
    user_adapt_prompt = 0;
    /* Tailor wording to be more appropriate for a GUI. */
    strcat( Prompt_msg_buf, "Click appropriate button below:\n" );
    User_adapt_prompt_popup( Prompt_msg_buf, num_adapt_buttons );
    num_adapt_buttons = 0;
  }
  else if( strstr( cp_buf, "The adaptation data was successfully merged." ) != NULL )
  {
    /* First part of key phrase. Set flag to indicate more lines
       need to be collected. */
    sprintf( Prompt_msg_buf, cp_buf );
    ok_prompt = 1;
  }
  else if( ok_prompt && strstr( cp_buf, " the tray and press Enter" ) != NULL )
  {
    /* Last part of multi-line key phrase. Set flag to indicate all
       lines have been collected and create the popup. */
    ok_prompt = 0;
    /* Substitute wording more appropriate for a GUI. */
    strcpy( strstr( cp_buf, "press Enter" ), "click OK." );
    strcat( Prompt_msg_buf, cp_buf );
    hci_custom_confirm_popup( Top_widget, Prompt_msg_buf, "OK", Ok_popup_ok_callback, "Cancel Merge", Cancel_merge_callback );
  }
  else if( strstr( cp_buf, "Insert previously created adaptation" ) != NULL )
  {
    /* This key phrase is only one line. Substitute wording more
       appropriate for a GUI. */
    strcpy( strstr( cp_buf, "press Enter" ), "click OK." );
    hci_custom_confirm_popup( Top_widget, cp_buf, "OK", Ok_popup_ok_callback, "Cancel Merge", Cancel_merge_callback );
  }
  else if( strstr( cp_buf, "Insert second blank CD" ) != NULL )
  {
    /* This key phrase is only one line. Substitute wording more
       appropriate for a GUI. */
    strcpy( strstr( cp_buf, "press Enter" ), "click OK." );
    hci_custom_confirm_popup( Top_widget, cp_buf, "OK", Ok_popup_ok_callback, "Cancel Merge", Cancel_merge_callback );
  }
  else if( strstr( cp_buf, "A problem was encountered while" ) != NULL )
  {
    /* First part of key phrase. Set flag to indicate more lines
       need to be collected. */
    sprintf( Prompt_msg_buf, cp_buf );
    error_reboot_prompt = 1;
  }
  else if( error_reboot_prompt && strstr( cp_buf, "reboot is complete." ) != NULL )
  {
    /* Last part of multi-line key phrase. Set flag to indicate all
       lines have been collected and create the popup. */
    error_reboot_prompt = 0;
    strcat( Prompt_msg_buf, cp_buf );
    HCI_LE_log( "Received error reboot key phrase" );
    hci_info_popup( Top_widget, Prompt_msg_buf, Ok_popup_ok_callback );
  }
  else if( user_adapt_prompt || ok_prompt || error_reboot_prompt )
  {
    /* Continue to collect stdout for multiple line prompt. Append
       to the buffer that will become the text for the popup. */

    if( user_adapt_prompt )
    {
      /* Count the number of unique buttons required for the user
         adapt prompt. This is not very elegant. At a later time,
         perhaps regcomp/regexec can be used to implement regular
         expression pattern matching. */
      if( strstr( cp_buf, "1 - " ) != NULL ){ num_adapt_buttons++; }
      else if( strstr( cp_buf, "2 - " ) != NULL ){ num_adapt_buttons++; }
      else if( strstr( cp_buf, "3 - " ) != NULL ){ num_adapt_buttons++; }
      else if( strstr( cp_buf, "4 - " ) != NULL ){ num_adapt_buttons++; }
      else if( strstr( cp_buf, "5 - " ) != NULL ){ num_adapt_buttons++; }
      else if( strstr( cp_buf, "6 - " ) != NULL ){ num_adapt_buttons++; }
    }
    strcat( Prompt_msg_buf, cp_buf );
  }
  else if( strstr( cp_buf, "be rebooted to complete the update" ) != NULL )
  {
    /* This key phrase is only one line. */
    HCI_LE_log( "Received reboot key phrase" );
    hci_info_popup( Top_widget, cp_buf, Ok_popup_ok_callback );
  }
  else
  {
    /* Stdout line did not contain a key phrase and was not part
       of a multiple line prompt. */
    Trim_coprocess_buf( cp_buf );
    HCI_LE_log( "Coprocess STDOUT:" );
    HCI_LE_log( "%ms", cp_buf );
    Update_scroll_display( cp_buf, HCI_CP_STDOUT );
  }
}

/**************************************************************************
 COPROCESS_HAS_STDERR_GENERIC: Handle stderr in a generic way.
**************************************************************************/

static void Coprocess_has_stderr_generic( char *cp_buf )
{
  HCI_LE_error( "Coprocess STDERR:" );
  HCI_LE_error( "%ms", cp_buf );
  Update_scroll_display( cp_buf, HCI_CP_STDERR );
}

/**************************************************************************
 COPROCESS_HAS_STDERR_SUDO: Handle stderr when using sudo.
**************************************************************************/

static void Coprocess_has_stderr_sudo( char *cp_buf )
{
  char *substr = NULL;

  /* Check for root password prompt from sudo and send
     root password if detected. */

  if( strstr( cp_buf, SUDO_PASSWD_PROMPT ) != NULL )
  {
    HCI_LE_log( "Sending root password to sudo" );
    if( Cp != NULL ){ MISC_cp_write_to_cp( Cp, Root_passwd ); }
    else
    {
      strcpy( Popup_buf, "Coprocess terminated. Unable to verify root." );
      hci_error_popup( Top_widget, Popup_buf, NULL );
    }
  }
  else
  {
    HCI_LE_error( "Coprocess STDERR:" ); 
    HCI_LE_error( "%ms", cp_buf );

    /* Ignore "already installed" RPMS.  No need to report as an error. */
    if( Cd_type == PATCH_CD )
    {
      if( ((substr = strstr( cp_buf, "already installed" )) != NULL )
                                   &&
          ((substr = strstr( cp_buf, "kernel" )) == NULL ) ) {return;}
    }

    Update_scroll_display( cp_buf, HCI_CP_STDERR );
  }
}

/**************************************************************************
 VERIFY_ROOT_USER: Verify user has root access.
**************************************************************************/

static void Verify_root_user()
{
  Widget ok_button;
  Widget cancel_button;
  Widget text_field;
  Widget sel_label;
  XmString ok_button_string;
  XmString cancel_button_string;
  XmString msg;

  /* If Root_dialog isn't null and isn't managed (visible), then
     password popup has already been displayed but user entered
     incorrect root password. When all other popups (Msg_dialog)
     have been closed, re-manage the Root_dialog so the user can
     re-enter the root password. */

  if( Root_dialog != ( Widget ) NULL &&
      XtIsManaged( Root_dialog ) == False &&
      XtIsManaged( Msg_dialog ) == False )
  {
    XtManageChild( Root_dialog );
    return;
  }

  /* If Root_dialog isn't null, then the popup is currently visible. */

  if( Root_dialog != ( Widget ) NULL ){ return; }

  /* Create popup for entering root password. */

  Root_dialog = XmCreatePromptDialog( Top_widget, "password", NULL, 0 );

  /* Set various string labels. */
   
  ok_button_string = XmStringCreateLocalized( "OK" );
  cancel_button_string = XmStringCreateLocalized( "Cancel" );
  if( Reverify_passwd_flag == YES )
  {
    Reverify_passwd_flag = NO;
    msg = XmStringCreateLtoR( "Authentication Failed\n\nEnter root password:", XmFONTLIST_DEFAULT_TAG );
  }
  else
  {
    msg = XmStringCreateLtoR( "Enter root password:", XmFONTLIST_DEFAULT_TAG );
  }

  /* Get rid of Help button on popup. */

  XtUnmanageChild( XmSelectionBoxGetChild( Root_dialog, XmDIALOG_HELP_BUTTON ));

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

  /* Set properties of selection label. */

  sel_label = XmSelectionBoxGetChild( Root_dialog, XmDIALOG_SELECTION_LABEL );

  XtVaSetValues( sel_label,
          XmNforeground, hci_get_read_color( TEXT_FOREGROUND ),
          XmNbackground, hci_get_read_color( WARNING_COLOR ),
          XmNfontList, hci_get_fontlist( LIST ),
          NULL );

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

/**************************************************************************
 CANCEL_VERIFY: Cancel root password verification.
**************************************************************************/

static void Cancel_verify( Widget w, XtPointer x, XtPointer y )
{
  /* User decided to not verify root password. Destroy root password
     prompt and reset GUI to initial state. Popup messsage to user
     that update cannot proceed without root authentication. */

  if( Root_dialog != ( Widget ) NULL )
  {
    XtDestroyWidget( Root_dialog );
    Root_dialog = ( Widget ) NULL;
  }
  HCI_LE_log( "User cancelled root authentication" );
  strcpy( Popup_buf, "Unable to update without root password" );
  hci_warning_popup( Top_widget, Popup_buf, NULL );
  Reset_to_initial_state();
}

/**************************************************************************
 GET_ROOT_PASSWD: Prompt user for root password. This function is
                  called by the Linux-PAM module.
**************************************************************************/

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

/**************************************************************************
 HIDE_INPUT: Hide password by replacing letters with "*".
**************************************************************************/

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

/**************************************************************************
 AUTHENTICATE_ROOT_USER: Use Linux-PAM module to verify root password.
**************************************************************************/

static void Authenticate_pam( Widget w, XtPointer x, XtPointer y )
{
  pam_handle_t *pamh = NULL;
  int ret;

  /* Start PAM session. */
  if( ( ret = pam_start( "passwd", ROOT_USER, &conv, &pamh ) ) == PAM_SUCCESS )
  {
    if( ( ret = pam_authenticate( pamh, 0 ) ) == PAM_SUCCESS )
    {
      HCI_LE_log( "PAM_SUCCESS: root password verified" );
      /* User entered correct password. Make sure accout is valid. */
      if( ( ret = pam_acct_mgmt( pamh, 0 ) ) == PAM_SUCCESS )
      {
        /* Root account is valid. */
        HCI_LE_log( "PAM_SUCCESS: root account valid" );
        Root_user_verified = YES;
        Root_passwd[strlen( Root_passwd )] = '\n';
      }
      else
      {
        /* Root account is not valid. */
        if( ret == PAM_NEW_AUTHTOK_REQD )
        {
          /* Root password expired. */
          strcpy( Popup_buf, "Authentication failed: root password expired.\nUnable to continue until password is updated." );
        }
        else
        {
          /* Operator needs help. */
          sprintf( Popup_buf, "Authentication failed: root account is invalid (%d).\nCall the WSR-88D Hotline for help.", ret );
        }
        HCI_LE_error( Popup_buf );
        hci_error_popup( Top_widget, Popup_buf, NULL );
        XtDestroyWidget( Root_dialog );
        Root_dialog = ( Widget ) NULL;
        Reset_to_initial_state();
      }
    }
    else
    {
      /* Wrong password. */
      strcpy( Popup_buf, "Authentication failed: invalid password" );
      HCI_LE_error( Popup_buf );
      XtDestroyWidget( Root_dialog );
      Root_dialog = ( Widget ) NULL;
      Reverify_passwd_flag = YES;
    }
  }
  else
  {
    /* PAM failed for some reason. */
    sprintf( Popup_buf, "Authentication failed: Pam module error (%d)", ret );
    HCI_LE_error( Popup_buf );
    hci_error_popup( Top_widget, Popup_buf, NULL );
    XtDestroyWidget( Root_dialog );
    Root_dialog = ( Widget ) NULL;
    Reset_to_initial_state();
  }

  pam_end( pamh, ret );
}

/**************************************************************************
 UPDATE_SCROLL_DISPLAY: Update scroll area with output from coprocess.
**************************************************************************/

static void Update_scroll_display( char *msg_buf, int msg_code )
{ 
  Widget label = ( Widget ) NULL;
  Widget rowcol = ( Widget ) NULL;
  int loop = -1;
  
  /* Modify appropriate msg in array. */

  sprintf( Output_msgs[Current_msg_pos], "%s", msg_buf );
  Output_msgs_code[Current_msg_pos] = msg_code; 

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

  /* The latest message is located at position current_msg_pos.
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
     work down to the index just above the current_msg_pos index.
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

/**************************************************************************
 UPDATE_LABEL: Update label with appropriate value and color.
 ************************************************************************/

static void Update_label( Widget w, int index )
{
  XmString msg;

  /* Convert c-buf to XmString variable. */

  msg = XmStringCreateLtoR( Output_msgs[index], XmFONTLIST_DEFAULT_TAG );

  /* Modify appropriate output label. */

  if( Output_msgs_code[index] == HCI_CP_STDERR )
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
 USER_ADAPT_PROMPT_POPUP: Customized prompt.
**************************************************************************/

static void User_adapt_prompt_popup( char *prompt_msg, int num_buttons )
{
  Widget dialog;
  Widget cancel_button;
  XmString cancel_string;
  Widget temp_button;
  Widget temp_vert_rowcol;
  Widget temp_horz_rowcol;
  Widget temp_label;
  char temp_button_label[5] = "";
  int loop = 0;

  void user_select_callback( Widget, XtPointer, XtPointer );

  /* Create Prompt dialog. */

  dialog = XmCreatePromptDialog( Top_widget, "prompt", NULL, 0 );

  /* Get rid of unused widgets. */

  XtUnmanageChild( XtNameToWidget( dialog, "OK" ) );
  XtUnmanageChild( XtNameToWidget( dialog, "Help" ) );
  XtUnmanageChild( XtNameToWidget( dialog, "Text" ) );
  XtUnmanageChild( XtNameToWidget( dialog, "Selection" ) );

  /* Set properties of Cancel button. */

  cancel_string = XmStringCreateLocalized( "Cancel Merge" );
  cancel_button = XmSelectionBoxGetChild( dialog, XmDIALOG_CANCEL_BUTTON );

  XtVaSetValues( cancel_button,
          XmNforeground,  hci_get_read_color( BUTTON_FOREGROUND ),
          XmNbackground,  hci_get_read_color( BUTTON_BACKGROUND ),
          NULL );

  XtAddCallback( dialog, XmNcancelCallback, Cancel_merge_callback, NULL );

  /* Create customized prompt. */

  temp_vert_rowcol = XtVaCreateManagedWidget( "vert_rowcol",
       xmRowColumnWidgetClass,  dialog,
       XmNbackground,           hci_get_read_color( WARNING_COLOR ),
       XmNforeground,           hci_get_read_color( TEXT_FOREGROUND ),
       XmNorientation,          XmVERTICAL,
       XmNpacking,              XmPACK_TIGHT,
       NULL );

  temp_label = XtVaCreateManagedWidget( prompt_msg,
          xmLabelWidgetClass,   temp_vert_rowcol,
          XmNforeground,        hci_get_read_color( TEXT_FOREGROUND ),
          XmNbackground,        hci_get_read_color( WARNING_COLOR ),
          XmNfontList,          hci_get_fontlist( LIST ),
          NULL );

  temp_horz_rowcol = XtVaCreateManagedWidget( "horz_rowcol",
       xmRowColumnWidgetClass,  temp_vert_rowcol,
       XmNbackground,           hci_get_read_color( WARNING_COLOR ),
       XmNforeground,           hci_get_read_color( TEXT_FOREGROUND ),
       XmNorientation,          XmHORIZONTAL,
       XmNpacking,              XmPACK_TIGHT,
       NULL );

  for( loop = 1; loop < num_buttons+1; loop++ )
  {
    sprintf( temp_button_label, " %d ", loop );
    temp_button = XtVaCreateManagedWidget( temp_button_label,
          xmPushButtonWidgetClass, temp_horz_rowcol,
          XmNforeground,  hci_get_read_color( BUTTON_FOREGROUND ),
          XmNbackground,  hci_get_read_color( BUTTON_BACKGROUND ),
          NULL );
    XtAddCallback( temp_button, XmNactivateCallback,
                   user_select_callback, (XtPointer) loop );
  }

  /* Set properties of popup. */

  XtVaSetValues (dialog,
          XmNcancelLabelString,   cancel_string,
          XmNbackground,          hci_get_read_color( WARNING_COLOR ),
          XmNforeground,          hci_get_read_color( TEXT_FOREGROUND ),
          XmNdialogStyle,         XmDIALOG_PRIMARY_APPLICATION_MODAL,
          XmNdeleteResponse,      XmDESTROY,
          NULL);

  /* Free allocated space. */

  XmStringFree( cancel_string );

  /* Have to do this for popup to appear. */

  XtManageChild( dialog );
  XtPopup( Top_widget, XtGrabNone );
}

/**************************************************************************
 USER_SELECT_CALLBACK: Callback for custom prompt.
**************************************************************************/

void user_select_callback( Widget w, XtPointer x, XtPointer y )
{
  char temp_buf[5] = "";
  sprintf( temp_buf, "%d\n", (int) x );
  if( Cp != NULL ){ MISC_cp_write_to_cp( Cp, temp_buf ); }
  else
  {
    strcpy( Popup_buf, "Coprocess terminated. Unable to continue." );
    hci_error_popup( Top_widget, Popup_buf, NULL );
  }
  /* To destroy dialog (without making it static/global), destroy
     the appropriat Widget. The Widget w is the push button, its
     parent is the horizontal rowcol, its parent is the vertical
     rowcol, and its parent is the dialog Widget. */
  XtDestroyWidget( XtParent( XtParent( XtParent( w ) ) ) );
}

/**************************************************************************
 OK_POPUP_OK_CALLBACK: Callback for "OK" button.
**************************************************************************/

static void Ok_popup_ok_callback( Widget w, XtPointer x, XtPointer y )
{
  if( Coprocess_flag == HCI_CP_STARTED )
  {
    HCI_LE_log( "Sending newline to coprocess" );
    if( Cp != NULL ){ MISC_cp_write_to_cp( Cp, "\n" ); }
    else
    {
      strcpy( Popup_buf, "Coprocess terminated. Unable to confirm." );
      hci_error_popup( Top_widget, Popup_buf, NULL );
    }
  }
  else
  {
    HCI_LE_log( "Coprocess not running. Not sending newline to coprocess" );
  }
}

/**************************************************************************
 CANCEL_MERGE_CALLBACK: Callback for "Cancel Merge" button.
**************************************************************************/

void Cancel_merge_callback( Widget w, XtPointer x, XtPointer y )
{
  HCI_LE_log( "Cancelling merge by closing coprocess" );
  Close_coprocess();
}

/**************************************************************************
 RESET_TO_INITIAL_STATE: Reset GUI/variables when action has finished.
**************************************************************************/

static void Reset_to_initial_state()
{
  HCI_LE_log( "Reset GUI to initial state" );
  Action_flag = NO_ACTION;
  Initial_prompt_for_CD = NO;
  User_inserted_CD = NO;
  Found_cd_type = NO;
  Cd_type = UNKNOWN_CD;
  Coprocess_has_stdout = NULL;
  Num_output_msgs = 0;
  Current_msg_pos = 0;
  HCI_default_cursor();
  XtVaSetValues( Build_button, XmNsensitive, True, NULL );
  XtVaSetValues( Update_button, XmNsensitive, True, NULL );
}

/**************************************************************************
 TRIM_COPROCESS_BUF: Remove trailing newline from coprocess output.
**************************************************************************/

static void Trim_coprocess_buf( char *coprocess_buf )
{
  /* Strip trailing newline. */
  if( coprocess_buf[strlen( coprocess_buf )-1] == '\n' )
  {
    coprocess_buf[strlen( coprocess_buf )-1] = '\0';
  }
}

/**************************************************************************
 CLOSE_COPROCESS: Close coprocess.
**************************************************************************/

static void Close_coprocess()
{
  HCI_LE_log( "Close coprocess" );

  MISC_cp_close( Cp );
  Cp = NULL;
  Coprocess_flag = HCI_CP_FINISHED;
}

/**************************************************************************
 INITIAL_CD_OK_CALLBACK: Callback for "OK" button.
**************************************************************************/

void Initial_CD_ok_callback( Widget w, XtPointer y, XtPointer x )
{
  HCI_LE_log( "Inital CD OK callback" );
  User_inserted_CD = YES;
}

/**************************************************************************
 INITIAL_CD_CANCEL_CALLBACK: Callback for "Cancel" button.
**************************************************************************/

void Initial_CD_cancel_callback( Widget w, XtPointer x, XtPointer y )
{
  HCI_LE_log( "Inital CD cancel callback...reset display" );
  Reset_to_initial_state();
}
