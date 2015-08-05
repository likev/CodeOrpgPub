/****************************************************************
      Module: hci_task_status.c
      Description: This module displays RPG task information
 ****************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2015/01/05 18:41:28 $
 * $Id: hci_RPG_status.c,v 1.162 2015/01/05 18:41:28 steves Exp $
 * $Revision: 1.162 $
 * $State: Exp $
 */

/* Local include files */

#include <hci.h>

/* Local definitions */

#define	MAX_MSGS_DISPLAY			250
#define	MAX_MSGS_BUFFER				10000
#define	MAX_FAILED_TASKS_DISPLAY		50
#define	FILTER_RPG_GENERAL_STATUS_MSG		0x0001 /* status msg */
#define	FILTER_RPG_WARNING_STATUS_MSG		0x0002 /* warning msg */
#define	FILTER_RPG_INFO_STATUS_MSG		0x0004 /* info msg */
#define	FILTER_RPG_COMMS_STATUS_MSG		0x0008 /* NB comms msg */
#define	FILTER_RDA_SEC_ALARM_ACTIVATED_MSG	0x0010 /* RDA Sec alarm msg */
#define	FILTER_RDA_MAR_ALARM_ACTIVATED_MSG	0x0020 /* RDA MAR alarm msg */
#define	FILTER_RDA_MAM_ALARM_ACTIVATED_MSG	0x0040 /* RDA MAM alarm msg */
#define	FILTER_RDA_INOP_ALARM_ACTIVATED_MSG	0x0080 /* RDA INOP alarm msg */
#define	FILTER_RDA_NA_ALARM_ACTIVATED_MSG	0x0100 /* RDA NA msg */
#define	FILTER_RDA_ALARM_CLEARED_MSG		0x0200 /* RDA cleared msg */
#define	FILTER_RPG_MAR_ALARM_ACTIVATED_MSG	0x0400 /* RPG MAR alarm msg */
#define	FILTER_RPG_MAM_ALARM_ACTIVATED_MSG	0x0800 /* RPG MAM alarm msg */
#define	FILTER_RPG_LS_ALARM_ACTIVATED_MSG	0x1000 /* RPG LS alarm msg */
#define	FILTER_RPG_ALARM_CLEARED_MSG		0x2000 /* RPG cleared msg */
#define	FILTER_ALL_MSGS				0x3fff /* All msgs */

/* Static/Global variables */

static Widget Top_widget = (Widget) NULL;
static Widget Filter_msgs_button = (Widget) NULL;
static Widget Page_left_button = (Widget) NULL;
static Widget Page_right_button = (Widget) NULL;
static Widget Msg_frame = (Widget) NULL;
static Widget Msg_scroll = (Widget) NULL;
static Widget Msg_form = (Widget) NULL;
static Widget Msg_entry[MAX_MSGS_DISPLAY] = {(Widget) NULL};
static Widget Failed_task_dialog = (Widget) NULL;
static Widget Failed_task_entry[MAX_FAILED_TASKS_DISPLAY] = {(Widget) NULL};
static Widget Failed_control_task_dialog = (Widget) NULL;
static Widget Failed_control_task_entry[MAX_FAILED_TASKS_DISPLAY] = {(Widget) NULL};
static Widget Filter_text = (Widget) NULL;
static Widget Rpg_state_label = (Widget) NULL;
static Widget Wx_mode_label = (Widget) NULL;
static Widget Filter_msgs_dialog = (Widget) NULL;
static Widget Load_shed_prod_storage = (Widget) NULL;
static Widget Load_shed_rda_radial = (Widget) NULL;
static Widget Load_shed_rpg_radial = (Widget) NULL;
static Widget Load_shed_narrowband = (Widget) NULL;
static Widget Maint_req_narrowband = (Widget) NULL;
static Widget Maint_req_RDA_wideband = (Widget) NULL;
static Widget Maint_req_replay_db = (Widget) NULL;
static Widget Maint_req_task_failure = (Widget) NULL;
static Widget Maint_req_RPG_link_fail= (Widget) NULL;
static Widget Maint_req_red_chan_error = (Widget) NULL;
static Widget Maint_man_RDA_wideband = (Widget) NULL;
static Widget Maint_man_task_failure = (Widget) NULL;
static Widget Maint_man_node_connect = (Widget) NULL;
static Widget Maint_man_media_failure = (Widget) NULL;
static Widget Rda_alarm_SEC_button = (Widget) NULL;
static Widget Rda_alarm_MR_button = (Widget) NULL;
static Widget Rda_alarm_MM_button = (Widget) NULL;
static Widget Rda_alarm_INOP_button = (Widget) NULL;
static Widget Rpg_alarm_MR_button = (Widget) NULL;
static Widget Rpg_alarm_MM_button = (Widget) NULL;
static Widget Rpg_alarm_LS_button = (Widget) NULL;
static Widget Rpg_info_button = (Widget) NULL;
static Widget Rpg_gen_status_button = (Widget) NULL;
static Widget Rpg_warn_status_button = (Widget) NULL;
static Widget Rpg_comms_status_button = (Widget) NULL;
static Boolean RDA_SEC_stat = True;
static Boolean RDA_MR_stat = True;
static Boolean RDA_MM_stat = True;
static Boolean RDA_INOP_stat = True;
static Boolean RPG_info_stat = True;
static Boolean RPG_gen_stat = True;
static Boolean RPG_warn_stat = True;
static Boolean RPG_comms_stat = True;
static Boolean RPG_MR_stat = True;
static Boolean RPG_MM_stat = True;
static Boolean RPG_LS_alarm_stat = True;
static Pixel Base_bg = (Pixel) NULL;
static Pixel Base2_bg = (Pixel) NULL;
static Pixel Button_fg = (Pixel) NULL;
static Pixel Button_bg = (Pixel) NULL;
static Pixel Edit_fg = (Pixel) NULL;
static Pixel Edit_bg = (Pixel) NULL;
static Pixel Text_fg = (Pixel) NULL;
static Pixel Text_bg = (Pixel) NULL;
static Pixel Normal_color = (Pixel) NULL;
static Pixel Warning_color = (Pixel) NULL;
static Pixel Alarm_color = (Pixel) NULL;
static Pixel Alarm2_color = (Pixel) NULL;
static Pixel White_color = (Pixel) NULL;
static Pixel Black_color = (Pixel) NULL;
static Pixel Cyan_color = (Pixel) NULL;
static Pixel Seagreen_color = (Pixel) NULL;
static Pixel Gray_color = (Pixel) NULL;
static XmFontList List_font = (XmFontList) NULL;
static char Old_search_string[128] = {""};
static char Search_string[128] = {""};
static int New_msg_flag = HCI_YES_FLAG;
static int List_index = 1;
static LB_status Log_status;
static int Msgs_read = 0;
static int Active_page = 0;
static int Filter_status = FILTER_ALL_MSGS;
static char Sys_msg[MAX_MSGS_BUFFER][HCI_LE_MSG_MAX_LENGTH];
static int Msg_type[MAX_MSGS_BUFFER] = {0};
static int System_flag = 0;

/* Function prototypes */

static void Close_system_status_callback( Widget, XtPointer, XtPointer );
static void Search_text_callback( Widget, XtPointer, XtPointer );
static void Select_log_message_callback( Widget, XtPointer, XtPointer );
static void Filter_button_callback( Widget, XtPointer, XtPointer );
static void Popup_failed_task_list( Widget, XtPointer, XtPointer );
static void Close_failed_task_dialog( Widget, XtPointer, XtPointer );
static void Popup_failed_control_task_list (Widget, XtPointer, XtPointer );
static void Close_failed_control_task_dialog( Widget, XtPointer, XtPointer );
static void Hci_close_msgs_dialog_callback( Widget, XtPointer, XtPointer );
static void Clear_callback( Widget, XtPointer, XtPointer );
static void Hci_page_button_callback( Widget, XtPointer, XtPointer );
static void Hci_select_msgs_callback( Widget, XtPointer, XtPointer );
static void Timer_proc();
static void New_system_log_msg( int, LB_id_t, int, void * );
static void Update_system_log_msgs();
static void Display_failed_task_list();
static void Display_failed_control_task_list();
static void Display_system_status();
static void Display_system_status_msgs( int );
static void Toggle_rda_alarms_button( Widget );
static void Toggle_rpg_alarms_button( Widget );
static void Toggle_rpg_msgs_button( Widget );

/************************************************************************
     Description: This is the main function for the RPG Status task.
 ************************************************************************/

