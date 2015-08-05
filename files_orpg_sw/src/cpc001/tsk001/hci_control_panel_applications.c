/************************************************************************
 *									*
 *	Module:  hci_control_panel_applications.c			*
 *									*
 *	Description:  This module is used to display a background for	*
 *		      applications buttons and position them based	*
 *		      on the width of the RPG Control/Status window.	*
 *									*
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/09/10 20:48:02 $
 * $Id: hci_control_panel_applications.c,v 1.23 2012/09/10 20:48:02 ccalvert Exp $
 * $Revision: 1.23 $
 * $State: Exp $
 */

/* Local include file definitions. */

#include <hci_control_panel.h>

/* Define macros. */

#define	BUTTON_WIDTH		63
#define	BUTTON_HEIGHT		63

/************************************************************************
 *	Description: This function manages the applications region in	*
 *		     the RPG Control/Status region.			*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void hci_control_panel_applications( int force_draw )
{
  int i;
  int x1, y1;
  int width;
  int height;
  int pixel;
  int scanl;

  hci_control_panel_object_t	*top;
  hci_control_panel_object_t	*user;
  hci_control_panel_object_t	*button;

  /* Set up the pointers to all of the object properties
     that are needed by this module. */

  top = hci_control_panel_object( TOP_WIDGET );
  user = hci_control_panel_object( USERS_BUTTON );

  /* If the top level widget is NULL then no other objects can
     exist either so return. */

  if( top->widget == ( Widget ) NULL )
  {
    return;
  }

  if( force_draw )
  {
    /* Use the position of the Users button to determine the left
       side of the region. */

    x1 = user->pixel + user->width + 3 * hci_control_panel_3d ();
    y1 = 1;

    /* Calculate the dimensions of the applications area. */

    width  = top->width - x1 - 2;
    height = top->height - 3;

    /* Draw a border around the region using the text foreground
       color. */

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( TEXT_FOREGROUND ) );

    XDrawRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    ( int ) ( x1 - 1 ),
                    ( int ) ( y1 - 1 ),
                    ( int ) ( width + 2 ),
                    ( int ) ( height + 1 ) );

    /* Fill the region with the secondary background color so
       it can be distinguished from the rest of the window. */

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( BACKGROUND_COLOR2 ) );

    XFillRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    ( int ) x1,
                    ( int ) y1,
                    ( int ) width,
                    ( int ) height );

    /* Set the foreground and background colors for text labels as
       we add them to the right of the application buttons. */

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( TEXT_FOREGROUND ) );

    XSetBackground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( BACKGROUND_COLOR2 ) );

    /* Define the position of the top button. */

    pixel = top->width;
    scanl = y1 - BUTTON_HEIGHT + 9;
    pixel = x1 + 5;

    /* For each button in the applications region, place it just
       below the previous button. */

    for( i = BASEDATA_BUTTON; i <= MISC_BUTTON; i++ )
    {
      char buf [64];

      button = hci_control_panel_object( i );

      scanl = scanl + BUTTON_HEIGHT;

      XtVaSetValues( button->widget, XmNx, pixel, XmNy, scanl, NULL );

      /* For each button, define a descriptive name to be placed
         to the right of the button. */

      switch( i )
      {
        case BASEDATA_BUTTON :
          sprintf( buf, "Base Data Display" );
          break;

        case CENSOR_ZONES_BUTTON :
          sprintf( buf, "Clutter Regions" );
          break;

        case BYPASS_MAP_BUTTON :
          sprintf( buf, "Bypass Map Display" );
          break;

        case PRF_CONTROL_BUTTON :
          sprintf( buf, "PRF Control" );
          break;

        case RDA_PERFORMANCE_BUTTON :
          sprintf( buf, "RDA Performance Data" );
          break;

        case CONSOLE_MESSAGE_BUTTON :
          sprintf( buf, "Console Messages" );
          break;

        case MISC_BUTTON :
          sprintf( buf, "Miscellaneous" );
          break;

        case BLOCKAGE_BUTTON :
          sprintf( buf, "Blockage Data Display" );
          break;

        case ENVIRONMENTAL_WINDS_BUTTON :
          sprintf( buf, "Environmental Data" );
          break;

      }

      /* Draw application name beside button. */

      XDrawImageString( HCI_get_display(),
			hci_control_panel_pixmap(),
			hci_control_panel_gc(),
			( int ) ( pixel + BUTTON_WIDTH ),
			( int ) ( scanl + BUTTON_HEIGHT/2 ),
			buf,
			strlen( buf ) );

      /* Frame each button/label combo by drawing a rectangle around them. */

      XDrawRectangle( HCI_get_display(),
                      hci_control_panel_pixmap(),
                      hci_control_panel_gc(),
                      ( int ) ( pixel - 1 ),
                      ( int ) ( scanl - 1 ),
                      ( int ) ( top->width - pixel - 5 ),
                      ( int ) ( BUTTON_HEIGHT - 2 ) );
    }
  }
}
