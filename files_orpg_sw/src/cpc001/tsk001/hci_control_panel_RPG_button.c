/************************************************************************
 *									*
 *	Module:  hci_control_panel_RPG_button.c				*
 *									*
 *	Description:  This module is used to define the attributes	*
 *	of the RPG button/box in the HCI control panel window. This	*
 *	module should be called whenever the main HCI display window	*
 *	is resized.							*
 *									*
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:25:55 $
 * $Id: hci_control_panel_RPG_button.c,v 1.9 2009/02/27 22:25:55 ccalvert Exp $
 * $Revision: 1.9 $
 * $State: Exp $
 */

/* Local include file definitions. */

#include <hci_control_panel.h>

/* Global/static variables. */

static	unsigned int	prev_rpg_op_status = 0;
static	int		first_time = 1;

/************************************************************************
 *	Description: This function updates the position of the RPG	*
 *		     container and its objects and redisplays them.	*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void hci_control_panel_RPG_button( int force_draw )
{
  XmString	str;
  XFontStruct	*fontinfo;
  int		height;
  int		width;
  int		retval;
  int		fg_color;
  int		bg_color;
  unsigned int	rpg_op_status;
  unsigned int	mm_filter;
  unsigned int	mr_filter;

  hci_control_panel_object_t	*top;
  hci_control_panel_object_t	*rda;
  hci_control_panel_object_t	*rpg;
  hci_control_panel_object_t	*status;
  hci_control_panel_object_t	*products;
  hci_control_panel_object_t	*control;
	
	
  /* Get the widget ID for the main drawing area widget. */

  top = hci_control_panel_object (TOP_WIDGET);

  if( top->widget == ( Widget ) NULL )
  {
    return;
  }

  /* Get the widget ID for the RDA container since the position of
     the RPG container is dependent of the position of the RDA
     container. */

  rda = hci_control_panel_object( RDA_BUTTON );

  if( rda->widget == ( Widget ) NULL )
  {
    return;
  }

  /* Get the widget ID for the RPG container. */

  rpg = hci_control_panel_object( RPG_BUTTONS_BACKGROUND );

  /* Check operability status of RPG and color the container accordingly. */

  retval = ORPGINFO_statefl_get_rpgopst( &rpg_op_status );

  if( retval < 0 )
  {
    HCI_LE_error( "Error in ORPGINFO_statefl_get_rpgopst( &rpg_op_status )" );
  }
  else if( rpg_op_status != prev_rpg_op_status || force_draw )
  {
    mr_filter = ORPGINFO_STATEFL_RPGOPST_MAR;
    mm_filter = ORPGINFO_STATEFL_RPGOPST_MAM;

    if( ( rpg_op_status & mm_filter ) == mm_filter )
    {
      bg_color = hci_get_read_color( ALARM_COLOR2 );
      fg_color = hci_get_read_color( TEXT_FOREGROUND );
    }
    else if( ( rpg_op_status & mr_filter ) == mr_filter )
    {
      bg_color = hci_get_read_color( WARNING_COLOR );
      fg_color = hci_get_read_color( TEXT_FOREGROUND );
    }
    else
    {
      bg_color = hci_get_read_color( SEAGREEN );
      fg_color = hci_get_read_color( TEXT_FOREGROUND );
    }

    /* Update the position/size variables to reflect resize of the
       RPG Control/Status window. */

    rpg->pixel  = rda->pixel + 2*rda->width;
    rpg->scanl  = rda->scanl - ( top->height/3 - rda->height )/2;
    rpg->width  = top->width/9;
    rpg->height = 4*top->height/15;

    /* Draw/fill container box. */

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    bg_color );

    XFillRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    rpg->pixel,
                    rpg->scanl,
                    rpg->width,
                    rpg->height );

    /* Set background/foreground colors. */

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    fg_color );

    XSetBackground( HCI_get_display(),
                    hci_control_panel_gc(),
                    bg_color );

    /* Set font properties. */

    fontinfo = hci_get_fontinfo( SCALED );
    width = XTextWidth( fontinfo, "RPG", 3 );
    height = fontinfo->ascent + fontinfo->descent;

    /* Draw "RPG" on container. */

    XDrawImageString( HCI_get_display(),
                      hci_control_panel_pixmap(),
                      hci_control_panel_gc(),
                      ( int ) ( rpg->pixel + rpg->width/2 - width/2 ),
                      ( int ) ( rpg->scanl + height ),
                      "RPG",
                      3 );

    /* Make the background looked raised from the main window
       background by adding the 3-D border along the top and right. */

    hci_control_panel_draw_3d( rpg->pixel,
                               rpg->scanl,
                               rpg->width,
                               rpg->height,
                               BLACK );

    /* Get the widget ID for the RPG Control button. */

    control = hci_control_panel_object( RPG_CONTROL_BUTTON );

    if( control->widget == ( Widget ) NULL )
    {
      return;
    }

    /* Update the RPG Control button position. */

    control->pixel  = rpg->pixel + rpg->width/16;
    control->scanl  = rpg->scanl + rpg->height/4 - 2;
    control->width  = 3*rpg->width/4;
    control->height = rpg->height/5;

    str = XmStringCreateLocalized( "Control" );

    XtVaSetValues( control->widget,
                   XmNlabelString, str,
                   XmNwidth, ( Dimension ) control->width,
                   XmNheight, ( Dimension ) control->height,
                   XmNx, control->pixel,
                   XmNy, control->scanl,
                   XmNfontList, hci_get_fontlist( SCALED ),
                   NULL );

    XmStringFree( str );

    /* Add 3d effect around RPG conrol button box. */

    hci_control_panel_draw_3d( control->pixel,
                               control->scanl,
                               control->width,
                               control->height,
                               BLACK );

    /* Get the widget ID for the RPG Products button. */

    products = hci_control_panel_object (RPG_PRODUCTS_BUTTON);

    if( products->widget == ( Widget ) NULL )
    {
      return;
    }

    /* Update the RPG Products button position. */

    products->pixel  = rpg->pixel + rpg->width/16;
    products->scanl  = rpg->scanl + ( 2*rpg->height/4 ) - 2;
    products->width  = 3*rpg->width/4;
    products->height = rpg->height/5;

    str = XmStringCreateLocalized( "Products" );

    XtVaSetValues( products->widget,
                   XmNlabelString, str,
                   XmNwidth, products->width,
                   XmNheight, products->height,
                   XmNx, products->pixel,
                   XmNy, products->scanl,
                   XmNfontList, hci_get_fontlist( SCALED ),
                   NULL );

    XmStringFree( str );

    /* Add 3d effect around RPG products button box. */

    hci_control_panel_draw_3d( products->pixel,
                               products->scanl,
                               products->width,
                               products->height,
                               BLACK );

    /* Get the widget ID for the RPG Status button. */

    status = hci_control_panel_object( RPG_STATUS_BUTTON );

    if( status->widget == ( Widget ) NULL )
    {
      return;
    }

    /* Update the RPG Status button position. */

    status->pixel = rpg->pixel + rpg->width/16;
    status->scanl = rpg->scanl + ( 3*rpg->height/4 ) - 2;
    status->width = 3*rpg->width/4;
    status->height = rpg->height/5;

    str = XmStringCreateLocalized( "Status" );

    XtVaSetValues( status->widget,
                   XmNlabelString, str,
                   XmNwidth, status->width,
                   XmNheight, status->height,
                   XmNx, status->pixel,
                   XmNy, status->scanl,
                   XmNfontList, hci_get_fontlist( SCALED ),
                   NULL );

    XmStringFree( str );

    /* Add 3d effect around RPG status button box. */

    hci_control_panel_draw_3d( status->pixel,
                               status->scanl,
                               status->width,
                               status->height,
                               BLACK );
				  
    prev_rpg_op_status = rpg_op_status;

    /* When the RPG container is redrawn, it partially covers
       the RPG/USERS narrowband link. Call the function that will
       redraw the narrowband link, thus uncovering it. Don't
       redraw if it's the first time this function is called,
       since the other widgets necessary to draw the narrowband
       connection may yet to be defined. */

    if( !first_time ){ hci_control_panel_rpg_users_connection( FORCE_REDRAW ); }
  }

  first_time = 0;
}

