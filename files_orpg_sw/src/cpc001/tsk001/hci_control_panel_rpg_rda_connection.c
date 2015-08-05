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
 * $Date: 2010/03/16 14:28:18 $
 * $Id: hci_control_panel_rpg_rda_connection.c,v 1.15 2010/03/16 14:28:18 ccalvert Exp $
 * $Revision: 1.15 $
 * $State: Exp $
 */


/* Local include file definitions. */

#include <hci_control_panel.h>

/* Define macros. */

#define	NUM_MOMENTS			4
#define	REFLECTIVITY_MOMENT_BITMASK	0x0001
#define	VELOCITY_MOMENT_BITMASK		0x0002
#define	SPECTRUM_WIDTH_MOMENT_BITMASK	0x0004
#define	DUAL_POL_MOMENT_BITMASK		0x0008

enum { REF_ROW = 1, VEL_ROW, SPW_ROW, DP_ROW };

/* Function prototypes. */

static void	draw_wideband( int wbstat );
static void	draw_reflectivity_moment( Pixel bg_color, Pixel fg_color );
static void	draw_velocity_moment( Pixel bg_color, Pixel fg_color );
static void	draw_spectrum_width_moment( Pixel bg_color, Pixel fg_color );
static void	draw_dual_pol_moment( Pixel bg_color, Pixel fg_color );
static void	check_for_rda_data_flow( int moments_status );

/* Global variables. */

static	int	Flow_marker = -1;
static	int	Data_flow_ref = HCI_NO_FLAG;
static	int	Data_flow_vel = HCI_NO_FLAG;
static	int	Data_flow_spw = HCI_NO_FLAG;
static	int	Data_flow_dp = HCI_NO_FLAG;
static	int	Data_bar_height = -1;
static	int	Data_bar_width = -1;

