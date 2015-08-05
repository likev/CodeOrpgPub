/************************************************************************
 *									*
 *	Module:  hci_control_panel_power.c				*
 *									*
 *	Description:  This module creates a graphical representation	*
 *		      of the RDA power source in the top level HCI	*
 *		      window.  Its position is based on the location	*
 *		      of the RDA button so the RDA button must be	*
 *		      positioned first.					*
 *									*
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:25:58 $
 * $Id: hci_control_panel_power.c,v 1.26 2009/02/27 22:25:58 ccalvert Exp $
 * $Revision: 1.26 $
 * $State: Exp $
 */

/* Local include file definitions. */

#include <hci_control_panel.h>

/* Global/static variables. */

static int prev_power_status = -1;
static int prev_fuel_level = -1;

/************************************************************************
 *	Description: This function creates a graphical representation	*
 *		     of the RDA power source.  It can be either a set	*
 *		     of power poles (utility) or a generator building	*
 *		     with a fuel tank (auxilliary).			*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void hci_control_panel_power( int force_draw )
{
  int i;
  int temp_int;
  int power_status;
  int power_source;
  int fuel_level = -1;
  int empty_level;
  Pixel color;
  int width;
  int height;
  int x1, y1;
  int cylinder_width;
  int y_arc_top, y_arc_bottom, y_rec;
  int arc_height, rec_height;
  int arc_start_angle, arc_end_angle;
  XPoint X[6];
  char power_label [12];
  XFontStruct *fontinfo;
  hci_control_panel_object_t *rda;
  hci_control_panel_object_t *power;

  /* If RDA isn't connected, we don't know what is going on, so return. */

  if( ORPGRDA_get_wb_status( ORPGRDA_WBLNSTAT ) != RS_CONNECTED )
  {
    return;
  }

  /* Get latest power status. */

  power_status = ORPGRDA_get_status( RS_AUX_POWER_GEN_STATE );

  /* Determine power source. If power source is generator,
     get latest fuel level. */

  if( power_status & BIT_0_MASK )
  {
    power_source = AUXILLIARY_POWER;
    if( ORPGRDA_get_rda_config( NULL ) == ORPGRDA_ORDA_CONFIG )
    {
      fuel_level = hci_orda_pmd( CNVRTD_GEN_FUEL_LEVEL );
    }
    else
    {
      fuel_level = ( hci_rda_performance_data( GEN_FUEL_LEVEL ) - 51 )/204.0;
    }
  }
  else
  {
    power_source = UTILITY_POWER;
  }

  /* If something has changed (or we're being forced to redraw), redraw. */

  if( power_status != prev_power_status ||
      fuel_level != prev_fuel_level ||
      force_draw )
  {
    /* We use the RDA container object as a reference for placing
       the power source graphic. */

    rda = hci_control_panel_object( RDA_BUTTON );

    if( rda->widget == ( Widget ) NULL )
    {
      return;
    }

    power = hci_control_panel_object( POWER_OBJECT );

    /* We want to use scaled fonts so the graphic and labels can
       change with the window size. */

    fontinfo = hci_get_fontinfo( SCALED );
    height = fontinfo->ascent + fontinfo->descent;

    /* Define the position/size of the power source object based on
       the RDA container object. */

    power->pixel  = 5;
    power->scanl  = rda->scanl + rda->height/6;
    power->width  = rda->pixel - 20;
    power->height = 2*rda->height/3;

    /* Set font properties for drawing. */

    XSetFont( HCI_get_display(),
              hci_control_panel_gc(),
              hci_get_font( SCALED ) );

    /* If not on generator power... */
 
    if( power_source == UTILITY_POWER )
    {
      /* Draw box to hold power source icon. */

      XSetForeground( HCI_get_display(),
                      hci_control_panel_gc(),
                      hci_get_read_color( BUTTON_BACKGROUND ) );

      XFillRectangle( HCI_get_display(),
                      hci_control_panel_pixmap(),
                      hci_control_panel_gc(),
                      ( int ) power->pixel,
                      ( int ) power->scanl,
                      ( int ) power->width,
                      ( int ) power->height );

      /* For utility power, draw a pair of telephone poles with a
         set of wires connecting them.  Draw a line representing
         a wire from the farthest pole to the RDA box. */

      XSetForeground( HCI_get_display(),
                      hci_control_panel_gc(),
                      hci_get_read_color( BROWN ) );

      XFillRectangle( HCI_get_display(),
                      hci_control_panel_pixmap(),
                      hci_control_panel_gc(),
                      ( int ) ( power->pixel + power->width/2 ),
                      ( int ) ( power->scanl + power->height/8 ),
                      ( int ) ( power->width/3 ),
                      ( int ) ( hci_control_panel_3d()/3 ) );

      XFillRectangle( HCI_get_display(),
                      hci_control_panel_pixmap(),
                      hci_control_panel_gc(),
                      ( int ) power->pixel,
                      ( int ) ( power->scanl + power->height/2 ),
                      ( int ) ( power->width/3 ), 
                      ( int ) ( hci_control_panel_3d()/3 ) );

      temp_int = hci_control_panel_3d(); /* Prevent line wrapping. */

      XFillRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    ( int ) ( power->pixel + power->width/6 - temp_int/2 + 1 ),
                    ( int ) ( power->scanl + power->height/2 - temp_int ),
                    ( int ) ( hci_control_panel_3d()/2 ),
                    ( int ) ( power->height/2 ) );

      temp_int = hci_control_panel_3d()/2 + 1; /* Prevent line wrapping. */

      XFillRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(), 
                    hci_control_panel_gc(),
                    ( int ) ( power->pixel + ( 4*power->width/6  ) - temp_int ),
                    ( int ) power->scanl,
                    ( int ) ( hci_control_panel_3d()/2 ),
                    ( int ) ( power->height/2 ) );

      XSetForeground( HCI_get_display(),
                      hci_control_panel_gc(),
                      hci_get_read_color( BLACK ) );

      for( i = 0; i <= power->width/3 + 1; i = i + power->width/9 )
      {
        XDrawLine( HCI_get_display(),
                   hci_control_panel_pixmap(),
                   hci_control_panel_gc(),
                   ( int ) ( power->pixel + i ),
                   ( int ) ( power->scanl + power->height/2 ),
                   ( int ) ( power->pixel + power->width/2 + i ),
                   ( int ) ( power->scanl + power->height/8 ) );
      }

      XDrawLine( HCI_get_display(),
                 hci_control_panel_pixmap(),
                 hci_control_panel_gc(),
                 (int) ( power->pixel + 5*power->width/6 ),
                 (int) power->scanl,
                 (int) rda->pixel,
                 (int) rda->scanl );

      /* Display textually the state of the auxilliary power generator. */

      XSetForeground( HCI_get_display(),
                      hci_control_panel_gc(),
                      hci_get_read_color( BUTTON_FOREGROUND ) );

      XSetBackground( HCI_get_display(),
                      hci_control_panel_gc(),
                      hci_get_read_color( BUTTON_BACKGROUND ) );

      /* Check the last RDA status message to see if the generator
         is on or off.  Display this information as a text string
         beneath the utility power icon. */

      if( power_status & BIT_2_MASK )
      {
        sprintf( power_label, "Gen On" );
      }
      else if( power_status & BIT_4_MASK )
      {
        sprintf( power_label, "Cmd Swtchvr" );
      }
      else
      {
        sprintf( power_label, "Gen Off" );
      }

      width  = XTextWidth( fontinfo, power_label, strlen( power_label ) );

      XDrawImageString( HCI_get_display(),
                        hci_control_panel_pixmap(),
                        hci_control_panel_gc(),
                        ( int ) ( power->pixel + ( power->width - width )/2 ),
                        ( int ) ( power->scanl + power->height - 2 ),
                        power_label,
                        strlen( power_label ) );
    }
    else
    {
      /* on generator power... */

      /* Draw box to hold power source icon. */

      XSetForeground( HCI_get_display(),
                      hci_control_panel_gc(),
                      hci_get_read_color( BUTTON_BACKGROUND ) );

      XFillRectangle( HCI_get_display(),
                      hci_control_panel_pixmap(),
                      hci_control_panel_gc(),
                      ( int ) power->pixel,
                      ( int ) power->scanl,
                      ( int ) power->width,
                      ( int ) power->height );

      /* For auxilliary power, draw a generator building along with
         an attached fule tank.  Indicate the amount of fuel left in
         the fuel tank.  If the fuel level drops below 25%, change
         the color of the fuel to red to indicate an alarm
         condition. */

      /* Draw fuel tank as a cylinder. Motif doesn't have a function
         to draw a cylinder, so it is represented with a rectangle
         that has a filled arc on top and bottom. All three shapes
         share the same x-coordinate and width. Each shape will have
         its own y-coordinate and height (the two arcs will have the
         same height). */

      x1 = power->width/8;
      cylinder_width = power->width/3;
      y_arc_top = power->scanl + 5*( power->height - height )/12 - 2;
      y_arc_bottom = power->scanl + 5*( power->height - height )/6 - 2;
      y_rec = power->scanl + ( power->height - height )/2 - 2;
      arc_height = power->height/6;
      rec_height = y_arc_bottom - y_arc_top;
      arc_start_angle = 0;
      arc_end_angle = -360*64;

      /* Draw fuel tank for generator next to building. */

      XSetForeground( HCI_get_display(),
                      hci_control_panel_gc(),
                      hci_get_read_color( BLACK ) );

      /* -----Draw Tank Base----- */

      XFillArc( HCI_get_display(),
                hci_control_panel_pixmap(),
                hci_control_panel_gc(),
                x1,
                y_arc_bottom,
                cylinder_width,
                arc_height, 
                arc_start_angle,
                arc_end_angle );

      /* -----Draw Fuel Level----- */

      XFillRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    x1,
                    y_rec,
                    cylinder_width,
                    rec_height );

      /* -----Draw Empty portion of Tank----- */

      empty_level = ( 1.0 - (fuel_level/100.0) )*power->height/3;

      color = hci_get_read_color( GRAY );

      XSetForeground( HCI_get_display(),
                      hci_control_panel_gc(),
                      color );

      XFillRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    x1,
                    y_rec,
                    cylinder_width,
                    empty_level );

      XFillArc( HCI_get_display(),
                hci_control_panel_pixmap(),
                hci_control_panel_gc(),
                x1,
                y_rec + empty_level - power->height/12,
                cylinder_width,
                arc_height,
                arc_start_angle,
                arc_end_angle );

      /* -----Draw Top of Tank----- */

      XFillArc( HCI_get_display(),
                hci_control_panel_pixmap(),
                hci_control_panel_gc(),
                x1,
                y_arc_top,
                cylinder_width,
                arc_height,
                arc_start_angle,
                arc_end_angle );

      XSetForeground( HCI_get_display(),
                      hci_control_panel_gc(),
                      hci_get_read_color( BLACK ) );

      XDrawArc( HCI_get_display(),
                hci_control_panel_pixmap(),
                hci_control_panel_gc(),
                x1,
                y_arc_top,
                cylinder_width,
                arc_height,
                arc_start_angle,
                arc_end_angle );

      XDrawArc( HCI_get_display(),
                hci_control_panel_pixmap(),
                hci_control_panel_gc(),
                x1,
                y_arc_bottom,
                cylinder_width,
                arc_height,
                arc_start_angle,
                arc_end_angle );

      x1 = x1 + power->width/3;
      y1 = power->scanl + power->height/2 - 2;

      /* Draw generator building. */

      XSetForeground( HCI_get_display(),
                      hci_control_panel_gc(),
                      hci_get_read_color( BROWN ) );

      X[0].x = x1;
      X[0].y = y1;
      X[1].x = x1;
      X[1].y = y1 + 5*( power->height - height )/12;
      X[2].x = x1 + power->width/2;
      X[2].y = y1 + 5*( power->height - height )/12;
      X[3].x = x1 + power->width/2;
      X[3].y = y1;
      X[4].x = x1 + power->width/4;
      X[4].y = y1 - power->height/4;

      XFillPolygon( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    X,
                    ( int ) 5,
                    Convex,
                    CoordModeOrigin );

      /* Draw door outline in generator building. */

      XSetForeground( HCI_get_display(),
                      hci_control_panel_gc(),
                      hci_get_read_color( BLACK ) );

      XDrawRectangle( HCI_get_display(),
                      hci_control_panel_pixmap(),
                      hci_control_panel_gc(),
                      ( int ) ( x1 + power->width/8 ),
                      ( int ) y1,
                      ( int ) ( power->width/4 ),
                      ( int ) ( 5*( power->height - height )/12 - 1 ) );

      /* Outline the left side of the building in black. */
 
      XDrawLine( HCI_get_display(),
                 hci_control_panel_pixmap(),
                 hci_control_panel_gc(),
                 ( int ) X[0].x,
                 ( int ) X[0].y,
                 ( int ) X[1].x,
                 ( int ) X[1].y );

      /* Connect line from generator building to RDA button. */
 
      XDrawLine( HCI_get_display(),
                 hci_control_panel_pixmap(),
                 hci_control_panel_gc(),
                 ( int ) ( x1 + power->width/4 ),
                 ( int ) ( y1 - power->width/4 ),
                 ( int ) rda->pixel,
                 ( int ) rda->scanl );

      /* Draw roof and 3-d effect for generator building. */

      XDrawLine( HCI_get_display(),
                 hci_control_panel_pixmap(),
                 hci_control_panel_gc(),
                 ( int ) ( x1 - 2 ),
                 ( int ) ( y1 + 2 ),
                 ( int ) ( x1 + power->width/4 ),
                 ( int ) ( y1 - power->width/4 ) );

      for( i = 0; i < hci_control_panel_3d(); i++ )
      {
        XDrawLine( HCI_get_display(),
                   hci_control_panel_pixmap(),
                   hci_control_panel_gc(),
                  ( int ) ( x1 + power->width/4 + i ),
                  ( int ) ( y1 - power->width/4 - i ),
                  ( int ) ( x1 + power->width/2 + 2 + i ),
                  ( int ) ( y1 + 2 - i ) );

        XDrawLine( HCI_get_display(),
                   hci_control_panel_pixmap(),
                   hci_control_panel_gc(),
                   ( int ) ( x1 + power->width/2 + i ),
                   ( int ) ( y1 - i ),
                   ( int ) ( x1 + power->width/2 + i ),
                   ( int ) ( y1 - i + 5*( power->height - height )/12 - 1 ) );
      }

      /* Display textually the state of the utility power beneath
         the auxilliary power icon. */

      XSetForeground( HCI_get_display(),
                      hci_control_panel_gc(),
                      hci_get_read_color( BUTTON_FOREGROUND ) );

      XSetBackground( HCI_get_display(),
                      hci_control_panel_gc(),
                      hci_get_read_color( BUTTON_BACKGROUND ) );

      if( power_status & BIT_1_MASK )
      {
        sprintf( power_label, "Util Avail" );
      }
      else if( power_status & BIT_2_MASK )
      {
        sprintf( power_label, "  Gen On  " );
      }
      else if( power_status & BIT_4_MASK )
      {
        sprintf( power_label, "Cmd Swtchvr" );
      }

      width  = XTextWidth( fontinfo, power_label, strlen( power_label ) );

      XDrawImageString( HCI_get_display(),
                        hci_control_panel_pixmap(),
                        hci_control_panel_gc(),
                        ( int ) ( power->pixel + ( power->width - width )/2 ),
                        ( int ) ( power->scanl + power->height - 2 ),
                        power_label,
                        strlen( power_label ) );
    }

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( BUTTON_BACKGROUND ) );

    XSetBackground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( BACKGROUND_COLOR1 ) );

    XDrawRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    ( int ) power->pixel,
                    ( int ) power->scanl,
                    ( int ) power->width,
                    ( int ) power->height );

    /* Add 3D look around power graphic to make it more intuitive
       to the user that it can be selected. */

    hci_control_panel_draw_3d( power->pixel,
                               power->scanl,
                               power->width,
                               power->height,
                               BLACK );

    prev_power_status = power_status;
    prev_fuel_level = fuel_level;
  }
}
