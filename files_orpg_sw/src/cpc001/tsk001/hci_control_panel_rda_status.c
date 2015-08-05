/************************************************************************
 *									*
 *	Module:  hci_control_panel_rda_status.c				*
 *									*
 *	Description:  This module is used to display the current	*
 *		      RDA status and RDA operability status above the	*
 *		      radome in the RPG Control/Status window.  This	*
 *		      information is retrieved from the latest RDA	*
 *		      status message.					*
 *									*
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:25:59 $
 * $Id: hci_control_panel_rda_status.c,v 1.26 2009/02/27 22:25:59 ccalvert Exp $
 * $Revision: 1.26 $
 * $State: Exp $
 */

/* Local include file definitions. */

#include <hci_control_panel.h>

/* Global/static variables. */

static	char	previous_state_buf[ 24 ] = "";
static	char	previous_oper_buf[ 24 ] = "";

/************************************************************************
 *	Description: This function displays a color coded pair of	*
 *		     labels representing the latest RDA status and	*
 *		     RDA operability status above the radome object.	*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void hci_control_panel_rda_status ( int force_draw )
{
  char buf1 [24];
  char buf2 [24];
  int tag_width, value_width, max_label_width;
  int tag_ctr, value_ctr;
  int height;
  int descent;
  Pixel fg_color = -1;
  Pixel bg_color = -1;
  int wbstat;
  int blanking;
  XFontStruct *fontinfo;
  hci_control_panel_object_t *radome;
  int x1 = -1;
  int x2 = -1;
  int x3 = -1;
  int y_state = -1;
  int y_mode = -1;

  /* Get reference to radome object since we want to display the
     RDA status and operability status above it. */

  radome = hci_control_panel_object( RADOME_OBJECT );

  /* Set font information. */

  fontinfo = hci_get_fontinfo( SCALED );
  height = fontinfo->ascent + fontinfo->descent;
  descent = fontinfo->descent;

  /******************* State and Oper labels *******************/

  /* Determine max width of possible values for State 
     and Oper labels above radome. This ensures
     that each label is the same width regardless of
     the string used to create it. The string used to
     define the max width isn't necessarily the one
     with the most characters. It depends on the value
     returned by the XTextWidth() function. Each
     possible string value will need to be tested to
     determine the widest one to use. */

  sprintf( buf1, " State: " ); /* Longest label tag. */
  sprintf( buf2, " OFFLINE OPER " ); /* Longest label value. */
  tag_width = XTextWidth( fontinfo, buf1, strlen( buf1 ) );
  value_width = XTextWidth( fontinfo, buf2, strlen( buf2 ) );
  max_label_width = tag_width + value_width;

  x1 = radome->pixel - ( ( max_label_width - radome->width )/2 );
  x2 = x1 + tag_width;
  x3 = x1 + max_label_width;
  tag_ctr = x1 + tag_width/2;
  value_ctr = x2 + value_width/2;
  y_mode = radome->scanl - 1.1*height;
  y_state = y_mode - height - 4;

  /* Get the latest wideband and display blanking status. */

  wbstat = ORPGRDA_get_wb_status( ORPGRDA_WBLNSTAT );
  blanking = ORPGRDA_get_wb_status( ORPGRDA_DISPLAY_BLANKING );

  /* If the wideband is connected or is spot blanking is set,
     display the latest RDA status. */

  if( ( wbstat == RS_CONNECTED || wbstat == RS_DISCONNECT_PENDING ) ||
      ( blanking != 0 && blanking == RS_RDA_STATUS ) )
  {
    /* Build the proper text string for the RDA status information. */

    switch( ORPGRDA_get_status( RS_RDA_STATUS ) )
    {
      case RDA_OPERATE :
        strcpy( buf2, " OPERATE " );
        bg_color = hci_get_read_color( NORMAL_COLOR );
        fg_color = hci_get_read_color( TEXT_FOREGROUND );
        break;

      case RDA_OFFLINE_OPERATE :
        strcpy( buf2, " OFFLINE OPER " );
        bg_color = hci_get_read_color( NORMAL_COLOR );
        fg_color = hci_get_read_color( TEXT_FOREGROUND );
        break;

      case RDA_RESTART :
        strcpy( buf2, " RESTART " );
        bg_color = hci_get_read_color( NORMAL_COLOR );
        fg_color = hci_get_read_color( TEXT_FOREGROUND );
        break;

      case RDA_STANDBY :
        strcpy( buf2, " STANDBY " );
        bg_color = hci_get_read_color( NORMAL_COLOR );
        fg_color = hci_get_read_color( TEXT_FOREGROUND );
        break;

      case RDA_STARTUP :
        strcpy( buf2, " START-UP " );
        bg_color = hci_get_read_color( NORMAL_COLOR );
        fg_color = hci_get_read_color( TEXT_FOREGROUND );
        break;

      case RDA_PLAYBACK :
        strcpy( buf2, " PLAYBACK " );
        bg_color = hci_get_read_color( NORMAL_COLOR );
        fg_color = hci_get_read_color( TEXT_FOREGROUND );
        break;

      default :
        strcpy( buf2, " UNKNOWN " );
        bg_color = hci_get_read_color( ALARM_COLOR1 );
        fg_color = hci_get_read_color( WHITE );
        break;
    }
  }
  else
  {
    strcpy( buf2, " UNKNOWN " );
    bg_color = hci_get_read_color( ALARM_COLOR1 );
    fg_color = hci_get_read_color( WHITE );
  }

  /* If redraw is forced, redraw tag. */

  if( force_draw )
  {
    strcpy( buf1, " State: " );

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( BACKGROUND_COLOR2 ) );

    XFillRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    x1,
                    y_state,
                    tag_width,
                    height );

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( TEXT_FOREGROUND ) );

    XDrawString( HCI_get_display(),
                 hci_control_panel_pixmap(),
                 hci_control_panel_gc(),
                 tag_ctr - XTextWidth( fontinfo, buf1, strlen( buf1 ) )/2,
                 y_state - descent + height,
                 buf1,
                 strlen( buf1 ) );
  }

  /* If value has changed (or redraw is forced), redraw value. */

  if( strcmp( buf2, previous_state_buf ) != 0 || force_draw )
  {
    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    bg_color );

    XFillRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    x2,
                    y_state,
                    value_width,
                    height );

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    fg_color );

    XSetBackground( HCI_get_display(),
                    hci_control_panel_gc(),
                    bg_color );

    XDrawImageString( HCI_get_display(),
                      hci_control_panel_pixmap(),
                      hci_control_panel_gc(),
                      value_ctr - XTextWidth( fontinfo, buf2, strlen( buf2 ) )/2,
                      y_state - descent + height,
                      buf2,
                      strlen( buf2 ) );

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( TEXT_FOREGROUND ) );

    XDrawRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    x1,
                    y_state - 1,
                    max_label_width,
                    height + 1 );

    strcpy( previous_state_buf, buf2 );
  }

  /* If the wideband is connected or if spot blanking is set,
     display the latest RDA operability status.  The color of the
     label is dependent on the operability status. */

  if( ( wbstat == RS_CONNECTED || wbstat == RS_DISCONNECT_PENDING ) ||
      ( blanking != 0 || blanking == RS_OPERABILITY_STATUS ) )
  {
    /* Build the proper text string for the operability status. */

    if( blanking == RS_OPERABILITY_STATUS )
    {
      strcpy( buf2, " UNKNOWN " );
      bg_color = hci_get_read_color( ALARM_COLOR1 );
      fg_color = hci_get_read_color( WHITE );
    }
    else
    {
      switch( ( ORPGRDA_get_status( RS_OPERABILITY_STATUS )/2 )*2 )
      {
        case OS_INDETERMINATE :
          strcpy( buf2, " UNKNOWN " );
          bg_color = hci_get_read_color( ALARM_COLOR1 );
          fg_color = hci_get_read_color( WHITE );
          break;

        case OS_ONLINE :
          strcpy( buf2, " ONLINE " );
          bg_color = hci_get_read_color( NORMAL_COLOR );
          fg_color = hci_get_read_color( TEXT_FOREGROUND );
          break;

        case OS_MAINTENANCE_REQ :
          strcpy( buf2, " MAINT REQD " );
          bg_color = hci_get_read_color( WARNING_COLOR );
          fg_color = hci_get_read_color( TEXT_FOREGROUND );
          break;

        case OS_MAINTENANCE_MAN :
          strcpy( buf2, " MAINT MAND " );
          bg_color = hci_get_read_color( ALARM_COLOR2 );
          fg_color = hci_get_read_color( TEXT_FOREGROUND );
          break;

        case OS_COMMANDED_SHUTDOWN :
          strcpy( buf2, " SHUTDOWN " );
          bg_color = hci_get_read_color( WARNING_COLOR );
          fg_color = hci_get_read_color( TEXT_FOREGROUND );
          break;

        case OS_INOPERABLE :
          strcpy( buf2, " INOPERABLE " );
          bg_color = hci_get_read_color( ALARM_COLOR1 );
          fg_color = hci_get_read_color( WHITE );
          break;

        default :
          strcpy( buf2, " UNKNOWN " );
          bg_color = hci_get_read_color( ALARM_COLOR1 );
          fg_color = hci_get_read_color( WHITE );
          break;
      }
    }
  }
  else
  {
    strcpy( buf2, " UNKNOWN " );
    bg_color = hci_get_read_color( ALARM_COLOR1 );
    fg_color = hci_get_read_color( WHITE );
  }

  /* If redraw is forced, redraw tag. */

  if( force_draw )
  {
    strcpy( buf1, " Oper: " );

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( BACKGROUND_COLOR2 ) );

    XFillRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    x1,
                    y_mode,
                    tag_width,
                    height );

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( TEXT_FOREGROUND ) );

    XDrawString( HCI_get_display(),
                 hci_control_panel_pixmap(),
                 hci_control_panel_gc(),
                 tag_ctr - XTextWidth( fontinfo, buf1, strlen( buf1 ) )/2,
                 y_mode - descent + height,
                 buf1,
                 strlen( buf1 ) );
  }

  /* If value has changed (or redraw is forced), redraw value. */

  if( strcmp( buf2, previous_oper_buf ) != 0 || force_draw )
  {
    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    bg_color );

    XFillRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    x2,
                    y_mode,
                    value_width,
                    height );

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    fg_color );

    XSetBackground( HCI_get_display(),
                    hci_control_panel_gc(),
                    bg_color );

    XDrawImageString (HCI_get_display(),
                      hci_control_panel_pixmap(),
                      hci_control_panel_gc(),
                      value_ctr - XTextWidth( fontinfo, buf2, strlen( buf2 ) )/2,
                      y_mode - descent + height,
                      buf2,
                      strlen( buf2 ) );

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( TEXT_FOREGROUND ) );

    XDrawRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    x1,
                    y_mode - 1,
                    max_label_width,
                    height + 1 );

    strcpy( previous_oper_buf, buf2 );
  }
}