/************************************************************************
 *	Description: This function draws the connection between the	*
 *		     RDA and RPG containers.				*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void hci_control_panel_rpg_rda_connection( int force_draw )
{
  int i;
  int rda_state;
  int moments_status;
  int wbstat;
  int ignore_moments = HCI_NO_FLAG;
  int temp_int;
  hci_control_panel_object_t *rda;
  hci_control_panel_object_t *rpg;
  hci_control_panel_object_t *wideband;
  Pixel bg_color = -1;
  Pixel fg_color = -1;
  static int first_time = 1;
  static int prev_moments_status = -1;
  static int prev_wb_status = -1;

  /* If the RDA object is not defined, return.  We assume that
     if the RDA object exists, the rpg and wideband objects also
     exist. */

  rda = hci_control_panel_object( RDA_BUTTON );

  if( rda->widget == ( Widget ) NULL )
  {
    return;
  }

  /* Get various status parameters. */

  rda_state = ORPGRDA_get_status( RS_RDA_STATUS );
  moments_status = hci_control_panel_moments();
  wbstat = ORPGRDA_get_wb_status( ORPGRDA_WBLNSTAT );
  Data_bar_height = hci_control_panel_data_height();
  Data_bar_width = hci_control_panel_data_width();

  /* Take RDA State into account for wideband color. */

  if( rda_state == RDA_STANDBY )
  {
    ignore_moments = HCI_YES_FLAG;
  }

  /* If this is the first time the function is called,
     initialize the position of the data flow marker. */

  if( first_time )
  {
    Flow_marker = rda->pixel + rda->width + 1.5*Data_bar_width;
    first_time = 0;
  }

  /* Check to see if data is flowing. */

  check_for_rda_data_flow( moments_status );

  /* Redraw if something changes or if flag forces it. */

  if( moments_status != prev_moments_status ||
      wbstat != prev_wb_status ||
      Data_flow_ref == HCI_YES_FLAG ||
      Data_flow_vel == HCI_YES_FLAG ||
      Data_flow_spw == HCI_YES_FLAG ||
      Data_flow_dp == HCI_YES_FLAG ||
      force_draw )
  {
    /* Get references to gui objects. */

    rpg = hci_control_panel_object( RPG_BUTTONS_BACKGROUND );
    wideband = hci_control_panel_object( WIDEBAND_OBJECT );

    /* Set the wideband data connection x,y values. */

    temp_int = NUM_MOMENTS*Data_bar_height;

    wideband->pixel  = rda->pixel + rda->width + hci_control_panel_3d()/2;
    wideband->scanl  = rda->scanl + ( rda->height -  temp_int )/2;
    wideband->width  = rpg->pixel - wideband->pixel;
    wideband->height = temp_int;

    /* If the wideband is not connected, break the connection
       graphic between the RDA and RPG boxes. Otherwise, create
       create a graphical link for all moments. */

    if( wbstat != RS_CONNECTED )
    {
      draw_wideband( wbstat );
    }
    else
    {
      /* If the Reflectivity moment is enabled, color the link green,
         otherwise make it white. */

      if( ignore_moments == HCI_NO_FLAG &&
          ( moments_status & REFLECTIVITY_MOMENT_BITMASK ) )
      {
        bg_color = hci_get_read_color( NORMAL_COLOR );
        fg_color = hci_get_read_color( TEXT_FOREGROUND );
      }
      else
      {
        bg_color = hci_get_read_color( WHITE );
        fg_color = hci_get_read_color( TEXT_FOREGROUND );
      }

      draw_reflectivity_moment( bg_color, fg_color );

      /* If the Velocity moment is enabled, color the link green,
         otherwise make it white. */

      if( ignore_moments == HCI_NO_FLAG &&
          ( moments_status & VELOCITY_MOMENT_BITMASK ) )
      {
        bg_color = hci_get_read_color( NORMAL_COLOR );
        fg_color = hci_get_read_color( TEXT_FOREGROUND );
      }
      else
      {
        bg_color = hci_get_read_color( WHITE );
        fg_color = hci_get_read_color( TEXT_FOREGROUND );
      }

      draw_velocity_moment( bg_color, fg_color );

      /* If the Spectrum Width moment is enabled, color the link green,
         otherwise make it white. */

      if( ignore_moments == HCI_NO_FLAG &&
          ( moments_status & SPECTRUM_WIDTH_MOMENT_BITMASK ) )
      {
        bg_color = hci_get_read_color( NORMAL_COLOR );
        fg_color = hci_get_read_color( TEXT_FOREGROUND );
      }
      else
      {
        bg_color = hci_get_read_color( WHITE );
        fg_color = hci_get_read_color( TEXT_FOREGROUND );
      }

      draw_spectrum_width_moment( bg_color, fg_color );

      /* If the Dual-Pol moment is enabled, color the link green,
         otherwise make it white. */

      if( ignore_moments == HCI_NO_FLAG &&
          ( moments_status & DUAL_POL_MOMENT_BITMASK ) )
      {
        bg_color = hci_get_read_color( NORMAL_COLOR );
        fg_color = hci_get_read_color( TEXT_FOREGROUND );
      }
      else
      {
        bg_color = hci_get_read_color( WHITE );
        fg_color = hci_get_read_color( TEXT_FOREGROUND );
      }

      draw_dual_pol_moment( bg_color, fg_color );
    }

    /* Draw 3d shadow on top of wideband connection. */

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( TEXT_FOREGROUND ) );

    for( i = 0; i < hci_control_panel_3d(); i++)
    {
      XDrawLine( HCI_get_display(),
                 hci_control_panel_pixmap(),
                 hci_control_panel_gc(),
                 wideband->pixel,
                 ( wideband->scanl - i ),
                 ( wideband->pixel + wideband->width ),
                 ( wideband->scanl - i ) );
    }

    prev_moments_status = moments_status;
    prev_wb_status = wbstat;
  }

  /* Reset font to scaled. */

  XSetFont( HCI_get_display(),
            hci_control_panel_gc(),
            hci_get_font( SCALED ) );
}

