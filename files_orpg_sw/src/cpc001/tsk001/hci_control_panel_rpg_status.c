/************************************************************************
 *									*
 *	Module:  hci_control_panel_rpg_status.c				*
 *									*
 *	Description:  This module is used to display the current	*
 *		      RPG operability status above the RPG box in the	*
 *		      HCI main window.  This information is obtained	*
 *		      from the ORPGINFO_STATEFL_MSGID message in the	*
 *		      ORPGDAT_RPG_INFO LB.				*
 *									*
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:26:00 $
 * $Id: hci_control_panel_rpg_status.c,v 1.49 2009/02/27 22:26:00 ccalvert Exp $
 * $Revision: 1.49 $
 * $State: Exp $
 */

/* Local include file definitions. */

#include <hci_control_panel.h>

/* Global/static variables. */

static	char		previous_state_buf[ 24 ] = "";
static	char		previous_oper_buf[ 24 ] = "";

/************************************************************************
 *	Description: This function displays the current RPG status and	*
 *		     operability status above the RPG container.	*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void hci_control_panel_rpg_status ( int force_draw )
{
  char buf1 [24];
  char buf2 [24];
  int tag_width, value_width, max_label_width;
  int tag_ctr, value_ctr;
  int height, font_descent;
  int status;
  Mrpg_state_t	mrpg;
  Pixel fg_color = -1;
  Pixel bg_color = -1;
  XFontStruct *fontinfo;
  unsigned int rpg_op_status;
  hci_control_panel_object_t *rpg;
  int x1 = -1;
  int x2 = -1;
  int x3 = -1;
  int y_state = -1;
  int y_oper = -1;

  /* Get a pointer to the RPG container object data since we want
     to display the RPG status and operability status above it. */

  rpg = hci_control_panel_object( RPG_BUTTONS_BACKGROUND );

  /* Set font information. */

  fontinfo = hci_get_fontinfo( SCALED );
  height = fontinfo->ascent + fontinfo->descent;
  font_descent = fontinfo->descent;

  /******************* State and Oper labels *******************/

  /* Determine max width of possible values for Oper
     and State labels above RPG button. This ensures
     that each label is the same width regardless of
     the string used to create it. The string used to
     define the max width isn't necessarily the one
     with the most characters. It depends on the value
     returned by the XTextWidth() function. Each
     possible string value will need to be tested to
     determine the widest one to use. */

  sprintf( buf1, " State: " ); /* Longest label tag. */
  sprintf( buf2, " MAINT MAND " ); /* Longest label value. */
  tag_width = XTextWidth( fontinfo, buf1, strlen( buf1 ) );
  value_width = XTextWidth( fontinfo, buf2, strlen( buf2 ) );
  max_label_width = tag_width + value_width;

  x1 = rpg->pixel - ( ( max_label_width - rpg->width )/2 );
  x2 = x1 + tag_width;
  x3 = x1 + max_label_width;
  tag_ctr = x1 + tag_width/2;
  value_ctr = x2 + value_width/2;
  y_oper = rpg->scanl - 1.8*height;
  y_state = y_oper - height - 4;

  /* Display RPG state info. */

  status = ORPGMGR_get_RPG_states( &mrpg );

  if( status == 0 )
  {
    /* Build a color-coded string based on the state. */

    switch( mrpg.state )
    {
      case MRPG_ST_SHUTDOWN :
        fg_color = hci_get_read_color( WHITE );
        bg_color = hci_get_read_color( ALARM_COLOR1 );
        sprintf( buf2, " SHUTDOWN " );
        break;

      case MRPG_ST_STANDBY :
        fg_color = hci_get_read_color( WHITE );
        bg_color = hci_get_read_color( ALARM_COLOR1 );
        sprintf( buf2, " STANDBY " );
        break;

      case MRPG_ST_OPERATING :
        fg_color = hci_get_read_color( TEXT_FOREGROUND );
        bg_color = hci_get_read_color( NORMAL_COLOR );
        sprintf( buf2, " OPERATE " );
        break;

      case MRPG_ST_TRANSITION :
        fg_color = hci_get_read_color( TEXT_FOREGROUND );
        bg_color = hci_get_read_color( WARNING_COLOR );
        sprintf( buf2, " TRANSITION " );
        break;

      case MRPG_ST_FAILED :
        fg_color = hci_get_read_color( WHITE );
        bg_color = hci_get_read_color( ALARM_COLOR1 );
        sprintf( buf2, " FAILED " );
        break;

      case MRPG_ST_POWERFAIL :
        fg_color = hci_get_read_color( WHITE );
        bg_color = hci_get_read_color( ALARM_COLOR1 );
        sprintf( buf2, " POWERFAIL " );
        break;

      default :
        fg_color = hci_get_read_color( WHITE );
        bg_color = hci_get_read_color( ALARM_COLOR1 );
        sprintf ( buf2," UNKNOWN " );
        break;
    }
  }
  else
  {
    fg_color = hci_get_read_color( WHITE );
    bg_color = hci_get_read_color( ALARM_COLOR1 );
    sprintf ( buf2," UNKNOWN " );
  }

  /* If redraw is forced, redraw tag. */

  if( force_draw )
  {
    sprintf( buf1, " State: " );

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
                 y_state - font_descent + height,
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
                      y_state - font_descent + height,
                      buf2,
                      strlen( buf2 ) );

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( BLACK ) );

    XDrawRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    x1,
                    y_state - 1,
                    max_label_width,
                    height + 1 );

    sprintf( previous_state_buf, buf2 );
  }

  /* Display RPG operability status info. */

  status = ORPGINFO_statefl_get_rpgopst( &rpg_op_status );

  if( status < 0 )
  {
    fg_color = hci_get_read_color( WHITE );
    bg_color = hci_get_read_color( ALARM_COLOR1 );
    sprintf( buf2, " UNKNOWN " );
    HCI_LE_error("Error in ORPGINFO_statefl_get_rpgopst( &rpg_op_status )");
  }
  else
  {
    if( ( rpg_op_status & ORPGINFO_STATEFL_RPGOPST_MAM ) > 0 )
    {
      bg_color = hci_get_read_color( ALARM_COLOR2 );
      fg_color = hci_get_read_color( TEXT_FOREGROUND );
      sprintf( buf2, " MAINT MAND " );
    }
    else if( ( rpg_op_status & ORPGINFO_STATEFL_RPGOPST_MAR ) > 0 )
    {
      bg_color = hci_get_read_color( WARNING_COLOR );
      fg_color = hci_get_read_color( TEXT_FOREGROUND );
      sprintf( buf2, " MAINT REQD " );
    }
    else if( ( rpg_op_status & ORPGINFO_STATEFL_RPGOPST_CMDSHDN ) > 0 )
    {
      bg_color = hci_get_read_color( WARNING_COLOR );
      fg_color = hci_get_read_color( TEXT_FOREGROUND );
      sprintf( buf2, " SHUTDOWN " );
    }
    else if( ( rpg_op_status & ORPGINFO_STATEFL_RPGOPST_ONLINE ) > 0 )
    {
      bg_color = hci_get_read_color( NORMAL_COLOR );
      fg_color = hci_get_read_color( TEXT_FOREGROUND );
      sprintf( buf2, " ONLINE " );
    }
  }

  /* If redraw is forced, redraw tag. */

  if( force_draw )
  {
    sprintf( buf1, " Oper: " );

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( BACKGROUND_COLOR2 ) );

    XFillRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    x1,
                    y_oper,
                    tag_width,
                    height );

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( TEXT_FOREGROUND ) );

    XDrawString( HCI_get_display(),
                 hci_control_panel_pixmap(),
                 hci_control_panel_gc(),
                 tag_ctr - XTextWidth( fontinfo, buf1, strlen( buf1 ) )/2,
                 y_oper - font_descent + height,
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
                    y_oper,
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
                      y_oper - font_descent + height,
                      buf2,
                      strlen( buf2 ) );

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( BLACK ) );

    XDrawRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    x1,
                    y_oper - 1,
                    max_label_width,
                    height + 1 );

    sprintf( previous_oper_buf, buf2 );
  }
}

