/************************************************************************
 *									*
 *	Module:  hci_control_panel_USERS_button.c			*
 *									*
 *	Description:  This module is used to define the attributes	*
 *	of the USERS button/box in the HCI control panel window. This	*
 *	module should be called whenever the main HCI display window	*
 *	is resized.							*
 *									*
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:25:56 $
 * $Id: hci_control_panel_USERS_button.c,v 1.2 2009/02/27 22:25:56 ccalvert Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/* Local include file definitions. */

#include <hci_control_panel.h>

/************************************************************************
 *	Description: This function updates the position of the Users	*
 *		     container and associated object and redisplays	*
 *		     them.						*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void hci_control_panel_USERS_button( int force_draw )
{
  int	width;
  int	height;
  XFontStruct	*fontinfo;
  hci_control_panel_object_t	*top;
  hci_control_panel_object_t	*rpg;
  hci_control_panel_object_t	*users;
  hci_control_panel_object_t	*comms;

  /* Get the widget ID for the main drawing area widget. */

  top = hci_control_panel_object( TOP_WIDGET );

  if( top->widget == ( Widget ) NULL )
  {
    return;
  }

  /* Get the widget ID for the RPG container since we use this to
     position the USERS container. */

  rpg = hci_control_panel_object( RPG_BUTTONS_BACKGROUND );

  /* Get the widget ID for the USERS container. */

  users = hci_control_panel_object( USERS_BUTTON );

  /* Get the widget ID for the Comms button. */

  comms = hci_control_panel_object( COMMS_BUTTON );

  if( comms->widget == ( Widget ) NULL )
  {
    return;
  }

  if( force_draw )
  {
    /* Resize and display the background area for the Users container. */

    users->widget = comms->widget;
    users->pixel  = rpg->pixel + 2*rpg->width + 8;
    users->scanl  = top->height/2 - 5;
    users->width  = top->width/9;
    users->height = top->height/8;

    /* Draw/fill container box. */

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( SEAGREEN ) );

    XFillRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    users->pixel,
                    users->scanl,
                    users->width,
                    users->height );

    /* Set background/foreground colors. */

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( TEXT_FOREGROUND ) );

    XSetBackground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( SEAGREEN ) );

    /* Set font properties. */

    fontinfo = hci_get_fontinfo( SCALED );
    width = XTextWidth( fontinfo, "USERS", 5 );
    height = fontinfo->ascent + fontinfo->descent;

    /* Draw "USERS" name on container. */

    XDrawImageString( HCI_get_display(),
                      hci_control_panel_pixmap(),
                      hci_control_panel_gc(),
                      ( int ) ( users->pixel + users->width/2 - width/2 ),
                      ( int ) ( users->scanl + height ),
                      "USERS",
                      5 );

    /* Make the background looked raised from the main window
       background by adding the 3-D border along the top and right. */

    hci_control_panel_draw_3d( users->pixel,
                               users->scanl,
                               users->width,
                               users->height,
                               BLACK );

    /* Update the properties for the distribution comms object. */

    comms->pixel  = users->pixel + users->width/16;
    comms->scanl  = users->scanl + users->height/3 ;
    comms->width  = 3*users->width/4;
    comms->height = users->height/2;

    XtVaSetValues( comms->widget,
                   XmNwidth, comms->width,
                   XmNheight, comms->height,
                   XmNx, comms->pixel,
                   XmNy, comms->scanl,
                   XmNfontList, hci_get_fontlist( SCALED ),
                   NULL );

    /* Add 3d effect around the distribution comms object. */

    hci_control_panel_draw_3d( comms->pixel,
                               comms->scanl,
                               comms->width,
                               comms->height,
                               BLACK );
  }
}