int main( int argc, char *argv[] )
{
  Widget button = (Widget) NULL;
  Widget label = (Widget) NULL;
  Widget form = (Widget) NULL;
  Widget rowcol = (Widget) NULL;
  Widget filter_frame = (Widget) NULL;
  Widget filter_rowcol = (Widget) NULL;
  Widget state_frame = (Widget) NULL;
  Widget state_form = (Widget) NULL;
  Widget rpg_info_rowcol = (Widget) NULL;
  Widget rpg_state_frame = (Widget) NULL;
  Widget wx_mode_frame = (Widget) NULL;
  Widget alarm_summary_frame = (Widget) NULL;
  Widget alarm_summary_form = (Widget) NULL;
  Widget alarm_maint_req_frame = (Widget) NULL;
  Widget alarm_maint_req_rowcol = (Widget) NULL;
  Widget alarm_maint_man_frame = (Widget) NULL;
  Widget alarm_maint_man_rowcol = (Widget) NULL;
  Widget alarm_load_shed_frame = (Widget) NULL;
  Widget alarm_load_shed_rowcol = (Widget) NULL;
  Widget clip = (Widget) NULL;
  int status = 0;
  char *msg = NULL;
  int offset = 0;
  int le_msg_len = 0;
  char buf[16];
  unsigned int rpg_msg = 0;
  unsigned int rpg_alarm = 0;
  unsigned int rda_alarm = 0;
  LE_critical_message *le_msg;
  XmString str;

  /* Initialize HCI. */

  HCI_init( argc, argv, HCI_STATUS_TASK );

  Top_widget = HCI_get_top_widget();

  /* Get system flag. */

  System_flag = HCI_get_system();

  /* Define colors and fonts */

  Base_bg = hci_get_read_color( BACKGROUND_COLOR1 );
  Base2_bg = hci_get_read_color( BACKGROUND_COLOR2 );
  Button_fg = hci_get_read_color( BUTTON_FOREGROUND );
  Button_bg = hci_get_read_color( BUTTON_BACKGROUND );
  Edit_fg = hci_get_read_color( EDIT_FOREGROUND );
  Edit_bg = hci_get_read_color( EDIT_BACKGROUND );
  Text_fg = hci_get_read_color( TEXT_FOREGROUND );
  Text_bg = hci_get_read_color( TEXT_BACKGROUND );
  Normal_color = hci_get_read_color( NORMAL_COLOR );
  Warning_color = hci_get_read_color( WARNING_COLOR );
  Alarm_color = hci_get_read_color( ALARM_COLOR1 );
  Alarm2_color = hci_get_read_color( ALARM_COLOR2 );
  Black_color = hci_get_read_color( BLACK );
  White_color = hci_get_read_color( WHITE );
  Cyan_color = hci_get_read_color( CYAN );
  Seagreen_color = hci_get_read_color( SEAGREEN );
  Gray_color = hci_get_read_color( GRAY );
  List_font = hci_get_fontlist( LIST );

  /* If low bandwidth, display a progress meter. */

  HCI_PM( "Reading System Log Messages" );

  /* Set write permission of ORPGDAT_SYSLOG LB. */

  ORPGDA_write_permission( ORPGDAT_SYSLOG );

  /* First check to see how many messages the log msg LB currently
     has in it.  This is what we will read if it is less than the
     max number of messages to process. */

  Msgs_read = 0;

  if( ( status = ORPGDA_stat( ORPGDAT_SYSLOG, &Log_status ) ) == LB_SUCCESS )
  {
    if( Log_status.n_msgs > MAX_MSGS_BUFFER )
    {
      status = ORPGDA_seek( ORPGDAT_SYSLOG, -(MAX_MSGS_BUFFER-1), LB_LATEST, NULL );
    }
    else
    {
      status = ORPGDA_seek( ORPGDAT_SYSLOG, 0, LB_FIRST, NULL );
    }

    msg = calloc( HCI_LE_MSG_MAX_LENGTH*MAX_MSGS_BUFFER, 1 );

    if( status >= 0 )
    {
      status = ORPGDA_read( ORPGDAT_SYSLOG,
              (char *) msg,
              HCI_LE_MSG_MAX_LENGTH*MAX_MSGS_BUFFER,
              ( LB_MULTI_READ | (MAX_MSGS_BUFFER - 1) ) );
    }

    if( status >= 0 )
    {
      offset = 0;

      while( status > offset )
      {
        le_msg = (LE_critical_message *) (msg+offset);
        le_msg_len = ALIGNED_SIZE( sizeof( LE_critical_message )+strlen( le_msg->text ) );
        offset += le_msg_len;

        memcpy( Sys_msg[Msgs_read], le_msg, le_msg_len );

        /* Get the RPG/RDA alarm level so we can set the appropriate msg type. 
           The message type is used for:
           1) color encoding the messages, and 
           2) message filtering
        */
        rpg_msg = ( le_msg->code & HCI_LE_RPG_STATUS_MASK );
        rpg_alarm = ( le_msg->code & HCI_LE_RPG_ALARM_MASK );
        rda_alarm = ( le_msg->code & HCI_LE_RDA_ALARM_MASK );

        if( rpg_alarm )
        {
          /* RPG alarm. */
          switch( rpg_alarm )
          {
            case HCI_LE_RPG_ALARM_MAR:
            default:
              Msg_type[Msgs_read] = FILTER_RPG_MAR_ALARM_ACTIVATED_MSG;
              break;

            case HCI_LE_RPG_ALARM_MAM:
              Msg_type[Msgs_read] = FILTER_RPG_MAM_ALARM_ACTIVATED_MSG;
              break;

            case HCI_LE_RPG_ALARM_LS:
              Msg_type[Msgs_read] = FILTER_RPG_LS_ALARM_ACTIVATED_MSG;
              break;
          }

          if( le_msg->code & HCI_LE_RPG_ALARM_CLEAR )
          {
            Msg_type[Msgs_read] |= FILTER_RPG_ALARM_CLEARED_MSG;
          }
        }
        else if( rda_alarm )
        {
          /* Check if it is an RDA alarm. */
          switch( rda_alarm )
          {
            case HCI_LE_RDA_ALARM_NA:
            default:
              Msg_type[Msgs_read] = FILTER_RDA_NA_ALARM_ACTIVATED_MSG;
              break;

            case HCI_LE_RDA_ALARM_SEC:
              Msg_type[Msgs_read] = FILTER_RDA_SEC_ALARM_ACTIVATED_MSG;
              break;

            case HCI_LE_RDA_ALARM_MAR:
              Msg_type[Msgs_read] = FILTER_RDA_MAR_ALARM_ACTIVATED_MSG;
              break;

            case HCI_LE_RDA_ALARM_MAM:
              Msg_type[Msgs_read] = FILTER_RDA_MAM_ALARM_ACTIVATED_MSG;
              break;

            case HCI_LE_RDA_ALARM_INOP:
              Msg_type[Msgs_read] = FILTER_RDA_INOP_ALARM_ACTIVATED_MSG;
              break;
          }

          if( le_msg->code & HCI_LE_RDA_ALARM_CLEAR )
          {
            Msg_type[Msgs_read] |= FILTER_RDA_ALARM_CLEARED_MSG;
          }
        }
        else if ( rpg_msg )
        {
          /* Check for RPG message. */
          if( rpg_msg == HCI_LE_RPG_STATUS_WARN ) 
          {
            Msg_type[Msgs_read] = FILTER_RPG_WARNING_STATUS_MSG;
          }
          else if( rpg_msg == HCI_LE_RPG_STATUS_INFO )
          {
            Msg_type[Msgs_read] = FILTER_RPG_INFO_STATUS_MSG;
          }
          else if( rpg_msg == HCI_LE_RPG_STATUS_COMMS ) 
          {
            Msg_type[Msgs_read] = FILTER_RPG_COMMS_STATUS_MSG;
          }
          else if( rpg_msg == HCI_LE_RPG_STATUS_GEN )
          {
            Msg_type[Msgs_read] = FILTER_RPG_GENERAL_STATUS_MSG;
          }
        }
        else
        {
          /* All other messages. */
          if( le_msg->code & HCI_LE_ERROR_BIT )
          {
            Msg_type[Msgs_read] = FILTER_RPG_WARNING_STATUS_MSG;
          }
          else
          {
            Msg_type[Msgs_read] = FILTER_RPG_GENERAL_STATUS_MSG;
          }
        }
        Msgs_read++;

        if( Msgs_read >= MAX_MSGS_BUFFER )
        {
          HCI_LE_error( "Too many messages read" );
          break;
        }
      }
    }
    free( msg );
  }

  /* Build wigets. */

  form = XtVaCreateWidget( "rpg_status_form",
                xmFormWidgetClass, Top_widget,
                XmNforeground, Base_bg,
                XmNbackground, Base_bg,
                NULL );

  HCI_PM( "Initialize RPG Status Information" );

  rowcol = XtVaCreateWidget( "rpg_status_rowcol",
                xmRowColumnWidgetClass, form,
                XmNtopAttachment, XmATTACH_FORM,
                XmNleftAttachment, XmATTACH_FORM,
                XmNrightAttachment, XmATTACH_FORM,
                XmNbackground, Base_bg,
                XmNorientation, XmHORIZONTAL,
                XmNpacking, XmPACK_TIGHT,
                NULL );

  button = XtVaCreateManagedWidget( "Close",
                xmPushButtonWidgetClass, rowcol,
                XmNforeground, Button_fg,
                XmNbackground, Button_bg,
                XmNfontList, List_font,
                XmNalignment, XmALIGNMENT_CENTER,
                NULL );

  XtAddCallback( button, XmNactivateCallback, Close_system_status_callback, NULL );

  XtManageChild( rowcol );

  state_frame = XtVaCreateManagedWidget( "state_frame",
                xmFrameWidgetClass, form,
                XmNbackground, Base_bg,
                XmNtopAttachment, XmATTACH_WIDGET,
                XmNtopWidget, rowcol,
                XmNleftAttachment, XmATTACH_FORM,
                XmNrightAttachment, XmATTACH_FORM,
                NULL );

  state_form = XtVaCreateWidget( "state_form",
                xmFormWidgetClass, state_frame,
                XmNforeground, Base_bg,
                XmNbackground, Base_bg,
                XmNverticalSpacing, 1,
                NULL );

  rpg_info_rowcol = XtVaCreateManagedWidget( "rpg_info_rowcol",
                xmRowColumnWidgetClass, state_form,
                XmNorientation, XmHORIZONTAL,
                XmNbackground, Base_bg,
                XmNnumColumns, 1,
                XmNpacking, XmPACK_COLUMN,
                XmNentryAlignment, XmALIGNMENT_CENTER,
                XmNspacing, 1,
                NULL );

  label = XtVaCreateManagedWidget( "  Space  ",
                xmLabelWidgetClass, rpg_info_rowcol,
                XmNforeground, Base_bg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                NULL );

  rpg_state_frame = XtVaCreateManagedWidget( "rpg_state_frame",
                xmFrameWidgetClass, rpg_info_rowcol,
                XmNbackground, Base_bg,
                XmNtopAttachment, XmATTACH_FORM,
                XmNleftAttachment, XmATTACH_WIDGET,
                XmNleftWidget, label,
                NULL );

  label = XtVaCreateManagedWidget( "RPG State",
                xmLabelWidgetClass, rpg_state_frame,
                XmNchildType, XmFRAME_TITLE_CHILD,
                XmNchildHorizontalAlignment, XmALIGNMENT_CENTER,
                XmNchildVerticalAlignment, XmALIGNMENT_CENTER,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                NULL );

  Rpg_state_label = XtVaCreateManagedWidget( "     UNKNOWN     ",
                xmLabelWidgetClass, rpg_state_frame,
                XmNforeground, Text_fg,
                XmNbackground, Base2_bg,
                XmNfontList, List_font,
                NULL );

  label = XtVaCreateManagedWidget( "  Space  ",
                xmLabelWidgetClass, rpg_info_rowcol,
                XmNforeground, Base_bg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                XmNtopAttachment, XmATTACH_FORM,
                XmNleftAttachment, XmATTACH_WIDGET,
                XmNleftWidget, rpg_state_frame,
                NULL );

  wx_mode_frame = XtVaCreateManagedWidget( "wx_mode_frame",
                xmFrameWidgetClass, rpg_info_rowcol,
                XmNbackground, Base_bg,
                XmNtopAttachment, XmATTACH_FORM,
                XmNleftAttachment, XmATTACH_WIDGET,
                XmNleftWidget, label,
                NULL );

  label = XtVaCreateManagedWidget( "Wx Mode",
                xmLabelWidgetClass, wx_mode_frame,
                XmNchildType, XmFRAME_TITLE_CHILD,
                XmNchildHorizontalAlignment, XmALIGNMENT_CENTER,
                XmNchildVerticalAlignment, XmALIGNMENT_CENTER,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                NULL );

  Wx_mode_label = XtVaCreateManagedWidget( " Precipitation (A) ",
                xmLabelWidgetClass, wx_mode_frame,
                XmNforeground, Text_fg,
                XmNbackground, Base2_bg,
                XmNfontList, List_font,
                NULL );

  label = XtVaCreateManagedWidget( "  Space  ",
                xmLabelWidgetClass, rpg_info_rowcol,
                XmNforeground, Base_bg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                NULL );

  alarm_summary_frame = XtVaCreateManagedWidget( "alarm_summary_frame",
                xmFrameWidgetClass, state_form,
                XmNbackground, Base_bg,
                XmNtopAttachment, XmATTACH_WIDGET,
                XmNtopWidget, rpg_state_frame,
                XmNleftAttachment, XmATTACH_FORM,
                XmNrightAttachment, XmATTACH_FORM,
                NULL );

  label = XtVaCreateManagedWidget( "RPG Alarm Summary",
                xmLabelWidgetClass, alarm_summary_frame,
                XmNchildType, XmFRAME_TITLE_CHILD,
                XmNchildHorizontalAlignment, XmALIGNMENT_CENTER,
                XmNchildVerticalAlignment, XmALIGNMENT_CENTER,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                NULL );

  alarm_summary_form = XtVaCreateWidget( "alarm_summary_form",
                xmFormWidgetClass, alarm_summary_frame,
                XmNforeground, Base_bg,
                XmNbackground, Base_bg,
                NULL );

  label = XtVaCreateManagedWidget( "Space",
                xmLabelWidgetClass, alarm_summary_form,
                XmNforeground, Base_bg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                XmNtopAttachment, XmATTACH_FORM,
                XmNleftAttachment, XmATTACH_FORM,
                NULL );

  alarm_load_shed_frame = XtVaCreateManagedWidget( "alarm_load_shed_frame",
                xmFrameWidgetClass, alarm_summary_form,
                XmNbackground, Base_bg,
                XmNtopAttachment, XmATTACH_WIDGET,
                XmNpacking, XmPACK_COLUMN,
                XmNtopAttachment, XmATTACH_FORM,
                XmNleftAttachment, XmATTACH_WIDGET,
                XmNleftWidget, label,
                NULL );

  label = XtVaCreateManagedWidget( "Load Shed",
                xmLabelWidgetClass, alarm_load_shed_frame,
                XmNchildType, XmFRAME_TITLE_CHILD,
                XmNchildHorizontalAlignment, XmALIGNMENT_CENTER,
                XmNchildVerticalAlignment, XmALIGNMENT_CENTER,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                NULL );

  alarm_load_shed_rowcol = XtVaCreateWidget( "alarm_load_shed_rowcol",
                xmRowColumnWidgetClass, alarm_load_shed_frame,
                XmNorientation, XmHORIZONTAL,
                XmNbackground, Black_color,
                XmNorientation, XmVERTICAL,
                XmNnumColumns, 1,
                XmNpacking, XmPACK_COLUMN,
                XmNentryAlignment, XmALIGNMENT_CENTER,
                XmNspacing, 1,
                NULL );

  Load_shed_prod_storage = XtVaCreateManagedWidget( " Product Storage ",
                xmLabelWidgetClass, alarm_load_shed_rowcol,
                XmNforeground, Text_fg,
                XmNbackground, Base2_bg,
                XmNfontList, List_font,
                XmNalignment, XmALIGNMENT_CENTER,
                NULL );

  Load_shed_rda_radial = XtVaCreateManagedWidget( "RDA Radial",
                xmLabelWidgetClass, alarm_load_shed_rowcol,
                XmNforeground, Text_fg,
                XmNbackground, Base2_bg,
                XmNfontList, List_font,
                XmNalignment, XmALIGNMENT_CENTER,
                NULL );

  Load_shed_rpg_radial = XtVaCreateManagedWidget( "RPG Radial",
                xmLabelWidgetClass, alarm_load_shed_rowcol,
                XmNforeground, Text_fg,
                XmNbackground, Base2_bg,
                XmNfontList, List_font,
                XmNalignment, XmALIGNMENT_CENTER,
                NULL );

  Load_shed_narrowband = XtVaCreateManagedWidget( "Distribution",
                xmLabelWidgetClass, alarm_load_shed_rowcol,
                XmNforeground, Text_fg,
                XmNbackground, Normal_color,
                XmNfontList, List_font,
                XmNalignment, XmALIGNMENT_CENTER,
                NULL );

  XtManageChild( alarm_load_shed_rowcol );

  label = XtVaCreateManagedWidget( "Space",
                xmLabelWidgetClass, alarm_summary_form,
                XmNforeground, Base_bg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                XmNtopAttachment, XmATTACH_FORM,
                XmNleftAttachment, XmATTACH_WIDGET,
                XmNleftWidget, alarm_load_shed_frame,
                NULL );

  alarm_maint_req_frame = XtVaCreateManagedWidget( "alarm_maint_req_frame",
                xmFrameWidgetClass, alarm_summary_form,
                XmNbackground, Base_bg,
                XmNtopAttachment, XmATTACH_WIDGET,
                XmNpacking, XmPACK_COLUMN,
                XmNtopAttachment, XmATTACH_FORM,
                XmNleftAttachment, XmATTACH_WIDGET,
                XmNleftWidget, label,
                NULL );

  label = XtVaCreateManagedWidget( "Maintenance Required",
                xmLabelWidgetClass, alarm_maint_req_frame,
                XmNchildType, XmFRAME_TITLE_CHILD,
                XmNchildHorizontalAlignment, XmALIGNMENT_CENTER,
                XmNchildVerticalAlignment, XmALIGNMENT_CENTER,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                NULL );

  alarm_maint_req_rowcol = XtVaCreateWidget( "alarm_maint_req_rowcol",
                xmRowColumnWidgetClass, alarm_maint_req_frame,
                XmNorientation, XmHORIZONTAL,
                XmNbackground, Black_color,
                XmNorientation, XmVERTICAL,
                XmNnumColumns, 1,
                XmNpacking, XmPACK_COLUMN,
                XmNentryAlignment, XmALIGNMENT_CENTER,
                XmNspacing, 1,
                NULL );

  Maint_req_narrowband = XtVaCreateManagedWidget( "Distribution",
                xmLabelWidgetClass, alarm_maint_req_rowcol,
                XmNforeground, Text_fg,
                XmNbackground, Normal_color,
                XmNfontList, List_font,
                XmNalignment, XmALIGNMENT_CENTER,
                NULL );

  Maint_req_replay_db = XtVaCreateManagedWidget( "Data Base Failure",
                xmLabelWidgetClass, alarm_maint_req_rowcol,
                XmNforeground, Text_fg,
                XmNbackground, Base2_bg,
                XmNfontList, List_font,
                XmNalignment, XmALIGNMENT_CENTER,
                NULL );

  Maint_req_task_failure = XtVaCreateManagedWidget( "Task Failure",
                xmPushButtonWidgetClass, alarm_maint_req_rowcol,
                XmNforeground, Text_fg,
                XmNbackground, Base2_bg,
                XmNfontList, List_font,
                XmNalignment, XmALIGNMENT_CENTER,
                NULL );

  XtAddCallback( Maint_req_task_failure, XmNactivateCallback, Popup_failed_task_list, (XtPointer) NULL );

  if( System_flag == HCI_NWS_SYSTEM || System_flag == HCI_NWSR_SYSTEM )
  {
    Maint_req_RDA_wideband = XtVaCreateManagedWidget( "RDA Wideband",
                xmLabelWidgetClass, alarm_maint_req_rowcol,
                XmNforeground, Text_fg,
                XmNbackground, Base2_bg,
                XmNfontList, List_font,
                XmNalignment, XmALIGNMENT_CENTER,
                NULL );
  }
  else if( System_flag == HCI_FAA_SYSTEM )
  {
    Maint_req_RPG_link_fail = XtVaCreateManagedWidget( "RPG/RPG Link Failure",
                xmLabelWidgetClass, alarm_maint_req_rowcol,
                XmNforeground, Text_fg,
                XmNbackground, Base2_bg,
                XmNfontList, List_font,
                XmNalignment, XmALIGNMENT_CENTER,
                NULL );

    Maint_req_red_chan_error = XtVaCreateManagedWidget( "Redundant Channel Error",
                xmLabelWidgetClass, alarm_maint_req_rowcol,
                XmNforeground, Text_fg,
                XmNbackground, Base2_bg,
                XmNfontList, List_font,
                XmNalignment, XmALIGNMENT_CENTER,
                NULL );
  }

  XtManageChild( alarm_maint_req_rowcol );

  label = XtVaCreateManagedWidget( "Space",
                xmLabelWidgetClass, alarm_summary_form,
                XmNforeground, Base_bg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                XmNtopAttachment, XmATTACH_FORM,
                XmNleftAttachment, XmATTACH_WIDGET,
                XmNleftWidget, alarm_maint_req_frame,
                NULL );

  alarm_maint_man_frame = XtVaCreateManagedWidget( "alarm_maint_man_frame",
                xmFrameWidgetClass, alarm_summary_form,
                XmNbackground, Base_bg,
                XmNtopAttachment, XmATTACH_WIDGET,
                XmNpacking, XmPACK_COLUMN,
                XmNtopAttachment, XmATTACH_FORM,
                XmNleftAttachment, XmATTACH_WIDGET,
                XmNleftWidget, label,
                NULL );

  label = XtVaCreateManagedWidget( "Maintenance Mandatory",
                xmLabelWidgetClass, alarm_maint_man_frame,
                XmNchildType, XmFRAME_TITLE_CHILD,
                XmNchildHorizontalAlignment, XmALIGNMENT_CENTER,
                XmNchildVerticalAlignment, XmALIGNMENT_CENTER,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                NULL );

  alarm_maint_man_rowcol = XtVaCreateWidget( "alarm_main_man_rowcol",
                xmRowColumnWidgetClass, alarm_maint_man_frame,
                XmNorientation, XmHORIZONTAL,
                XmNbackground, Black_color,
                XmNorientation, XmVERTICAL,
                XmNnumColumns, 1,
                XmNpacking, XmPACK_COLUMN,
                XmNentryAlignment, XmALIGNMENT_CENTER,
                XmNspacing, 1,
                NULL );

  Maint_man_task_failure = XtVaCreateManagedWidget( "Control Task Failure",
                xmPushButtonWidgetClass, alarm_maint_man_rowcol,
                XmNforeground, Text_fg,
                XmNbackground, Base2_bg,
                XmNfontList, List_font,
                XmNalignment, XmALIGNMENT_CENTER,
                NULL );

  XtAddCallback( Maint_man_task_failure, XmNactivateCallback, Popup_failed_control_task_list, (XtPointer) NULL );

  Maint_man_node_connect = XtVaCreateManagedWidget( "Node Connectivity",
                xmLabelWidgetClass, alarm_maint_man_rowcol,
                XmNforeground, Text_fg,
                XmNbackground, Base2_bg,
                XmNfontList, List_font,
                XmNalignment, XmALIGNMENT_CENTER,
                NULL );

  Maint_man_RDA_wideband = XtVaCreateManagedWidget( "RDA Wideband Failure",
                xmLabelWidgetClass, alarm_maint_man_rowcol,
                XmNforeground, Text_fg,
                XmNbackground, Base2_bg,
                XmNfontList, List_font,
                XmNalignment, XmALIGNMENT_CENTER,
                NULL );

  Maint_man_media_failure = XtVaCreateManagedWidget ("Media Failure",
                xmLabelWidgetClass, alarm_maint_man_rowcol,
                XmNforeground, Text_fg,
                XmNbackground, Base2_bg,
                XmNfontList, List_font,
                XmNalignment, XmALIGNMENT_CENTER,
                NULL );

  XtManageChild( alarm_maint_man_rowcol );

  label = XtVaCreateManagedWidget( "Space",
                xmLabelWidgetClass, alarm_summary_form,
                XmNforeground, Base_bg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                XmNtopAttachment, XmATTACH_FORM,
                XmNleftAttachment, XmATTACH_WIDGET,
                XmNleftWidget, alarm_maint_man_frame,
                NULL );

  XtManageChild( alarm_summary_form );
  XtManageChild( state_form );

  filter_frame = XtVaCreateManagedWidget( "filter_frame",
                xmFrameWidgetClass, form,
                XmNbackground, Base_bg,
                XmNtopAttachment, XmATTACH_WIDGET,
                XmNtopWidget, state_frame,
                XmNleftAttachment, XmATTACH_FORM,
                XmNrightAttachment, XmATTACH_FORM,
                NULL );

  label = XtVaCreateManagedWidget( "Message Filter",
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
                XmNorientation, XmHORIZONTAL,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                XmNorientation, XmHORIZONTAL,
                XmNnumColumns, 1,
                XmNpacking, XmPACK_TIGHT,
                NULL );

  XtVaCreateManagedWidget( "Prev",
                xmLabelWidgetClass, filter_rowcol,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                NULL );

  Page_left_button = XtVaCreateManagedWidget( "page_left",
                xmArrowButtonWidgetClass, filter_rowcol,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                XmNsensitive, False,
                XmNarrowDirection, XmARROW_LEFT,
                XmNborderWidth, 0,
                NULL );

  XtAddCallback( Page_left_button, XmNactivateCallback, Hci_page_button_callback, (XtPointer) XmARROW_LEFT );

  label = XtVaCreateManagedWidget( "page_label",
                xmLabelWidgetClass, filter_rowcol,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                NULL );

  sprintf( buf, "--" );
  str = XmStringCreateLocalized( buf );
  XtVaSetValues( label, XmNlabelString, str, NULL );
  XmStringFree( str );

  Page_right_button = XtVaCreateManagedWidget( "page_right",
                xmArrowButtonWidgetClass, filter_rowcol,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                XmNsensitive, False,
                XmNarrowDirection, XmARROW_RIGHT,
                XmNborderWidth, 0,
                NULL );

  XtAddCallback( Page_right_button, XmNactivateCallback, Hci_page_button_callback, (XtPointer) XmARROW_RIGHT );

  XtVaCreateManagedWidget( "Next",
                xmLabelWidgetClass, filter_rowcol,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                NULL );

  label = XtVaCreateManagedWidget( "  ",
                xmLabelWidgetClass, filter_rowcol,
                XmNforeground, Base_bg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                NULL );

  Filter_msgs_button = XtVaCreateManagedWidget( "Message Filter",
                xmPushButtonWidgetClass, filter_rowcol,
                XmNselectColor, Warning_color,
                XmNforeground, Button_fg,
                XmNbackground, Button_bg,
                XmNfontList, List_font,
                NULL );

  XtAddCallback( Filter_msgs_button, XmNactivateCallback, Hci_select_msgs_callback, NULL );

  label = XtVaCreateManagedWidget( "  ",
                xmLabelWidgetClass, filter_rowcol,
                XmNforeground, Base_bg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                NULL );

  label = XtVaCreateManagedWidget( "  Search: ",
                xmLabelWidgetClass, filter_rowcol,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                NULL );

  Filter_text = XtVaCreateManagedWidget( "filter_text",
                xmTextFieldWidgetClass, filter_rowcol,
                XmNforeground, Edit_fg,
                XmNbackground, Edit_bg,
                XmNfontList, List_font,
                XmNcolumns, 12,
                NULL );

  if( strlen( Search_string ) )
  {
    XmTextSetString( Filter_text, Search_string );
  }

  XtAddCallback( Filter_text, XmNactivateCallback, Search_text_callback, NULL );
  XtAddCallback( Filter_text, XmNlosingFocusCallback, Search_text_callback, NULL );

  XtVaCreateManagedWidget( "Sp",
                xmLabelWidgetClass, filter_rowcol,
                XmNforeground, Base_bg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                NULL );

  button = XtVaCreateManagedWidget( "Clear",
                xmPushButtonWidgetClass, filter_rowcol,
                XmNforeground, Button_fg,
                XmNbackground, Button_bg,
                XmNfontList, List_font,
                NULL );

  XtAddCallback( button, XmNactivateCallback, Clear_callback, NULL );

  XtManageChild( filter_rowcol );

  Msg_frame = XtVaCreateManagedWidget( "Msg_frame",
                xmFrameWidgetClass, form,
                XmNbackground, Base_bg,
                XmNtopAttachment, XmATTACH_WIDGET,
                XmNtopWidget, filter_frame,
                XmNbottomAttachment, XmATTACH_WIDGET,
                XmNleftAttachment, XmATTACH_FORM,
                XmNrightAttachment, XmATTACH_FORM,
                NULL );

  label = XtVaCreateManagedWidget( "System Log Messages",
                xmLabelWidgetClass, Msg_frame,
                XmNchildType, XmFRAME_TITLE_CHILD,
                XmNchildHorizontalAlignment, XmALIGNMENT_CENTER,
                XmNchildVerticalAlignment, XmALIGNMENT_CENTER,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                NULL );

  Msg_scroll = XtVaCreateManagedWidget( "Msg_scroll",
                xmScrolledWindowWidgetClass, Msg_frame,
                XmNheight, 382,
                XmNwidth, 930,
                XmNscrollingPolicy, XmAUTOMATIC,
                XmNforeground, Base_bg,
                NULL );

  XtVaGetValues( Msg_scroll, XmNclipWindow, &clip, NULL );
  XtVaSetValues( clip,
                XmNforeground, Base_bg,
                XmNbackground, Base_bg,
                NULL );

  Display_system_status();
  Display_system_status_msgs( 1 );

  XtManageChild( form );
  XtPopup( Top_widget, XtGrabNone );

  /* Register for system log update events. */

  status = ORPGDA_UN_register( ORPGDAT_SYSLOG, LB_ANY, New_system_log_msg );

  if( status != LB_SUCCESS )
  {
    HCI_LE_error( "ORPGDA_UN_register (ORPGDAT_SYSLOG) failed (%d)", status );
  }

  /* Register for system log expire events (handle clearing log). */

  status = ORPGDA_UN_register( ORPGDAT_SYSLOG, LB_MSG_EXPIRED, New_system_log_msg );

  if( status != LB_SUCCESS )
  {
    HCI_LE_error( "ORPGDA_UN_register (ORPGDAT_SYSLOG) failed (%d)", status );
  }

  /* Start HCI loop. */
  HCI_start( Timer_proc, HCI_ONE_SECOND, RESIZE_HCI );

  return 0;
}

