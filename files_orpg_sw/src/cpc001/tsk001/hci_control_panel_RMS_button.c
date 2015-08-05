/************************************************************************
 *									*
 *	Module:  hci_control_panel_RMS_button.c				*
 *									*
 *	Description:  This module is used to define the attributes	*
 *	of the RMS button/box in the HCI control panel window. This	*
 *	module should be called whenever the main HCI display window	*
 *	is resized.							*
 *									*
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:25:55 $
 * $Id: hci_control_panel_RMS_button.c,v 1.4 2009/02/27 22:25:55 ccalvert Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */

/* Local include file definitions. */

#include <hci_control_panel.h>

/* Global/static variables. */

static	int	prev_rms_down_flag = -1;
static	int	first_time = -1;

/************************************************************************
 *	Description: This function updates the position of the RMS	*
 *		     and redisplays it.					*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void hci_control_panel_RMS_button( int force_draw )
{
  hci_control_panel_object_t	*top;
  hci_control_panel_object_t	*rda;
  hci_control_panel_object_t	*rpg;
  hci_control_panel_object_t	*rms;
  hci_control_panel_object_t	*rms_control;
  int	rms_down_flag = -1;
  int	width;
  int	height;
  XFontStruct	*fontinfo;
  int	fg_color;
  int	bg_color;

  /* Get the widget IDs for various widgets on HCI. */

  top = hci_control_panel_object( TOP_WIDGET );

  if( top->widget == ( Widget ) NULL ){ return; }

  rda = hci_control_panel_object( RDA_BUTTON );
  rpg = hci_control_panel_object( RPG_BUTTONS_BACKGROUND );
  rms = hci_control_panel_object( RMS_BUTTON );

  /* Get the widget ID for the RMS Control button. */

  rms_control = hci_control_panel_object( RMS_CONTROL_BUTTON );

  if( rms_control->widget == ( Widget ) NULL ) { return; }

  /* Get RMS connect status. */

  rms_down_flag = hci_get_rms_down_flag();

  if( rms_down_flag != prev_rms_down_flag || force_draw )
  {
    if( rms_down_flag )
    {
      fg_color = hci_get_read_color( WHITE );
      bg_color = hci_get_read_color( ALARM_COLOR1 );
    }
    else
    {
      fg_color = hci_get_read_color( TEXT_FOREGROUND );
      bg_color = hci_get_read_color( SEAGREEN );
    }

    /* Update the position/size of the RMS Container. */

    rms->widget = rms_control->widget;
    rms->scanl  = rda->scanl + 7*rda->height/5;
    rms->width  = top->width/9;
    rms->height = top->height/8;
    rms->pixel  = rpg->pixel - ( rms->width - rpg->width )/2;

    /* Draw/fill container box. */

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    bg_color );

    XFillRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    rms->pixel,
                    rms->scanl,
                    rms->width,
                    rms->height );

    /* Set background/foreground colors. */

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    fg_color );

    XSetBackground( HCI_get_display(),
                    hci_control_panel_gc(),
                    bg_color );

    /* Set font properties. */

    fontinfo = hci_get_fontinfo( SCALED );
    width = XTextWidth( fontinfo, "RMS", 3 );
    height = fontinfo->ascent + fontinfo->descent;

    /* Draw "RMS" on container. */

    XDrawImageString( HCI_get_display(),
                      hci_control_panel_pixmap(),
                      hci_control_panel_gc(),
                      ( int ) ( rms->pixel + rms->width/2 - width/2 ),
                      ( int ) ( rms->scanl + height ),
                      "RMS",
                      3 );

    /* Make the background looked raised from the main window
       background by adding the 3-D border along the top and right. */

    hci_control_panel_draw_3d( rms->pixel,
                               rms->scanl,
                               rms->width,
                               rms->height,
                               BLACK );

    /* Update the properties for the control object. */

    rms_control->pixel  = rms->pixel + rms->width/16;
    rms_control->scanl  = rms->scanl + rms->height/3 ;
    rms_control->width  = 3*rms->width/4;
    rms_control->height = rms->height/2;

    XtVaSetValues( rms_control->widget,
                   XmNwidth, rms_control->width,
                   XmNheight, rms_control->height,
                   XmNx, rms_control->pixel,
                   XmNy, rms_control->scanl,
                   XmNfontList, hci_get_fontlist( SCALED ),
                   XmNforeground, hci_get_read_color( BUTTON_FOREGROUND ),
                   XmNbackground, hci_get_read_color( BUTTON_BACKGROUND ),
                   NULL );

    /* Add 3d effect around the distribution comms object. */

    hci_control_panel_draw_3d( rms_control->pixel,
                               rms_control->scanl,
                               rms_control->width,
                               rms_control->height,
                               BLACK );

    prev_rms_down_flag = rms_down_flag;

    /* When the RMS container is redrawn, it partially covers
       the RPG/RMS data flow link. Call the function that will
       redraw the link, thus uncovering it. Don't redraw if it's
       the first time this function is called, since the other
       widgets necessary to draw the link may yet to be defined. */

    if( !first_time ){ hci_control_panel_rpg_rms_connection( FORCE_REDRAW ); }
  }

  first_time = 0;
}

