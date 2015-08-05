/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/08/08 17:20:07 $
 * $Id: hci_console_message_callback.c,v 1.78 2013/08/08 17:20:07 steves Exp $
 * $Revision: 1.78 $
 * $State: Exp $
 */

/************************************************************************
     Module: hci_console_message_callback.c
     Description: This module handles all functions pertaining to
                  receiving and sending console messages. It can be
                  invoked directly from the RPG Control/Status window
                  or automatically by the RPG Control/Status task when
                  new messages are received.
 ************************************************************************/
 
/* Local include files */

#include <hci_control_panel.h>

/* Definitions */

#define	MTYP_RDACON		10 /* Msg type for send RDA command */
#define	RMS_MAX_SIZE		400 /* Max size for RMS msg */
#define	RDA_MAX_SIZE		404 /* Max size for RMS msg */
#define	PUP_MAX_SIZE		48*80 /* Max size for RMS msg */
#define	MAX_NARROWBAND_LINES	48 /* Max # narrowband lines allowed */
#define	RDA_MSG			MAX_NARROWBAND_LINES
#define	RMS_MSG			(RDA_MSG+1)
#define	CLASS_MSG		(RMS_MSG+1)
#define	NUM_NB_BUTTONS		MAX_NARROWBAND_LINES*2
#define	RDA_MSG_MASK		1 /* Mask for RDA msg bit */
#define	RMS_MSG_MASK		2 /* Mask for RMS msg bit */
#define	HCI_RDA_MSG		1 /* Flag for RDA console msg */
#define	HCI_PUP_MSG		2 /* Flag for PUP text msg */
#define	MAX_NB_PARAMS		10
#define	MAX_CONSOLE_MSG_SIZE	48*80+1 /* Maximum size of a console msg */

/* Static/Global variables */

static Widget Console_dialog = (Widget) NULL;
static Widget Incoming_messages = (Widget) NULL;
static Widget Outgoing_messages = (Widget) NULL;
static Widget Button[NUM_NB_BUTTONS] = { (Widget) NULL };
static char Cmd[128];
static char Message[MAX_CONSOLE_MSG_SIZE];
static Pd_line_entry *Lines = NULL;
static int N_lines = 0;
static int Rda_message_flag = HCI_NO_FLAG;
static short Parameters[MAX_NB_PARAMS] = {0}; /* Msg destination bits */
static Pixel Base_bg = (Pixel) NULL;
static Pixel Base2_bg = (Pixel) NULL;
static Pixel Text_fg = (Pixel) NULL;
static Pixel Edit_fg = (Pixel) NULL;
static Pixel Edit_bg = (Pixel) NULL;
static Pixel Button_fg = (Pixel) NULL;
static Pixel Button_bg = (Pixel) NULL;
static Pixel Black_color = (Pixel) NULL;
static Pixel White_color = (Pixel) NULL;
static XmFontList List_font = (XmFontList) NULL;
static XmString Str_ptrs[RDA_MAX_SIZE];

/* Function prototypes */

static void Console_msg_close_callback( Widget, XtPointer, XtPointer );
static void Console_msg_send_callback( Widget, XtPointer, XtPointer );
static void Console_msg_clear_callback(Widget, XtPointer, XtPointer );
static void Console_msg_filter_callback( Widget, XtPointer, XtPointer );
static void Hci_initialize_console_msg( Widget );
static int Hci_read_nb_line_info();
static void Reset_message_area();
static void Reset_parameters_array();
       int hci_free_txt_msg( char *, short * );
       void byte_limit_callback( Widget text, XtPointer client_data, 
                                 XtPointer call_data );

/************************************************************************
     Description: This is the callback for the Console messages
                  pushbutton in the RPG Control/Status window. It
                  activates the Console Messages window.  It is also
                  invoked indirectly when a new RDA console text
                  message is detected.
 ************************************************************************/