/************************************************************************
     Description: This is the timer proc for the RPG Status task.
                  It checks to see if any pertinent RPG status has
                  changed or if new messages have been written to
                  the system status log.  If so, the objects in the
                  window are updated.
 ************************************************************************/

static void Timer_proc()
{
  int status = 0;
  static unsigned int old_rpgalrm = 0;
  unsigned int new_rpgalrm = 0;
  static int old_rpg_state = 0;
  static int old_rpg_mode = 0;
  static int old_value = 0;
  int cur_value = 0;
  static int old_alarm_threshold = 0;
  int cur_alarm_threshold = 0;
  static int old_wxmode = -1;
  static int old_wb_status = -1;
  int new_wxmode = 0;
  int update_flag = HCI_NO_FLAG;
  Mrpg_state_t mrpg;

  /* If the RDA-RPG Interface status has changed we want to refresh
     the display. */

  if( ( status = ORPGRDA_get_wb_status( ORPGRDA_WBLNSTAT ) ) != old_wb_status )
  {
    update_flag = HCI_YES_FLAG;
    old_wb_status = status;
  }

  /* Next we want to check the RPG state and mode. If either has
     changed we want to refresh the display. */

  if( ( status = ORPGMGR_get_RPG_states( &mrpg ) ) == 0 )
  {
    /* First the RPG state */
    if( old_rpg_state != mrpg.state )
    {
      update_flag = HCI_YES_FLAG;
      old_rpg_state = mrpg.state;
    }

    /* Next the RPG operability mode (test or operate) */

    if( old_rpg_mode != mrpg.test_mode )
    {
      update_flag = HCI_YES_FLAG;
      old_rpg_mode = mrpg.test_mode;
    }
  }

  /* Next, lets see if the state of the RPG alarm bits have change.
     If so, we want to refresh the display. */

  if( ( status = ORPGINFO_statefl_get_rpgalrm( &new_rpgalrm ) ) == 0 )
  {
    if( old_rpgalrm != new_rpgalrm )
    {
      update_flag = HCI_YES_FLAG;
      old_rpgalrm = new_rpgalrm;
    }
  }

  /* If the weather mode has changed we want to force a refresh. */

  if( ( new_wxmode = ORPGVST_get_mode() ) != old_wxmode )
  {
    update_flag = HCI_YES_FLAG;
    old_wxmode = new_wxmode;
  }

  /* Next, we need to check the product distribution load shed data
     to see if we need to update the Distribution load shed alarm */

  status = ORPGLOAD_get_data( LOAD_SHED_CATEGORY_PROD_DIST,
                LOAD_SHED_CURRENT_VALUE,
                &cur_value );

  if( status == 0 )
  {
    if( cur_value != old_value )
    {
      old_value = cur_value;
      update_flag = HCI_YES_FLAG;
    }
  }

  status = ORPGLOAD_get_data( LOAD_SHED_CATEGORY_PROD_DIST,
              LOAD_SHED_ALARM_THRESHOLD,
              &cur_alarm_threshold );

  if( status == 0 )
  {
    if( cur_alarm_threshold != old_alarm_threshold )
    {
      old_alarm_threshold = cur_alarm_threshold;
      update_flag = HCI_YES_FLAG;
    }
  }

  /* If the update flag was set by any of the previous steps lets
     refresh the status display. */

  if( update_flag == HCI_YES_FLAG ){ Display_system_status(); }

  /*  If any new system status log messages were written we need to
     update the display list. */

  if( New_msg_flag )
  {
    New_msg_flag = HCI_NO_FLAG;
    Update_system_log_msgs();
  }

  /* If the task status has been updated, update the task and
     control task popups if they are open. */

  if( !hci_info_update_status( HCI_TASK_INFO_MSG_ID ) )
  {
    Display_failed_task_list();
    Display_failed_control_task_list();
  }
}