/************************************************************************
 *	Description: This function draws the RDA/RPG wideband		*
 *		     connection without individual moments. 		*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

static void draw_wideband( int wbstat )
{
  hci_control_panel_object_t *wideband;
  char Cbuf1[ HCI_BUF_64 ] = "";
  char Cbuf2[ HCI_BUF_64 ] = "";
  int width;
  int height;
  XFontStruct *fontinfo;
  Pixel bg_color = -1;
  Pixel fg_color = -1;

  /* Get references to gui objects. */

  wideband = hci_control_panel_object( WIDEBAND_OBJECT );

  /* Determine wideband status text/color. */

  switch( wbstat )
  {
    case RS_NOT_IMPLEMENTED :
      sprintf( Cbuf1, "Not" );
      sprintf( Cbuf2, "Implemented" );
      bg_color = hci_get_read_color( WHITE );
      fg_color = hci_get_read_color( TEXT_FOREGROUND );
      break;

    case RS_CONNECT_PENDING :
      sprintf( Cbuf1, " Connect" );
      sprintf( Cbuf2, " Pending" );
      bg_color = hci_get_read_color( WARNING_COLOR );
      fg_color = hci_get_read_color( TEXT_FOREGROUND );
      break;

    case RS_DISCONNECT_PENDING :
      sprintf( Cbuf1, "Disconnect" );
      sprintf( Cbuf2, "Pending" );
      bg_color = hci_get_read_color( WARNING_COLOR );
      fg_color = hci_get_read_color( TEXT_FOREGROUND );
      break;

    case RS_DISCONNECTED_HCI :
      sprintf( Cbuf1, "Disconnected" );
      sprintf( Cbuf2, "From RPG" );
      bg_color = hci_get_read_color( WARNING_COLOR );
      fg_color = hci_get_read_color( TEXT_FOREGROUND );
      break;

    case RS_DISCONNECTED_CM :
      sprintf( Cbuf1,"Disconnected" );
      sprintf( Cbuf2,"CM" );
      bg_color = hci_get_read_color( WARNING_COLOR );
      fg_color = hci_get_read_color( TEXT_FOREGROUND );
      break;

    case RS_DISCONNECTED_SHUTDOWN :
      sprintf( Cbuf1, "Disconnected" );
      sprintf( Cbuf2, "SHUTDOWN" );
      bg_color = hci_get_read_color( WARNING_COLOR );
      fg_color = hci_get_read_color( TEXT_FOREGROUND );
      break;

    case RS_DISCONNECTED_RMS :
      sprintf( Cbuf1, "Disconnected" );
      sprintf( Cbuf2, "RMS" );
      bg_color = hci_get_read_color( WARNING_COLOR );
      fg_color = hci_get_read_color( TEXT_FOREGROUND );
      break;

    case RS_DOWN :
      sprintf( Cbuf1, "Down" );
      sprintf( Cbuf2, " " );
      bg_color = hci_get_read_color( ALARM_COLOR2 );
      fg_color = hci_get_read_color( TEXT_FOREGROUND );
      break;

    case RS_WBFAILURE :
      sprintf( Cbuf1, "Failure" );
      sprintf( Cbuf2, " " );
      bg_color = hci_get_read_color( ALARM_COLOR2 );
      fg_color = hci_get_read_color( TEXT_FOREGROUND );
      break;

    default :
      sprintf( Cbuf1, "Unknown" );
      sprintf( Cbuf2, " " );
      bg_color = hci_get_read_color( ALARM_COLOR2 );
      fg_color = hci_get_read_color( TEXT_FOREGROUND );
      break;
  }

  fontinfo = hci_get_fontinfo( SCALED );
  width = XTextWidth( fontinfo, Cbuf1, strlen( Cbuf1 ) );
  height = fontinfo->ascent + fontinfo->descent;

  /* Draw wideband connection without moments. */

  XSetForeground( HCI_get_display(),
                  hci_control_panel_gc(),
                  bg_color );

  XSetBackground( HCI_get_display(),
                  hci_control_panel_gc(),
                  bg_color );

  XFillRectangle( HCI_get_display(),
                  hci_control_panel_pixmap(),
                  hci_control_panel_gc(),
                  wideband->pixel,
                  wideband->scanl,
                  wideband->width,
                  wideband->height );

  XSetForeground( HCI_get_display(),
                  hci_control_panel_gc(),
                  fg_color );

  XDrawString( HCI_get_display(),
               hci_control_panel_pixmap(),
               hci_control_panel_gc(),
               ( wideband->pixel + wideband->width/2 - width/2 ),
               ( wideband->scanl + height + 1 ),
                Cbuf1,
                strlen( Cbuf1 ) );

  width = XTextWidth( fontinfo, Cbuf2, strlen( Cbuf2 ) );

  XDrawString( HCI_get_display(),
               hci_control_panel_pixmap(),
               hci_control_panel_gc(),
               ( wideband->pixel + wideband->width/2 - width/2 ),
               ( wideband->scanl + VEL_ROW*height + 1 ),
               Cbuf2,
               strlen( Cbuf2 ) );

  /* If wideband color is background color, then draw line
     at bottom to show outline of wideband connection. For
     any other color, no outline is needed. */

  if( bg_color == hci_get_read_color( BACKGROUND_COLOR1 ) )
  {
    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( TEXT_FOREGROUND ) );

    XDrawLine( HCI_get_display(),
               hci_control_panel_pixmap(),
               hci_control_panel_gc(),
               wideband->pixel,
               ( wideband->scanl + wideband->height ),
               ( wideband->pixel + wideband->width ),
               ( wideband->scanl + wideband->height ) );
  }
}