void hci_console_message_callback( Widget w, XtPointer y, XtPointer z )
{
  Widget form = (Widget) NULL;
  Widget rowcol = (Widget) NULL;
  Widget button = (Widget) NULL;
  Widget separator = (Widget) NULL;
  Widget label = (Widget) NULL;
  Widget other_frame = (Widget) NULL;
  Widget other_rowcol = (Widget) NULL;
  Widget filter_frame = (Widget) NULL;
  Widget filter_rowcol = (Widget) NULL;
  Widget outgoing_rowcol = (Widget) NULL;
  Widget class_frame = (Widget) NULL;
  Widget class_rowcol = (Widget) NULL;
  static RPG_up_rec_t *class_rec = NULL;
  static RPG_up_rec_t *line_user_rec = NULL;
  static int n_line_users = 0;
  static int n_classes = 0;
  int i = 0;
  int j = 0;
  int l = 0;
  char buf[128];
  char tbuf[128];
  int cnt = 0;
  int num_classes = 0;
  int class_list[100];
  int found = HCI_NO_FLAG;
  int n = 0;
  int lind = 0;
  int class_n = 0;
  int cn = 0;
  RPG_up_rec_t *rec = NULL;
  Arg args[16];

  HCI_LE_log( "Console message function selected" );

  /* If the window is already defined, pop it up. */

  if( Console_dialog != NULL )
  {
    HCI_Shell_popup( Console_dialog );
    return;
  }

  /* Unset all destinations. */

  Reset_parameters_array();

  /* Get line configuration information (from comms_link.conf) so
     we can determine which lines are dedicated and should be in the
     Destination menu. */

  HCI_PM( "Reading Line Information" );

  /* Read the narrowband line information.  If there is an error
     then do nothing and return. */

  if( Hci_read_nb_line_info() < 0 ){ return; }

  HCI_Shell_init( &Console_dialog, "Console Messages" );

  /* Define colors and font */

  Base_bg = hci_get_read_color( BACKGROUND_COLOR1 );
  Base2_bg = hci_get_read_color( BACKGROUND_COLOR2 );
  Text_fg = hci_get_read_color( TEXT_FOREGROUND );
  Edit_fg = hci_get_read_color( EDIT_FOREGROUND );
  Edit_bg = hci_get_read_color( EDIT_BACKGROUND );
  Button_fg = hci_get_read_color( BUTTON_FOREGROUND );
  Button_bg = hci_get_read_color( BUTTON_BACKGROUND );
  Black_color = hci_get_read_color( BLACK );
  White_color = hci_get_read_color( WHITE );
  List_font = hci_get_fontlist( LIST );

  /* Set permission of RMS text msg LB. */

  ORPGDA_write_permission( ORPGDAT_RMS_TEXT_MSG );

  /* Create the widgets for the Console Messages window.  The window
     consists of 3 main regions: Destination selections, incoming
     message region, outgoing message region. */

  /* Use a form as the manager of the window. */

  form = XtVaCreateWidget( "console_msg_form",
                xmFormWidgetClass, Console_dialog,
                NULL );

  /* Put standard control buttons in a row at the top of the window. */

  rowcol = XtVaCreateWidget( "console_msg_rowcol",
                xmRowColumnWidgetClass, form,
                XmNpacking, XmPACK_COLUMN,
                XmNorientation, XmHORIZONTAL,
                XmNcolumns, 1,
                XmNforeground, Black_color,
                XmNbackground, Base_bg,
                XmNtopAttachment, XmATTACH_FORM,
                XmNleftAttachment, XmATTACH_FORM,
                XmNrightAttachment, XmATTACH_FORM,
                XmNalignment, XmALIGNMENT_CENTER,
                NULL );

  /* Define window Close button */

  button = XtVaCreateManagedWidget( "Close",
                xmPushButtonWidgetClass, rowcol,
                XmNbackground, Button_bg,
                XmNforeground, Button_fg,
                XmNfontList, List_font,
                NULL );

  XtAddCallback( button, XmNactivateCallback, Console_msg_close_callback, NULL );

  /* Define console message Send button */

  button = XtVaCreateManagedWidget( "Send",
                xmPushButtonWidgetClass, rowcol,
                XmNbackground, Button_bg,
                XmNforeground, Button_fg,
                XmNfontList, List_font,
                NULL );

  XtAddCallback( button, XmNactivateCallback, Console_msg_send_callback, NULL );

  XtManageChild( rowcol );

  /* Define the Destination section of the window.  This section
     contains buttons for all defined external users.  This
     information is defined in "user_profiles" with the exception
     of the RDA SOT. */

  filter_frame = XtVaCreateManagedWidget( "filter_frame",
                xmFrameWidgetClass, form,
                XmNbackground, Base_bg,
                XmNtopWidget, rowcol,
                XmNtopAttachment, XmATTACH_WIDGET,
                XmNleftAttachment, XmATTACH_FORM,
                XmNrightAttachment, XmATTACH_FORM,
                NULL );

  label = XtVaCreateManagedWidget( "Destinations",
                xmLabelWidgetClass, filter_frame,
                XmNchildType, XmFRAME_TITLE_CHILD,
                XmNchildHorizontalAlignment, XmALIGNMENT_CENTER,
                XmNchildVerticalAlignment, XmALIGNMENT_CENTER,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                NULL );

  filter_rowcol = XtVaCreateWidget( "filter_rowcol",
                xmRowColumnWidgetClass, filter_frame,
                XmNbackground, Base_bg,
                XmNorientation, XmHORIZONTAL,
                XmNpacking, XmPACK_TIGHT,
                XmNnumColumns, 1,
                XmNentryAlignment, XmALIGNMENT_CENTER,
                NULL );

  /* The "other" group contains RDA, RMS items. */

  other_frame = XtVaCreateManagedWidget( "other_frame",
                xmFrameWidgetClass, filter_rowcol,
                XmNbackground, Base_bg,
                NULL );

  label = XtVaCreateManagedWidget( "Other",
                xmLabelWidgetClass, other_frame,
                XmNchildType, XmFRAME_TITLE_CHILD,
                XmNchildHorizontalAlignment, XmALIGNMENT_CENTER,
                XmNchildVerticalAlignment, XmALIGNMENT_CENTER,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                NULL );

  other_rowcol = XtVaCreateWidget( "other_rowcol",
                xmRowColumnWidgetClass, other_frame,
                XmNbackground, Base_bg,
                XmNorientation, XmVERTICAL,
                XmNpacking, XmPACK_COLUMN,
                XmNnumColumns, 1,
                XmNentryAlignment, XmALIGNMENT_CENTER,
                NULL );

  Button[CLASS_MSG] = XtVaCreateManagedWidget( "All   ",
                xmToggleButtonWidgetClass, other_rowcol,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNselectColor, White_color,
                XmNfontList, List_font,
                XmNset, False,
                XmNuserData, (XtPointer) 0,
                NULL );

  XtAddCallback( Button[CLASS_MSG],
                XmNvalueChangedCallback, Console_msg_filter_callback,
                (XtPointer) CLASS_MSG );

  Button[RDA_MSG] = XtVaCreateManagedWidget( "RDA   ",
                xmToggleButtonWidgetClass, other_rowcol,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNselectColor, White_color,
                XmNfontList, List_font,
                XmNset, False,
                XmNuserData, (XtPointer) 0,
                NULL );

  XtAddCallback( Button[RDA_MSG],
                XmNvalueChangedCallback, Console_msg_filter_callback,
                (XtPointer) RDA_MSG );

  /* If the site is configured for RMS, create an RMS button if the
     interface is up. */

  if( HCI_has_rms() )
  {
    if( hci_get_rms_down_flag() )
    {
      if( Button[RMS_MSG] != (Widget) NULL )
      {
        Button[RMS_MSG] = (Widget) NULL;
      }
    }
    else
    {
      Button[RMS_MSG] = XtVaCreateManagedWidget( "RMS   ",
                xmToggleButtonWidgetClass, other_rowcol,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNselectColor, White_color,
                XmNfontList, List_font,
                XmNset, False,
                XmNuserData, (XtPointer) 0,
                NULL );

      XtAddCallback( Button[RMS_MSG],
                XmNvalueChangedCallback, Console_msg_filter_callback,
                (XtPointer) RMS_MSG );
    }
  }

  /* Now find any other dedicated users who are not associated
     with a class and add them to the "Other" list.  We need to
     start with reading all lines defined in comms_link.conf and
     then finding a corresponding Line_user match in user_profiles. */

  /* free previous query records */
  for( i = 0; i < n_line_users; i++ ){ free( line_user_rec[i].user_name ); }

  line_user_rec = (RPG_up_rec_t *) STR_reset( (char *) line_user_rec, 0 );
  n_line_users = 0;

  sprintf( tbuf, "up_type = %d", UP_LINE_USER ); /* search for all line users */
  n_line_users = hci_search_users( tbuf );
  for( i = 0; i < n_line_users; i++ )
  {
    /* save records in an array */
    RPG_up_rec_t *rec, r;
    rec = (RPG_up_rec_t *) hci_get_next_user_rec();
    memcpy( &r, rec, sizeof( RPG_up_rec_t ) );
    r.user_name = MISC_malloc( strlen( rec->user_name ) + 1 );
    strcpy( r.user_name, rec->user_name );
    line_user_rec = (RPG_up_rec_t *) STR_append( (char *) line_user_rec, 
                (char *) &r, sizeof( RPG_up_rec_t ) );
  }

  for( i = 0; i < N_lines; i++ )
  {
    /* If the line is for the wideband, ignore it. */

    if( Lines[i].line_ind == ( ORPGCMI_rda_response() - ORPGDAT_CM_RESPONSE ) )
    {
      continue;
    }

    /* Go through all user_profile entries and match the line
       index to a Line_user record.  If a match is found, check
       the line to make sure it is dedicated.  Messages are not
       sent to dial-in lines. */

    for( j = 0; j < n_line_users; j++ )
    {
      lind = Lines[i].line_ind;
      rec = (RPG_up_rec_t *) line_user_rec + j;
      if( rec->line_ind == lind &&
          rec->class_num == 0 &&
          Lines[lind].line_type == DEDICATED )
      {
        /* A match is found and no class is defined so
           create a button for this line. */

        sprintf( buf,"%s(%d)", rec->user_name, (int) Lines[lind].line_ind );

        Button[ (int) Lines[lind].line_ind] = XtVaCreateManagedWidget( buf,
                xmToggleButtonWidgetClass, other_rowcol,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNselectColor, White_color,
                XmNfontList, List_font,
                XmNset, False,
                XmNuserData, (XtPointer) 0,
                NULL );

        XtAddCallback( Button[ (int) Lines[lind].line_ind],
                XmNvalueChangedCallback, Console_msg_filter_callback,
                (XtPointer) ( (int) Lines[lind].line_ind ) );
      }
    }
  }

  XtManageChild( other_rowcol );

  /* Define selections for each defined Class.  Only one list is
     allowed per class. */

  num_classes = 0;

  /* free previous query records */
  for( i = 0; i < n_classes; i++ )
  {
    free( class_rec[i].user_name );
  }
  class_rec = (RPG_up_rec_t *) STR_reset( (char *) class_rec, 0 );
  n_classes = 0;

  sprintf( tbuf, "up_type = %d and class_num <> 98 and class_num <> 99", UP_CLASS );
  n_classes = hci_search_users( tbuf );

  for( i = 0; i < n_classes; i++ )
  {
    /* save all records in an array */
    RPG_up_rec_t *rec, r;
    rec = (RPG_up_rec_t *) hci_get_next_user_rec();
    memcpy( &r, rec, sizeof( RPG_up_rec_t ) );
    r.user_name = MISC_malloc( strlen( rec->user_name ) + 1 );
    strcpy( r.user_name, rec->user_name );
    class_rec = (RPG_up_rec_t *) STR_append( (char *) class_rec, (char * ) &r, sizeof( RPG_up_rec_t ) );
  }

  for( i = 0; i < n_classes; i++ )
  {
    RPG_up_rec_t *rec = class_rec + i;
    class_n = rec->class_num;

    /* First check to see if the class is already defined.
       If it is, ignore it. */

    found = HCI_NO_FLAG;

    for( j = 0; j < num_classes; j++ )
    {
      if( class_list[j] == class_n )
      {
        found = HCI_YES_FLAG;
        break;
      }
    }

    if( found == HCI_YES_FLAG ){ continue; }

    class_list[num_classes] = class_n;
    num_classes++;

    cnt = 0;

    class_frame = XtVaCreateManagedWidget( "class_frame",
                xmFrameWidgetClass, filter_rowcol,
                XmNbackground, Base_bg,
                NULL );

    sprintf( buf, "Class %d ", class_n );

  /* First find out how many items there are so we can
     determine the number of columns to define for the
     group rowcol. */

  for( l = 0; l < N_lines; l++ )
  {
    if( Lines[l].line_ind == ( ORPGCMI_rda_response() - ORPGDAT_CM_RESPONSE ) )
    {
      continue;
    }

    if( Lines[l].line_type == DEDICATED )
    {
      for( j = 0; j < n_line_users; j++ )
      {
        RPG_up_rec_t *r;
        r = line_user_rec + j;
        if( r->line_ind != Lines[l].line_ind ){ continue; }
        if( ( class_n == r->class_num ) ||
            ( ( class_n == 1 ) && ( r->class_num == 98 ) ) ||
            ( ( class_n == 1 ) && ( r->class_num == 99 ) ) )
        {
          cnt++;
        }
      }
    }
  }

  if( cnt == 0 )
  {
    /* If there are no users defined for this class then
       destroy the frame created for it. */
    XtDestroyWidget( class_frame );
    continue;
  }

  label = XtVaCreateManagedWidget( buf,
                xmLabelWidgetClass, class_frame,
                XmNchildType, XmFRAME_TITLE_CHILD,
                XmNchildHorizontalAlignment, XmALIGNMENT_CENTER,
                XmNchildVerticalAlignment, XmALIGNMENT_CENTER,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                NULL );

  class_rowcol = XtVaCreateWidget( "class_rowcol",
                xmRowColumnWidgetClass, class_frame,
                XmNbackground, Base_bg,
                XmNorientation, XmVERTICAL,
                XmNpacking, XmPACK_COLUMN,
                XmNnumColumns, (int) ( ( cnt + 7 ) / 8 ),
                XmNentryAlignment, XmALIGNMENT_CENTER,
                NULL );

  Button[CLASS_MSG+class_n] = XtVaCreateManagedWidget( "All",
                xmToggleButtonWidgetClass, class_rowcol,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNselectColor, White_color,
                XmNfontList, List_font,
                XmNset, False,
                XmNuserData, (XtPointer) class_n,
                NULL );

  XtAddCallback( Button[CLASS_MSG+class_n],
                XmNvalueChangedCallback, Console_msg_filter_callback,
                (XtPointer) ( CLASS_MSG + class_n ) );

  /* For each dedicated line defined in comms_link.conf, find
     a matching Line_user record in user profiles and create
     a button for it if it matches the group class. */

  for( l = 0; l < N_lines; l++ )
  {
    if( Lines[l].line_ind == ( ORPGCMI_rda_response() - ORPGDAT_CM_RESPONSE ) )
    {
      continue;
    }

    if( Lines[l].line_type == DEDICATED )
    {
      for( j = 0; j < n_line_users; j++ )
      {
        RPG_up_rec_t *r;
        r = line_user_rec + j;
        if( r->line_ind != Lines[l].line_ind ){ continue; }
        if( ( class_n == r->class_num ) ||
            ( ( class_n == 1 ) && ( r->class_num == 98 ) ) ||
            ( ( class_n == 1 ) && ( r->class_num == 99 ) ) )
        {
          if( strlen( r->user_name ) )
          {
            sprintf( buf, "%s(%d)", r->user_name, (int) Lines[l].line_ind );
          }
          else
          {
            sprintf( buf, "Line (%d)", (int) Lines[l].line_ind );
          }

          cn = r->class_num;
          Button[ (int) Lines[r->line_ind].line_ind] = XtVaCreateManagedWidget( buf,
                xmToggleButtonWidgetClass, class_rowcol,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNselectColor, White_color,
                XmNfontList, List_font,
                XmNuserData, (XtPointer) cn,
                XmNset, False,
                NULL );

          XtAddCallback( Button[ (int) Lines[r->line_ind].line_ind],
                XmNvalueChangedCallback, Console_msg_filter_callback,
                (XtPointer) ( (int) Lines[l].line_ind ) );
        }
      }
    }
  }

  /* Manage the container. */

  XtManageChild( class_rowcol );
  }

  XtManageChild( filter_rowcol );

  /* Define the section for displaying incoming messages. */

  label = XtVaCreateManagedWidget( "Incoming Messages:",
                xmLabelWidgetClass, form,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                XmNtopAttachment, XmATTACH_WIDGET,
                XmNtopWidget, filter_frame,
                XmNleftAttachment, XmATTACH_FORM,
                XmNrightAttachment, XmATTACH_FORM,
                NULL );

  n = 0;
  XtSetArg( args[n], XmNvisibleItemCount, 10 );
  n++;
  XtSetArg( args[n], XmNforeground, Text_fg );
  n++;
  XtSetArg( args[n], XmNbackground, Base2_bg );
  n++;
  XtSetArg( args[n], XmNtopAttachment, XmATTACH_WIDGET );
  n++;
  XtSetArg( args[n], XmNfontList, List_font );
  n++;
  XtSetArg( args[n], XmNtopWidget, label );
  n++;
  XtSetArg( args[n], XmNlistSizePolicy, XmCONSTANT );
  n++;
  XtSetArg( args[n], XmNscrollingPolicy, XmAUTOMATIC );
  n++;
  XtSetArg( args[n], XmNscrollBarDisplayPolicy, XmAS_NEEDED );
  n++;
  XtSetArg( args[n], XmNleftAttachment, XmATTACH_FORM );
  n++;
  XtSetArg( args[n], XmNrightAttachment, XmATTACH_FORM );
  n++;

  Incoming_messages = XmCreateScrolledList( form, "incoming_msgs", args, n );

  XtManageChild( Incoming_messages );

  /* Fill in the incoming messages section reading all messages that
     are currently defined in the message LBs. */

  Hci_initialize_console_msg( Incoming_messages );

  /* Define the section for displaying outgoing messages. */

  separator = XtVaCreateManagedWidget( "separator",
                xmSeparatorWidgetClass, form,
                XmNforeground, Black_color,
                XmNbackground, Base_bg,
                XmNtopAttachment, XmATTACH_WIDGET,
                XmNtopWidget, Incoming_messages,
                XmNleftAttachment, XmATTACH_FORM,
                XmNrightAttachment, XmATTACH_FORM,
                NULL );

  outgoing_rowcol = XtVaCreateWidget( "outgoing_rowcol",
                xmRowColumnWidgetClass, form,
                XmNbackground, Base_bg,
                XmNorientation, XmHORIZONTAL,
                XmNpacking, XmPACK_TIGHT,
                XmNnumColumns, 1,
                XmNentryAlignment, XmALIGNMENT_CENTER,
                XmNtopAttachment, XmATTACH_WIDGET,
                XmNtopWidget, separator,
                XmNleftAttachment, XmATTACH_FORM,
                XmNrightAttachment, XmATTACH_FORM,
                NULL );

  button = XtVaCreateManagedWidget( "Clear",
                xmPushButtonWidgetClass, outgoing_rowcol,
                XmNbackground, Button_bg,
                XmNforeground, Button_fg,
                XmNfontList, List_font,
                NULL );

  XtAddCallback( button, XmNactivateCallback, Console_msg_clear_callback, NULL );

  label = XtVaCreateManagedWidget( "                         Outgoing Messages:",
                xmLabelWidgetClass, outgoing_rowcol,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                NULL );

  XtManageChild( outgoing_rowcol );

  n = 0;
  XtSetArg( args[n], XmNrows, 10 );
  n++;
  XtSetArg( args[n], XmNcolumns, 80 );
  n++;
  XtSetArg( args[n], XmNforeground, Edit_fg );
  n++;
  XtSetArg( args[n], XmNbackground, Edit_bg );
  n++;
  XtSetArg( args[n], XmNeditMode, XmMULTI_LINE_EDIT );
  n++;
  XtSetArg( args[n], XmNtopAttachment, XmATTACH_WIDGET );
  n++;
  XtSetArg( args[n], XmNtopWidget, outgoing_rowcol );
  n++;
  XtSetArg( args[n], XmNfontList, List_font );
  n++;
  XtSetArg( args[n], XmNwordWrap, True );
  n++;
  XtSetArg( args[n], XmNscrollHorizontal, False );
  n++;
  XtSetArg( args[n], XmNleftAttachment, XmATTACH_FORM );
  n++;
  XtSetArg( args[n], XmNrightAttachment, XmATTACH_FORM );
  n++;
  XtSetArg( args[n], XmNbottomAttachment, XmATTACH_FORM );
  n++;

  Outgoing_messages = XmCreateScrolledText( form, "outgoing_msgs", args, n );

  XtManageChild( Outgoing_messages );

  XtAddCallback( Outgoing_messages, XmNmodifyVerifyCallback, byte_limit_callback, NULL );

  XtManageChild( form );


  HCI_Shell_start( Console_dialog, RESIZE_HCI );
}