/************************************************************************
     Description: This function is activated when the user selects
                  the "Close" button.
 ************************************************************************/

static void Close_system_status_callback( Widget w, XtPointer y, XtPointer z )
{
  HCI_LE_log( "RPG Status Close selected" );
  HCI_task_exit( HCI_EXIT_SUCCESS );
}

/************************************************************************
     Description: This function displays a list of failed RPG tasks
                  in the Failed Task List window.
 ************************************************************************/

static void Display_failed_task_list()
{
  int i = 0;
  int num_tasks = 0;
  char buf[256];
  XmString str;

  /* If the failed task list window is not defined, do nothing. */

  if( Failed_task_dialog == (Widget) NULL ){ return; }

  /* Remove all previously listed failed tasks. */

  for( i = 0; i < MAX_FAILED_TASKS_DISPLAY; i++ )
  {
    XtUnmanageChild( Failed_task_entry[i] );
  }

  /* Display failed tasks. */

  for( i = 0; i < hci_info_failed_task_num(); i++ )
  {
    if( hci_info_failed_task_type( i ) == 0 )
    {
      num_tasks++;
      sprintf( buf, "%s", hci_info_failed_task_name( i ) );
      str = XmStringCreateLocalized( buf );
      XtVaSetValues( Failed_task_entry[i], XmNlabelString, str, NULL );
      XtManageChild( Failed_task_entry[i] );
      XmStringFree( str );
    }
  }

  if( num_tasks == 0 )
  {
    sprintf( buf, "No failed non-control tasks" );
    str = XmStringCreateLocalized( buf );
    XtVaSetValues( Failed_task_entry[0], XmNlabelString, str, NULL );
    XtManageChild( Failed_task_entry[0] );
    XmStringFree( str );
  }
}

/************************************************************************
     Description: This function displays a list of failed RPG
                  control tasks in the Failed Task List window.
 ************************************************************************/

static void Display_failed_control_task_list()
{
  int i = 0;
  int num_control_tasks = 0;
  char buf[256];
  XmString str;

  /* If the failed task list window is not defined, do nothing. */

  if( Failed_control_task_dialog == (Widget) NULL ){ return; }

  /* Remove all previously listed failed control tasks. */

  for( i = 0; i < MAX_FAILED_TASKS_DISPLAY; i++ )
  {
    XtUnmanageChild( Failed_control_task_entry[i] );
  }

  /* Display failed control tasks. */

  for( i = 0; i < hci_info_failed_task_num(); i++ )
  {
    if( hci_info_failed_task_type( i ) != 0 )
    {
      num_control_tasks++;
      sprintf( buf, "%s", hci_info_failed_task_name( i ) );
      str = XmStringCreateLocalized( buf );
      XtVaSetValues( Failed_control_task_entry[i], XmNlabelString, str, NULL );
      XtManageChild( Failed_control_task_entry[i] );
      XmStringFree( str );
    }
  }

  if( num_control_tasks == 0 )
  {
    sprintf( buf, "No failed control tasks" );
    str = XmStringCreateLocalized( buf );
    XtVaSetValues( Failed_control_task_entry[0], XmNlabelString, str, NULL );
    XtManageChild( Failed_control_task_entry[0] );
    XmStringFree( str );
  }
}

/************************************************************************
     Description: This function displays the lastest RPG system log
                  messages in the RPG Status window.  Each message
                  is color coded based on its type.
 ************************************************************************/