/************************************************************************
 *	Description: This function draws the reflectivity moment	*
 *		     on the RDA/RPG wideband connection.		*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

static void draw_reflectivity_moment( Pixel bg_color, Pixel fg_color )
{
  hci_control_panel_object_t *wideband;
  
  /* Get references to wideband object. */

  wideband = hci_control_panel_object( WIDEBAND_OBJECT );

  /* Draw moment rectangle. */
  
  XSetForeground( HCI_get_display(),
                  hci_control_panel_gc(),
                  bg_color );

  XFillRectangle( HCI_get_display(),
                  hci_control_panel_pixmap(),
                  hci_control_panel_gc(),
                  wideband->pixel,
                  wideband->scanl,
                  wideband->width,
                  Data_bar_height );

  /* Draw line below moment rectangle to separate from next moment. */

  XSetForeground( HCI_get_display(),
                  hci_control_panel_gc(),
                  hci_get_read_color( TEXT_FOREGROUND ) );

  XDrawLine( HCI_get_display(),
             hci_control_panel_pixmap(),
             hci_control_panel_gc(),
             wideband->pixel,
             wideband->scanl + REF_ROW*Data_bar_height - 1,
             wideband->pixel + wideband->width,
             wideband->scanl + REF_ROW*Data_bar_height - 1 );

  /* Draw moment letter. */
  
  XSetForeground( HCI_get_display(),
                  hci_control_panel_gc(),
                  fg_color );

  XDrawString( HCI_get_display(),
               hci_control_panel_pixmap(),
               hci_control_panel_gc(),
               ( wideband->pixel + 2 ),
               ( wideband->scanl + REF_ROW*Data_bar_height - 1 ),
               "R",
               ( int ) 1 );

  /* Draw data flow marker if needed. */

  if( Data_flow_ref == HCI_YES_FLAG )
  {
    XFillRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    Flow_marker,
                    wideband->scanl,
                    Data_bar_width,
                    Data_bar_height );
  }
}

/************************************************************************
 *	Description: This function draws the velocity moment		*
 *		     on the RDA/RPG wideband connection.		*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

static void draw_velocity_moment( Pixel bg_color, Pixel fg_color )
{
  hci_control_panel_object_t *wideband;
  
  /* Get references to wideband object. */

  wideband = hci_control_panel_object( WIDEBAND_OBJECT );

  /* Draw moment rectangle. */
  
  XSetForeground( HCI_get_display(),
                  hci_control_panel_gc(),
                  bg_color );

  XFillRectangle( HCI_get_display(),
                  hci_control_panel_pixmap(),
                  hci_control_panel_gc(),
                  wideband->pixel,
                  ( wideband->scanl + REF_ROW*Data_bar_height ),
                  wideband->width,
                  Data_bar_height );

  /* Draw line below moment rectangle to separate from next moment. */

  XSetForeground( HCI_get_display(),
                  hci_control_panel_gc(),
                  hci_get_read_color( TEXT_FOREGROUND ) );

  XDrawLine( HCI_get_display(),
             hci_control_panel_pixmap(),
             hci_control_panel_gc(),
             wideband->pixel,
             wideband->scanl + VEL_ROW*Data_bar_height - 1,
             wideband->pixel + wideband->width,
             wideband->scanl + VEL_ROW*Data_bar_height - 1 );

  /* Draw moment letter. */
  
  XSetForeground( HCI_get_display(),
                  hci_control_panel_gc(),
                  fg_color );

  XDrawString( HCI_get_display(),
               hci_control_panel_pixmap(),
               hci_control_panel_gc(),
               ( wideband->pixel + 2 ),
               ( wideband->scanl + VEL_ROW*Data_bar_height - 1),
               "V",
               ( int ) 1 );

  /* Draw data flow marker if needed. */

  if( Data_flow_vel == HCI_YES_FLAG )
  {
    XFillRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    Flow_marker,
                    ( wideband->scanl + REF_ROW*Data_bar_height ),
                    Data_bar_width,
                    Data_bar_height );
  }
}