/************************************************************************
     Description: Check the outgoing message size for being too long.
 ************************************************************************/
void byte_limit_callback( Widget text_w, XtPointer client_data, XtPointer call_data )
{
  XmTextVerifyCallbackStruct *cbs = (XmTextVerifyCallbackStruct *) call_data;
  int len = XmTextGetLastPosition( text_w );
  char buf[256];

  /* If user typed a backspace or user pressed delete, just return. */
  if( (cbs->startPos < cbs->currInsert)
                     ||
      (cbs->text->length == 0) )
  {
     return;
  }

  /* Don't allow paste if the paste causes more than the 
     limit number of characters. */
  if( (cbs->text->length > 1)
               &&
      ((len + cbs->text->length) > RDA_MAX_SIZE) )
  {
    cbs->doit = False;
    return;
  }

  /* Don't allow the user to type a message longer than allowed .. */
  if( len >= RDA_MAX_SIZE )
  {
    cbs->doit = False;
    sprintf( buf, "The message length character limit reached." );
    hci_warning_popup( Console_dialog, buf, NULL );

    return;
  }
}

/************************************************************************
     Description: Clear outgoing message area.
 ************************************************************************/

static void Reset_message_area()
{
  XmTextSetString (Outgoing_messages,"");
}

/************************************************************************
     Description: Reset parameters array to all zeros.
 ************************************************************************/