static void Display_system_status_msgs( int mode )
{
  int i = 0;
  int cnt = 0;
  int msg_cnt = 0;
  char buf[256];
  int month = 0;
  int day = 0;
  int year = 0;
  int hour = 0;
  int minute = 0;
  int second = 0;
  int value = 0;
  int slider_size = 0;
  static int old_cnt = 0;
  static int old_filter_status = 0;
  char *ptr = NULL;
  Widget vsb = (Widget) NULL;
  LE_critical_message *le_msg;

  /* If the message list hasn't been created, create it now. */

  if( Msg_form == (Widget) NULL )
  {
    Msg_form = XtVaCreateWidget( "Msg_form",
                xmFormWidgetClass, Msg_scroll,
                XmNbackground, Base_bg,
                XmNverticalSpacing, 0,
                NULL );

    Msg_entry[0] = XtVaCreateManagedWidget( "label",
                xmTextWidgetClass, Msg_form,
                XmNfontList, List_font,
                XmNcolumns, 128,
                XmNeditable, False,
                XmNleftAttachment, XmATTACH_FORM,
                XmNrightAttachment, XmATTACH_FORM,
                XmNtopAttachment, XmATTACH_FORM,
                XmNmarginHeight, 0,
                XmNborderWidth, 0,
                XmNshadowThickness, 0,
                XmNhighlightColor, Black_color,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                NULL );

    XtAddCallback( Msg_entry[0],
                XmNfocusCallback, Select_log_message_callback,
                (XtPointer) 0 );

    for( i = 1; i < MAX_MSGS_DISPLAY; i++ )
    {
      Msg_entry[i] = XtVaCreateManagedWidget( "label",
                xmTextWidgetClass, Msg_form,
                XmNfontList, List_font,
                XmNcolumns, 128,
                XmNeditable, False,
                XmNleftAttachment, XmATTACH_FORM,
                XmNrightAttachment, XmATTACH_FORM,
                XmNtopAttachment, XmATTACH_WIDGET,
                XmNtopWidget, Msg_entry[i-1],
                XmNmarginHeight, 0,
                XmNborderWidth,  0,
                XmNshadowThickness, 0,
                XmNhighlightColor, Black_color,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                NULL );

      XtAddCallback( Msg_entry[i],
                XmNfocusCallback, Select_log_message_callback,
                (XtPointer) i );
    }
  }

  msg_cnt = 0;
  cnt = 0;

  /* For each message read from the log, put it in the list and set
     the background color based on meaage severity. */

  for( i = Msgs_read-1; i >= 0; i-- )
  {
    if( cnt >= MAX_MSGS_DISPLAY ){ break; }

    le_msg = (LE_critical_message *) Sys_msg[i];

    unix_time( &le_msg->time, &year, &month, &day, &hour, &minute, &second );

    year = year%100;

    ptr = strstr( le_msg->text, ":" );

    if( ptr == NULL ){ ptr = le_msg->text; }
    else{ ptr = ptr + 1; }

    sprintf( buf,"%s %d,%2.2d [%2.2d:%2.2d:%2.2d] >> %s",
                HCI_get_month( month ), day, year, hour,
                minute, second, ptr );

    if( !strlen( Search_string ) ||
        ( strlen( Search_string ) &&
          hci_string_in_string( buf, Search_string ) != 0 ) )
    {
      if( ( Msg_type[i] & Filter_status ) == Msg_type[i] )
      {
        if( msg_cnt >= Active_page*MAX_MSGS_DISPLAY )
        {
          if( ( Msg_type[i] & FILTER_RPG_INFO_STATUS_MSG ) == FILTER_RPG_INFO_STATUS_MSG )
          {
            XtVaSetValues( Msg_entry[cnt],
                XmNforeground, Text_fg,
                XmNbackground, Gray_color,
                NULL );
          }
          else if( ( Msg_type[i] & FILTER_RPG_GENERAL_STATUS_MSG ) == FILTER_RPG_GENERAL_STATUS_MSG )
          {
            XtVaSetValues( Msg_entry[cnt],
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                NULL );
          }
          else if( ( Msg_type[i] & FILTER_RPG_COMMS_STATUS_MSG ) == FILTER_RPG_COMMS_STATUS_MSG )
          {
            XtVaSetValues( Msg_entry[cnt],
                XmNforeground, White_color,
                XmNbackground, Seagreen_color,
                NULL );
          }
          else if( ( Msg_type[i] & FILTER_RPG_ALARM_CLEARED_MSG ) )
          {
            /* This needs to be before the checks for RPG Alarm Type to
               ensure the alarm is displayed in the proper color. */

            XtVaSetValues( Msg_entry[cnt],
                XmNforeground, Text_fg,
                XmNbackground, Normal_color,
                NULL );
          }
          else if( ( Msg_type[i] & FILTER_RPG_MAM_ALARM_ACTIVATED_MSG ) == FILTER_RPG_MAM_ALARM_ACTIVATED_MSG )
          {
            XtVaSetValues( Msg_entry[cnt],
                XmNforeground, Text_fg,
                XmNbackground, Alarm2_color,
                NULL );
          }
          else if( ( Msg_type[i] & FILTER_RPG_MAR_ALARM_ACTIVATED_MSG ) == FILTER_RPG_MAR_ALARM_ACTIVATED_MSG )
          {
            XtVaSetValues( Msg_entry[cnt],
                XmNforeground, Text_fg,
                XmNbackground, Warning_color,
                NULL );
          }
          else if( ( Msg_type[i] & FILTER_RPG_LS_ALARM_ACTIVATED_MSG ) == FILTER_RPG_LS_ALARM_ACTIVATED_MSG )
          {
            XtVaSetValues( Msg_entry[cnt],
                XmNforeground, Text_fg,
                XmNbackground, Cyan_color,
                NULL );
          }
          else if( ( Msg_type[i] & FILTER_RPG_WARNING_STATUS_MSG ) == FILTER_RPG_WARNING_STATUS_MSG )
          {
            XtVaSetValues( Msg_entry[cnt],
                XmNforeground, Text_fg,
                XmNbackground, Warning_color,
                NULL );
          }
          else if( Msg_type[i] & FILTER_RDA_ALARM_CLEARED_MSG )
          {
            /* This needs to be before the checks for RDA Alarm Type to
               ensure the alarm is displayed in the proper color. */
            XtVaSetValues( Msg_entry[cnt],
                XmNforeground, Text_fg,
                XmNbackground, Normal_color,
                NULL );
          }
          else if( ( Msg_type[i] & FILTER_RDA_NA_ALARM_ACTIVATED_MSG ) == FILTER_RDA_NA_ALARM_ACTIVATED_MSG )
          {
            XtVaSetValues( Msg_entry[cnt],
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                NULL );
          }
          else if( ( Msg_type[i] & FILTER_RDA_SEC_ALARM_ACTIVATED_MSG ) == FILTER_RDA_SEC_ALARM_ACTIVATED_MSG )
          {
            XtVaSetValues( Msg_entry[cnt],
                XmNforeground, Text_fg,
                XmNbackground, White_color,
                NULL );
          }
          else if( ( Msg_type[i] & FILTER_RDA_MAM_ALARM_ACTIVATED_MSG ) == FILTER_RDA_MAM_ALARM_ACTIVATED_MSG )
          {
            XtVaSetValues( Msg_entry[cnt],
                XmNforeground, Text_fg,
                XmNbackground, Alarm2_color,
                NULL );
          }
          else if( ( Msg_type[i] & FILTER_RDA_MAR_ALARM_ACTIVATED_MSG ) == FILTER_RDA_MAR_ALARM_ACTIVATED_MSG )
          {
            XtVaSetValues( Msg_entry[cnt],
                XmNforeground, Text_fg,
                XmNbackground, Warning_color,
                NULL );
          }
          else if( (Msg_type[i] & FILTER_RDA_INOP_ALARM_ACTIVATED_MSG ) == FILTER_RDA_INOP_ALARM_ACTIVATED_MSG )
          {
            XtVaSetValues( Msg_entry[cnt],
                XmNforeground, White_color,
                XmNbackground, Alarm_color,
                NULL );
          }
          else
          {
            XtVaSetValues( Msg_entry[cnt],
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                NULL );
          }

          XmTextSetString( Msg_entry[cnt], buf );
          XtVaSetValues( Msg_entry[cnt], XmNuserData, (XtPointer) i, NULL );
          cnt++;
        }
        msg_cnt++;
      }
    }
  }

  if( i > 0 )
  {
    XtVaSetValues( Page_right_button,
                XmNforeground, Button_fg,
                XmNbackground, Button_bg,
                XmNsensitive, True,
                NULL );
  }
  else
  {
    XtVaSetValues( Page_right_button,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNsensitive, False,
                NULL );
  }

  if( Active_page == 0 )
  {
    XtVaSetValues( Page_left_button,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNsensitive, False,
                NULL );
  }
  else
  {
    XtVaSetValues( Page_left_button,
                XmNforeground, Button_fg,
                XmNbackground, Button_bg,
                XmNsensitive, True,
                NULL );
  }

  if( cnt < old_cnt )
  {
    for( i = cnt; i < old_cnt; i++ )
    {
      XmTextSetString( Msg_entry[i], " " );
      XtVaSetValues( Msg_entry[i],
                XmNforeground, Base_bg,
                XmNbackground, Base_bg,
                XmNuserData, (XtPointer) -1,
                NULL );
    }
  }

  old_cnt = cnt;

  XtManageChild( Msg_form );
  XtManageChild( Msg_scroll );

  /* We want to get the ID of the vertical scrollbar so we can
     control the scroll increment to keep it from jumping around. */

  XtVaGetValues( Msg_scroll, XmNverticalScrollBar, &vsb, NULL );
  XtVaGetValues( vsb,
                XmNsliderSize, &slider_size,
                XmNvalue, &value,
                NULL );

  if( Filter_status != old_filter_status || mode )
  {
    old_filter_status = Filter_status;
    XmScrollBarSetValues( vsb, 1, slider_size, 10, 100, True );
  }
  else
  {
    XmScrollBarSetValues( vsb, value, slider_size, 10, 100, False );
  }
}

/************************************************************************
     Description: This function is activated when the user defines
                  a search string in the Search edit box.
 ************************************************************************/

static void Search_text_callback( Widget w, XtPointer y, XtPointer z )
{
  char *text = NULL;

  Active_page = 0;

  XtVaSetValues( Page_left_button,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNsensitive, False,
                NULL );

  text = XmTextGetString( w );
  strcpy( Search_string, text );
  XtFree( text );

  /* If the new string is dfferent from the old one, redisplay the
     system status log messages. */

  if( ( strlen( Search_string ) != strlen( Old_search_string ) ) &&
      strcmp( Search_string, Old_search_string ) )
  {
    Display_system_status_msgs( 1 );
    strcpy( Old_search_string, Search_string );
  }
}

/************************************************************************
     Description: This function is activated when the user selects
                  the "Clear" button.  The contents of the search
                  string exit box is cleared.
 ************************************************************************/

static void Clear_callback( Widget w, XtPointer y, XtPointer z )
{
  strcpy( Old_search_string, "" );
  strcpy( Search_string, "" );
  XmTextSetString( Filter_text, "" );
  Display_system_status_msgs( 1 );
}

/************************************************************************
     Description: This function is activated when a new system log
                  message is written to the system log file.
 ************************************************************************/

static void New_system_log_msg( int fd, LB_id_t mid, int minfo, void *arg )
{
  New_msg_flag = HCI_YES_FLAG;
}

/************************************************************************
     Description: This function reads all new system status log
                  messages and displays them.
 ************************************************************************/

static void Update_system_log_msgs()
{
  static int num_msgs = 0;
  int status = 0;
  int i = 0;
  char *new_msg = NULL;
  LB_status lstat;
  LE_critical_message *le_msg = NULL;
  unsigned int rpg_msg = 0;
  unsigned int rpg_alarm = 0;
  unsigned int rda_alarm = 0;

  lstat.attr = NULL;
  lstat.n_check = 0;

  if( ORPGDA_stat( ORPGDAT_SYSLOG, &lstat ) == LB_SUCCESS )
  {
    /* If the number of messages in the log is smaller than the
       last time, then we assume it has been cleaned and needs
       to be initialized. */

    if( lstat.n_msgs < num_msgs )
    {
      ORPGDA_seek( ORPGDAT_SYSLOG, 0, LB_FIRST, NULL );
      Msgs_read = 0;
    }

    num_msgs = lstat.n_msgs;
  }

  while( 1 )
  {
    status = ORPGDA_read( ORPGDAT_SYSLOG, (char *) &new_msg, LB_ALLOC_BUF, LB_NEXT );

    if( status < 0 ){ break; }

    /* if the message buffer is full, then we must expire the
       oldest message before we can add the new message.  Lets
       shift everything in the buffer one place. */

    if( Msgs_read == MAX_MSGS_BUFFER )
    {
      for( i = 1; i < MAX_MSGS_BUFFER; i++ )
      {
        memcpy( Sys_msg[i-1], Sys_msg[i], HCI_LE_MSG_MAX_LENGTH );
        Msg_type[i-1] = Msg_type[i];
      }
      Msgs_read--;
    }

    memcpy( Sys_msg[Msgs_read], new_msg, status );

    le_msg = (LE_critical_message *) new_msg;

    /* Get the RPG/RDA alarm level so we can set the appropriate type. */
    rpg_msg = ( le_msg->code & HCI_LE_RPG_STATUS_MASK );
    rpg_alarm = ( le_msg->code & HCI_LE_RPG_ALARM_MASK );
    rda_alarm = ( le_msg->code & HCI_LE_RDA_ALARM_MASK );

    if( rpg_alarm )
    {
      /* RPG alarm. */
      switch( rpg_alarm )
      {
        case HCI_LE_RPG_ALARM_MAR:
        default:
          Msg_type[Msgs_read] = FILTER_RPG_MAR_ALARM_ACTIVATED_MSG;
          break;

        case HCI_LE_RPG_ALARM_MAM:
          Msg_type[Msgs_read] = FILTER_RPG_MAM_ALARM_ACTIVATED_MSG;
          break;

        case HCI_LE_RPG_ALARM_LS:
          Msg_type[Msgs_read] = FILTER_RPG_LS_ALARM_ACTIVATED_MSG;
          break;
      }

      if( le_msg->code & HCI_LE_RPG_ALARM_CLEAR )
      {
        Msg_type[Msgs_read] |= FILTER_RPG_ALARM_CLEARED_MSG;
      }
    }
    else if( rda_alarm )
    {
      /* RDA alarm. */
      switch( rda_alarm )
      {
        case HCI_LE_RDA_ALARM_NA:
          Msg_type[Msgs_read] = FILTER_RDA_NA_ALARM_ACTIVATED_MSG;
          break;

        case HCI_LE_RDA_ALARM_SEC:
        default:
          Msg_type[Msgs_read] = FILTER_RDA_SEC_ALARM_ACTIVATED_MSG;
          break;

        case HCI_LE_RDA_ALARM_MAR:
          Msg_type[Msgs_read] = FILTER_RDA_MAR_ALARM_ACTIVATED_MSG;
          break;

        case HCI_LE_RDA_ALARM_MAM:
          Msg_type[Msgs_read] = FILTER_RDA_MAM_ALARM_ACTIVATED_MSG;
          break;

        case HCI_LE_RDA_ALARM_INOP:
          Msg_type[Msgs_read] = FILTER_RDA_INOP_ALARM_ACTIVATED_MSG;
          break;
      }

      if( le_msg->code & HCI_LE_RDA_ALARM_CLEAR )
      {
        Msg_type[Msgs_read] |= FILTER_RDA_ALARM_CLEARED_MSG;
      }
    }
    else if( rpg_msg )
    {
      if( rpg_msg == HCI_LE_RPG_STATUS_WARN )
      {
        Msg_type[Msgs_read] = FILTER_RPG_WARNING_STATUS_MSG;
      }
      else if( rpg_msg == HCI_LE_RPG_STATUS_INFO )
      {
        Msg_type[Msgs_read] = FILTER_RPG_INFO_STATUS_MSG;
      }
      else if( rpg_msg == HCI_LE_RPG_STATUS_COMMS )
      {
        Msg_type[Msgs_read] = FILTER_RPG_COMMS_STATUS_MSG;
      }
      else if( rpg_msg == HCI_LE_RPG_STATUS_GEN )
      {
        Msg_type[Msgs_read] = FILTER_RPG_GENERAL_STATUS_MSG;
      }
    }
    else
    {
      /* All other types of messages. */
      if( le_msg->code & HCI_LE_ERROR_BIT )
      {
        Msg_type[Msgs_read] = FILTER_RPG_WARNING_STATUS_MSG;
      }
      else
      {
        Msg_type[Msgs_read] = FILTER_RPG_GENERAL_STATUS_MSG;
      }
    }

    Msgs_read++;

    free( new_msg );
  }

  Display_system_status_msgs( 0 );
}

