/************************************************************************
 *	Module:	hci_display_feedback_string.c				*
 *									*
 *	Description:	This module is used to display textually,	*
 *			in the HCI Status/Control Window, feedback	*
 *			on selected commands.				*
 *									*
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:26:04 $
 * $Id $
 * $Revision: 1.15 $
 * $State: Exp $
 */

/* Local include file definitions. */

#include <hci_control_panel.h>

/* Define macros. */

#define	FEEDBACK_MSG_LEN_MAX	128
#define	FEEDBACK_TIME_LEN_MAX	32

/* Global/static variables. */

static	int  new_msg_flag = 0;
static	char current_msg[ FEEDBACK_MSG_LEN_MAX + 1 ] = "";
static	char prev_msg[ FEEDBACK_TIME_LEN_MAX + FEEDBACK_MSG_LEN_MAX + 1 ] = "";

/************************************************************************
 *	Description: This function displays the input message in the	*
 *		     feedback line of the RPG Control/Status window.	*
 *									*
 *	Input:  *msg - pointer to start of message.			*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void hci_display_feedback_string( int force_draw )
{
  char string [ FEEDBACK_TIME_LEN_MAX + FEEDBACK_MSG_LEN_MAX + 1 ] = "";
  char time_buf[ FEEDBACK_TIME_LEN_MAX ];
  int height;
  int width;
  int day, year, month, hour, minute, second;
  time_t current_time;
  XFontStruct *fontinfo;
  hci_control_panel_object_t *top;

  /* If the top widget doesn't exist, do nothing. */

  top  = hci_control_panel_object( TOP_WIDGET );

  if( top->widget == ( Widget ) NULL )
  {
    return;
  }

  /* We use a medium font for the feedback message, regardless of
     the window size. */

  XSetFont( HCI_get_display(),
            hci_control_panel_gc(),
            hci_get_font( MEDIUM ) );

  /* Get the font properties so we will know where to begin to
     display the message. */

  fontinfo = hci_get_fontinfo( MEDIUM );
  height = fontinfo->ascent + fontinfo->descent;
  width = XTextWidth( fontinfo, "Feedback: ", 10 );

  if( force_draw )
  {
    /* First display the label "Feedback:" using the normal window
       background color. */

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( TEXT_FOREGROUND ) );

    XSetBackground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( BACKGROUND_COLOR1 ) );

    XDrawImageString( HCI_get_display(),
                      hci_control_panel_pixmap(),
                      hci_control_panel_gc(),
                      ( int ) 2,
                      ( int ) ( top->height - 5 ),
                      "Feedback: ",
                      10 );
  }

  /* Create the new string. If msg isn't new, use previous msg. */

  if( new_msg_flag )
  {
    new_msg_flag = 0;
    current_time = time( NULL );
    unix_time( &current_time, &year, &month, &day, &hour, &minute, &second );
    year%=100;
    sprintf( time_buf, "%s %d,%2.2d [%2.2d:%2.2d:%2.2d] >> ",
             (HCI_get_months())[ month ], day, year, hour, minute, second );
    sprintf( string, "%s%s", time_buf, strtok( current_msg, "\n" ) );
  }
  else
  {
    strcpy( string, prev_msg );
  }

  if( strcmp( string, prev_msg ) != 0 || force_draw )
  {
    /* First, overwrite any existing string by redrawing it using
       the background color. */

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( WHITE ) );

    XFillRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    ( int ) ( width + 2 ),
                    ( int ) ( top->height - 3 - height ),
                    ( int ) ( top->width - width - 3 ),
                    ( int ) ( height + 2 ) );

    /* Lastly, display the message. */

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( TEXT_FOREGROUND ) );

    XSetBackground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( WHITE ) );

    XDrawImageString( HCI_get_display(),
                      hci_control_panel_pixmap(),
                      hci_control_panel_gc(),
                      ( int ) ( 5 + width ),
                      ( int ) ( top->height - 5 ),
                      string,
                      strlen( string ) );

    XDrawRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    ( int ) ( width + 2 ),
                    ( int ) ( top->height - 3 - height ),
                    ( int ) ( top->width - width - 3 ),
                    ( int ) ( height + 1 ) );

    XSetBackground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( BACKGROUND_COLOR1 ) );

    XSetFont( HCI_get_display(),
              hci_control_panel_gc(),
              hci_get_font( SCALED ) );

    strcpy( prev_msg, string );
  }
}

/************************************************************************
 *	Description: This function set the message to display on the	*
 *		     feedback line of the RPG Control/Status window.	*
 *									*
 *	Input:  *msg - pointer to start of message.			*
 ************************************************************************/

void hci_set_display_feedback_string( char *msg )
{
  new_msg_flag = 1;
  strncpy( current_msg, msg, FEEDBACK_MSG_LEN_MAX );
}
