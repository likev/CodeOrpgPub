/************************************************************************
 *	Module:	hci_control_panel_system_log_messages.c			*
 *									*
 *	Description:	This module is used to display textually,	*
 *			in the HCI Status/Control Window, the latest	*
 *			system status and alarm messages from the	*
 *			system log file.				*
 *									*
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:26:01 $
 * $Id $
 * $Revision: 1.31 $
 * $State: Exp $
 */

/* Local include file definitions. */

#include <hci_control_panel.h>

/* Define macros. */

#define	STATUS_UPDATE_TIME	300

/* Global/static variables. */

static	char	Rpg_status_msg[ 256 ]; /* Latest status message buffer */
static	char	Rpg_alarm_msg[ 256 ];  /* Latest alarm message buffer */
static	char	prev_Rpg_status_msg[ 256 ]; /* Previous status message buffer */
static	char	prev_Rpg_alarm_msg[ 256 ];  /* Previous alarm message buffer */
static	time_t	system_update_time = 0; /* Time of last system msg. */
static	int	first_time = 1; /* Is this the first time through? */
static	int	alarm_fg = -1;
static	int	alarm_bg = -1;
static	int	status_fg = -1;
static	int	status_bg = -1;

/************************************************************************
 *	Description: This function displays the latest status and alarm	*
 *		     messages in the status and alarm lines of the RPG	*
 *		     Control/Status window.				*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void hci_control_panel_system_log_messages( int force_draw )
{
  int height;
  int width;
  int string_length;
  int string_width;
  int status;
  int month, day, year;
  int hour,minute, second;
  char *ptr;
  XFontStruct *fontinfo;
  hci_control_panel_object_t *top;
  char msg [HCI_LE_MSG_MAX_LENGTH];
  LE_critical_message *sys_msg;
  int msg_id = HCI_SYSLOG_LATEST_ALARM;
  unsigned int rpg_msg, rpg_alarm, rda_alarm;
  char no_change_msg_buf[ 50 ] = "No System Status Change in last 5 minutes";

  /* If the top level widget doesn't exist, do nothing. */

  top  = hci_control_panel_object( TOP_WIDGET );

  if( top->widget == ( Widget ) NULL )
  {
    return;
  }

  /* If this is the first time called, initialize the status
     and alarm colors and message buffers. */

  if( first_time )
  {
    first_time = 0;
    alarm_fg  = hci_get_read_color( TEXT_FOREGROUND );
    alarm_bg  = hci_get_read_color( WHITE );
    status_fg = hci_get_read_color( TEXT_FOREGROUND );
    status_bg = hci_get_read_color( WHITE );
    sprintf( Rpg_status_msg," " );
    sprintf( Rpg_alarm_msg, " " );
    hci_set_system_log_update_flag( 1 );
    system_update_time = time( NULL );
  }

  status = 1;

  /* If one of the messages has been updated, update the
     status and feedback lines. If it hasn't been updated,
     check if it has been longer than STATUS_UPDATE_TIME
     since the last update. If it has, display a "No System
     Status Change" message. */

  if( hci_get_system_log_update_flag() )
  {
    /* Reset system status timer. */

    system_update_time = time( NULL );

    /* Read the indicated message. */

    while( msg_id != 0 )
    {
      status = ORPGDA_read( ORPGDAT_SYSLOG_LATEST,
                            (char *) msg,
                            HCI_LE_MSG_MAX_LENGTH,
                            msg_id );

      if( status > 0 )
      {
        sys_msg = ( LE_critical_message * ) msg;

        /* If the inhibit RDA status/alarm messages set,
           then ignore RDA messages. */

        ptr = strstr( sys_msg->text, ":" );

        if( ptr == NULL )
        {
          ptr = sys_msg->text;
        }
        else
        {
          ptr = ptr + 2;
        }

        if( !hci_info_inhibit_RDA_messages() ||
            ( hci_info_inhibit_RDA_messages() &&
              ( strncmp( ptr, "RDA", 3 ) != 0 ) ) )
        {
          unix_time( &sys_msg->time, &year, &month, &day,
                     &hour, &minute, &second );

          year %= 100;

          rpg_msg = ( sys_msg->code & HCI_LE_RPG_STATUS_MASK );
          rpg_alarm = ( sys_msg->code & HCI_LE_RPG_ALARM_MASK );
          rda_alarm = ( sys_msg->code & HCI_LE_RDA_ALARM_MASK );

          /* If the message is an alarm, display it in red in the
             alarm line. If the message does not indicate an alarm,
             but the error bit is set, display it in yellow in the
             status line. Else, display it in normal background
             color in the status line. */


          if( rpg_alarm || rda_alarm )
          {
            sprintf( Rpg_alarm_msg, "%s %d,%2.2d [%2.2d:%2.2d:%2.2d] >> %s",
                     HCI_get_month(month), day, year,
                     hour, minute, second, strtok( ptr, "\n" ) );

            if( rda_alarm )
            {
              switch( rda_alarm )
              {
                case HCI_LE_RDA_ALARM_NA:
                  alarm_fg = hci_get_read_color( TEXT_FOREGROUND );
                  alarm_bg = hci_get_read_color( BACKGROUND_COLOR1 );
                  break;

                case HCI_LE_RDA_ALARM_SEC:
                  alarm_fg = hci_get_read_color( TEXT_FOREGROUND );
                  alarm_bg = hci_get_read_color( BACKGROUND_COLOR1 );
                  break;

                case HCI_LE_RDA_ALARM_MAR:
                  alarm_fg = hci_get_read_color( TEXT_FOREGROUND );
                  alarm_bg = hci_get_read_color( WARNING_COLOR );
                  break;

                case HCI_LE_RDA_ALARM_MAM:
                  alarm_fg = hci_get_read_color( TEXT_FOREGROUND );
                  alarm_bg = hci_get_read_color( ALARM_COLOR2 );
                  break;

                case HCI_LE_RDA_ALARM_INOP:
                  alarm_fg = hci_get_read_color( WHITE );
                  alarm_bg = hci_get_read_color( ALARM_COLOR1 );
                  break;
              }

              if( sys_msg->code & HCI_LE_RDA_ALARM_CLEAR )
              {
                sprintf( Rpg_alarm_msg,
                         "%s %d,%2.2d [%2.2d:%2.2d:%2.2d] >> %s",
                         HCI_get_month(month), day, year,
                         hour, minute, second, strtok( ptr, "\n" ) );

                alarm_fg = hci_get_read_color( TEXT_FOREGROUND );
                alarm_bg = hci_get_read_color( NORMAL_COLOR );
              }

            }
            else if( rpg_alarm )
            {
              switch( rpg_alarm )
              {
                case HCI_LE_RPG_ALARM_MAR:
                  alarm_fg = hci_get_read_color( TEXT_FOREGROUND );
                  alarm_bg = hci_get_read_color( WARNING_COLOR );
                  break;

                case HCI_LE_RPG_ALARM_MAM:
                  alarm_fg = hci_get_read_color( TEXT_FOREGROUND );
                  alarm_bg = hci_get_read_color( ALARM_COLOR2 );
                  break;

                case HCI_LE_RPG_ALARM_LS:
                  alarm_fg = hci_get_read_color( TEXT_FOREGROUND );
                  alarm_bg = hci_get_read_color( CYAN );
                  break;

                default:	
                  alarm_fg = hci_get_read_color( TEXT_FOREGROUND );
                  alarm_bg = hci_get_read_color( BACKGROUND_COLOR1 );
                  break;
              }

              if( sys_msg->code & HCI_LE_RPG_ALARM_CLEAR )
              {
                sprintf( Rpg_alarm_msg,
                         "%s %d,%2.2d [%2.2d:%2.2d:%2.2d] >> %s",
                         HCI_get_month(month), day, year,
                         hour, minute, second, strtok( ptr, "\n" ) );

                alarm_fg = hci_get_read_color( TEXT_FOREGROUND );
                alarm_bg = hci_get_read_color( NORMAL_COLOR );
              }
            }
          }
          else if ( rpg_msg )
          {
            sprintf( Rpg_status_msg,
                     "%s %d,%2.2d [%2.2d:%2.2d:%2.2d] >> %s",
                     HCI_get_month(month), day, year,
                     hour, minute, second, strtok( ptr, "\n" ) );

            switch( rpg_msg )
            {
              case HCI_LE_RPG_STATUS_WARN:
                status_fg = hci_get_read_color( TEXT_FOREGROUND );
                status_bg = hci_get_read_color( WARNING_COLOR );
                break;

              case HCI_LE_RPG_STATUS_INFO:
                status_fg = hci_get_read_color( TEXT_FOREGROUND );
                status_bg = hci_get_read_color( GRAY );
                break;

              case HCI_LE_RPG_STATUS_COMMS:
                status_fg = hci_get_read_color( WHITE );
                status_bg = hci_get_read_color( SEAGREEN );
                break;

              case HCI_LE_RPG_STATUS_GEN:
                status_fg = hci_get_read_color( TEXT_FOREGROUND );
                status_bg = hci_get_read_color( BACKGROUND_COLOR1 );
                break;
            }
          }
          else
          {
            sprintf( Rpg_status_msg,
                     "%s %d,%2.2d [%2.2d:%2.2d:%2.2d] >> %s",
                     HCI_get_month(month), day, year,
                     hour, minute, second, strtok( ptr, "\n" ) );

            if( sys_msg->code & HCI_LE_ERROR_BIT )
            {
              status_fg = hci_get_read_color( TEXT_FOREGROUND );
              status_bg = hci_get_read_color( WARNING_COLOR );
            }
            else
            {
              status_fg = hci_get_read_color( TEXT_FOREGROUND );
              status_bg = hci_get_read_color( BACKGROUND_COLOR1 );
            }
          }
        }
      }

      if( msg_id == HCI_SYSLOG_LATEST_ALARM )
      {
        msg_id = HCI_SYSLOG_LATEST_STATUS;
      }
      else
      {
        msg_id = 0;
      }
    }

    hci_set_system_log_update_flag( 0 );
  }
  else
  {
    time_t current_time = time( NULL );

    if( ( current_time - system_update_time ) >= STATUS_UPDATE_TIME )
    {
      system_update_time = current_time;

      unix_time( &current_time, &year, &month, &day, &hour, &minute, &second );
      year %= 100;

      sprintf( Rpg_status_msg,
               "%s %d,%2.2d [%2.2d:%2.2d:%2.2d] >> %s",
               HCI_get_month(month), day, year,
               hour, minute, second, no_change_msg_buf );

      status_fg = hci_get_read_color( TEXT_FOREGROUND );
      status_bg = hci_get_read_color( BACKGROUND_COLOR1 );
    }
  }

  /* Always use a medium font to display the alarm and status messages. */

  XSetFont( HCI_get_display(),
            hci_control_panel_gc(),
            hci_get_font( MEDIUM ) );

  fontinfo = hci_get_fontinfo( MEDIUM );

  /* Determine height/width of the "Feedback: " tag, since it is the
     widest. This information is used to calculate the starting
     x,y coordinates of the status/alarm message. */ 

  height = fontinfo->ascent + fontinfo->descent;
  width  = XTextWidth( fontinfo, "Feedback: ", 10 );

  /* Draw Status and Alarms tags. */

  if( force_draw )
  {
    XSetBackground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( BACKGROUND_COLOR1 ) );

    XDrawImageString( HCI_get_display(),
                      hci_control_panel_pixmap(),
                      hci_control_panel_gc(),
                      ( int ) 2,
                      ( int ) ( top->height + height - 2 ),
                      "Status: ",
                      8 );

    XDrawImageString( HCI_get_display(),
                      hci_control_panel_pixmap(),
                      hci_control_panel_gc(),
                      ( int ) 2,
                      ( int ) ( top->height + 2*height + 1 ),
                      "Alarms: ",
                      8 );
  }

  /* Draw status message. */

  if( strcmp( Rpg_status_msg, prev_Rpg_status_msg ) != 0 || force_draw )
  {
    /* Overwrite any existing string by filling with the background color. */

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    status_bg );

    XFillRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    ( int ) ( width + 2 ),
                    ( int ) top->height,
                    ( int ) ( top->width - width - 3 ),
                    ( int ) ( height + 1 ) );

    /* Set new background/foreground colors. */

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    status_fg );

    XSetBackground( HCI_get_display(),
                    hci_control_panel_gc(),
                    status_bg );

    /* Make sure the status string will fit in the allotted space.
       If it is too big, chop off characters until it does. */

    string_length = strlen( Rpg_status_msg );
    string_width = XTextWidth( fontinfo, Rpg_status_msg, string_length );

    while( string_width >= ( top->width - width - 3 ) )
    {
      string_length--;
      string_width = XTextWidth( fontinfo, Rpg_status_msg, string_length );
    }

    /* Draw message to screen. */

    XDrawImageString( HCI_get_display(),
                      hci_control_panel_pixmap(),
                      hci_control_panel_gc(),
                      ( int ) ( width + 5 ),
                      ( int ) ( top->height + height - 2 ),
                      Rpg_status_msg,
                      string_length );

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( TEXT_FOREGROUND ) );

    XDrawRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    ( int ) ( width + 2 ),
                    ( int ) top->height,
                    ( int ) ( top-> width - width - 3 ),
                    ( int ) ( height + 1 ) );

    strcpy( prev_Rpg_status_msg, Rpg_status_msg );
  }

  /* Draw alarm message. */

  if( strcmp( Rpg_alarm_msg, prev_Rpg_alarm_msg ) != 0 || force_draw )
  {
    /* Overwrite any existing string by filling with the background color. */

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    alarm_bg );

    XFillRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    ( int ) ( width + 2 ),
                    ( int ) ( top->height + height + 3 ),
                    ( int ) ( top->width - width - 3 ),
                    ( int ) ( height + 1 ) );

    /* Set new background/foreground colorse. */

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    alarm_fg );

    XSetBackground( HCI_get_display(),
                    hci_control_panel_gc(),
                    alarm_bg );

    /* Make sure the alarm string will fit in the allotted space.
       If it is too big, chop off characters until it does. */

    string_length = strlen( Rpg_alarm_msg );
    string_width = XTextWidth( fontinfo, Rpg_alarm_msg, string_length );

    while( string_width >= ( top->width - width - 3 ) )
    {
      string_length--;
      string_width = XTextWidth( fontinfo, Rpg_alarm_msg, string_length );
    }

    /* Draw message to screen. */

    XDrawImageString( HCI_get_display(),
                      hci_control_panel_pixmap(),
                      hci_control_panel_gc(),
                      ( int ) ( width + 5 ),
                      ( int ) ( top->height + 2*height + 1 ),
                      Rpg_alarm_msg,
                      string_length );

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( TEXT_FOREGROUND ) );

    XDrawRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    ( int ) ( width + 2 ),
                    ( int ) ( top->height + height + 3 ),
                    ( int ) ( top-> width - width - 3 ),
                    ( int ) height );

    strcpy( prev_Rpg_alarm_msg, Rpg_alarm_msg );
  }

  /* Reset background color and font. */

  XSetBackground( HCI_get_display(),
                  hci_control_panel_gc(),
                  hci_get_read_color( BACKGROUND_COLOR1 ) );

  XSetFont( HCI_get_display(),
            hci_control_panel_gc(),
            hci_get_font( SCALED ) );
}