static void Reset_parameters_array()
{
  int i = 0;
  for( i = 0; i < MAX_NB_PARAMS; i++ ){ Parameters[i] = 0; }
}

/************************************************************************
     Description: This is the callback invoked when the Console
                  Messages "Close" button is selected.
 ************************************************************************/

static void Console_msg_close_callback( Widget w, XtPointer y, XtPointer z )
{
  Reset_message_area();
  HCI_Shell_popdown( Console_dialog );
}

/************************************************************************
     Description: This is the callback invoked when the Console
                  Messages "Clear" button is selected.  Text is
                  cleared from the outgoing message region.
 ************************************************************************/

static void Console_msg_clear_callback( Widget w, XtPointer y, XtPointer z )
{
  Reset_message_area();
}

/************************************************************************
     Description: This is the callback invoked when the Console
                  Messages "Send" button is selected.
 ************************************************************************/

static void Console_msg_send_callback( Widget w, XtPointer y, XtPointer z )
{
  char *string = NULL;
  int status = 0;
  int msg_len = 0;
  int i = 0;
  int count = 0;
  char *tbuf = NULL;
  char buf[256];

  /* If no destinations were selected, popup an informative message
     to the operator so they can select one or more. */

  if( (Parameters[0] == 0)
                &&
      (Parameters[1] == 0)
                &&
      (Parameters[2] == 0)
                &&
      (Parameters[3] == 0) )
  {
    hci_warning_popup( Console_dialog,
               "A destination(s) must be selected before\nthe message can be sent", NULL );
    return;
  }

  /* If low bandwidth, display the progress meter since we are going
     to do I/O. */

  HCI_PM( "Sending Console Message" );

  /* Get the outgoing message from the text widgets compound string. */

  string = XmTextGetString( Outgoing_messages );

  /* If any of the narrowband users are selected, send the message
     to them. */

  if( Parameters[0] || Parameters[1] || Parameters[2] )
  {
    /* If the message size is acceptable for PUP then send it.
       Otherwise, popup an informative message and shorten it. */

    msg_len = strlen( string );
    tbuf = malloc( msg_len + 1 );
    strncpy( tbuf, string, msg_len );

    /* First, strip off all special characters (i.e., carraige
       returns and line feeds) and replace them with spaces since
       the free text formatter will break the message up into
       80 character lines. */

    count = 0;

    for( i = 0; i < msg_len; i++ )
    {
      if( iscntrl( (int) * ( tbuf + i ) ) )
      {
        count++;
        *(tbuf+i) = ' ';
      }
    }

    /* If the message length is too large, truncate it and display
       an information popup to the operator. */

    if( msg_len > PUP_MAX_SIZE )
    {
      sprintf( buf, "The message to the PUP was truncated\nto %d characters. %d characters were\ndiscarded.", 
               PUP_MAX_SIZE, msg_len - PUP_MAX_SIZE );
      hci_warning_popup( Console_dialog, buf, NULL );
      msg_len = PUP_MAX_SIZE;
    }

    /* Clear the message buffer first so any old message data is removed */

    memset( Message, 0, MAX_CONSOLE_MSG_SIZE );
    strncpy( Message, tbuf, msg_len );
    free( tbuf );

    /* Build free text message and set product parameters
       to narrowband bits. */

    if( ( status = hci_free_txt_msg( Message, Parameters ) ) != 0 )
    {
      HCI_LE_error( "hci_free_txt_msg(): ret = %d", status );
    }
    else
    {
      /* Post an event so p_server will be notified of the new
         message to be sent. */
      EN_post( ORPGEVT_FREE_TXT_MSG, NULL, 0, 0 );
      HCI_LE_log( "Message sent to narrowband users" );
    }
  }

  /* If the RDA destination is selected, send the message to the RDA */

  if( Parameters[3] & RDA_MSG_MASK )
  {
    /* If the message size is acceptable for RDA then send it.
       Otherwise, popup an informative message and shorten it. */
    msg_len = strlen( string );

    if( msg_len > RDA_MAX_SIZE )
    {
      sprintf( buf, "The message to the RDA was truncated\nto %d characters. %d characters were\ndiscarded.", 
               RDA_MAX_SIZE, msg_len - RDA_MAX_SIZE );
      hci_warning_popup( Console_dialog, buf, NULL );
      msg_len = RDA_MAX_SIZE;
    }

    /* Clear the message buffer first so any old message data is removed */

    memset( Message, 0, MAX_CONSOLE_MSG_SIZE );
    strncpy( Message, string, msg_len );
    HCI_LE_log( "Message sent to RDA" );
    ORPGRDA_send_cmd( COM4_WBMSG, HCI_INITIATED_RDA_CTRL_CMD, MTYP_RDACON,
                msg_len, 0, 0, 0, Message, msg_len + 1 );
  }

  /* If the RMS destination is selected, send the message to the RMS */

  if( ( Parameters[3] & RMS_MSG_MASK ) &&
      ( hci_get_rms_down_flag() != HCI_YES_FLAG ) )
  {
    /* If the message size is acceptable for RMS then send it.
       Otherwise, popup an informative message so the operator
       can shorten it. */
    msg_len = strlen( string );

    if( msg_len > RMS_MAX_SIZE )
    {
      sprintf( buf, "The message to the RMS was truncated\nto %d characters. %d characters were\ndiscarded.", 
               RMS_MAX_SIZE, msg_len - RMS_MAX_SIZE );
      hci_warning_popup( Console_dialog, buf, NULL );
      msg_len = RMS_MAX_SIZE;
    }

  /* Clear the message buffer first so any old message data is removed */

    memset( Message, 0, MAX_CONSOLE_MSG_SIZE );
    strncpy( Message, string, msg_len );
    ORPGDA_write( ORPGDAT_RMS_TEXT_MSG, (char*) &Message, msg_len, LB_ANY );
    EN_post( ORPGEVT_RMS_TEXT_MSG, NULL, 0, 0 );
    HCI_LE_log( "Message sent to RMS" );
  }

  /* Display a feedback message in the RPG Control/Status window */

  sprintf( Cmd, "Sending Console Message" );
  HCI_display_feedback( Cmd );

  XtFree( string );

  /* Clear outgoing message area. */

  Reset_message_area();
}

