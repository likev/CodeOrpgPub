/************************************************************************
 *									*
 *	Module:  hci_control_panel_vcp.c				*
 *									*
 *	Description:  This module is used to display the current VCP	*
 *		      in the HCI window.  It should only be called	*
 *		      after the RDA button is resized and positioned.	*
 *									*
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2014/07/21 20:05:29 $
 * $Id: hci_control_panel_vcp.c,v 1.38 2014/07/21 20:05:29 ccalvert Exp $
 * $Revision: 1.38 $
 * $State: Exp $
 */

/* Local include file definitions. */

#include <hci_control_panel.h>

/* Definitions */

#define VCP_DATA_LABEL_LENGTH	75
#define LABEL_PIXEL_OFFSET	20
#define LABEL_EDGE_MARGIN	2
#define MARGIN_BETWEEN_LABELS	4

/* Gobal/static variables. */

char prev_Vcp_buf[64] = "";
char prev_avset_buf[16] = "";
int  prev_avset_state = -1;
char prev_sails_buf[16] = "";
int  prev_sails_status = -1;
char prev_PRF_mode_buf[16] = "";
char prev_perfcheck_buf[16] = "";

/************************************************************************
 *	Description: This function is used to graphically and textually	*
 *		     display information about the current VCP in the	*
 *		     RPG Control/Status window.				*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void hci_control_panel_vcp( int force_draw )
{
  int status_vcp_number = 0;
  int scan_vcp_number = 0;
  int scan_mode_operation = 0;
  int wbstat = 0;
  int avset_status = 0;
  int avset_state = 0;
  int sails_status = 0;
  int sails_cuts = 0;
  int x1 = 0;
  int x2 = 0;
  int x3 = 0;
  int y1 = 0;
  int max_tag_width = 0;
  int max_value_width = 0;
  int max_label_width = 0;
  int tag_width = 0;
  int value_width = 0;
  int tag_ctr = 0;
  int value_ctr = 0;
  int temp_height = 0;
  unsigned char flag = 0;
  int rda_state = 0;
  int font_height = 0;
  Pixel tag_bg_color = hci_get_read_color( BACKGROUND_COLOR2 );
  Pixel tag_fg_color = hci_get_read_color( TEXT_FOREGROUND );
  Pixel value_bg_color = (Pixel) NULL;
  Pixel value_fg_color = (Pixel) NULL;
  XFontStruct *fontinfo = (XFontStruct *) NULL;
  hci_control_panel_object_t *top = (hci_control_panel_object_t *) NULL;
  hci_control_panel_object_t *radome = (hci_control_panel_object_t *) NULL;
  hci_control_panel_object_t *vcp = (hci_control_panel_object_t *) NULL;
  hci_control_panel_object_t *rpg = (hci_control_panel_object_t *) NULL;
  hci_control_panel_object_t *avset = (hci_control_panel_object_t *) NULL;
  hci_control_panel_object_t *sails = (hci_control_panel_object_t *) NULL;
  hci_control_panel_object_t *prfmode = (hci_control_panel_object_t *) NULL;
  hci_control_panel_object_t *perfcheck = (hci_control_panel_object_t *) NULL;
  hci_perfcheck_info_t *pc_info = (hci_perfcheck_info_t *) NULL;
  char buf1[ 24 ] = "";
  char buf2[ 24 ] = "";

  /* Get referene to gui objects. */

  top = hci_control_panel_object( TOP_WIDGET );
  radome = hci_control_panel_object( RADOME_OBJECT );
  vcp = hci_control_panel_object( VCP_CONTROL_OBJECT );
  rpg = hci_control_panel_object( RPG_BUTTONS_BACKGROUND );
  avset = hci_control_panel_object( AVSET_STATUS_OBJECT );
  sails = hci_control_panel_object( SAILS_STATUS_OBJECT );
  prfmode = hci_control_panel_object( PRFMODE_STATUS_OBJECT );
  perfcheck = hci_control_panel_object( PERFCHECK_STATUS_OBJECT );

  /* If the top widget does not exist, do nothing. */

  if( top->widget == ( Widget ) NULL )
  {
    HCI_LE_error( "hci_control_panel_vcp() top->widget == NULL" );
    return;
  }

  /* We want to use a scaled font based on the size of the main HCI. */

  XSetFont( HCI_get_display(),
            hci_control_panel_gc(),
            hci_get_font( SCALED ) );

  /* Set font information. */

  fontinfo = hci_get_fontinfo( SCALED );
  font_height = fontinfo->ascent + fontinfo->descent;

  /* Calculate width of status labels by using longest tag and
     longest value (the tag and value may not be related) */

  sprintf( buf1, "Perf Check In: " );
  sprintf( buf2, " MULTI-STORM " );

  /* After finding widths, determine pixel position of start of tag
     section (x1), end of tag/beginning of value section (x2),
     end of value section (x3) and scanl position (y1) */

  max_tag_width = XTextWidth( fontinfo, buf1, strlen( buf1 ) );
  max_value_width = XTextWidth( fontinfo, buf2, strlen( buf2 ) );
  max_label_width = max_tag_width + max_value_width;

  x1 = rpg->pixel + rpg->width + hci_control_panel_3d() + LABEL_PIXEL_OFFSET;
  x2 = x1 + max_tag_width;
  x3 = x1 + max_label_width;
  tag_ctr = x1 + max_tag_width/2;
  value_ctr = x2 + max_value_width/2;
  y1 = radome->scanl/2;
  
  /******************* Volume Coverage Pattern (VCP) *******************/

  /* Get wideband connection status. If not connected, remove VCP button. */

  wbstat = ORPGRDA_get_wb_status( ORPGRDA_WBLNSTAT );
  rda_state = ORPGRDA_get_status( RS_RDA_STATUS );

  if( wbstat != RS_CONNECTED &&
      wbstat != RS_DISCONNECT_PENDING &&
      ORPGMISC_is_operational() )
  {
    sprintf( buf2, "N/A" );
  }
  else if( ( rda_state == RDA_STANDBY || rda_state == RDA_OFFLINE_OPERATE ) &&
           ORPGMISC_is_operational() )
  {
    sprintf( buf2, "N/A" );
  }
  else
  {
    /* Get VCP information from the latest RDA status data message and
       from the volume status message. */

    status_vcp_number = ORPGRDA_get_status( RS_VCP_NUMBER );
    scan_vcp_number = ORPGVST_get_vcp();
    scan_mode_operation = ORPGVST_get_mode();

    /* If the VCP number in the latest RDA status data message is
       negative, this is a local pattern.  Preceed the VCP number
       with the letter "L". If the VCP number in the latest RDA
       status message is positive, this is a remote pattern.
       Proceed the VCP number with the letter "R". */

    if( status_vcp_number < 0 )
    {
      if( scan_mode_operation == CLEAR_AIR_MODE )
      {
        sprintf( buf2, "L%d/B", scan_vcp_number );
      }
      else if( scan_mode_operation == PRECIPITATION_MODE )
      {
        sprintf( buf2, "L%d/A", scan_vcp_number );
      }
      else
      {
        sprintf( buf2, "L%d", scan_vcp_number );
      }
    }
    else if( status_vcp_number > 0 )
    {
      if( scan_mode_operation == CLEAR_AIR_MODE )
      {
        sprintf( buf2, "R%d/B", scan_vcp_number );
      }
      else if( scan_mode_operation == PRECIPITATION_MODE )
      {
        sprintf( buf2, "R%d/A", scan_vcp_number );
      }
      else
      {
        sprintf( buf2, "R%d/M", scan_vcp_number );
      }
    }
    else
    {
      if( scan_mode_operation == CLEAR_AIR_MODE )
      {
        sprintf( buf2, "%d/B", scan_vcp_number );
      }
      else if( scan_mode_operation == PRECIPITATION_MODE )
      {
        sprintf( buf2, "%d/A", scan_vcp_number );
      }
      else
      {
        sprintf( buf2, "%d", scan_vcp_number );
      }
    }
  }

  sprintf( buf1, "VCP:" );
  tag_width = XTextWidth( fontinfo, buf1, strlen( buf1 ) );
  value_width = XTextWidth( fontinfo, buf2, strlen( buf2 ) );

  vcp->pixel = x2;
  vcp->scanl = y1 - font_height;
  vcp->height = font_height;
  vcp->width = max_value_width;

  if( force_draw )
  {
    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    tag_bg_color );

    XFillRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    x1 - LABEL_EDGE_MARGIN,
                    vcp->scanl,
                    max_tag_width + LABEL_EDGE_MARGIN,
                    font_height + LABEL_EDGE_MARGIN );

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    tag_fg_color );

    XDrawString( HCI_get_display(),
                 hci_control_panel_pixmap(),
                 hci_control_panel_gc(),
                 x1,
                 vcp->scanl + font_height,
                 buf1,
                 strlen( buf1 ) );
  }

  if( strcmp( buf2, prev_Vcp_buf ) != 0 || force_draw )
  {
    value_bg_color = hci_get_read_color( WHITE );
    value_fg_color = hci_get_read_color( TEXT_FOREGROUND );

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    value_bg_color);

    XFillRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    x2,
                    vcp->scanl,
                    max_value_width,
                    font_height + LABEL_EDGE_MARGIN );

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    value_fg_color );

    XDrawString( HCI_get_display(),
                 hci_control_panel_pixmap(),
                 hci_control_panel_gc(),
                 value_ctr - value_width/2,
                 vcp->scanl + font_height,
                 buf2,
                 strlen( buf2) );

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    value_fg_color );

    XDrawRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    x1 - LABEL_EDGE_MARGIN,
                    vcp->scanl, 
                    max_label_width + LABEL_EDGE_MARGIN,
                    font_height + LABEL_EDGE_MARGIN );

    strcpy( prev_Vcp_buf, buf2 );
  }

  temp_height = vcp->scanl;

  /***** Automated Volume Scan Evaluation and Termination (AVSET) *****/

  ORPGINFO_statefl_flag( ORPGINFO_STATEFL_FLG_AVSET_ENABLED,
                         ORPGINFO_STATEFL_GET,
                         &flag );

  avset_state = (int) flag;
  avset_status = ORPGRDA_get_status( RS_AVSET );

  if( ( ORPGRDA_get_rda_config( NULL ) == ORPGRDA_ORDA_CONFIG )
                               &&
      ( wbstat == RS_CONNECTED ) )
  {
    switch( flag )
    {
      case 1 : /* Auto Volume Scan Evaluation and Termination Enabled */
      {
        if( avset_status == AVSET_DISABLED || avset_status != AVSET_ENABLED )
        {
          sprintf( buf2, "PENDING" );
          value_bg_color = hci_get_read_color( NORMAL_COLOR );
          value_fg_color = hci_get_read_color( TEXT_FOREGROUND );
        }
        else
        {
          sprintf( buf2, "ENABLED" );
          value_bg_color = hci_get_read_color( NORMAL_COLOR );
          value_fg_color = hci_get_read_color( TEXT_FOREGROUND );
        }
        break;
      }

      case 0 : /* Auto Volume Scan Evaluation and Termination Disabled */
      {
        if( avset_status == AVSET_ENABLED )
        {
          sprintf( buf2, "PENDING" );
          value_bg_color = hci_get_read_color( WARNING_COLOR );
          value_fg_color = hci_get_read_color( TEXT_FOREGROUND );
        }
        else
        {
          sprintf( buf2, "DISABLED" );
          value_bg_color = hci_get_read_color( WARNING_COLOR );
          value_fg_color = hci_get_read_color( TEXT_FOREGROUND );
        }
        break;
      }

      default : /* Shouldn't happen */
      {
        sprintf( buf2, "????" );
        value_bg_color = hci_get_read_color( WARNING_COLOR );
        value_fg_color = hci_get_read_color( TEXT_FOREGROUND );
        break;
      }
    }
  }
  else
  {
    sprintf( buf2, "????" );
    value_bg_color = hci_get_read_color( WARNING_COLOR );
    value_fg_color = hci_get_read_color( TEXT_FOREGROUND );
  }

  sprintf( buf1, "AVSET:" );
  tag_width = XTextWidth( fontinfo, buf1, strlen( buf1 ) );
  value_width = XTextWidth( fontinfo, buf2, strlen( buf2 ) );

  avset->pixel  = x2;
  avset->scanl  = temp_height + font_height + MARGIN_BETWEEN_LABELS;
  avset->height = font_height;
  avset->width  = max_value_width;

  if( force_draw )
  {
    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    tag_bg_color );

    XFillRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    x1 - LABEL_EDGE_MARGIN,
                    avset->scanl,
                    max_tag_width + LABEL_EDGE_MARGIN,
                    font_height + LABEL_EDGE_MARGIN );

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    tag_fg_color );

    XDrawString( HCI_get_display(),
                 hci_control_panel_pixmap(),
                 hci_control_panel_gc(),
                 x1,
                 avset->scanl + font_height,
                 buf1,
                 strlen( buf1 ) );
  }

  if( (strcmp( buf2, prev_avset_buf ) != 0)
                       ||
      (force_draw) || (avset_state != prev_avset_state) )
  {
    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    value_bg_color );

    XFillRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    x2,
                    avset->scanl,
                    max_value_width,
                    font_height + LABEL_EDGE_MARGIN );

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    value_fg_color );

    XDrawString( HCI_get_display(),
                 hci_control_panel_pixmap(),
                 hci_control_panel_gc(),
                 value_ctr - value_width/2,
                 avset->scanl + font_height,
                 buf2,
                 strlen( buf2 ) );

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    value_fg_color );

    XDrawRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    x1 - LABEL_EDGE_MARGIN,
                    avset->scanl,
                    max_label_width + LABEL_EDGE_MARGIN,
                    font_height + LABEL_EDGE_MARGIN );

    strcpy( prev_avset_buf, buf2 );
    prev_avset_state = avset_state;
  }

  temp_height = avset->scanl;

  /***** SAILS *****/

  switch( ( sails_status = hci_sails_get_status() ) )
  {
    case HCI_SAILS_STATUS_ACTIVE:

      if( ( sails_cuts = hci_sails_get_num_cuts() ) < 1 )
      {
        sprintf( buf2, "ACTIVE/?" );
      }
      else
      {
        sprintf( buf2, "ACTIVE/%1d", sails_cuts );
      }
      value_bg_color = hci_get_read_color( NORMAL_COLOR );
      value_fg_color = hci_get_read_color( TEXT_FOREGROUND );
      break;

    case HCI_SAILS_STATUS_INACTIVE:

      sprintf( buf2, "INACTIVE" );
      value_bg_color = hci_get_read_color( NORMAL_COLOR );
      value_fg_color = hci_get_read_color( TEXT_FOREGROUND );
      break;

    case HCI_SAILS_STATUS_DISABLED:

      sprintf( buf2, "DISABLED" );
      value_bg_color = hci_get_read_color( WARNING_COLOR );
      value_fg_color = hci_get_read_color( TEXT_FOREGROUND );
      break;

    default : /* Shouldn't happen */

      sprintf( buf2, "????" );
      value_bg_color = hci_get_read_color( WARNING_COLOR );
      value_fg_color = hci_get_read_color( TEXT_FOREGROUND );
      break;
  }

  sprintf( buf1, "SAILS:" );
  tag_width = XTextWidth( fontinfo, buf1, strlen( buf1 ) );
  value_width = XTextWidth( fontinfo, buf2, strlen( buf2 ) );

  sails->pixel  = x2;
  sails->scanl  = temp_height + font_height + MARGIN_BETWEEN_LABELS;
  sails->height = font_height;
  sails->width  = max_value_width;

  if( force_draw )
  {
    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    tag_bg_color );

    XFillRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    x1 - LABEL_EDGE_MARGIN,
                    sails->scanl,
                    max_tag_width + LABEL_EDGE_MARGIN,
                    font_height + LABEL_EDGE_MARGIN );

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    tag_fg_color );

    XDrawString( HCI_get_display(),
                 hci_control_panel_pixmap(),
                 hci_control_panel_gc(),
                 x1,
                 sails->scanl + font_height,
                 buf1,
                 strlen( buf1 ) );
  }

  if( (strcmp( buf2, prev_sails_buf ) != 0)
                       ||
      (force_draw) || (sails_status != prev_sails_status) )
  {
    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    value_bg_color );

    XFillRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    x2,
                    sails->scanl,
                    max_value_width,
                    font_height + LABEL_EDGE_MARGIN );

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    value_fg_color );

    XDrawString( HCI_get_display(),
                 hci_control_panel_pixmap(),
                 hci_control_panel_gc(),
                 value_ctr - value_width/2,
                 sails->scanl + font_height,
                 buf2,
                 strlen( buf2 ) );

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    value_fg_color );

    XDrawRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    x1 - LABEL_EDGE_MARGIN,
                    sails->scanl,
                    max_label_width + LABEL_EDGE_MARGIN,
                    font_height + LABEL_EDGE_MARGIN );

    strcpy( prev_sails_buf, buf2 );
    prev_sails_status = sails_status;
  }

  temp_height = sails->scanl;

  /***************************** PRF Mode *****************************/
  
  switch( hci_get_PRF_Mode_state() )
  {
    case HCI_PRF_MODE_MANUAL :
    
      sprintf( buf2, "MANUAL" );
      value_bg_color = hci_get_read_color( WARNING_COLOR );
      value_fg_color = hci_get_read_color( TEXT_FOREGROUND );
      break;

    case HCI_PRF_MODE_AUTO_ELEVATION :

      sprintf( buf2, "AUTO" );
      value_bg_color = hci_get_read_color( NORMAL_COLOR );
      value_fg_color = hci_get_read_color( TEXT_FOREGROUND );
      break;

    case HCI_PRF_MODE_AUTO_STORM :

      sprintf( buf2, "MULTI-STORM" );
      value_bg_color = hci_get_read_color( NORMAL_COLOR );
      value_fg_color = hci_get_read_color( TEXT_FOREGROUND );
      break;

    case HCI_PRF_MODE_AUTO_CELL :

      sprintf( buf2, "SINGLE-STORM" );
      value_bg_color = hci_get_read_color( WARNING_COLOR );
      value_fg_color = hci_get_read_color( TEXT_FOREGROUND );
      break;

    default : /* Shouldn't happen */

      sprintf( buf2, "????" );
      value_bg_color = hci_get_read_color( WARNING_COLOR );
      value_fg_color = hci_get_read_color( TEXT_FOREGROUND );
      break;
  }

  sprintf( buf1, "PRF Mode:" );
  tag_width = XTextWidth( fontinfo, buf1, strlen( buf1 ) );
  value_width = XTextWidth( fontinfo, buf2, strlen( buf2 ) );

  prfmode->pixel  = x2;
  prfmode->scanl  = temp_height + font_height + MARGIN_BETWEEN_LABELS;
  prfmode->height = font_height;
  prfmode->width  = max_value_width;

  if( force_draw )
  {
    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    tag_bg_color );

    XFillRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    x1 - LABEL_EDGE_MARGIN,
                    prfmode->scanl,
                    max_tag_width + LABEL_EDGE_MARGIN,
                    font_height + LABEL_EDGE_MARGIN );

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    tag_fg_color );

    XDrawString( HCI_get_display(),
                 hci_control_panel_pixmap(),
                 hci_control_panel_gc(),
                 x1,
                 prfmode->scanl + font_height,
                 buf1,
                 strlen( buf1 ) );
  }

  if( strcmp( buf2, prev_PRF_mode_buf ) != 0 || force_draw )
  {
    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    value_bg_color );

    XFillRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    x2,
                    prfmode->scanl,
                    max_value_width,
                    font_height + LABEL_EDGE_MARGIN );

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    value_fg_color );

    XDrawString( HCI_get_display(),
                 hci_control_panel_pixmap(),
                 hci_control_panel_gc(),
                 value_ctr - value_width/2,
                 prfmode->scanl + font_height,
                 buf2,
                 strlen( buf2 ) );

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    value_fg_color );

    XDrawRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    x1 - LABEL_EDGE_MARGIN,
                    prfmode->scanl,
                    max_label_width + LABEL_EDGE_MARGIN,
                    font_height + LABEL_EDGE_MARGIN );

    strcpy( prev_PRF_mode_buf, buf2 );
  }

  temp_height = prfmode->scanl;

  /************************* Performance Check *************************/
  
  pc_info = hci_get_perfcheck_info();

  if( pc_info->state == HCI_PC_STATE_PENDING )
  {
    sprintf( buf2, "  PENDING  " );
    value_bg_color = hci_get_read_color( NORMAL_COLOR );
    value_fg_color = hci_get_read_color( TEXT_FOREGROUND );
  }
  else if( pc_info->status == HCI_PC_STATUS_PENDING )
  {
    sprintf( buf2, "  PENDING  " );
    value_bg_color = hci_get_read_color( NORMAL_COLOR );
    value_fg_color = hci_get_read_color( TEXT_FOREGROUND );
  }
  else if( pc_info->num_hrs > 0 )
  {
    sprintf( buf2, "  %02dh %02dm  ", pc_info->num_hrs, pc_info->num_mins );
    value_bg_color = hci_get_read_color( WHITE );
    value_fg_color = hci_get_read_color( TEXT_FOREGROUND );
  }
  else
  {
    sprintf( buf2, "  %02dh %02dm  ", pc_info->num_hrs, pc_info->num_mins );
    value_bg_color = hci_get_read_color( WARNING_COLOR );
    value_fg_color = hci_get_read_color( TEXT_FOREGROUND );
  }

  sprintf( buf1, "Perf Check In:" );
  tag_width = XTextWidth( fontinfo, buf1, strlen( buf1 ) );
  value_width = XTextWidth( fontinfo, buf2, strlen( buf2 ) );

  perfcheck->pixel  = x2;
  perfcheck->scanl  = temp_height + font_height + MARGIN_BETWEEN_LABELS;
  perfcheck->height = font_height;
  perfcheck->width  = max_value_width;

  if( force_draw )
  {
    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    tag_bg_color );

    XFillRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    x1 - LABEL_EDGE_MARGIN,
                    perfcheck->scanl,
                    max_tag_width + LABEL_EDGE_MARGIN,
                    font_height + LABEL_EDGE_MARGIN );

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    tag_fg_color );

    XDrawString( HCI_get_display(),
                 hci_control_panel_pixmap(),
                 hci_control_panel_gc(),
                 x1,
                 perfcheck->scanl + font_height,
                 buf1,
                 strlen( buf1 ) );
  }

  if( strcmp( buf2, prev_perfcheck_buf ) != 0 || force_draw )
  {
    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    value_bg_color );

    XFillRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    x2,
                    perfcheck->scanl,
                    max_value_width,
                    font_height + LABEL_EDGE_MARGIN );

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    value_fg_color );

    XDrawString( HCI_get_display(),
                 hci_control_panel_pixmap(),
                 hci_control_panel_gc(),
                 value_ctr - value_width/2,
                 perfcheck->scanl + font_height,
                 buf2,
                 strlen( buf2 ) );

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( TEXT_FOREGROUND ) );

    XDrawRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    x1 - LABEL_EDGE_MARGIN,
                    perfcheck->scanl,
                    max_label_width + LABEL_EDGE_MARGIN,
                    font_height + LABEL_EDGE_MARGIN );

    strcpy( prev_perfcheck_buf, buf2 );
  }

  temp_height = perfcheck->scanl;
}