/************************************************************************
     Description: This function updates the RPG status and alarm
                  summary labels in the RPG Status window.
 ************************************************************************/

static void Display_system_status()
{
  char buf[32];
  int status = 0;
  unsigned int value = 0;
  int fg_color = 0;
  int bg_color = 0;
  int cur_value = 0;
  int alarm_threshold = 0;
  Mrpg_state_t mrpg;
  XmString str;

  /* Get the current RPG status published by the manage RPG task */

  if( ( status = ORPGMGR_get_RPG_states( &mrpg ) ) == 0 )
  {
    /* Display the RPG state */
    switch( mrpg.state )
    {
      case MRPG_ST_SHUTDOWN :

        fg_color = White_color;
        bg_color = Alarm_color;
        sprintf( buf, "    SHUTDOWN     " );
        break;

      case MRPG_ST_STANDBY :

        fg_color = White_color;
        bg_color = Alarm_color;
        sprintf( buf, "     STANDBY      " );
        break;

      case MRPG_ST_OPERATING :

        fg_color = Text_fg,
        bg_color = Normal_color,
        sprintf( buf, "     OPERATE     " );
        break;

      case MRPG_ST_TRANSITION :

        fg_color = Text_fg,
        bg_color = Warning_color,
        sprintf( buf, "   TRANSITION    " );
        break;

      case MRPG_ST_FAILED :

        fg_color = White_color;
        bg_color = Alarm_color;
        sprintf( buf, "     FAILED      " );
        break;

      case MRPG_ST_POWERFAIL :

        fg_color = White_color;
        bg_color = Alarm_color;
        sprintf( buf, "    POWERFAIL    " );
        break;

      default :

        fg_color = White_color;
        bg_color = Alarm_color;
        sprintf( buf, "     UNKNOWN     " );
        break;
    }

    str = XmStringCreateLocalized( buf );
    XtVaSetValues( Rpg_state_label,
                XmNlabelString, str,
                XmNforeground, fg_color,
                XmNbackground, bg_color,
                NULL );
    XmStringFree( str );
  }

  /* Get the current active weather mode and display it. */

  value = ORPGVST_get_mode();

  switch( value )
  {
    case PRECIPITATION_MODE :

      fg_color = Text_fg;
      bg_color = Normal_color;
      strcpy( buf, "PRECIPITATION (A)" );
      break;

    case CLEAR_AIR_MODE :

      fg_color = Text_fg;
      bg_color = Normal_color;
      strcpy( buf, "  CLEAR AIR (B)  " );
      break;

    default:

      fg_color = White_color;
      bg_color = Alarm_color;
      strcpy( buf, "     UNKNOWN     " );
      break;
  }

  str = XmStringCreateLocalized( buf );
  XtVaSetValues( Wx_mode_label,
                XmNlabelString, str,
                XmNforeground, fg_color,
                XmNbackground, bg_color,
                NULL );
  XmStringFree( str );

  /* Set the various RPG alarm fields. */

  if( ( status = ORPGINFO_statefl_get_rpgalrm( &value ) ) == 0 )
  {
    if( value & ORPGINFO_STATEFL_RPGALRM_PRDSTGLS )
    {
      fg_color = Text_fg;
      bg_color = Cyan_color;
    }
    else
    {
      fg_color = Text_fg;
      bg_color = Normal_color;
    }

    XtVaSetValues( Load_shed_prod_storage,
                XmNforeground, fg_color,
                XmNbackground, bg_color,
                NULL );

    if( value & ORPGINFO_STATEFL_RPGALRM_RDAINLS )
    {
      fg_color = Text_fg;
      bg_color = Cyan_color;
    }
    else
    {
      fg_color = Text_fg;
      bg_color = Normal_color;
    }

    XtVaSetValues( Load_shed_rda_radial,
                XmNforeground, fg_color,
                XmNbackground, bg_color,
                NULL );

    if( value & ORPGINFO_STATEFL_RPGALRM_RPGINLS )
    {
      fg_color = Text_fg;
      bg_color = Cyan_color;
    }
    else
    {
      fg_color = Text_fg;
      bg_color = Normal_color;
    }

    XtVaSetValues( Load_shed_rpg_radial,
                XmNforeground, fg_color,
                XmNbackground, bg_color,
                NULL );

    if( value & ORPGINFO_STATEFL_RPGALRM_DISTRI )
    {
      fg_color = Text_fg;
      bg_color = Warning_color;
    }
    else
    {
      fg_color = Text_fg;
      bg_color = Normal_color;
    }

    XtVaSetValues( Maint_req_narrowband,
                XmNforeground, fg_color,
                XmNbackground, bg_color,
                NULL );

    if( System_flag == HCI_NWS_SYSTEM || System_flag == HCI_NWSR_SYSTEM )
    {
      if( value & ORPGINFO_STATEFL_RPGALRM_RDAWB )
      {
        fg_color = Text_fg;
        bg_color = Warning_color;
      }
      else
      {
        fg_color = Text_fg;
        bg_color = Normal_color;
      }

      XtVaSetValues( Maint_req_RDA_wideband,
                  XmNforeground, fg_color,
                  XmNbackground, bg_color,
                  NULL );
    }

    if( value & ORPGINFO_STATEFL_RPGALRM_RPGTSKFL )
    {
      fg_color = Text_fg;
      bg_color = Warning_color;
    }
    else
    {
      fg_color = Text_fg;
      bg_color = Normal_color;
    }

    XtVaSetValues( Maint_req_task_failure,
                XmNforeground, fg_color,
                XmNbackground, bg_color,
                NULL );

    if( value & ORPGINFO_STATEFL_RPGALRM_RPGCTLFL )
    {
      fg_color = Text_fg;
      bg_color = Alarm2_color;
    }
    else
    {
      fg_color = Text_fg;
      bg_color = Normal_color;
    }

    XtVaSetValues( Maint_man_task_failure,
                XmNforeground, fg_color,
                XmNbackground, bg_color,
                NULL );

    if( value & ORPGINFO_STATEFL_RPGALRM_NODE_CON )
    {
      fg_color = Text_fg;
      bg_color = Alarm2_color;
    }
    else
    {
      fg_color = Text_fg;
      bg_color = Normal_color;
    }

    XtVaSetValues( Maint_man_node_connect,
                XmNforeground, fg_color,
                XmNbackground, bg_color,
                NULL );

    if( value & ORPGINFO_STATEFL_RPGALRM_DBFL )
    {
      fg_color = Text_fg;
      bg_color = Warning_color;
    }
    else
    {
      fg_color = Text_fg;
      bg_color = Normal_color;
    }

    XtVaSetValues( Maint_req_replay_db,
                XmNforeground, fg_color,
                XmNbackground, bg_color,
                NULL );

    if( value & ORPGINFO_STATEFL_RPGALRM_WBFAILRE )
    {
      fg_color = Text_fg;
      bg_color = Alarm2_color;
    }
    else
    {
      fg_color = Text_fg;
      bg_color = Normal_color;
    }

    XtVaSetValues( Maint_man_RDA_wideband,
                XmNforeground, fg_color,
                XmNbackground, bg_color,
                NULL );

    if( value & ORPGINFO_STATEFL_RPGALRM_MEDIAFL )
    {
      fg_color = Text_fg;
      bg_color = Alarm2_color;
    }
    else
    {
      fg_color = Text_fg;
      bg_color = Normal_color;
    }

    XtVaSetValues( Maint_man_media_failure,
                XmNforeground, fg_color,
                XmNbackground, bg_color,
                NULL );


    if( System_flag == HCI_FAA_SYSTEM )
    {
      if( value & ORPGINFO_STATEFL_RPGALRM_RPGRPGFL )
      {
        fg_color = Text_fg;
        bg_color = Warning_color;
      }
      else
      {
        fg_color = Text_fg;
        bg_color = Normal_color;
      }

      XtVaSetValues( Maint_req_RPG_link_fail,
                XmNforeground, fg_color,
                XmNbackground, bg_color,
                NULL );

      if( value & ORPGINFO_STATEFL_RPGALRM_REDCHNER )
      {
        fg_color = Text_fg;
        bg_color = Warning_color;
      }
      else
      {
        fg_color = Text_fg;
        bg_color = Normal_color;
      }

      XtVaSetValues( Maint_req_red_chan_error,
                XmNforeground, fg_color,
                XmNbackground, bg_color,
                NULL );

    }
  }

  /* Narrowband load shed is not set as an RPG alarm like the other
     load shed alarms.  Here we need to read the current narrowband
     utilization and compare it against the alarm value and set the
     colors accordingly. */

  status = ORPGLOAD_get_data( LOAD_SHED_CATEGORY_PROD_DIST,
                              LOAD_SHED_CURRENT_VALUE,
                              &cur_value );

  if( status == 0 )
  {
    status = ORPGLOAD_get_data( LOAD_SHED_CATEGORY_PROD_DIST,
                                LOAD_SHED_ALARM_THRESHOLD,
                                &alarm_threshold );

    if( status == 0 )
    {
      if( cur_value >= alarm_threshold )
      {
        fg_color = Text_fg;
        bg_color = Cyan_color;
      }
      else
      {
        fg_color = Text_fg;
        bg_color = Normal_color;
      }

      XtVaSetValues( Load_shed_narrowband,
                XmNforeground, fg_color,
                XmNbackground, bg_color,
                NULL );
    }
  }
}

/************************************************************************
     Description: This function is activated when the user selects
                  a system log message from the list.
 ************************************************************************/

static void Select_log_message_callback( Widget w, XtPointer y, XtPointer z )
{
  XtPointer data = (XtPointer) NULL;

  XtVaGetValues( w, XmNuserData, &data, NULL );

  List_index = (int) data;
}

/************************************************************************
     Description: This function is activated when the user selects
                  one of the Message Filter Display check boxes.
 ************************************************************************/

static void Filter_button_callback( Widget w, XtPointer y, XtPointer z )
{
  XmToggleButtonCallbackStruct *state;

  state = (XmToggleButtonCallbackStruct *) z;

  Active_page = 0;

  XtVaSetValues( Page_left_button,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNsensitive, False,
                NULL );

  /* make sure to change the state of the rda and rpg buttons */
  Toggle_rda_alarms_button( w );
  Toggle_rpg_alarms_button( w );
  Toggle_rpg_msgs_button( w );

  if( state->set )
  {
    Filter_status = Filter_status | ( (int) y );
  }
  else
  {
    Filter_status = Filter_status & ( ~( (int) y ) );
  }

  Display_system_status_msgs( 1 );
}

/************************************************************************
     Description: This function is activated when the user selects the
                  "Task Failure" button in the RPG Alarm Summary table.
 ************************************************************************/