/************************************************************************
     Description: This module reads new console messages from each
                  source type and displays them.
 ************************************************************************/

void hci_get_console_message()
{
  int len = 0;
  char *buf = NULL;
  char *buf1 = NULL;
  time_t tm = 0;
  int month = 0;
  int day = 0;
  int year = 0;
  int hour = 0;
  int minute = 0;
  int second = 0;
  int items = 0;
  int break_loop = HCI_NO_FLAG;
  int i = 0;
  XmString str;
  RDA_RPG_console_message_t Rda_msg;

  /* If the window isnt currently defined then create it. */

  if( Console_dialog == (Widget) NULL )
  {
    hci_console_message_callback( NULL, NULL, NULL );
    return;
  }

  /* If low bandwidth, create a progress meter. */

  HCI_PM( "Reading RDA Console Messages" );

  /* Loop through any new messages */

  while( break_loop == HCI_NO_FLAG )
  {
    /* First process RDA Console messages */

    len = ORPGDA_read( ORPGDAT_RDA_CONSOLE_MSG, (char *) &Rda_msg, sizeof( Rda_msg ), LB_NEXT );

    /* If a message was read, extract the time from the message header,
       append the message and display it at the top of the list. */

    if( len > 0 )
    {
      int from_added = 0;
      int start = 0;
      int len = 0;
      int n_strings = 0;

      tm = ( Rda_msg.msg_hdr.julian_date - 1 ) * HCI_SECONDS_PER_DAY;
      tm += ( Rda_msg.msg_hdr.milliseconds / HCI_MILLISECONDS_PER_SECOND );

      unix_time( &tm, &year, &month, &day, &hour, &minute, &second );
      buf = (char *) calloc( Rda_msg.size + 1, 1 );
      buf1 = (char *) calloc( Rda_msg.size + 1 + 64, 1 );

      strncpy( buf, Rda_msg.message, Rda_msg.size );
      len = strlen( buf );
      n_strings = 0;
      for( i = 0; i < len; i++ )
      {
        if( (iscntrl( (int) buf[i] ))
                      ||
            (isspace( (int) buf[i] )) ){ 

          /* If space, just continue. */
          if( buf[i] == ' ' )
          {
             continue;
          }
          else
          {
            if( !from_added )
            {
              sprintf( buf1, "[RDA] %2.2d/%2.2d/%4.4d - %2.2d:%2.2d:%2.2d -",
                       month, day, year, hour, minute, second );
              from_added = 1;
            }

            /* Terminate the string, then copy to buf1 before adding to list. */
            buf[i] = '\0';
            strcat( buf1, &buf[start] );
            str = XmStringCreateLocalized( buf1 );
            Str_ptrs[n_strings] = str;
            n_strings++;
            memset( (char *) buf1, 0, Rda_msg.size + 1 + 64 );
            start = i+1;
            continue;
          }
        }
      }

      /* Output any text remaining. */
      if( (start+1) <= len )
      {
        if( !from_added )
        {
          sprintf( buf1, "[RDA] %2.2d/%2.2d/%4.4d - %2.2d:%2.2d:%2.2d -",
                   month, day, year, hour, minute, second );
          from_added = 1;
        }
        
        strcat( buf1, &buf[start] );
        str = XmStringCreateLocalized( buf1 );
        Str_ptrs[n_strings] = str;
        n_strings++;
      }

      if( buf != NULL )
        free( (char *) buf );
      if( buf1 != NULL )
        free( (char *) buf1 );

      /* Output the strings in reverse order so they show up in
         the correct order. */
      for( i = n_strings-1; i >= 0; i-- )
      {
        XmListAddItemUnselected( Incoming_messages, Str_ptrs[i], 1 );
        XmStringFree( Str_ptrs[i] );
        XtVaGetValues( Incoming_messages, XmNitemCount, &items, NULL );
      }      
    }
    else if( len == LB_EXPIRED )
    {
      /* End of LB, go to first slot */
      ORPGDA_seek( ORPGDAT_RDA_CONSOLE_MSG, 0, LB_FIRST, NULL );
      continue;
    }
    else if( len == LB_TO_COME )
    {
      /* All new msgs read */
      break_loop = HCI_YES_FLAG;
    }
    else if (len < 0 )
    {
      /* Unexpected error */
      HCI_LE_error( "ERROR reading ORPGDAT_RDA_CONSOLE_MSG (%d)", len );
      break_loop = HCI_YES_FLAG;
    }
  }
   
  XmListSetPos( Incoming_messages, 1 );

  HCI_Shell_popup( Console_dialog );
}

