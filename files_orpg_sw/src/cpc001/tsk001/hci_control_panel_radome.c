/************************************************************************
 *									*
 *	Module:  hci_control_panel_radome.c				*
 *									*
 *	Description:  This module is used to display a filled circle	*
 *		      in the main HCI window representing the radar	*
 *		      radome.  Its color signifies whether an alarm	*
 *		      condition has occurred (Red = alarm, White =	*
 *		      nominal).  To ensure proper placement of the	*
 *		      radome, this routine should be called AFTER	*
 *		      the call to "hci_control_panel_RDA_button ()".	*
 *									*
 ************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/10/03 16:32:50 $
 * $Id: hci_control_panel_radome.c,v 1.14 2014/10/03 16:32:50 steves Exp $
 * $Revision: 1.14 $
 * $State: Exp $
 */

/* Local include file definitions. */

#include <hci_control_panel.h>

/* Define macros. */

#define	ELEVATION_NOT_DEFINED		-99.0
#define SUPER_RES_NOT_DEFINED		-1
#define SAILS_CUT_NOT_DEFINED  		 0
#define LAST_ELEV_NOT_DEFINED  		 0

/* Global/static variables. */

static	int	Elev_num = -1;
static	int	Super_res = SUPER_RES_NOT_DEFINED;
static	int	N_sails_cuts = SAILS_CUT_NOT_DEFINED;
static	int	Sails_cut_seq = SAILS_CUT_NOT_DEFINED;
static	float	Elevation = ELEVATION_NOT_DEFINED;
static	int	Last_ele_flag = LAST_ELEV_NOT_DEFINED;

