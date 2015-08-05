/************************************************************************
 *									*
 *	Module:  hci_update_antenna_position.c				*
 *									*
 *	Description:  This module is used to graphically show the	*
 *		      current position of the radar antenna inside	*
 *		      the radome in the RPG Control/Status window.	*
 *									*
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:26:26 $
 * $Id: hci_update_antenna_position.c,v 1.48 2009/02/27 22:26:26 ccalvert Exp $
 * $Revision: 1.48 $
 * $State: Exp $
 */

/* Local include file definitions. */

#include <hci_control_panel.h>

/* Local constants. */

#define	THICKNESS_NORMAL	1	/* Normal line thickness */
#define	THICKNESS_WIDE		3	/* Fat line thickness */

/* Global/static variables. */

static	int	first_time = 1;
static	float	Previous_azimuth = -1.0;
static	float	Sin[ 720 ];
static	float	Cos[ 720 ];

/************************************************************************
 *	Description: This function graphically updates the position of	*
 *		     the radar antenna using azimuth info from base	*
 *		     data.						*
 *									*
 *	Input: NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void hci_update_antenna_position()
{
  int pixel1, pixel2;
  int scanl1, scanl2;
  float azimuth, first_azimuth;
  int azimuth1, azimuth2;
  int i;
  hci_control_panel_object_t *radome;

  /* Get reference to radome object. */

  radome = hci_control_panel_object( RADOME_OBJECT );

  /* Check to see if this is first time. If so, clear the
     radome and initialize the Sin and Cosine arrays (in
     half degree intervals). */

  if( first_time )
  {
    first_time = 0;
    hci_control_panel_radome( FORCE_REDRAW );
    for( i = 0; i < 720; i++ )
    {
      Sin[ i ] = ( float ) sin( ( double ) ( i/2.0 + 90 )*HCI_DEG_TO_RAD );
      Cos[ i ] = ( float ) cos( ( double ) ( i/2.0 - 90 )*HCI_DEG_TO_RAD );
    }
  }

  azimuth = hci_control_panel_azimuth();
  first_azimuth = hci_control_panel_first_azimuth();

  if( azimuth == Previous_azimuth || azimuth == HCI_INITIAL_AZIMUTH )
  {
    /* If no data to process, do nothing. */
    return;
  }
  else
  {
    /* Account for azimuth 360 or while loop (azimuth1 != azimuth2)
       may hang. */
    if( ( int ) azimuth == 360 ){ azimuth = 0.0; }
    Previous_azimuth = azimuth;
  }

  /* The antenna azimuth position is currently denoted by a
     thick ring which is drawn inside the radome circle. The
     last piece of the ring drawn is the current azimuthal
     position.  The first piece drawn is the start of the
     elevation sweep. */

  XSetForeground( HCI_get_display(),
                  hci_control_panel_gc(),
                  hci_get_read_color( BLACK ) );


  XSetLineAttributes( HCI_get_display(),
                      hci_control_panel_gc(),
                      THICKNESS_WIDE,
                      LineSolid,
                      CapButt,
                      JoinMiter );

  /* The ring is composed of a set of line segments running
     from the outermost radome circle inward. Each line segment
     represents a spoke along a given azimuth. */

  azimuth1 = ( ( int ) first_azimuth )*2;
  azimuth2 = ( ( int ) azimuth )*2;

  while( azimuth1 != azimuth2 )
  {
    pixel1 =  ( radome->width/2.0 + 1 )*Cos[ azimuth1 ];
    pixel1 += radome->pixel + radome->width/2;
    scanl1 = -( radome->height/2.0 + 1 )*Sin[ azimuth1 ];
    scanl1 += radome->scanl + radome->height/2;
    pixel2 =  ( 0.8*radome->width/2.0 )*Cos[ azimuth1 ];
    pixel2 += radome->pixel + radome->width/2;
    scanl2 = -( 0.8*radome->height/2.0 )*Sin[ azimuth1 ];
    scanl2 += radome->scanl + radome->height/2;

    XDrawLine( HCI_get_display(),
               hci_control_panel_pixmap(),
               hci_control_panel_gc(),
               pixel1, scanl1,
               pixel2, scanl2 );

    azimuth1++;
    azimuth1++;

    if( azimuth1 >= 720 )
    {
      azimuth1 = 0;
    }
  }

  /* Reset line attributes so other drawing does not use fat lines. */

  XSetLineAttributes( HCI_get_display(),
                      hci_control_panel_gc(),
                      THICKNESS_NORMAL,
                      LineSolid,
                      CapButt,
                      JoinMiter );
}