/************************************************************************
     Description: This module reads all text messages from all
                  sources and builds a list.
 ************************************************************************/

static void Hci_initialize_console_msg( Widget w )
{
  int rda_status = 0;
  char *buf = NULL;
  char *buf1 = NULL;
  time_t tm_rda = 0;
  int month = 0;
  int day = 0;
  int year = 0;
  int hour = 0;
  int minute = 0;
  int second = 0;
  int i = 0;
  RDA_RPG_console_message_t Rda_msg;

  /* Since we are reading console messages from more than one
     source we need to read them simultaneously so we can put
     them in chronological order. */

  /* Start with the first RDA Console message. */

  rda_status = ORPGDA_seek( ORPGDAT_RDA_CONSOLE_MSG, 0, LB_FIRST, NULL );

  if( rda_status < 0 )
  {
    HCI_LE_error( "ORPGDA_seek error RDA_CONSOLE_MSG (%d)", rda_status );
    return;
  }

  /* Need to loop through LB and put them in chronological order. */

  rda_status = 1;
  tm_rda = 0;

  while( rda_status > 0 )
  {
    HCI_PM( "Reading RDA text messages" );

    rda_status = ORPGDA_read( ORPGDAT_RDA_CONSOLE_MSG, (char *) &Rda_msg,
                sizeof( RDA_RPG_console_message_t ), LB_NEXT );

    if( rda_status > 0 )
    {
      int from_added = 0;
      int start = 0;
      int len = 0, n_strings = 0;

      tm_rda = ( Rda_msg.msg_hdr.julian_date - 1 ) * HCI_SECONDS_PER_DAY;
      tm_rda += ( Rda_msg.msg_hdr.milliseconds / HCI_MILLISECONDS_PER_SECOND );

      unix_time( &tm_rda, &year, &month, &day, &hour, &minute, &second );

      buf = (char *) calloc( Rda_msg.size + 1, 1 );
      buf1 = (char *) calloc( Rda_msg.size + 1 + 64, 1 );

      strncpy( buf, Rda_msg.message, Rda_msg.size );

      len = strlen( buf );
      n_strings = 0;
      for( i = 0; i < len; i++ )
      {
        if( (iscntrl( (int) buf[i] ))
                      ||
            (isspace( (int) buf[i] )) ){ 

          /* If space, just continue. */
          if( buf[i] == ' ' )
          {
             continue;
          }
          else
          {
            if( !from_added )
            {
              sprintf( buf1, "[RDA] %2.2d/%2.2d/%4.4d - %2.2d:%2.2d:%2.2d -",
                       month, day, year, hour, minute, second );
              from_added = 1;
            }

            /* Terminate the string, then copy to buf1 before adding to list. */
            buf[i] = '\0';
            strcat( buf1, &buf[start] );
            Str_ptrs[n_strings] = XmStringCreateLocalized( buf1 );
            n_strings++;
            memset( (char *) buf1, 0, Rda_msg.size + 1 + 64 );
            start = i+1;
          }
        }
      }

      /* Output any text remaining. */
      if( (start+1) <= len )
      {
        if( !from_added )
        {
          sprintf( buf1, "[RDA] %2.2d/%2.2d/%4.4d - %2.2d:%2.2d:%2.2d -",
                   month, day, year, hour, minute, second );
        }

        strcat( buf1, &buf[start] );
        Str_ptrs[n_strings] = XmStringCreateLocalized( buf1 );
        n_strings++;
      }

      if( buf != NULL )
        free( (char *) buf );
      if( buf1 != NULL )
        free( (char *) buf1 );

      /* Output the strings in reverse order so they show up in
         the correct order. */
      for( i = n_strings-1; i >= 0; i-- )
      {
        XmListAddItemUnselected( Incoming_messages, Str_ptrs[i], 1 );
        XmStringFree( Str_ptrs[i] );
      }      
    }
  }

  XmListSetPos( Incoming_messages, 1 );
}

