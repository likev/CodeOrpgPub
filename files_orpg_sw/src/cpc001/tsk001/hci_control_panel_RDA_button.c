/************************************************************************
 *									*
 *	Module:  hci_control_panel_RDA_button.c				*
 *									*
 *	Description:  This module is used to define the attributes	*
 *	of the RDA button/box in the HCI control panel window. This	*
 *	module should be called whenever the main HCI display window	*
 *	is resized.							*
 *									*
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:25:55 $
 * $Id: hci_control_panel_RDA_button.c,v 1.6 2009/02/27 22:25:55 ccalvert Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */

/* Local include file definitions. */

#include <hci_control_panel.h>

/* Define macros. */

#define	OPERABLE	1
#define	INOPERABLE	0
#define	ICAO_LEN	5

/* Global/static variables. */

static	int	prev_op_status = -1;
static	int	first_time = 1;
static	char	rda_id[ HCI_ICAO_LEN ];

/************************************************************************
 *	Description: This function updates the position of the RDA	*
 *		     container and its objects and redisplays them.	*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void hci_control_panel_RDA_button( int force_draw )
{
  int	width;
  int	height;
  int 	op_status;
  int	fg_color;
  int 	bg_color;
  XFontStruct	*fontinfo;
  hci_control_panel_object_t	*top;
  hci_control_panel_object_t	*rda;
  hci_control_panel_object_t	*control;
  hci_control_panel_object_t	*alarms;

  /* Get the widget ID for the main drawing area widget. */

  top = hci_control_panel_object( TOP_WIDGET );

  if( top->widget == ( Widget ) NULL )
  {
    return;
  }

  /* Get the widget IDs for the RDA container and buttons. */

  rda     = hci_control_panel_object( RDA_BUTTON );
  control = hci_control_panel_object( RDA_CONTROL_BUTTON );
  alarms  = hci_control_panel_object( RDA_ALARMS_BUTTON );

  if( control->widget == ( Widget ) NULL )
  {
    return;
  }

  /* Determine the RDA Operability/Wideband Status. Color encode the RDA
     container based on the status values. */

  op_status = ORPGRDA_get_status( ORPGRDA_OPERABILITY_STATUS );

  if( op_status == OS_INOPERABLE )
  {
    fg_color = hci_get_read_color( WHITE );
    bg_color = hci_get_read_color( ALARM_COLOR1 );
  }
  else if( op_status == OS_MAINTENANCE_REQ )
  {
    fg_color = hci_get_read_color( TEXT_FOREGROUND );
    bg_color = hci_get_read_color( WARNING_COLOR );
  }
  else if( op_status == OS_MAINTENANCE_MAN )
  {
    fg_color = hci_get_read_color( TEXT_FOREGROUND );
    bg_color = hci_get_read_color( ALARM_COLOR2 );
  }
  else
  {
    fg_color = hci_get_read_color( TEXT_FOREGROUND );
    bg_color = hci_get_read_color( SEAGREEN );
  }

  /* If anything has changed, redraw RDA container. */

  if( op_status != prev_op_status || force_draw )
  {
    /* Update the position/size variables to reflect resize
       of the RPG Control/Status window. */

    rda->widget = control->widget;
    rda->pixel  = top->width/8;
    rda->scanl  = top->height/2;
    rda->width  = top->width/9;
    rda->height = top->height/5;

    /* Draw/fill container box. */

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    bg_color );

    XFillRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    rda->pixel,
                    rda->scanl,
                    rda->width,
                    rda->height );

    /* Set background/foreground colors. */

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    fg_color );

    XSetBackground( HCI_get_display(),
                    hci_control_panel_gc(),
                    bg_color );

    /* Set font properties. */

    fontinfo = hci_get_fontinfo( SCALED );
    height = fontinfo->ascent + fontinfo->descent; 

    /* Draw "RDA" on container. */

    width = XTextWidth( fontinfo, "RDA", 3 );

    XDrawImageString( HCI_get_display(),
                      hci_control_panel_pixmap(),
                      hci_control_panel_gc(),
                      ( int ) ( rda->pixel + rda->width/2 - width/2 ),
                      ( int ) ( rda->scanl + height ),
                      "RDA",
                      3 );

    /* Draw site name on container. */

    if( first_time )
    {
      if( HCI_rpg_name( rda_id ) < 0 )
      {
        strcpy( rda_id, "UNKN" );
      }
    }

    width = XTextWidth( fontinfo, rda_id, ICAO_LEN-1 );

    XDrawImageString( HCI_get_display(),
                      hci_control_panel_pixmap(),
                      hci_control_panel_gc(),
                      ( int ) ( rda->pixel + rda->width/2 - width/2 ),
                      ( int ) ( rda->scanl + 2 * height ),
                      rda_id,
                      ICAO_LEN-1 );

    /* Make the background looked raised from the RPG Control/Status
       window background by adding the 3-D border along the top and
       right. */

    hci_control_panel_draw_3d( rda->pixel,
                               rda->scanl,
                               rda->width,
                               rda->height,
                               BLACK );

    /* Update the properties of the RDA Control button. */

    control->pixel  = rda->pixel + rda->width/16;
    control->scanl  = rda->scanl + rda->height/3 - 2;
    control->width  = 3 * rda->width/4;
    control->height = rda->height/4;

    XtVaSetValues( control->widget,
                   XmNwidth, control->width,
                   XmNheight, control->height,
                   XmNx, control->pixel,
                   XmNy, control->scanl,
                   XmNfontList, hci_get_fontlist( SCALED ),
                   NULL );

    /* Add 3d effect around RDA control button box. */

    hci_control_panel_draw_3d( control->pixel,
                               control->scanl,
                               control->width,
                               control->height,
                               BLACK );

    /* Update the properties of the RDA Alarms button. */

    alarms->pixel  = rda->pixel + rda->width/16;
    alarms->scanl  = rda->scanl + 2*rda->height/3 - 2;
    alarms->width  = 3 * rda->width/4;
    alarms->height = rda->height/4;

    XtVaSetValues( alarms->widget,
                   XmNwidth, alarms->width,
                   XmNheight, alarms->height,
                   XmNx, alarms->pixel,
                   XmNy, alarms->scanl,
                   XmNfontList, hci_get_fontlist( SCALED ),
                   NULL );

    /* Add 3d effect around RDA alarms button box. */

    hci_control_panel_draw_3d( alarms->pixel,
                               alarms->scanl,
                               alarms->width,
                               alarms->height,
                               BLACK );

    prev_op_status = op_status;

    /* When the RDA container is redrawn, it partially covers
       the RDA/RPG wideband link. Call the function that will
       redraw the wideband link, thus uncovering it. Don't
       redraw if it's the first time this function is called,
       since the other widgets necessary to draw the wideband
       connection may yet to be defined. */

    if( !first_time ){ hci_control_panel_rpg_rda_connection( FORCE_REDRAW ); }
  }

  first_time = 0;
}

