/************************************************************************
 *									*
 *	Module:  hci_control_panel_tower.c				*
 *									*
 *	Description:  This module is used to display the tower that	*
 *		      holds the radar antenna and radome in the HCI	*
 *		      window.  Since the placement of the tower is	*
 *		      anchored by the RDA button, it should be only	*
 *		      called AFTER the RDA button is resized and	*
 *		      relocated.					*
 *									*
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:26:01 $
 * $Id: hci_control_panel_tower.c,v 1.10 2009/02/27 22:26:01 ccalvert Exp $
 * $Revision: 1.10 $
 * $State: Exp $
 */

/* Local include file definitions. */

#include <hci_control_panel.h>

/* Define macros. */

#define TPS_NOT_INSTALLED       0
#define TPS_OFF                 1
#define TPS_OK                  3

/* Function prototypes. */

static void	draw_tps_status( int );

/* Global/static variables. */

static	int	Prev_wb_status = -99;
static	int	Prev_tps_status = -99;

/************************************************************************
 *	Description: This function displays the radar tower above the	*
 *		     RDA container object.				*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void hci_control_panel_tower( int force_draw )
{
  int i;
  int sections;
  int section_height;
  int panel_3d;
  int wbstat;
  int tps_stat;
  int temp_int, temp_int1;
  hci_control_panel_object_t *top;
  hci_control_panel_object_t *rda;
  hci_control_panel_object_t *tower;

  /* If the RDA container object doesn't exist, do nothing. */

  rda = hci_control_panel_object( RDA_BUTTON );

  if( rda->widget == ( Widget ) NULL )
  {
    return;
  }

  /* Get the current RDA-RPG wideband and TPS status. */

  wbstat = ORPGRDA_get_wb_status( ORPGRDA_WBLNSTAT );
  tps_stat = ORPGRDA_get_status( RS_TPS_STATUS );

  /* Redraw if something has changed or the flag forces it. */

  if( wbstat != Prev_wb_status || tps_stat != Prev_tps_status || force_draw )
  {
    /* Get reference to objects. */

    top = hci_control_panel_object( TOP_WIDGET );
    tower = hci_control_panel_object( TOWER_OBJECT );

    /* Set foreground color. */

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( BLACK ) );

    /* Determine the number of sections in the tower by dividing
       the height of the tower by its width.  The number of sections
       is equal to the integer result plus one. */

    sections = ( top->height/4 )/rda->width + 1;
    section_height = ( top->height/4 )/sections;

    tower->pixel = rda->pixel;
    tower->scanl = rda->scanl - sections*section_height - 1;
    tower->width = rda->width;
    tower->height = sections*section_height;

    /* Set this variable for reuse, save some function calls. */

    panel_3d = hci_control_panel_3d();

    /* Draw each sect as a rectangle with crossing lines to each corner. */

    for( i = 0; i < sections; i++ )
    {
      temp_int = section_height*i;
      temp_int1 = section_height*( i + 1 );

      XDrawRectangle( HCI_get_display(),
                      hci_control_panel_pixmap(),
                      hci_control_panel_gc(),
                      ( int ) rda->pixel,
                      ( int ) ( rda->scanl - temp_int1 - 1 ),
                      ( int ) rda->width,
                      ( int ) section_height );

      XDrawRectangle( HCI_get_display(),
                      hci_control_panel_pixmap(),
                      hci_control_panel_gc(),
                      ( int ) ( rda->pixel + panel_3d ),
                      ( int ) ( rda->scanl - temp_int1 - 1 - panel_3d ),
                      ( int ) rda->width,
                      ( int ) section_height );

      XDrawLine( HCI_get_display(),
                 hci_control_panel_pixmap(),
                 hci_control_panel_gc(),
                 rda->pixel,
                 ( int ) ( rda->scanl - temp_int ),
                 ( int ) ( rda->pixel + rda->width ),
                 ( int ) ( rda->scanl - temp_int1 ) );

      XDrawLine( HCI_get_display(),
                 hci_control_panel_pixmap(),
                 hci_control_panel_gc(),
                 ( int ) rda->pixel,
                 ( int ) ( rda->scanl - temp_int1 ),
                 ( int ) ( rda->pixel + rda->width ),
                 ( int ) ( rda->scanl - temp_int ) );

      XDrawLine( HCI_get_display(),
                 hci_control_panel_pixmap(),
                 hci_control_panel_gc(),
                 ( int ) ( rda->pixel + panel_3d ),
                 ( int ) ( rda->scanl - temp_int - panel_3d ),
                 ( int ) ( rda->pixel + rda->width + panel_3d ),
                 ( int ) ( rda->scanl - temp_int1 - panel_3d ) );

      XDrawLine( HCI_get_display(),
                 hci_control_panel_pixmap(),
                 hci_control_panel_gc(),
                 ( int ) ( rda->pixel + panel_3d ),
                 ( int ) ( rda->scanl - temp_int1 - panel_3d ),
                 ( int ) ( rda->pixel + rda->width + panel_3d ),
                 ( int ) ( rda->scanl - temp_int - panel_3d ) );
    }

    /* Draw thick vertical line in center of tower. */

    for( i = 3; i < 8; i++ )
    {
      XDrawLine( HCI_get_display(),
                 hci_control_panel_pixmap(),
                 hci_control_panel_gc(),
                 ( int ) ( rda->pixel + rda->width/2 + i ),
                 ( int ) ( rda->scanl - ( top->height/4 ) - panel_3d/2 ),
                 ( int ) ( rda->pixel + rda->width/2 + i ),
                 ( int ) ( rda->scanl - panel_3d/2 ) );
    }

    for( i = 1; i <= panel_3d; i++ )
    {
      XDrawLine( HCI_get_display(),
                 hci_control_panel_pixmap(),
                 hci_control_panel_gc(),
                 ( int ) ( rda->pixel + i ),
                 ( int ) ( rda->scanl - ( top->height/4 ) - i ),
                 ( int ) ( rda->pixel + rda->width + i ),
                 ( int ) ( rda->scanl - ( top->height/4 ) - i ) );
    }

    /* Draw TPS status if the wideband is connected. */

    if( wbstat == RS_CONNECTED || wbstat == RS_DISCONNECT_PENDING )
    {
      draw_tps_status( tps_stat );
    }

    Prev_wb_status = wbstat;
    Prev_tps_status = tps_stat;
  }
}