/************************************************************************
 *	Description: This function draws the spectrum width moment	*
 *		     on the RDA/RPG wideband connection.		*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

static void draw_spectrum_width_moment( Pixel bg_color, Pixel fg_color )
{
  hci_control_panel_object_t *wideband;
  
  /* Get references to wideband object. */

  wideband = hci_control_panel_object( WIDEBAND_OBJECT );

  /* Draw moment rectangle. */
  
  XSetForeground( HCI_get_display(),
                  hci_control_panel_gc(),
                  bg_color );

  XFillRectangle( HCI_get_display(),
                  hci_control_panel_pixmap(),
                  hci_control_panel_gc(),
                  wideband->pixel,
                  ( wideband->scanl + VEL_ROW*Data_bar_height ),
                  wideband->width,
                  Data_bar_height );

  /* Draw line below moment rectangle. */

  XSetForeground( HCI_get_display(),
                  hci_control_panel_gc(),
                  hci_get_read_color( TEXT_FOREGROUND ) );

  XDrawLine( HCI_get_display(),
             hci_control_panel_pixmap(),
             hci_control_panel_gc(),
             wideband->pixel,
             wideband->scanl + SPW_ROW*Data_bar_height - 1,
             wideband->pixel + wideband->width,
             wideband->scanl + SPW_ROW*Data_bar_height - 1 );

  /* Draw moment letter. */
  
  XSetForeground( HCI_get_display(),
                  hci_control_panel_gc(),
                  fg_color );

  XDrawString( HCI_get_display(),
               hci_control_panel_pixmap(),
               hci_control_panel_gc(),
               ( wideband->pixel + 2 ),
               ( wideband->scanl + SPW_ROW*Data_bar_height - 1 ),
               "W",
               ( int ) 1 );

  /* Draw data flow marker if needed. */

  if( Data_flow_spw == HCI_YES_FLAG )
  {
    XFillRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    Flow_marker,
                    ( wideband->scanl + VEL_ROW*Data_bar_height ),
                    Data_bar_width,
                    Data_bar_height );
  }
}

/************************************************************************
 *	Description: This function draws the Dual-Pol moment		*
 *		     on the RDA/RPG wideband connection.		*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

static void draw_dual_pol_moment( Pixel bg_color, Pixel fg_color )
{
  hci_control_panel_object_t *wideband;
  
  /* Get references to wideband object. */

  wideband = hci_control_panel_object( WIDEBAND_OBJECT );

  /* Draw moment rectangle. */
  
  XSetForeground( HCI_get_display(),
                  hci_control_panel_gc(),
                  bg_color );

  XFillRectangle( HCI_get_display(),
                  hci_control_panel_pixmap(),
                  hci_control_panel_gc(),
                  wideband->pixel,
                  ( wideband->scanl + SPW_ROW*Data_bar_height ),
                  wideband->width,
                  Data_bar_height );

  /* Draw line below moment rectangle. */

  XSetForeground( HCI_get_display(),
                  hci_control_panel_gc(),
                  hci_get_read_color( TEXT_FOREGROUND ) );

  XDrawLine( HCI_get_display(),
             hci_control_panel_pixmap(),
             hci_control_panel_gc(),
             wideband->pixel,
             wideband->scanl + DP_ROW*Data_bar_height - 1,
             wideband->pixel + wideband->width,
             wideband->scanl + DP_ROW*Data_bar_height - 1 );

  /* Draw moment letter. */
  
  XSetForeground( HCI_get_display(),
                  hci_control_panel_gc(),
                  fg_color );

  XDrawString( HCI_get_display(),
               hci_control_panel_pixmap(),
               hci_control_panel_gc(),
               ( wideband->pixel + 2 ),
               ( wideband->scanl + DP_ROW*Data_bar_height - 1 ),
               "D",
               ( int ) 1 );

  /* Draw data flow marker if needed. */

  if( Data_flow_dp == HCI_YES_FLAG )
  {
    XFillRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    Flow_marker,
                    ( wideband->scanl + SPW_ROW*Data_bar_height ),
                    Data_bar_width,
                    Data_bar_height );
  }
}