/************************************************************************
 *	Description: This function saves data to be later used to	*
 *		     populate the radome.				*
 *									*
 *	Input:  elev_num - RDA elevation number.			*
 *		super_res - Super Res this cut (1) or not (0).		*
 *		elevation - elevation angle (deg).			*
 *		n_sails_cuts - number of SAILS cuts this VCP.		*
 *		sails_cut_seq - SAILS cut sequence number.		*
 *		last_elev_flag - Last elevation flag Yes (1), No (0).	*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void hci_control_panel_radome_data( int elev_num, int super_res,
                                    float elevation, int n_sails_cuts,
                                    int sails_cut_seq, int last_elev_flag )
{
   Elev_num = elev_num;
   Super_res = super_res;
   Elevation = elevation;
   N_sails_cuts = n_sails_cuts;
   Sails_cut_seq = sails_cut_seq;
   Last_ele_flag = last_elev_flag;
}

/************************************************************************
 *	Description: This function draws a filled cirlce representing	*
 *		     the radome.  Other functions are used to populate	*
 *		     the inside.					*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void hci_control_panel_radome( int force_draw )
{
  int center_pixel;
  int center_scanl;
  int temp_int;
  int elev_num;
  int last_ele_flag = LAST_ELEV_NOT_DEFINED;
  int rda_state;
  int super_res;
  int sails_cut = SAILS_CUT_NOT_DEFINED;
  int n_sails_cuts = SAILS_CUT_NOT_DEFINED;
  int wbstat;
  int wb_connect;
  float elevation;
  int x_off, y_off;
  char buf[ HCI_BUF_32 ];
  XFontStruct *fontinfo;
  hci_control_panel_object_t *top;
  hci_control_panel_object_t *rda;
  hci_control_panel_object_t *radome;
  static int prev_wb_connect = -1;
  static float prev_elevation = ELEVATION_NOT_DEFINED;
  static char *qualifier[] = { "SAILS", "1st ", "2nd ", "3rd " };

  /* The RDA button is the anchor for the tower and radome objects.
     Therefore, it must be defined before the radome position is
     calculated. */

  rda = hci_control_panel_object( RDA_BUTTON );

  if( rda->widget == ( Widget ) NULL )
  {
    return;
  }

  /* Get wideband status. If connected, get current elevation. If
     not, note that elevation isn't defined. */

  wbstat = ORPGRDA_get_wb_status( ORPGRDA_WBLNSTAT );

  if( wbstat == RS_CONNECTED ||
      wbstat == RS_DISCONNECT_PENDING ||
      !ORPGMISC_is_operational() )
  {
    wb_connect = WIDEBAND_CONNECT;
    elev_num = hci_control_panel_elevation_num();
    if( elev_num < 0 || elev_num >= HCI_MAX_CUTS_IN_LIST )
    {
      HCI_LE_error( "Invalid Elevation number: (%d)", elev_num );
      elevation = ELEVATION_NOT_DEFINED;
      super_res = SUPER_RES_NOT_DEFINED;
      last_ele_flag = LAST_ELEV_NOT_DEFINED;
      sails_cut = SAILS_CUT_NOT_DEFINED;
      n_sails_cuts = SAILS_CUT_NOT_DEFINED;
    }
    else
    {
      if( elev_num == Elev_num )
      {
        super_res = Super_res;
        if( !super_res ){ super_res = SUPER_RES_NOT_DEFINED; }
        elevation = Elevation;
        sails_cut = Sails_cut_seq;
        n_sails_cuts = N_sails_cuts;
        last_ele_flag  = Last_ele_flag;
      }
      else
      {
        super_res = SUPER_RES_NOT_DEFINED;
        elevation = ELEVATION_NOT_DEFINED;
        last_ele_flag = LAST_ELEV_NOT_DEFINED;
        sails_cut = SAILS_CUT_NOT_DEFINED;
        n_sails_cuts = SAILS_CUT_NOT_DEFINED;
      }
    }

    /* If RDA state is STANDBY or OFFLINE OPERATE, elevation isn't defined. */

    rda_state = ORPGRDA_get_status( RS_RDA_STATUS );

    if( ( rda_state == RDA_STANDBY || rda_state == RDA_OFFLINE_OPERATE ) &&
        ORPGMISC_is_operational() )
    {
      elevation = ELEVATION_NOT_DEFINED;
      super_res = SUPER_RES_NOT_DEFINED;
      sails_cut = SAILS_CUT_NOT_DEFINED;
      n_sails_cuts = SAILS_CUT_NOT_DEFINED;
      last_ele_flag = LAST_ELEV_NOT_DEFINED;
    }
  }
  else
  {
    wb_connect = WIDEBAND_DISCONNECT;
    elevation = ELEVATION_NOT_DEFINED;
    super_res = SUPER_RES_NOT_DEFINED;
    last_ele_flag = LAST_ELEV_NOT_DEFINED;
    sails_cut = SAILS_CUT_NOT_DEFINED;
    n_sails_cuts = SAILS_CUT_NOT_DEFINED;
  }

  if( wb_connect != prev_wb_connect ||
      elevation != prev_elevation ||
      force_draw )
  {
    /* Get reference to top-level and radome objects. */

    top = hci_control_panel_object( TOP_WIDGET );
    radome = hci_control_panel_object( RADOME_OBJECT );

    /* Determine the screen coordinates for the radome. It is to
       be drawn at the top of the tower. */

    temp_int = hci_control_panel_3d()/2;

    radome->pixel = rda->pixel - 10 + temp_int;
    radome->scanl = rda->scanl - top->height/4 - rda->width - 20 - temp_int;
    radome->width = rda->width + 20;
    radome->height = radome->width;

    center_pixel = radome->pixel + rda->width/2;
    center_scanl = radome->scanl + rda->width/2;

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( WHITE ) );

    /* Draw a filled circle to represent the radome. The radome is
       normally white. */

    XFillArc( HCI_get_display(),
              hci_control_panel_pixmap(),
              hci_control_panel_gc(),
              ( int ) radome->pixel,
              ( int ) radome->scanl,
              ( int ) radome->width,
              ( int ) radome->width,
              ( int ) ( 90*64 ),
              ( int ) ( -360*64 ) );

    if( wb_connect == WIDEBAND_CONNECT  && elevation != ELEVATION_NOT_DEFINED )
    {
      /* Textually display the current elevation angle inside
         the radome. Scale the font so it stands out. */

      XSetFont( HCI_get_display(),
                hci_control_panel_gc(),
                hci_get_font_adj( SCALED, 2.5 ) );

      XSetBackground( HCI_get_display(),
                      hci_control_panel_gc(),
                      hci_get_read_color( WHITE ) );

      XSetForeground( HCI_get_display(),
                      hci_control_panel_gc(),
                      hci_get_read_color( BLACK ) );

      /* Take elevation angle format into account for centering purposes. */

      if( sails_cut == SAILS_CUT_NOT_DEFINED )
      {
         if( elevation < 10.0 )
         {
           sprintf( buf, "%4.1f ", elevation );
         }
         else
         {
           sprintf( buf, "%5.1f ", elevation );
         }
      }
      else
      {
         if( (sails_cut == 1) && (n_sails_cuts == 1) )
         {
           sprintf( buf, "%s", qualifier[0] );
         }
         else if( (sails_cut > 0) && (sails_cut <= n_sails_cuts) )
         {
           sprintf( buf, "%s", qualifier[sails_cut] );
         }
         else
         {
           sprintf( buf, "??? " );
         }
      }

      /* Find offset to center text box inside radome. Offset in X
         direction is simply half the text box width. Offset in Y
         direction is half the ascent and descent. However, since
         XDrawImageString automatically subtracts the ascent from the Y
         coordinate passed in, we have to add the ascent to the offset
         to get a "true" center. Again, scale the font so it stands out.
         If there is an error obtaining the new font, revert to the
         unadjusted font. */

      fontinfo = hci_get_fontinfo_adj( SCALED, 2.5 );

      if( fontinfo == NULL )
      {
        HCI_LE_error( "Could not adjust radome elevation font." );
        fontinfo = hci_get_fontinfo( SCALED );
      }

      if( ((sails_cut != SAILS_CUT_NOT_DEFINED) && (n_sails_cuts > 1))
                     ||
          (last_ele_flag != LAST_ELEV_NOT_DEFINED) )
      {
        temp_int = 6*( fontinfo->ascent + fontinfo->descent )/5;
      }
      else if( (sails_cut == 1) && (n_sails_cuts == 1) )
      {
        temp_int = ( fontinfo->ascent + fontinfo->descent )/2;
      }
      else if( super_res != SUPER_RES_NOT_DEFINED )
      {
        temp_int = 4*( fontinfo->ascent + fontinfo->descent )/5;
      }
      else
      {
        temp_int = ( fontinfo->ascent + fontinfo->descent )/2;
      }

      x_off = XTextWidth( fontinfo, buf, strlen( buf ) )/2;
      y_off = fontinfo->ascent - temp_int;
      
      /* Draw text boxes (ImageString) in radome. */

      XDrawImageString( HCI_get_display(),
                        hci_control_panel_pixmap(),
                        hci_control_panel_gc(),
                        ( int ) ( radome->pixel + radome->width/2 - x_off ),
                        ( int ) ( radome->scanl + radome->height/2 + y_off ),
                        buf,
                        strlen( buf ) );

      buf[0] = '\0';
      if( (sails_cut == SAILS_CUT_NOT_DEFINED) 
                     && 
          ((super_res != SUPER_RES_NOT_DEFINED)
                     ||
           (last_ele_flag != LAST_ELEV_NOT_DEFINED)) )
      {
        if( last_ele_flag )
           sprintf( buf, "%s", "LAST" );
        else
           sprintf( buf, "%s", "SR" );
      }
      else if( (sails_cut != SAILS_CUT_NOT_DEFINED) && (n_sails_cuts > 1))
      {
        sprintf( buf, "%s", "SAILS" );
      }

      if( strlen(buf) > 0 )
      {
        x_off = XTextWidth( fontinfo, buf, strlen( buf ) )/2;

        XDrawImageString( HCI_get_display(),
                        hci_control_panel_pixmap(),
                        hci_control_panel_gc(),
                        ( int ) ( radome->pixel + radome->width/2 - x_off ),
                        ( int ) ( radome->scanl + 3*radome->height/4 + y_off ),
                        buf,
                        strlen( buf ) );
      }

      /* Must call after hci_get_fontinfo_adj or there will be
         a memory leak. */

      if( fontinfo != NULL )
        hci_free_fontinfo_adj( fontinfo );

      /* Reset colors/font to original values. */

      XSetBackground( HCI_get_display(),
                      hci_control_panel_gc(),
                      hci_get_read_color( BACKGROUND_COLOR1 ) );

      XSetFont( HCI_get_display(),
                hci_control_panel_gc(),
                hci_get_font( SCALED ) );

    }

    prev_wb_connect = wb_connect;
    prev_elevation = elevation;
  }
}