static void Popup_failed_task_list( Widget w, XtPointer y, XtPointer z )
{
  Widget form = (Widget) NULL;
  Widget form_rowcol = (Widget) NULL;
  Widget control_rowcol = (Widget) NULL;
  Widget frame = (Widget) NULL;
  Widget scroll = (Widget) NULL;
  Widget task_form = (Widget) NULL;
  Widget label = (Widget) NULL;
  Widget button = (Widget) NULL;
  Widget clip = (Widget) NULL;
  int i = 0;

  HCI_LE_log( "Display failed task list selected" );

  /* If the window is already defined, do nothing. */

  if (Failed_task_dialog != NULL )
  {
    HCI_Shell_popup( Failed_task_dialog );
    return;
  }

  /* Create a popup dialog shell for window. */

  HCI_Shell_init( &Failed_task_dialog, "Failed Task List" );

  /* Use a form to manage the contents of the window. */

  form = XtVaCreateWidget( "form",
                xmFormWidgetClass, Failed_task_dialog,
                XmNbackground, Base_bg,
                NULL );

  form_rowcol = XtVaCreateManagedWidget( "form_rowcol",
                xmRowColumnWidgetClass, form,
                XmNbackground, Base_bg,
                XmNorientation, XmVERTICAL,
                XmNpacking, XmPACK_TIGHT,
                XmNnumColumns, 1,
                NULL );

  /* Control button(s). */

  control_rowcol = XtVaCreateManagedWidget( "control_rowcol",
                xmRowColumnWidgetClass, form_rowcol,
                XmNbackground, Base_bg,
                XmNorientation, XmHORIZONTAL,
                XmNpacking, XmPACK_COLUMN,
                XmNnumColumns, 1,
                NULL );

  button = XtVaCreateManagedWidget( "Close",
                xmPushButtonWidgetClass, control_rowcol,
                XmNforeground, Button_fg,
                XmNbackground, Button_bg,
                XmNfontList, List_font,
                XmNtopAttachment, XmATTACH_WIDGET,
                XmNleftAttachment, XmATTACH_WIDGET,
                NULL );

  XtAddCallback( button, XmNactivateCallback, Close_failed_task_dialog, NULL );

  frame = XtVaCreateManagedWidget( "frame",
                xmFrameWidgetClass, form_rowcol,
                XmNbackground, Base_bg,
                XmNtopAttachment, XmATTACH_WIDGET,
                XmNtopWidget, button,
                XmNleftAttachment, XmATTACH_FORM,
                XmNrightAttachment, XmATTACH_FORM,
                XmNbottomAttachment, XmATTACH_FORM,
                NULL );

  label = XtVaCreateManagedWidget( "Failed Tasks",
                xmLabelWidgetClass, frame,
                XmNchildType, XmFRAME_TITLE_CHILD,
                XmNchildHorizontalAlignment, XmALIGNMENT_CENTER,
                XmNchildVerticalAlignment, XmALIGNMENT_CENTER,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                NULL );

  scroll = XtVaCreateManagedWidget( "scroll",
                xmScrolledWindowWidgetClass, frame,
                XmNheight, 100,
                XmNwidth, 250,
                XmNscrollingPolicy, XmAUTOMATIC,
                XmNforeground, Base_bg,
                NULL );

  XtVaGetValues( scroll, XmNclipWindow, &clip, NULL );

  XtVaSetValues( clip,
                XmNforeground, Base_bg,
                XmNbackground, Base_bg,
                NULL );

  task_form = XtVaCreateWidget( "task_form",
                xmFormWidgetClass, scroll,
                XmNbackground, Base_bg,
                XmNverticalSpacing, 0,
                NULL );

  Failed_task_entry[0] = XtVaCreateManagedWidget( "label",
                xmLabelWidgetClass, task_form,
                XmNforeground, Text_fg,
                XmNbackground, Text_bg,
                XmNfontList, List_font,
                XmNtopAttachment, XmATTACH_FORM,
                XmNleftAttachment, XmATTACH_FORM,
                XmNrightAttachment, XmATTACH_FORM,
                NULL );

  for( i = 1; i < MAX_FAILED_TASKS_DISPLAY; i++ )
  {
    Failed_task_entry[i] = XtVaCreateManagedWidget( "label",
                xmLabelWidgetClass, task_form,
                XmNforeground, Text_fg,
                XmNbackground, Text_bg,
                XmNfontList, List_font,
                XmNtopAttachment, XmATTACH_WIDGET,
                XmNtopWidget, Failed_task_entry[i-1],
                XmNleftAttachment, XmATTACH_FORM,
                XmNrightAttachment, XmATTACH_FORM,
                NULL );
  }

  Display_failed_task_list();

  XtManageChild( form );
  XtManageChild( task_form );

  HCI_Shell_start( Failed_task_dialog, NO_RESIZE_HCI );
}

/************************************************************************
     Description: This function is activated when the user selects
                  the "Close" button from the Failed Task List window.
 ************************************************************************/

static void Close_failed_task_dialog( Widget w, XtPointer y, XtPointer z )
{
  HCI_Shell_popdown( Failed_task_dialog );
}

/************************************************************************
     Description: This function is activated when the user selects
                  the "Control Task Failure" button in the RPG Alarm
                  summary table.
 ************************************************************************/

static void Popup_failed_control_task_list( Widget w, XtPointer y, XtPointer z )
{
  Widget form = (Widget) NULL;
  Widget form_rowcol = (Widget) NULL;
  Widget control_rowcol = (Widget) NULL;
  Widget frame = (Widget) NULL;
  Widget scroll = (Widget) NULL;
  Widget task_form = (Widget) NULL;
  Widget label = (Widget) NULL;
  Widget button = (Widget) NULL;
  Widget clip = (Widget) NULL;
  int i = 0;

  HCI_LE_log( "Display failed control task list selected" );

  /* If the window is already defined, do nothing. */

  if( Failed_control_task_dialog != NULL )
  {
    HCI_Shell_popup( Failed_control_task_dialog );
    return;
  }

  /* Create a popup dialog shell for window. */

  HCI_Shell_init( &Failed_control_task_dialog, "Failed Control Task List" );

  /* Use a form to manage the contents of the window. */

  form = XtVaCreateWidget( "form",
                xmFormWidgetClass, Failed_control_task_dialog,
                XmNbackground, Base_bg,
                NULL );

  form_rowcol = XtVaCreateManagedWidget( "form_rowcol",
                xmRowColumnWidgetClass, form,
                XmNbackground, Base_bg,
                XmNorientation, XmVERTICAL,
                XmNpacking, XmPACK_TIGHT,
                XmNnumColumns, 1,
                NULL );

  /* Control button(s). */

  control_rowcol = XtVaCreateManagedWidget( "control_rowcol",
                xmRowColumnWidgetClass, form_rowcol,
                XmNbackground, Base_bg,
                XmNorientation, XmHORIZONTAL,
                XmNpacking, XmPACK_COLUMN,
                XmNnumColumns, 1,
                NULL );

  button = XtVaCreateManagedWidget( "Close",
                xmPushButtonWidgetClass, control_rowcol,
                XmNforeground, Button_fg,
                XmNbackground, Button_bg,
                XmNfontList, List_font,
                XmNtopAttachment, XmATTACH_WIDGET,
                XmNleftAttachment, XmATTACH_WIDGET,
                NULL );

  XtAddCallback( button, XmNactivateCallback, Close_failed_control_task_dialog, NULL );

  frame = XtVaCreateManagedWidget( "frame",
                xmFrameWidgetClass, form_rowcol,
                XmNbackground, Base_bg,
                XmNtopAttachment, XmATTACH_WIDGET,
                XmNtopWidget, button,
                XmNleftAttachment, XmATTACH_FORM,
                XmNrightAttachment, XmATTACH_FORM,
                XmNbottomAttachment, XmATTACH_FORM,
                NULL );

  label = XtVaCreateManagedWidget( "Failed Control Tasks",
                xmLabelWidgetClass, frame,
                XmNchildType, XmFRAME_TITLE_CHILD,
                XmNchildHorizontalAlignment, XmALIGNMENT_CENTER,
                XmNchildVerticalAlignment, XmALIGNMENT_CENTER,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                NULL );

  scroll = XtVaCreateManagedWidget( "scroll",
                xmScrolledWindowWidgetClass, frame,
                XmNheight, 100,
                XmNwidth, 250,
                XmNscrollingPolicy, XmAUTOMATIC,
                XmNforeground, Base_bg,
                NULL );

  XtVaGetValues( scroll, XmNclipWindow, &clip, NULL );

  XtVaSetValues( clip,
                XmNforeground, Base_bg,
                XmNbackground, Base_bg,
                NULL );

  task_form = XtVaCreateWidget( "task_form",
                xmFormWidgetClass, scroll,
                XmNbackground, Base_bg,
                XmNverticalSpacing, 0,
                NULL );

  Failed_control_task_entry[0] = XtVaCreateManagedWidget( "label",
                xmLabelWidgetClass, task_form,
                XmNforeground, Text_fg,
                XmNbackground, Text_bg,
                XmNfontList, List_font,
                XmNtopAttachment, XmATTACH_FORM,
                XmNleftAttachment, XmATTACH_FORM,
                XmNrightAttachment, XmATTACH_FORM,
                NULL );

  for( i = 1; i < MAX_FAILED_TASKS_DISPLAY; i++ )
  {
    Failed_control_task_entry[i] = XtVaCreateManagedWidget( "label",
                xmLabelWidgetClass, task_form,
                XmNforeground, Text_fg,
                XmNbackground, Text_bg,
                XmNfontList, List_font,
                XmNtopAttachment, XmATTACH_WIDGET,
                XmNtopWidget, Failed_control_task_entry[i-1],
                XmNleftAttachment, XmATTACH_FORM,
                XmNrightAttachment, XmATTACH_FORM,
                NULL );
  }

  Display_failed_control_task_list();

  XtManageChild( form );
  XtManageChild( task_form );

  HCI_Shell_start( Failed_control_task_dialog, NO_RESIZE_HCI );
}

/************************************************************************
     Description: This function is activated when the user selects
                  the "Close" button from the Failed Control Task
                  List window.
 ************************************************************************/

static void Close_failed_control_task_dialog( Widget w, XtPointer y, XtPointer z )
{
  HCI_Shell_popdown( Failed_control_task_dialog );
}

/************************************************************************
     Description: This function is activated when one selects one of
                  the message list page arrow buttons.
 ************************************************************************/

static void Hci_page_button_callback( Widget w, XtPointer y, XtPointer z )
{
  switch( (int) y )
  {
    case XmARROW_LEFT:

      Active_page--;
      if( Active_page < 0 ){ Active_page = 0; }
      break;

    case XmARROW_RIGHT:

      Active_page++;
      break;
  }

  Display_system_status_msgs( 1 );
}

/************************************************************************
     Description: This function is activated when the user selects
                  the Message Filter button.
 ************************************************************************/

