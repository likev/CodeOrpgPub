/************************************************************************
 *									*
 *	Module:  hci_control_panel_elevation_lines.c			*
 *									*
 *	Description:  This module is used to display the current 	*
 *		      elevation angles contained in the VCP and		*
 *		      highlight the current elevation being processed.	*
 *									*
 ************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/04/30 19:16:06 $
 * $Id: hci_control_panel_elevation_lines.c,v 1.8 2013/04/30 19:16:06 steves Exp $
 * $Revision: 1.8 $
 * $State: Exp $
 */

/* Local include file definitions. */

#include <hci_control_panel.h>

/* Global/static variables. */

static	int	prev_vcp_number = -1;
static	int	prev_elev_num = -1;

/* Function prototypes. */

void draw_elevation_line( float elevation, Pixel line_color );

/************************************************************************
 *	Description: This function is used to graphically display the	*
 *		     elevation angles contained in the current VCP and	*
 *		     highlight the current elevation being processed.	*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void hci_control_panel_elevation_lines( int force_draw )
{
  int i;
  int wbstat;
  int rda_state;
  int vcp_number;
  int elev_num;
  int highlight_current_elevation;
  float already_highlighted = -1.0;
  float elevation = -1.0;
  float previous_elevation = -1.0;
  hci_control_panel_object_t *top;

  /* If the top widget does not exist, do nothing. */

  top = hci_control_panel_object( TOP_WIDGET );

  if( top->widget == ( Widget ) NULL ){ return; }

  /* Get wideband connection status. If not connected, do nothing. */

  wbstat = ORPGRDA_get_wb_status( ORPGRDA_WBLNSTAT );

  if( ( wbstat != RS_CONNECTED && wbstat != RS_DISCONNECT_PENDING )
      && ORPGMISC_is_operational() )
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

  /* Get VCP/elevation number. */

  vcp_number = ORPGVST_get_vcp();
  elev_num = hci_control_panel_elevation_num() - 1;

  if( vcp_number != prev_vcp_number ||
      elev_num != prev_elev_num ||
      force_draw )
  {
    /* If the RDA is in operate or playback mode, or if data
       is being read from tape, set flag to highlight the
       current elevation. */

    if( ORPGRDA_get_status( RS_RDA_STATUS ) == RDA_OPERATE ||
        ORPGRDA_get_status( RS_RDA_STATUS ) == RDA_PLAYBACK ||
        ORPGRDA_get_status( RS_OPERABILITY_STATUS ) == OS_INDETERMINATE )
    {
      highlight_current_elevation = 1;
    }
    else
    {
      highlight_current_elevation = 0;
    }

    /* Initialize the angle that has already been highlighted.  
       Don't set to BLACK if already set to YELLOW.  This supports
       SAILS where angles are repeated. */

    already_highlighted = -2.0;

    /* Draw each elevation line. Skip repeating values. */
    
    for( i = 0; i < ORPGVST_get_number_elevations(); i++ )
    {
      elevation = ORPGVST_get_elevation( i )/10.0;

      if( i == elev_num && highlight_current_elevation )
      {
        draw_elevation_line( elevation, hci_get_read_color( YELLOW ) );
        already_highlighted = elevation;
      }
      else if( (elevation != previous_elevation)
                          &&
                 (elevation != already_highlighted) )
      {
        draw_elevation_line( elevation, hci_get_read_color( BLACK ) );
      }

      previous_elevation = elevation;
    }

    prev_vcp_number = vcp_number;
    prev_elev_num = elev_num;
  }
}

/************************************************************************
 *	Description: This function draws an elevation line radiating	*
 *		     from the center of the radome.			*
 *									*
 *	Input:  elevation to draw and color of the elevation line	*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void draw_elevation_line( float elevation, Pixel line_color )
{
  int x_ctr,x1,x2,y_ctr,y1,y2;
  double cos1,sin1;
  hci_control_panel_object_t	*top;
  hci_control_panel_object_t	*radome;

  /* Get reference to gui objects. */

  top = hci_control_panel_object( TOP_WIDGET );
  radome = hci_control_panel_object( RADOME_OBJECT );

  /* Set color for elevation lines. */

  XSetForeground( HCI_get_display(),
                  hci_control_panel_gc(),
                  line_color );

  /* Find center point of radome. */

  x_ctr = radome->pixel + radome->width/2;
  y_ctr = radome->scanl + radome->height/2;

  /* Find cos/sin to use for calculations. Multiply */
  /* angle by 1.5 for better visial separation.     */

  cos1 =  cos( ( double ) 1.5*elevation*HCI_DEG_TO_RAD );
  sin1 =  sin( ( double ) 1.5*elevation*HCI_DEG_TO_RAD );

  /* X1/Y1 is starting point of beam (at edge of radome) */
  /* X2/Y2 is ending point of beam */

  x1 = x_ctr + ( radome->width/2 )*cos1;
  y1 = y_ctr - ( radome->width/2 )*sin1;
  x2 = x_ctr + ( top->width/3.5 )*cos1;
  y2 = y_ctr - ( top->width/2 )*sin1;

  /* Draw the beam */

  XDrawLine( HCI_get_display(),
    hci_control_panel_pixmap(),
    hci_control_panel_gc(),
    ( int ) x1,
    ( int ) y1,
    ( int ) x2,
    ( int ) y2 );
}