/************************************************************************
     Description: This module returns the flag which determines if a new
                  RDA console message needs to be read.  If the flag is 1,
                  a new message is waiting to be read.  Otherwise it is 0.
 ************************************************************************/

int hci_get_RDA_message_flag()
{
  return Rda_message_flag;
}

/************************************************************************
     Description: This module sets the flag which determines if a new
                  RDA console message needs to be read.  If the flag is 1,
                  a new message is waiting to be read.  Otherwise it is 0.
 ************************************************************************/

void hci_set_RDA_message_flag( int flag )
{
  Rda_message_flag = flag;
}

/************************************************************************
     Description: This is the callback invoked when one of the
                  destination toggle buttons is selected
 ************************************************************************/

static void Console_msg_filter_callback( Widget w, XtPointer y, XtPointer z )
{
  int i = 0;
  int class_no = 0;
  int bit = 0;
  int word = 0;
  XtPointer data = (XtPointer) NULL;
  XmToggleButtonCallbackStruct *state;

  state = (XmToggleButtonCallbackStruct *) z;

  XtVaGetValues( w, XmNuserData, &data, NULL );

  class_no = (int) data;

  /* If the All button for a class group was selected, then
     either set all group lines or clear them. */

  if( (int) y >= CLASS_MSG )
  {
    for( i = 0; i < CLASS_MSG; i++ )
    {
      if( Button[i] != (Widget) NULL )
      {
        XtVaGetValues( Button[i], XmNuserData, &data, NULL );

        /* Since the RPGOP class (99) and class 1 should be
           grouped together, we need a special check. */

        if( ( class_no == (int) data ) ||
            ( ( class_no == 99 ) && ( (int) data ==  1 ) ) ||
            ( ( class_no ==  1 ) && ( (int) data == 99 ) ) ||
            ( ( class_no == 98 ) && ( (int) data ==  1 ) ) ||
            ( ( class_no ==  1 ) && ( (int) data == 98 ) ) )
        {
          word = i/16;
          bit = i%16;

          if( state->set )
          {
            Parameters[word] = Parameters[word] | ( 1 << bit );
            XtVaSetValues( Button[i], XmNset, True, NULL );
          }
          else
          {
            Parameters[word] = Parameters[word] & ( ~( 1 << bit ) );
            XtVaSetValues( Button[i], XmNset, False, NULL );
          }
        }
      }
    }
  }
  else
  {
    i = (int) y;
    word = i/16;
    bit = i%16;

    if( state->set )
    {
      Parameters[word] = Parameters[word] | ( 1 << bit );
      XtVaSetValues( Button[i], XmNset, True, NULL );
    }
    else
    {
      Parameters[word] = Parameters[word] & ( ~( 1 << bit ) );
      XtVaSetValues( Button[i], XmNset, False, NULL );
    }
  }
}

