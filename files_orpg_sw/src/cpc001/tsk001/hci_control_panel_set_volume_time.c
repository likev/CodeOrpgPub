/************************************************************************
 *									*
 *	Module:  hci_control_panel_set_volume_time.c			*
 *									*
 *	Description:  This module is used to display the latest		*
 *		      volume scan start time inside the HCI window.	*
 *									*
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/11/16 15:25:40 $
 * $Id: hci_control_panel_set_volume_time.c,v 1.9 2012/11/16 15:25:40 ccalvert Exp $
 * $Revision: 1.9 $
 * $State: Exp $
 */

/* Local include file definitions. */

#include <hci_control_panel.h>

/* Definitions. */

#define PIXEL_OFFSET	10

/* Global/static variables. */

static	int	prev_volume_date = -99;

/************************************************************************
 *	Description: This function displays the current volume time	*
 *		     in the RPG Control/Status window.			*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return:	NONE							*
 ************************************************************************/

void hci_control_panel_set_volume_time( int force_draw )
{
  int height, width;
  int top_scanl;
  int left_pixel;
  int vol_number;
  int wbstat;
  int rda_state;
  int volume_date;
  int hours;
  int minutes;
  int seconds;
  int year, month, day;
  char buf2 [135];
  time_t j_seconds;
  XFontStruct *fontinfo;
  hci_control_panel_object_t *top;
  hci_control_panel_object_t *rda;

  /* Get references to gui objects. */

  top = hci_control_panel_object( TOP_WIDGET );
  rda = hci_control_panel_object( RDA_BUTTON );

  /* If the top object doesn't exist, do nothing. */

  if( top->widget == ( Widget ) NULL )
  {
    HCI_LE_error( "hci_control_panel_set_volume_time top->widget == NULL" );
    return;
  }

  /* Get wideband connection. Do nothing if not connected. */

  wbstat = ORPGRDA_get_wb_status( ORPGRDA_WBLNSTAT );

  if( wbstat != RS_CONNECTED &&
      wbstat != RS_DISCONNECT_PENDING &&
      ORPGMISC_is_operational() )
  {
    return;
  }

  /* Get RDA state. If RDA is in STANDBY or OFFLINE OPERATE, do nothing. */

  rda_state = ORPGRDA_get_status( RS_RDA_STATUS );

  if( ( rda_state == RDA_STANDBY || rda_state == RDA_OFFLINE_OPERATE ) &&
      ORPGMISC_is_operational() )
  {
    return;
  }

  volume_date = ORPGVST_get_volume_date();

  if( volume_date != prev_volume_date || force_draw )
  {
    /* We want to use a scaled font based on the window size. */

    XSetFont( HCI_get_display(),
              hci_control_panel_gc(),
              hci_get_font( SCALED ) );

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( TEXT_FOREGROUND ) );

    fontinfo = hci_get_fontinfo( SCALED );

    /* If the volume date isn't defined yet, display a string
       without the date/time. */

    if( ORPGVST_get_volume_date() <= 1 )
    {
      vol_number = ORPGVST_get_volume_number();

      sprintf( buf2, "Volume %d (seq: %d) Start:",
               ORPGMISC_vol_scan_num ( ( unsigned int ) vol_number ),
               vol_number );
    }
    else
    {
      /* Else, get the current volume time, remove the fraction
         of seconds, and then convert from julian seconds to
         normal date/time. */

      seconds = ORPGVST_get_volume_time()/1000;  /* Drop milliseconds */
      j_seconds = ( ORPGVST_get_volume_date() - 1 ) * HCI_SECONDS_PER_DAY + seconds;

      unix_time( &j_seconds, &year, &month, &day, &hours, &minutes, &seconds );

      vol_number = ORPGVST_get_volume_number();

      sprintf( buf2,
               "Volume %d (Seq: %d) Start: %s %d,%d   %2.2d:%2.2d:%2.2d UT",
               ORPGMISC_vol_scan_num( ( unsigned int ) vol_number ),
               vol_number,
               (HCI_get_months())[month], day, year, hours, minutes, seconds );
    }

    /* Based on the current font, determine the width and
       height of the string so it can be centered in the
       graphical part of the RPG Control/Status window. */

    width = XTextWidth( fontinfo, buf2, strlen( buf2 ) );
    height = fontinfo->ascent + fontinfo->descent;

    left_pixel = rda->pixel + rda->width + hci_control_panel_3d();
    left_pixel += PIXEL_OFFSET;
    top_scanl  = rda->scanl - rda->height;

    XSetBackground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( BACKGROUND_COLOR1 ) );

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( TEXT_FOREGROUND ) );

    XDrawImageString( HCI_get_display(),
                      hci_control_panel_pixmap(),
                      hci_control_panel_gc(),
                      ( int ) left_pixel,
                      ( int ) top_scanl,
                      buf2,
                      strlen( buf2 ) );

    prev_volume_date = volume_date;
  }
}