/************************************************************************
 *	Description: This function displays the TPS status in the	*
 *		     tower.						*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

static void draw_tps_status( int tps_stat )
{
  int width1, width2, label_width;
  int x_pt, y_pt, font_asc, font_des, height, x_off;
  char buf1[ 8 ] = "";
  char buf2[ 8 ] = "";
  Pixel background_color;
  XFontStruct *fontinfo;
  hci_control_panel_object_t *tower;

  /* Get reference to tower object. */

  tower = hci_control_panel_object( TOWER_OBJECT );

  /* Set font. */

  fontinfo = hci_get_fontinfo( SCALED );
  font_asc = fontinfo->ascent;
  font_des = fontinfo->descent;
  height = font_asc + font_des;

  /* The " OFF " label is the longest (with respect to XTextWidth()),
     so use it to determine label width. */

  sprintf( buf1, " TPS: " );
  width1 = XTextWidth( fontinfo, buf1, strlen( buf1 ) );
  sprintf( buf2, " OFF " );
  width2 = XTextWidth( fontinfo, buf2, strlen( buf2 ) );
  label_width = width1 + width2;

  /* Set starting x,y coordinates. */

  x_pt = tower->pixel + tower->width/2 - label_width/2;
  y_pt = tower->scanl + tower->height/2;

  /* Determine tps string to display. */

  switch( tps_stat )
  {
    case TPS_NOT_INSTALLED :
      return;

    case TPS_OFF :
      strcpy( buf2, " OFF " );
      background_color = hci_get_read_color( ALARM_COLOR1 );
      break;

    case TPS_OK :
      strcpy( buf2, " OK  " );
      background_color = hci_get_read_color( NORMAL_COLOR );
      break;

    default :
      return;
  }

  /* Find the string width so we can tell where to put the second
     string relative to the first. */

  width2 = XTextWidth( fontinfo, buf2, strlen( buf2 ) );

  /* Display the TPS status label string. */

  XSetForeground( HCI_get_display(),
                  hci_control_panel_gc(),
                  hci_get_read_color( WHITE ) );

  XSetBackground( HCI_get_display(),
                  hci_control_panel_gc(),
                  hci_get_read_color( WHITE ) );

  XFillRectangle( HCI_get_display(),
                  hci_control_panel_pixmap(),
                  hci_control_panel_gc(),
                  x_pt,
                  y_pt,
                  width1,
                  height + font_des );     

  XSetForeground( HCI_get_display(),
                  hci_control_panel_gc(),
                  hci_get_read_color( TEXT_FOREGROUND ) );

  XDrawImageString( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    x_pt,
                    y_pt + height,
                    buf1,
                    strlen( buf1 ) );

  XSetForeground( HCI_get_display(),
                  hci_control_panel_gc(),
                  background_color );

  XFillRectangle( HCI_get_display(),
                  hci_control_panel_pixmap(),
                  hci_control_panel_gc(),
                  x_pt + width1,
                  y_pt,
                  label_width - width1,
                  height + font_des );     

  XSetForeground( HCI_get_display(),
                  hci_control_panel_gc(),
                  hci_get_read_color( TEXT_FOREGROUND ) );

  XSetBackground( HCI_get_display(),
                  hci_control_panel_gc(),
                  background_color );

  /* Determine x_offset to center label. */

  x_off = ( label_width - width1 - width2 )/2;

  XDrawImageString( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    x_pt + width1 + x_off,
                    y_pt + height,
                    buf2,
                    strlen( buf2 ) );

  /* Draw "box" around label. */

  XDrawRectangle( HCI_get_display(),
                  hci_control_panel_pixmap(),
                  hci_control_panel_gc(),
                  x_pt,
                  y_pt,
                  label_width,
                  height + font_des );
}

