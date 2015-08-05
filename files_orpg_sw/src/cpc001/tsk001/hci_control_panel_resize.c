/************************************************************************
 *									*
 *	Module:  hci_control_panel_resize.c				*
 *									*
 *	Description:  This module contains the resize callback for	*
 *		      the RPG Control/Status window.			*
 *									*
 ************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/08/02 17:34:33 $
 * $Id: hci_control_panel_resize.c,v 1.40 2013/08/02 17:34:33 steves Exp $
 * $Revision: 1.40 $
 * $State: Exp $
 */

/* Local include file definitions. */

#include <hci_control_panel.h>

/* Define macros. */

#define	RATIO		1.1	/* Window width-to-height ratio. */

/* Global/static variables. */

static	int	busy_flag = 0;
static  int     force_resize = 0;

/************************************************************************
 *	Description: This function handles all resize events for the	*
 *		     RPG Control/Status window.  It can be called	*
 *		     by any function wishing to refresh the display.	*
 *									*
 *	Input:  w	    - ID of widget when invoked by resize	*
 *			      event.					*
 *		client_data - *unused*					*
 *		call_data   - *unused*					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_control_panel_resize( Widget w, XtPointer call_data, XtPointer client_data )
{
  float ratio;
  Dimension width;
  Dimension height;
  hci_control_panel_object_t *top;
  XFontStruct *fontinfo;
  int font_height;

  /* Check to see if another instance of this module
     is active. If so, return. */

  if( busy_flag ){ return; }

  busy_flag = 1;

  /* Get reference to top-level widget. If it is null, return. */

  top = hci_control_panel_object( TOP_WIDGET );

  if( top->widget == ( Widget ) NULL )
  {
    busy_flag = 0;
    return;
  }

  /* Get the current size of the main window and
      save in the top objects data. */

  XtVaGetValues( top->widget,
                 XmNwidth, &width,
                 XmNheight, &height,
                 NULL);

  /* The ratio calculation was added to make sure that the window
     dimensions allow all widgets to be drawn properly. Set the
     top-level widget's height such that the ratio of width to
     height is less than (or equal to) RATIO. */

  ratio = ( ( float ) width )/height;

  if( (ratio > RATIO) || force_resize )
  { 
    force_resize = 0;
    height = width/RATIO;

    XtVaSetValues( top->widget,
                   XmNheight, height,
                   NULL);
  }

  /* Calculate width/height of the top-level widget that can be drawn on.
     Subtract font height from the height, so the two labels at the bottom
     of the HCI will be visible. */

  fontinfo = hci_get_fontinfo( MEDIUM );
  font_height = 2*( fontinfo->ascent + fontinfo->descent ) + 5;

  top->width  = width;
  top->height = height - font_height;

  /* Rescale the font size, given the top-level widget's width.
     If the font is not found, the current default font is used. */

  hci_set_font( SCALED, ( int ) ( width/65 ) );
  XSetFont( HCI_get_display(),
            hci_control_panel_gc(),
            hci_get_font( SCALED ) );

  /* Create the top-level widget's pixmap. */

  hci_control_panel_new_pixmap( width, height );

  /* Erase Pixmap (fill with background color). */

  hci_control_panel_erase();

  /* Draw control panel. */

  hci_control_panel_draw( FORCE_REDRAW );

  /* Reset busy flag. */

  busy_flag = 0;

}

/************************************************************************
 *	Description: This function when called sets a flag to force 	*
 *		     resize.						*
 *									*
 *	Input:  flag	    - flag to set force_resize			*
 *			      event.					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/
void 
hci_control_panel_force_resize( int flag )
{
   force_resize = flag;
}
