/************************************************************************
 *									*
 *	Module:  hci_control_panel_rpg_rda_connection.c			*
 *									*
 *	Description:  This module contains the routines used to		*
 *	draw the connection and data flow between the RPG and RDA	*
 *	containers on the RPG Control/Status window.			*
 *									*
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:25:56 $
 * $Id: hci_control_panel_control_status.c,v 1.3 2009/02/27 22:25:56 ccalvert Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */


/* Local include file definitions. */

#include <hci_control_panel.h>

/* Function prototypes. */

void	erase_control_info();
void	draw_control_info();

/* Global variables. */

static	char	RDA_control_buf[ 10 ] = "";
static	int	prev_control_status = -1;
static	int	prev_wb_status = -1;

/************************************************************************
 *	Description: This function draws the connection between the	*
 *		     RDA and RPG containers.				*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void hci_control_panel_control_status ( int force_draw )
{
  int control_status;
  int wbstat;
  hci_control_panel_object_t *rda;

  /* If the RDA object is not defined, return. */

  rda = hci_control_panel_object( RDA_BUTTON );

  if( rda->widget == ( Widget ) NULL )
  {
    return;
  }

  /* Get various status parameters. */

  control_status = ORPGRDA_get_status( RS_CONTROL_STATUS );
  wbstat = ORPGRDA_get_wb_status( ORPGRDA_WBLNSTAT );

  /* Redraw if something has changed or flag forces it. */

  if( control_status != prev_control_status ||
      wbstat != prev_wb_status ||
      force_draw )
  {
    /* If the wideband is connected, draw the control status line.
       Otherwise, erase it. */

    erase_control_info();

    if( wbstat == RS_CONNECTED )
    {
      /* Generate a label indicating the RDA control state. */

      switch( control_status )
      {
        case RDA_IN_CONTROL:
          strcpy( RDA_control_buf, "RDA" );
          break;

        case RPG_IN_CONTROL:
          strcpy( RDA_control_buf, "RPG" );
          break;

        case EITHER_IN_CONTROL:
          strcpy( RDA_control_buf, "EITHER" );
          break;

        default:
          strcpy( RDA_control_buf, "????" );
          break;
      }

      draw_control_info();
    }

    /* Reset line attributes. */

    XSetLineAttributes( HCI_get_display(),
                        hci_control_panel_gc(),
                        ( unsigned int ) 1,
                        LineSolid,
                        CapProjecting,
                        JoinMiter );

    prev_control_status = control_status;
    prev_wb_status = wbstat;
  }
}

/************************************************************************
 *	Description: This function removes the RDA control status line.	*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void erase_control_info()
{
  char max_buf[ 10 ] = "EITHER"; /* Longest string possible. */
  int left_pixel;
  int right_pixel;
  int top_scanl;
  XFontStruct *fontinfo;
  int height;
  hci_control_panel_object_t *rpg;
  hci_control_panel_object_t *rda;

  rpg = hci_control_panel_object( RPG_BUTTONS_BACKGROUND );
  rda = hci_control_panel_object( RDA_BUTTON );

  /* Define font info. Calculate where RDA control status will be drawn. */

  fontinfo = hci_get_fontinfo( SCALED );
  height = fontinfo->ascent + fontinfo->descent;

  left_pixel  = rda->pixel + rda->width + hci_control_panel_3d() + 1;

  right_pixel = rpg->pixel;
  top_scanl = rpg->scanl;

  /* Erase RDA control status string/line. */

  XSetForeground( HCI_get_display(),
                  hci_control_panel_gc(),
                  hci_get_read_color( BACKGROUND_COLOR1 ) );

  XFillRectangle( HCI_get_display(),
                  hci_control_panel_pixmap(),
                  hci_control_panel_gc(),
                  left_pixel + 8,
                  top_scanl - height,
                  XTextWidth( fontinfo, max_buf, strlen( max_buf ) ),
                  height );

  XSetLineAttributes( HCI_get_display(),
                      hci_control_panel_gc(),
                      (unsigned int) 3,
                      LineSolid,
                      CapButt,
                      JoinMiter );

  XDrawLine( HCI_get_display(),
             hci_control_panel_pixmap(),
             hci_control_panel_gc(),
             left_pixel,
             top_scanl,
             right_pixel,
             top_scanl );
}

/************************************************************************
 *	Description: This function draws the RDA control status line.	*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void draw_control_info()
{
  int	left_pixel;
  int	right_pixel;
  int	top_scanl;
  XFontStruct *fontinfo;
  int	height;
  hci_control_panel_object_t	*rpg;
  hci_control_panel_object_t	*rda;

  rpg = hci_control_panel_object( RPG_BUTTONS_BACKGROUND );
  rda = hci_control_panel_object( RDA_BUTTON );

  /* Define font info. Calculate where RDA control status will be drawn. */

  fontinfo = hci_get_fontinfo( SCALED );
  height = fontinfo->ascent + fontinfo->descent;

  left_pixel = rda->pixel + rda->width + hci_control_panel_3d() + 1;

  right_pixel = rpg->pixel;
  top_scanl = rpg->scanl;

  /* Draw RDA control status. */

  XSetForeground( HCI_get_display(),
                  hci_control_panel_gc(),
                  hci_get_read_color( BLACK ) );

  XSetFont( HCI_get_display(),
            hci_control_panel_gc(),
            hci_get_font( SCALED ) );

  XSetLineAttributes( HCI_get_display(),
                      hci_control_panel_gc(),
                      (unsigned int) 3,
                      LineSolid,
                      CapButt,
                      JoinMiter );

  XDrawLine( HCI_get_display(),
             hci_control_panel_pixmap(),
             hci_control_panel_gc(),
             left_pixel,
             top_scanl,
             right_pixel,
             top_scanl );

  XDrawString( HCI_get_display(),
    hci_control_panel_pixmap(),
    hci_control_panel_gc(),
    ( int ) ( left_pixel + 8 ),
    ( int ) ( top_scanl - 2 ),
    RDA_control_buf,
    strlen( RDA_control_buf ) );

  /* Reset font to scaled. */

  XSetFont( HCI_get_display(),
            hci_control_panel_gc(),
            hci_get_font( SCALED ) );

}