/************************************************************************
     Description: This module reads the configured line information
                  and return the read status.
 ************************************************************************/

static int Hci_read_nb_line_info()
{
  int ret = 0;
  static Pd_distri_info *Pd_info = NULL;

  if( Pd_info != NULL ){ free( Pd_info ); }
  Pd_info = NULL;

  /* Read the latest line information message. */

  ret = ORPGDA_read( ORPGDAT_PROD_INFO, &Pd_info, LB_ALLOC_BUF, PD_LINE_INFO_MSG_ID );

  if( ret <= 0 )
  {
    HCI_LE_error( "ORPGDA_read (ORPGDAT_PROD_INFO, PD_LINE_INFO_MSG_ID) failed (%d)", ret );
    return( ret );
  }

  /* If the size of the message read is smaller than expected,
     return with an error. */

  if( ret < sizeof( Pd_distri_info ) ||
      Pd_info->line_list < sizeof( Pd_distri_info ) ||
      ret < Pd_info->line_list + Pd_info->n_lines * sizeof( Pd_line_entry ) )
  {
    return( -1 );
  }

  N_lines = Pd_info->n_lines;

  /* Set the line info pointer to the beginning of the table. */

  Lines = (Pd_line_entry *) ( (char *) Pd_info + Pd_info->line_list );

  return( 0 );
}

