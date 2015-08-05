/************************************************************************
 *									*
 *	Module:  hci_control_panel_set_system_time.c			*
 *									*
 *	Description:  This module is used to display the current	*
 *		      system time in the HCI window header.		*
 *									*
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:26:00 $
 * $Id: hci_control_panel_set_system_time.c,v 1.3 2009/02/27 22:26:00 ccalvert Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

/* Local include file definitions. */

#include <hci_control_panel.h>

/* Global/static variables. */

static	char	prev_time_buf[ HCI_BUF_64 ] = "";

/************************************************************************
 *	Description: This function displays the current system time	*
 *		     in the RPG Control/Status window			*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return:	NONE							*
 ************************************************************************/

void hci_control_panel_set_system_time( int force_draw )
{
  time_t times;
  struct tm *gmt_time;
  int height, width;
  char buf1[ HCI_BUF_64 ] = "";
  XFontStruct *fontinfo;
  hci_control_panel_object_t *top;

  /* Get reference to gui objects. */

  top = hci_control_panel_object( TOP_WIDGET );

  /* We want to use a scaled font based on the window size. */

  XSetFont( HCI_get_display(),
            hci_control_panel_gc(),
            hci_get_font( SCALED ) );

  XSetForeground( HCI_get_display(),
                  hci_control_panel_gc(),
                  hci_get_read_color( TEXT_FOREGROUND ) );

  fontinfo = hci_get_fontinfo( SCALED );

  /* Make a system call to get current time information. */

  times = time( NULL );

  /* Use the gmtime function to break up julian seconds into
     a human readable date/time. */

  gmt_time = gmtime( &times );

  /* Format the date/time for display. */

  if( strftime( buf1, HCI_BUF_64, " %A %B %d, %Y   %H:%M:%S UT ", gmt_time ) == 0 )
  {
    return;
  }

  if( strcmp( buf1, prev_time_buf ) != 0 || force_draw )
  {
    /* Based on the current font, determine the width and
       height of the string so it can be centered in the
       graphical part of the RPG Control/Status window. */

    width = XTextWidth( fontinfo, buf1, strlen( buf1 ) );
    height = fontinfo->ascent + fontinfo->descent;

    XSetBackground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( BACKGROUND_COLOR2 ) );

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( TEXT_FOREGROUND ) );

    XDrawImageString( HCI_get_display(),
                      hci_control_panel_pixmap(),
                      hci_control_panel_gc(),
                      10,
                      ( int ) height,
                      buf1,
                      strlen( buf1 ) );

    XDrawRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    10,
                    ( int ) 2,
                    ( int ) width,
                    ( int ) height );

    strcpy( prev_time_buf, buf1 );
  }
}