static void Hci_select_msgs_callback( Widget w, XtPointer y, XtPointer z )
{
  Widget form = (Widget) NULL;
  Widget form_rowcol = (Widget) NULL;
  Widget control_rowcol = (Widget) NULL;
  Widget rpg_msgs_rowcol = (Widget) NULL;
  Widget rpg_msgs_filter_frame = (Widget) NULL;
  Widget rda_alarm_rowcol = (Widget) NULL;
  Widget rda_alarm_filter_frame = (Widget) NULL;
  Widget rpg_alarm_rowcol = (Widget) NULL;
  Widget rpg_alarm_filter_frame = (Widget) NULL;
  Widget button = (Widget) NULL;
  Widget label = (Widget) NULL;

  /* If the window is already defined, do nothing. */

  if( Filter_msgs_dialog != NULL )
  {
    HCI_Shell_popup( Filter_msgs_dialog );
    return;
  }

  /* Create a popup dialog shell for window. */

  HCI_Shell_init( &Filter_msgs_dialog, "Message Filter" );

  /* Use a form to manage the contents of the window. */

  form = XtVaCreateWidget( "filter_msgs_form",
                xmFormWidgetClass, Filter_msgs_dialog,
                XmNbackground, Base_bg,
                NULL );

  form_rowcol = XtVaCreateManagedWidget( "control_rowcol",
                xmRowColumnWidgetClass, form,
                XmNbackground, Base_bg,
                XmNorientation, XmVERTICAL,
                XmNpacking, XmPACK_TIGHT,
                XmNnumColumns, 1,
                NULL );

  /* Control button(s). */

  control_rowcol = XtVaCreateManagedWidget( "control_rowcol",
                xmRowColumnWidgetClass, form_rowcol,
                XmNbackground, Base_bg,
                XmNorientation, XmHORIZONTAL,
                XmNpacking, XmPACK_COLUMN,
                XmNnumColumns, 1,
                NULL );

  button = XtVaCreateManagedWidget( "Close",
                xmPushButtonWidgetClass, control_rowcol,
                XmNforeground, White_color,
                XmNbackground, Button_bg,
                XmNfontList, List_font,
                NULL );

  XtAddCallback( button, XmNactivateCallback, Hci_close_msgs_dialog_callback, NULL );

  /* RPG Messages. */

  rpg_msgs_filter_frame = XtVaCreateManagedWidget( "rpg_msgs_frame",
                xmFrameWidgetClass, form_rowcol,
                XmNbackground, Base_bg,
                XmNtopAttachment, XmATTACH_WIDGET,
                XmNtopWidget, control_rowcol,
                XmNleftAttachment, XmATTACH_FORM,
                XmNrightAttachment, XmATTACH_FORM,
                NULL );

  label = XtVaCreateManagedWidget( "Select RPG Message",
                xmLabelWidgetClass, rpg_msgs_filter_frame,
                XmNchildType, XmFRAME_TITLE_CHILD,
                XmNchildHorizontalAlignment, XmALIGNMENT_CENTER,
                XmNchildVerticalAlignment, XmALIGNMENT_CENTER,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                NULL );

  rpg_msgs_rowcol = XtVaCreateManagedWidget( "rpg_msgs_rowcol",
                xmRowColumnWidgetClass, rpg_msgs_filter_frame,
                XmNbackground, Base_bg,
                XmNorientation, XmVERTICAL,
                XmNpacking, XmPACK_COLUMN,
                XmNnumColumns, 1,
                XmNleftAttachment, XmATTACH_WIDGET,
                XmNrightAttachment, XmATTACH_WIDGET,
                NULL );

  Rpg_info_button = XtVaCreateManagedWidget( "Informational",
                xmToggleButtonWidgetClass, rpg_msgs_rowcol,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNselectColor, Gray_color,
                XmNindicatorOn, XmINDICATOR_CHECK_BOX,
                XmNfontList, List_font,
                XmNset, RPG_info_stat,
                NULL );

  XtAddCallback( Rpg_info_button,
                XmNvalueChangedCallback, Filter_button_callback,
                (XtPointer) FILTER_RPG_INFO_STATUS_MSG );

  Rpg_gen_status_button = XtVaCreateManagedWidget( "General Status",
                xmToggleButtonWidgetClass, rpg_msgs_rowcol,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNfillOnSelect, False,
                XmNselectColor, Base_bg,
                XmNindicatorOn, XmINDICATOR_CHECK_BOX,
                XmNfontList, List_font,
                XmNset, RPG_gen_stat,
                NULL );

  XtAddCallback( Rpg_gen_status_button,
                XmNvalueChangedCallback, Filter_button_callback,
                (XtPointer) FILTER_RPG_GENERAL_STATUS_MSG );

  Rpg_comms_status_button = XtVaCreateManagedWidget( "Narrowband Communications",
                xmToggleButtonWidgetClass, rpg_msgs_rowcol,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNselectColor, Seagreen_color,
                XmNindicatorOn, XmINDICATOR_CHECK_BOX,
                XmNfontList, List_font,
                XmNset, RPG_comms_stat,
                NULL );

  XtAddCallback( Rpg_comms_status_button,
                XmNvalueChangedCallback, Filter_button_callback,
                (XtPointer) FILTER_RPG_COMMS_STATUS_MSG );

  Rpg_warn_status_button = XtVaCreateManagedWidget( "Warnings/Errors",
                xmToggleButtonWidgetClass, rpg_msgs_rowcol,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNselectColor, Warning_color,
                XmNindicatorOn, XmINDICATOR_CHECK_BOX,
                XmNfontList, List_font,
                XmNset, RPG_warn_stat,
                NULL );

  XtAddCallback( Rpg_warn_status_button,
                XmNvalueChangedCallback, Filter_button_callback,
                (XtPointer) FILTER_RPG_WARNING_STATUS_MSG );

  /* Empty label for spacing. */

  XtVaCreateManagedWidget( " ",
              xmLabelWidgetClass, form_rowcol,
              XmNforeground, Base_bg,
              XmNbackground, Base_bg,
              NULL );

  /* RPG Alarms. */

  rpg_alarm_filter_frame = XtVaCreateManagedWidget( "rpg_alarm_frame",
                xmFrameWidgetClass, form_rowcol,
                XmNbackground, Base_bg,
                XmNtopAttachment, XmATTACH_WIDGET,
                XmNtopWidget, rpg_msgs_filter_frame,
                XmNleftAttachment, XmATTACH_FORM,
                XmNrightAttachment, XmATTACH_FORM,
                NULL );

  label = XtVaCreateManagedWidget( "Select RPG Alarm",
                xmLabelWidgetClass, rpg_alarm_filter_frame,
                XmNchildType, XmFRAME_TITLE_CHILD,
                XmNchildHorizontalAlignment, XmALIGNMENT_CENTER,
                XmNchildVerticalAlignment, XmALIGNMENT_CENTER,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                NULL );

  rpg_alarm_rowcol = XtVaCreateManagedWidget( "rpg_alarm_rowcol",
                xmRowColumnWidgetClass, rpg_alarm_filter_frame,
                XmNbackground, Base_bg,
                XmNorientation, XmVERTICAL,
                XmNpacking, XmPACK_COLUMN,
                XmNnumColumns, 1,
                XmNleftAttachment, XmATTACH_WIDGET,
                XmNrightAttachment, XmATTACH_WIDGET,
                NULL );

  Rpg_alarm_LS_button = XtVaCreateManagedWidget( "Load Shed (LS)",
                xmToggleButtonWidgetClass, rpg_alarm_rowcol,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNselectColor, Cyan_color,
                XmNindicatorOn, XmINDICATOR_CHECK_BOX,
                XmNfontList, List_font,
                XmNset, RPG_LS_alarm_stat,
                NULL );

  XtAddCallback( Rpg_alarm_LS_button,
                XmNvalueChangedCallback, Filter_button_callback,
                (XtPointer) FILTER_RPG_LS_ALARM_ACTIVATED_MSG );

  Rpg_alarm_MR_button = XtVaCreateManagedWidget( "Maintenance Required (MR)",
                xmToggleButtonWidgetClass, rpg_alarm_rowcol,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNselectColor, Warning_color,
                XmNindicatorOn, XmINDICATOR_CHECK_BOX,
                XmNfontList, List_font,
                XmNset, RPG_MR_stat,
                NULL );

  XtAddCallback( Rpg_alarm_MR_button,
                XmNvalueChangedCallback, Filter_button_callback,
                (XtPointer) FILTER_RPG_MAR_ALARM_ACTIVATED_MSG );

  Rpg_alarm_MM_button = XtVaCreateManagedWidget( "Maintenance Mandatory (MM)",
                xmToggleButtonWidgetClass, rpg_alarm_rowcol,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNselectColor, Alarm2_color,
                XmNindicatorOn, XmINDICATOR_CHECK_BOX,
                XmNfontList, List_font,
                XmNset, RPG_MM_stat,
                NULL );

  XtAddCallback( Rpg_alarm_MM_button,
                XmNvalueChangedCallback, Filter_button_callback,
                (XtPointer) FILTER_RPG_MAM_ALARM_ACTIVATED_MSG );

  /* Empty label for spacing. */

  XtVaCreateManagedWidget( " ",
                xmLabelWidgetClass, form_rowcol,
                XmNforeground, Base_bg,
                XmNbackground, Base_bg,
                NULL );

  /* RDA Alarms. */

  rda_alarm_filter_frame = XtVaCreateManagedWidget( "rda_alarm_frame",
                xmFrameWidgetClass, form_rowcol,
                XmNbackground, Base_bg,
                XmNtopAttachment, XmATTACH_WIDGET,
                XmNtopWidget, rpg_alarm_filter_frame,
                XmNleftAttachment, XmATTACH_FORM,
                XmNrightAttachment, XmATTACH_FORM,
                NULL );

  label = XtVaCreateManagedWidget( "Select RDA Alarm",
                xmLabelWidgetClass, rda_alarm_filter_frame,
                XmNchildType, XmFRAME_TITLE_CHILD,
                XmNchildHorizontalAlignment, XmALIGNMENT_CENTER,
                XmNchildVerticalAlignment, XmALIGNMENT_CENTER,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                NULL );

  rda_alarm_rowcol = XtVaCreateManagedWidget( "rda_alarm_rowcol",
                xmRowColumnWidgetClass, rda_alarm_filter_frame,
                XmNbackground, Base_bg,
                XmNorientation, XmVERTICAL,
                XmNpacking, XmPACK_COLUMN,
                XmNnumColumns, 1,
                XmNleftAttachment, XmATTACH_WIDGET,
                XmNrightAttachment, XmATTACH_WIDGET,
                NULL );

  Rda_alarm_SEC_button = XtVaCreateManagedWidget( "Secondary (SEC)",
                xmToggleButtonWidgetClass, rda_alarm_rowcol,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNselectColor, White_color,
                XmNindicatorOn, XmINDICATOR_CHECK_BOX,
                XmNfontList, List_font,
                XmNset, RDA_SEC_stat,
                NULL );

  XtAddCallback( Rda_alarm_SEC_button,
                XmNvalueChangedCallback, Filter_button_callback,
                (XtPointer) FILTER_RDA_SEC_ALARM_ACTIVATED_MSG );

  Rda_alarm_MR_button = XtVaCreateManagedWidget( "Maintenance Required (MR)",
                xmToggleButtonWidgetClass, rda_alarm_rowcol,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNselectColor, Warning_color,
                XmNindicatorOn, XmINDICATOR_CHECK_BOX,
                XmNfontList, List_font,
                XmNset, RDA_MR_stat,
                NULL );

  XtAddCallback( Rda_alarm_MR_button,
                XmNvalueChangedCallback, Filter_button_callback,
                (XtPointer) FILTER_RDA_MAR_ALARM_ACTIVATED_MSG );

  Rda_alarm_MM_button = XtVaCreateManagedWidget( "Maintenance Mandatory (MM)",
                xmToggleButtonWidgetClass, rda_alarm_rowcol,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNselectColor, Alarm2_color,
                XmNindicatorOn, XmINDICATOR_CHECK_BOX,
                XmNfontList, List_font,
                XmNset, RDA_MM_stat,
                NULL );

  XtAddCallback( Rda_alarm_MM_button,
                XmNvalueChangedCallback, Filter_button_callback,
                (XtPointer) FILTER_RDA_MAM_ALARM_ACTIVATED_MSG );

  Rda_alarm_INOP_button = XtVaCreateManagedWidget( "Inoperable (INOP)",
                xmToggleButtonWidgetClass, rda_alarm_rowcol,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNselectColor, Alarm_color,
                XmNindicatorOn, XmINDICATOR_CHECK_BOX,
                XmNfontList, List_font,
                XmNset, RDA_INOP_stat,
                NULL );

  XtAddCallback( Rda_alarm_INOP_button,
                XmNvalueChangedCallback, Filter_button_callback,
                (XtPointer) FILTER_RDA_INOP_ALARM_ACTIVATED_MSG );

  XtManageChild( form );

  HCI_Shell_start( Filter_msgs_dialog, NO_RESIZE_HCI );
}

/************************************************************************
     Description: This function is activated when the user selects
                  the "Close" button from the Message Filter window.
 ************************************************************************/

static void Hci_close_msgs_dialog_callback( Widget w, XtPointer y, XtPointer z )
{
  HCI_Shell_popdown( Filter_msgs_dialog );
}


/************************************************************************
     Description: This function toggles the saved state of respective
                  rda alarm filter buttons.
 ************************************************************************/

static void Toggle_rda_alarms_button( Widget w )
{
  /* Just toggle the value of the variable that is passed if it is an
     rda Widget, otherwise do nothing */

  if( w == Rda_alarm_SEC_button )
  {
    RDA_SEC_stat = !RDA_SEC_stat;
  }
  else if( w ==  Rda_alarm_MR_button )
  {
    RDA_MR_stat = !RDA_MR_stat;
  }
  else if( w == Rda_alarm_MM_button )
  {
    RDA_MM_stat = !RDA_MM_stat;
  }
  else if( w == Rda_alarm_INOP_button )
  {
    RDA_INOP_stat = !RDA_INOP_stat;
  }
}

/************************************************************************
     Description: This function toggles the saved state of respective
                  respective rpg alarm filter buttons.
 ************************************************************************/

static void Toggle_rpg_alarms_button( Widget w )
{
  /* Just toggle the value of the variable that is passed if it is an
     rda Widget, otherwise do nothing */

  if( w ==  Rpg_alarm_MR_button )
  {
    RPG_MR_stat = !RPG_MR_stat;
  }
  else if( w == Rpg_alarm_MM_button )
  {
    RPG_MM_stat = !RPG_MM_stat;
  }
  else if( w == Rpg_alarm_LS_button )
  {
    RPG_LS_alarm_stat = !RPG_LS_alarm_stat;
  }
}

/************************************************************************
     Description: This function toggles the saved state of respective
                  rpg alarm filter buttons.
 ************************************************************************/

static void Toggle_rpg_msgs_button( Widget w )
{
  /* Just toggle the value of the variable that is passed if it is an rda
     Widget, otherwise do nothing */

  if( w ==  Rpg_info_button )
  {
    RPG_info_stat = !RPG_info_stat;
  }
  else if( w == Rpg_gen_status_button )
  {
    RPG_gen_stat = !RPG_gen_stat;
  }
  else if( w == Rpg_warn_status_button )
  {
    RPG_warn_stat = !RPG_warn_stat;
  }
  else if( w == Rpg_comms_status_button )
  {
    RPG_comms_stat = !RPG_comms_stat;
  }
}