/************************************************************************
 *	Description: This function checks to see if data flow markers	*
 *		     need to be drawn.					*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

static void check_for_rda_data_flow( int moments_status )
{
  int radial_number;
  int elevation_number;
  int vcp_wave_type;
  int super_res;
  hci_control_panel_object_t *rpg;
  hci_control_panel_object_t *rda;
  static int previous_radial_number = -1;

  /* Get references to gui objects. */

  rda = hci_control_panel_object( RDA_BUTTON );
  rpg = hci_control_panel_object( RPG_BUTTONS_BACKGROUND );

  /* Default is for no data flow. */

  Data_flow_ref = HCI_NO_FLAG;
  Data_flow_vel = HCI_NO_FLAG;
  Data_flow_spw = HCI_NO_FLAG;
  Data_flow_dp = HCI_NO_FLAG;

  /* If radial numbers are changing, assume data is flowing. Set
     appropriate flag for each moment. */

  radial_number = hci_control_panel_azimuth_num();

  if( previous_radial_number != radial_number )
  {
    previous_radial_number = radial_number;
    elevation_number = hci_control_panel_elevation_num() - 1;
    super_res = hci_control_panel_super_res();
    vcp_wave_type = hci_current_vcp_wave_type( elevation_number );

    /* Increment data flow marker. */

    Flow_marker += Data_bar_width;

    if( Flow_marker > ( rpg->pixel - Data_bar_width ) )
    {
      Flow_marker = rda->pixel + rda->width + 1.5*Data_bar_width;
    }

    /* Is reflectivity data flowing? */

    if( moments_status & REFLECTIVITY_MOMENT_BITMASK &&
        ( vcp_wave_type == WAVEFORM_CONTIGUOUS_SURVEILLANCE ||
          vcp_wave_type == WAVEFORM_CONTIGUOUS_DOPPLER_WITHOUT_AMB ||
         (vcp_wave_type == WAVEFORM_CONTIGUOUS_DOPPLER_WITH_AMB && super_res) ||
          vcp_wave_type == WAVEFORM_STAGGERED_PULSE_PAIR ||
          vcp_wave_type == WAVEFORM_BATCH ) )
    {
      Data_flow_ref = HCI_YES_FLAG;
    }

    /* Is velocity data flowing? */

    if( moments_status & VELOCITY_MOMENT_BITMASK &&
        ( vcp_wave_type == WAVEFORM_CONTIGUOUS_DOPPLER_WITH_AMB ||
          vcp_wave_type == WAVEFORM_CONTIGUOUS_DOPPLER_WITHOUT_AMB ||
          vcp_wave_type == WAVEFORM_STAGGERED_PULSE_PAIR ||
          vcp_wave_type == WAVEFORM_BATCH ) )
    {
      Data_flow_vel = HCI_YES_FLAG;
    }

    /* Is spectrum width data flowing? */

    if( moments_status & SPECTRUM_WIDTH_MOMENT_BITMASK &&
        ( vcp_wave_type == WAVEFORM_CONTIGUOUS_DOPPLER_WITH_AMB ||
          vcp_wave_type == WAVEFORM_CONTIGUOUS_DOPPLER_WITHOUT_AMB ||
          vcp_wave_type == WAVEFORM_STAGGERED_PULSE_PAIR ||
          vcp_wave_type == WAVEFORM_BATCH ) )
    {
      Data_flow_spw = HCI_YES_FLAG;
    }

    /* Is dual-pol data flowing? */

    if( moments_status & DUAL_POL_MOMENT_BITMASK &&
        ( vcp_wave_type == WAVEFORM_CONTIGUOUS_SURVEILLANCE ||
          vcp_wave_type == WAVEFORM_CONTIGUOUS_DOPPLER_WITH_AMB ||
          vcp_wave_type == WAVEFORM_CONTIGUOUS_DOPPLER_WITHOUT_AMB ||
          vcp_wave_type == WAVEFORM_STAGGERED_PULSE_PAIR ||
          vcp_wave_type == WAVEFORM_BATCH ) )
    {
      Data_flow_dp = HCI_YES_FLAG;
    }

  }
}
