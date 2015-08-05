/************************************************************************
 *									*
 *	Module:  hci_control_panel_status.c				*
 *									*
 *	Description:  This module is used to display the status of	*
 *	various ORPG variables.  These include:				*
 *									*
 *		PRF Mode	- AUTO/STORM/MANUAL			*
 *		VAD Update	- ON/OFF				*
 *		Precip Status	- ACCUM/NO ACCUM			*
 *		Load Shed Cat	- Normal/Warning/Alarm			*
 *		Audio Alarms    - Enabled/Disabled			*
 *		RDA Messages	- Enabled/Disabled			*
 *		FAA Redundant	- Match/Mismatch			*
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/11/16 15:25:40 $
 * $Id: hci_control_panel_status.c,v 1.65 2012/11/16 15:25:40 ccalvert Exp $
 * $Revision: 1.65 $
 * $State: Exp $
 */

/* Local include file definitions. */

#include <hci_control_panel.h>

/* Global/static variables. */

static	char	prev_precip_status_buf[ 10 ] = "";
static	char	prev_VAD_buf[ 6 ] = "";
static	char	prev_Model_EWT_buf[ 6 ] = "";
static	char	prev_super_res_buf[ 10 ] = "";
static  int     prev_super_res_status = 1;
static	char	prev_cmd_buf[ 10 ] = "";
static  int     prev_cmd_status = 1;
static	char	prev_load_shed_buf[ 9 ] = "";
static	char	prev_inhibit_buf[ 10 ] = "";
static	char	prev_faa_buf[ 10 ] = "";
static	int	User_selected_vad_flag = -1;
static	int	User_selected_model_update_flag = -1;

/* Function prototypes. */

void	verify_vad_update_change_accept( Widget, XtPointer, XtPointer );
void	verify_vad_update_change_cancel( Widget, XtPointer, XtPointer );
void	verify_model_ewt_update_change_accept( Widget, XtPointer, XtPointer );
void	verify_model_ewt_update_change_cancel( Widget, XtPointer, XtPointer );

/************************************************************************
 *	Description: This function positions and displays objects in	*
 *		     the status region which is displayed below the	*
 *		     USERS container.					*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_control_panel_status ( int force_draw )
{
  int i;
  int temp_int;
  int x1;
  int x2;
  int x3;
  int y1;
  Pixel background_color;
  Pixel foreground_color;
  int font_height;
  int temp_height;
  char buf1[ 24 ];
  char buf2[ 24 ];
  int status;
  int super_res_status;
  int cmd_status;
  int config;
  unsigned char	flag;
  int rda_status;
  int wb_status;
  int current;
  int warn;
  int alarm;
  XFontStruct *fontinfo;
  hci_control_panel_object_t *rpg;
  hci_control_panel_object_t *precip;
  hci_control_panel_object_t *super_res;
  hci_control_panel_object_t *cmd;
  hci_control_panel_object_t *enw;
  hci_control_panel_object_t *model_ewt;
  hci_control_panel_object_t *load_shed;
  hci_control_panel_object_t *user;
  hci_control_panel_object_t *inhibit;
  hci_control_panel_object_t *faa;

  /* Get the attributes of the RPG button. It is used to
     determine the position of the status area. */

  rpg = hci_control_panel_object( RPG_BUTTONS_BACKGROUND );

  if( rpg->pixel < 1 )
  {
    return;
  }

  /* Get pointers to the attributes for the status objects. */

  precip    = hci_control_panel_object (PRECIP_STATUS_OBJECT);
  super_res = hci_control_panel_object (SUPER_RES_STATUS_OBJECT);
  cmd       = hci_control_panel_object (CMD_STATUS_OBJECT);
  enw       = hci_control_panel_object (ENW_STATUS_OBJECT);
  model_ewt = hci_control_panel_object (MODEL_EWT_STATUS_OBJECT);
  load_shed = hci_control_panel_object (LOAD_SHED_OBJECT);
  user      = hci_control_panel_object (USERS_BUTTON);
  inhibit   = hci_control_panel_object (RDA_INHIBIT_OBJECT);
  faa       = hci_control_panel_object (FAA_REDUNDANT_OBJECT);

  /* We want to use a scaled font based on the size of the RPG
     Control/Status window. */

  XSetFont( HCI_get_display(),
            hci_control_panel_gc(),
            hci_get_font( SCALED ) );

  fontinfo = hci_get_fontinfo( SCALED );

  /* Get the height of a label using set font. */

  font_height   = fontinfo->ascent + fontinfo->descent;

  /* The RDA Messages label is the larget so lets use
     this to determine label width so value fields line up. */

  sprintf( buf1, "RDA Messages: " );

  /* Calculate the coordinate of the upper left corner of
     the status area. */

  x1 = rpg->pixel + rpg->width + 3*hci_control_panel_3d();
  x2 = x1 + XTextWidth( fontinfo, buf1, strlen ( buf1 ) );
  x3 = user->pixel + user->width + 2*hci_control_panel_3d();
  y1 = rpg->scanl + rpg->height;

  /************************** Precip Status ****************************/

  sprintf( buf1,"Precip Status:" );

  switch( hci_get_precip_status().current_precip_status )
  {
    case PRECIP_ACCUM :	/* Accumulating */

      sprintf( buf2, "ACCUM" );
      background_color = hci_get_read_color( NORMAL_COLOR );
      foreground_color = hci_get_read_color( TEXT_FOREGROUND );
      break;

    case PRECIP_NOT_ACCUM :	/* Not accumulating */

      sprintf( buf2, "NO ACCUM" );
      background_color = hci_get_read_color( NORMAL_COLOR );
      foreground_color = hci_get_read_color( TEXT_FOREGROUND );
      break;

    default :	/* Shouldn't happen */

      sprintf( buf2, "????" );
      background_color = hci_get_read_color( WARNING_COLOR );
      foreground_color = hci_get_read_color( TEXT_FOREGROUND );
      break;
  }

  precip->pixel  = x2;
  precip->scanl  = y1 - font_height;
  precip->height = font_height;
  precip->width  = x3 - x2;

  if( force_draw )
  {
    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( BACKGROUND_COLOR2 ) );

    XFillRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    ( int ) ( x1 - 2 ),
                    ( int ) precip->scanl,
                    ( int ) ( x2 - x1 + 2 ),
                    ( int ) ( font_height + 2 ) );

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( TEXT_FOREGROUND ) );

    XDrawString( HCI_get_display(),
                 hci_control_panel_pixmap(),
                 hci_control_panel_gc(),
                 ( int ) x1,
                 ( int ) ( precip->scanl + font_height ),
                 buf1,
                 strlen( buf1 ) );
  }

  if( strcmp( buf2, prev_precip_status_buf ) != 0 || force_draw )
  {
    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    background_color);

    XFillRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    ( int ) x2,
                    ( int ) precip->scanl,
                    ( int ) ( x3 - x2 ),
                    ( int ) ( font_height + 2 ) );

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    foreground_color );

    temp_int = XTextWidth( fontinfo, buf2, strlen( buf2 ) );
    temp_int = x3 - x2 - temp_int;
    temp_int /= 2;

    XDrawString( HCI_get_display(),
                 hci_control_panel_pixmap(),
                 hci_control_panel_gc(),
                 ( int ) ( precip->pixel + temp_int ),
                 ( int ) ( precip->scanl + font_height ),
                 buf2,
                 strlen( buf2 ) );

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( TEXT_FOREGROUND ) );

    XDrawRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    ( int ) ( x1 - 2 ),
                    ( int ) precip->scanl,
                    ( int ) ( x3 - x1 + 2 ),
                    ( int ) ( font_height + 2 ) );

    strcpy( prev_precip_status_buf, buf2 );
  }

  temp_height = precip->scanl;

  /************************* VAD Update *****************************/

  sprintf( buf1, "VAD Update:" );

  switch( hci_get_vad_update_flag() )
  {
    case HCI_YES_FLAG :	/* VAD Update ON */

      sprintf( buf2, "ON" );
      background_color = hci_get_read_color( NORMAL_COLOR );
      foreground_color = hci_get_read_color( TEXT_FOREGROUND );
      break;

    case HCI_NO_FLAG :	/* VAD Update OFF */

      sprintf( buf2, "OFF" );
      background_color = hci_get_read_color( WARNING_COLOR );
      foreground_color = hci_get_read_color( TEXT_FOREGROUND );
      break;

    default :	/* Shouldn't happen */

      sprintf( buf2, "????" );
      background_color = hci_get_read_color( WARNING_COLOR );
      foreground_color = hci_get_read_color( TEXT_FOREGROUND );
      break;
  }

  enw->pixel  = x2;
  enw->scanl  = temp_height + font_height + 4;
  enw->height = font_height;
  enw->width  = x3 - x2;

  if( force_draw )
  {
    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( BACKGROUND_COLOR2 ) );

    XFillRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    ( int ) ( x1 - 2 ),
                    ( int ) enw->scanl,
                    ( int ) ( x2 - x1 + 2 ),
                    ( int ) ( font_height + 2 ) );

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( TEXT_FOREGROUND ) );

    XDrawString( HCI_get_display(),
                 hci_control_panel_pixmap(),
                 hci_control_panel_gc(),
                 ( int ) x1,
                 ( int ) ( enw->scanl + font_height ),
                 buf1,
                 strlen ( buf1 ) );
  }

  if( strcmp( buf2, prev_VAD_buf ) != 0 || force_draw )
  {
    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    background_color );

    XFillRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    ( int ) x2,
                    ( int ) enw->scanl,
                    ( int ) ( x3 - x2 ),
                    ( int ) ( font_height + 2 ) );

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    foreground_color );

    temp_int = XTextWidth( fontinfo, buf2, strlen( buf2 ) );
    temp_int = x3 - x2 - temp_int;
    temp_int /= 2;

    XDrawString( HCI_get_display(),
		     hci_control_panel_pixmap(),
		     hci_control_panel_gc(),
		     ( int ) ( enw->pixel + temp_int ),
		     ( int ) ( enw->scanl + font_height ),
		     buf2,
		     strlen( buf2 ) );

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( TEXT_FOREGROUND ) );

    XDrawRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    ( int ) ( x1 - 2 ),
                    ( int ) enw->scanl,
                    ( int ) ( x3 - x1 + 2 ),
                    ( int ) ( font_height + 2 ) );

    strcpy( prev_VAD_buf, buf2 );
  }

  temp_height = enw->scanl;

  /************************* Model EWT Update *****************************/

  sprintf( buf1, "Model Update:" );

  switch( hci_get_model_update_flag() )
  {
    case HCI_YES_FLAG :  /* Model EWT Update ON */

      sprintf( buf2, "ON" );
      background_color = hci_get_read_color( NORMAL_COLOR );
      foreground_color = hci_get_read_color( TEXT_FOREGROUND );
      break;

    case HCI_NO_FLAG :   /* Model EWT Update OFF */

      sprintf( buf2, "OFF" );
      background_color = hci_get_read_color( WARNING_COLOR );
      foreground_color = hci_get_read_color( TEXT_FOREGROUND );
      break;

    default :   /* Shouldn't happen */

      sprintf( buf2, "????" );
      background_color = hci_get_read_color( WARNING_COLOR );
      foreground_color = hci_get_read_color( TEXT_FOREGROUND );
      break;
  }

  model_ewt->pixel  = x2;
  model_ewt->scanl  = temp_height + font_height + 4;
  model_ewt->height = font_height;
  model_ewt->width  = x3 - x2;

  if( force_draw )
  {
    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( BACKGROUND_COLOR2 ) );

    XFillRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    ( int ) ( x1 - 2 ),
                    ( int ) model_ewt->scanl,
                    ( int ) ( x2 - x1 + 2 ),
                    ( int ) ( font_height + 2 ) );

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( TEXT_FOREGROUND ) );

    XDrawString( HCI_get_display(),
                 hci_control_panel_pixmap(),
                 hci_control_panel_gc(),
                 ( int ) x1,
                 ( int ) ( model_ewt->scanl + font_height ),
                 buf1,
                 strlen ( buf1 ) );
  }

  if( strcmp( buf2, prev_Model_EWT_buf ) != 0 || force_draw )
  {
    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    background_color );

    XFillRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    ( int ) x2,
                    ( int ) model_ewt->scanl,
                    ( int ) ( x3 - x2 ),
                    ( int ) ( font_height + 2 ) );

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    foreground_color );

    temp_int = XTextWidth( fontinfo, buf2, strlen( buf2 ) );
    temp_int = x3 - x2 - temp_int;
    temp_int /= 2;

    XDrawString( HCI_get_display(),
                     hci_control_panel_pixmap(),
                     hci_control_panel_gc(),
                     ( int ) ( model_ewt->pixel + temp_int ),
                     ( int ) ( model_ewt->scanl + font_height ),
                     buf2,
                     strlen( buf2 ) );

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( TEXT_FOREGROUND ) );

    XDrawRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    ( int ) ( x1 - 2 ),
                    ( int ) model_ewt->scanl,
                    ( int ) ( x3 - x1 + 2 ),
                    ( int ) ( font_height + 2 ) );

    strcpy( prev_Model_EWT_buf, buf2 );
  }

  temp_height = model_ewt->scanl;

  /************************* Super Res *****************************/

  sprintf( buf1, "Super Res:" );

  status = ORPGINFO_statefl_flag( ORPGINFO_STATEFL_FLG_SUPER_RES_ENABLED,
                                  ORPGINFO_STATEFL_GET,
                                  &flag );

  super_res_status = (int) flag;
  rda_status = ORPGRDA_get_status( RS_SUPER_RES );
  wb_status = ORPGRDA_get_wb_status( ORPGRDA_WBLNSTAT );

  if( ((config = ORPGRDA_get_rda_config( NULL ) ) == ORPGRDA_ORDA_CONFIG)
                               &&
      (wb_status == RS_CONNECTED) ){

     switch( flag )
     {
       case 1 : /* Super Resolution Enabled */
       {
         if( rda_status == SR_DISABLED ){

            sprintf( buf2, "PENDING" );
            background_color = hci_get_read_color( NORMAL_COLOR );
            foreground_color = hci_get_read_color( TEXT_FOREGROUND );

         }
         else{

            sprintf( buf2, "ENABLED" );
            background_color = hci_get_read_color( NORMAL_COLOR );
            foreground_color = hci_get_read_color( TEXT_FOREGROUND );

         }
         break;
       }
       case 0 : /* Super Resolution Disabled */
       {
         if( rda_status == SR_ENABLED ){

            sprintf( buf2, "PENDING" );
            background_color = hci_get_read_color( WARNING_COLOR );
            foreground_color = hci_get_read_color( TEXT_FOREGROUND );

         }
         else{

            sprintf( buf2, "DISABLED" );
            background_color = hci_get_read_color( WARNING_COLOR );
            foreground_color = hci_get_read_color( TEXT_FOREGROUND );

         }
         break;
       }

       default : /* Shouldn't happen */
       {
         sprintf( buf2, "????" );
         background_color = hci_get_read_color( WARNING_COLOR );
         foreground_color = hci_get_read_color( TEXT_FOREGROUND );
         break;
       }

     }

  }
  else{

      sprintf( buf2, "????" );
      background_color = hci_get_read_color( WARNING_COLOR );
      foreground_color = hci_get_read_color( TEXT_FOREGROUND );

  }

  super_res->pixel  = x2;
  super_res->scanl  = temp_height + font_height + 4;
  super_res->height = font_height;
  super_res->width  = x3 - x2;

  if( force_draw )
  {
    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( BACKGROUND_COLOR2 ) );

    XFillRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    ( int ) ( x1 - 2 ),
                    ( int ) super_res->scanl,
                    ( int ) ( x2 - x1 + 2 ),
                    ( int ) ( font_height + 2 ) );

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( TEXT_FOREGROUND ) );

    XDrawString( HCI_get_display(),
                 hci_control_panel_pixmap(),
                 hci_control_panel_gc(),
                 ( int ) x1,
                 ( int ) ( super_res->scanl + font_height ),
                 buf1,
                 strlen( buf1 ) );
  }

  if( (strcmp( buf2, prev_super_res_buf ) != 0) 
                       || 
      (force_draw) || (super_res_status != prev_super_res_status) )
  {
    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    background_color );

    XFillRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    ( int ) x2,
                    ( int ) super_res->scanl,
                    ( int ) ( x3 - x2 ),
                    ( int ) ( font_height + 2 ) );

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    foreground_color );

    temp_int = XTextWidth( fontinfo, buf2, strlen( buf2 ) );
    temp_int = x3 - x2 - temp_int;
    temp_int /= 2;

    XDrawString( HCI_get_display(),
                 hci_control_panel_pixmap(),
                 hci_control_panel_gc(),
                 ( int ) ( super_res->pixel + temp_int ),
                 ( int ) ( super_res->scanl + font_height),
                 buf2,
                 strlen( buf2 ) );

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( TEXT_FOREGROUND ) );

    XDrawRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    ( int ) ( x1 - 2 ),
                    ( int ) super_res->scanl,
                    ( int ) ( x3 - x1 + 2 ),
                    ( int ) ( font_height + 2 ) );

    strcpy( prev_super_res_buf, buf2 );
    prev_super_res_status = super_res_status;
  }

  temp_height = super_res->scanl;

  /****************** Clutter Mitigation Decision ******************/

  sprintf( buf1, "CMD:" );

  status = ORPGINFO_statefl_flag( ORPGINFO_STATEFL_FLG_CMD_ENABLED,
                                  ORPGINFO_STATEFL_GET,
                                  &flag );

  cmd_status = (int) flag;
  rda_status = ORPGRDA_get_status( RS_CMD ) & 0x1;
  wb_status = ORPGRDA_get_wb_status( ORPGRDA_WBLNSTAT );

  if( ( ( config = ORPGRDA_get_rda_config( NULL ) ) == ORPGRDA_ORDA_CONFIG )
                               &&
      ( wb_status == RS_CONNECTED ) )
  {
    switch( flag )
    {
      case 1 : /* Clutter Mitigation Decision Enabled */
      {
        if( rda_status == CMD_DISABLED )
        {
          sprintf( buf2, "PENDING" );
          background_color = hci_get_read_color( NORMAL_COLOR );
          foreground_color = hci_get_read_color( TEXT_FOREGROUND );
        }
        else
        {
          sprintf( buf2, "ENABLED" );
          background_color = hci_get_read_color( NORMAL_COLOR );
          foreground_color = hci_get_read_color( TEXT_FOREGROUND );
        }
        break;
      }
      case 0 : /* Clutter Mitigation Decision Disabled */
      {
        if( rda_status == CMD_ENABLED )
        {
          sprintf( buf2, "PENDING" );
          background_color = hci_get_read_color( WARNING_COLOR );
          foreground_color = hci_get_read_color( TEXT_FOREGROUND );
        }
        else
        {
          sprintf( buf2, "DISABLED" );
          background_color = hci_get_read_color( WARNING_COLOR );
          foreground_color = hci_get_read_color( TEXT_FOREGROUND );
        }
        break;
      }

      default : /* Shouldn't happen */
      {
        sprintf( buf2, "????" );
        background_color = hci_get_read_color( WARNING_COLOR );
        foreground_color = hci_get_read_color( TEXT_FOREGROUND );
        break;
      }
    }
  }
  else
  {
    sprintf( buf2, "????" );
    background_color = hci_get_read_color( WARNING_COLOR );
    foreground_color = hci_get_read_color( TEXT_FOREGROUND );
  }

  cmd->pixel  = x2;
  cmd->scanl  = temp_height + font_height + 4;
  cmd->height = font_height;
  cmd->width  = x3 - x2;

  if( force_draw )
  {
    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( BACKGROUND_COLOR2 ) );

    XFillRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    ( int ) ( x1 - 2 ),
                    ( int ) cmd->scanl,
                    ( int ) ( x2 - x1 + 2 ),
                    ( int ) ( font_height + 2 ) );

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( TEXT_FOREGROUND ) );

    XDrawString( HCI_get_display(),
                 hci_control_panel_pixmap(),
                 hci_control_panel_gc(),
                 ( int ) x1,
                 ( int ) ( cmd->scanl + font_height ),
                 buf1,
                 strlen( buf1 ) );
  }

  if( ( strcmp( buf2, prev_cmd_buf ) != 0 )
                       ||
      ( force_draw ) || ( cmd_status != prev_cmd_status ) )
  {
    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    background_color );

    XFillRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    ( int ) x2,
                    ( int ) cmd->scanl,
                    ( int ) ( x3 - x2 ),
                    ( int ) ( font_height + 2 ) );

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    foreground_color );

    temp_int = XTextWidth( fontinfo, buf2, strlen( buf2 ) );
    temp_int = x3 - x2 - temp_int;
    temp_int /= 2;

    XDrawString( HCI_get_display(),
                 hci_control_panel_pixmap(),
                 hci_control_panel_gc(),
                 ( int ) ( cmd->pixel + temp_int ),
                 ( int ) ( cmd->scanl + font_height),
                 buf2,
                 strlen( buf2 ) );

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( TEXT_FOREGROUND ) );

    XDrawRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    ( int ) ( x1 - 2 ),
                    ( int ) cmd->scanl,
                    ( int ) ( x3 - x1 + 2 ),
                    ( int ) ( font_height + 2 ) );

    strcpy( prev_cmd_buf, buf2 );
    prev_cmd_status = cmd_status;
  }

  temp_height = cmd->scanl;

  /**************************Load Shed Categories************************
    The first thing we want to do is to determine if any of the
    categories have exceded warning or alarm thresholds.  Display
    the value of the most severe level detected.  The three levels,
    in increasing severity, are: Normal, Warning, Alarm.  The
    assigned colors are green, yellow, and red, respectively.
   **********************************************************************/

  sprintf( buf1, "Load Shed:" );

  sprintf( buf2, "NORMAL" );
  background_color = hci_get_read_color( NORMAL_COLOR );
  foreground_color = hci_get_read_color( TEXT_FOREGROUND );

  for( i = LOAD_SHED_CATEGORY_PROD_DIST; i < LOAD_SHED_CATEGORY_WB_USER; i++ )
  {
    if( i != LOAD_SHED_CATEGORY_INPUT_BUF )
    {
      status = ORPGLOAD_get_data( i, LOAD_SHED_WARNING_THRESHOLD, &warn );
      status = ORPGLOAD_get_data( i, LOAD_SHED_ALARM_THRESHOLD, &alarm );
      status = ORPGLOAD_get_data( i, LOAD_SHED_CURRENT_VALUE, &current );

      if( current >= alarm )
      {
        sprintf( buf2, "ALARM" );
        background_color = hci_get_read_color( CYAN );
        foreground_color = hci_get_read_color( TEXT_FOREGROUND );
        break;
      }
      else if( current >= warn )
      {
        sprintf( buf2, "WARNING" );
        background_color = hci_get_read_color( NORMAL_COLOR );
        foreground_color = hci_get_read_color( TEXT_FOREGROUND );
      }
    }
  }

  load_shed->pixel  = x2;
  load_shed->scanl  = temp_height + font_height + 4;
  load_shed->height = font_height;
  load_shed->width  = x3 - x2;

  if( force_draw )
  {
    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( BACKGROUND_COLOR2 ) );

    XFillRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    ( int ) ( x1 - 2 ),
                    ( int ) load_shed->scanl,
                    ( int ) ( x2 - x1 + 2 ),
                    ( int ) ( font_height + 2 ) );

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( TEXT_FOREGROUND ) );

    XDrawString( HCI_get_display(),
                 hci_control_panel_pixmap(),
                 hci_control_panel_gc(),
                 ( int ) x1,
                 ( int ) ( load_shed->scanl + font_height ),
                 buf1,
                 strlen( buf1 ) );
  }

  if( strcmp( buf2, prev_load_shed_buf ) != 0 || force_draw )
  {
    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    background_color );

    XFillRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    ( int ) x2,
                    ( int ) load_shed->scanl,
                    ( int ) ( x3 - x2 ),
                    ( int ) ( font_height + 2 ) );

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    foreground_color );

    temp_int = XTextWidth( fontinfo, buf2, strlen( buf2 ) );
    temp_int = x3 - x2 - temp_int;
    temp_int /= 2;

    XDrawString( HCI_get_display(),
                 hci_control_panel_pixmap(),
                 hci_control_panel_gc(),
                 ( int ) (load_shed->pixel + temp_int ),
                 ( int ) ( load_shed->scanl + font_height ),
                 buf2,
                 strlen( buf2 ) );

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( TEXT_FOREGROUND ) );

    XDrawRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    ( int ) ( x1 - 2 ),
                    ( int ) load_shed->scanl,
                    ( int ) ( x3 - x1 + 2 ),
                    ( int ) ( font_height + 2 ) );

    strcpy( prev_load_shed_buf, buf2 );
  }

  temp_height = load_shed->scanl;

  /********************** RDA Messages ***************************/

  sprintf( buf1, "RDA Messages:" );

  inhibit->pixel  = x2;
  inhibit->scanl  = temp_height + font_height + 4;
  inhibit->height = font_height;
  inhibit->width  = x3 - x2;

  if( hci_info_inhibit_RDA_messages() )
  {
    sprintf( buf2, "DISABLED" );
    foreground_color = hci_get_read_color( TEXT_FOREGROUND );
    background_color = hci_get_read_color( WARNING_COLOR );
  }
  else
  {
    sprintf( buf2, "ENABLED" );
    foreground_color = hci_get_read_color( TEXT_FOREGROUND );
    background_color = hci_get_read_color( NORMAL_COLOR );
  }

  if( force_draw )
  {
    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( BACKGROUND_COLOR2 ) );

    XFillRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    ( int ) ( x1 - 2 ),
                    ( int ) inhibit->scanl,
                    ( int ) ( x2 - x1 + 2 ),
                    ( int ) ( font_height + 2 ) );

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( TEXT_FOREGROUND ) );

    XDrawString( HCI_get_display(),
                 hci_control_panel_pixmap(),
                 hci_control_panel_gc(),
                 ( int ) x1,
                 ( int ) ( inhibit->scanl + font_height ),
                 buf1,
                 strlen( buf1 ) );
  }

  if( strcmp( buf2, prev_inhibit_buf ) != 0 || force_draw )
  {
    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    background_color );

    XFillRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    ( int ) x2,
                    ( int ) inhibit->scanl,
                    ( int ) ( x3 - x2 ),
                    ( int ) ( font_height + 2 ) );

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    foreground_color );

    temp_int = XTextWidth( fontinfo, buf2, strlen( buf2 ) );
    temp_int = x3 - x2 - temp_int;
    temp_int /= 2;

    XDrawString( HCI_get_display(),
                 hci_control_panel_pixmap(),
                 hci_control_panel_gc(),
                 ( int ) ( inhibit->pixel + temp_int ),
                 ( int ) ( inhibit->scanl + font_height ),
                 buf2,
                 strlen( buf2 ) );

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( TEXT_FOREGROUND ) );

    XDrawRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    ( int ) ( x1 - 2 ),
                    ( int ) inhibit->scanl,
                    ( int ) ( x3 - x1 + 2 ),
                    ( int ) ( font_height + 2 ) );

    strcpy( prev_inhibit_buf, buf2 );
  }

  temp_height = inhibit->scanl;

  /******************FAA Redundant Adaptation Data Status****************
    If this is an FAA redundant configuration, then we need to
    display the "force adaptation data state".  All this indicates
    is whether or not any of the redundant data stores have been
    updated and not copied to the redundant channel.  This lets
    the user determine if they want to issue a "Force Adaptation
    Data Update command.
   **********************************************************************/

  if( HCI_get_system() == HCI_FAA_SYSTEM )
  {
    sprintf( buf1, "Adapt Times: " );

    faa->pixel  = x2;
    faa->scanl  = temp_height + font_height + 4;
    faa->height = font_height;
    faa->width  = x3 - x2;

    if( ORPGRED_channel_state( ORPGRED_OTHER_CHANNEL ) !=
        ORPGRED_CHANNEL_ACTIVE )
    {
      faa->flags = OBJECT_SELECTABLE;
    }
    else
    {
      faa->flags = 0;
    }

    if( ORPGRED_adapt_dat_time( ORPGRED_MY_CHANNEL ) !=
        ORPGRED_adapt_dat_time( ORPGRED_OTHER_CHANNEL ) )
    {
      sprintf( buf2, "MISMATCH" );
      foreground_color = hci_get_read_color( WHITE );
      background_color = hci_get_read_color( ALARM_COLOR1 );
    }
    else
    {
      sprintf( buf2, "MATCH" );
      foreground_color = hci_get_read_color( TEXT_FOREGROUND );
      background_color = hci_get_read_color( NORMAL_COLOR );
    }

    if( force_draw )
    {
      XSetForeground( HCI_get_display(),
                      hci_control_panel_gc(),
                      hci_get_read_color( BACKGROUND_COLOR2 ) );

      XFillRectangle( HCI_get_display(),
                      hci_control_panel_pixmap(),
                      hci_control_panel_gc(),
                      ( int ) ( x1 - 2 ),
                      ( int ) faa->scanl,
                      ( int ) ( x2 - x1 + 2 ),
                      ( int ) ( font_height + 2 ) );

      XSetForeground( HCI_get_display(),
                      hci_control_panel_gc(),
                      hci_get_read_color( TEXT_FOREGROUND ) );

      XDrawString( HCI_get_display(),
                   hci_control_panel_pixmap(),
                   hci_control_panel_gc(),
                   ( int ) x1,
                   ( int ) ( faa->scanl + font_height ),
                   buf1,
                   strlen( buf1 ) );
    }

    if( strcmp( buf2, prev_faa_buf ) != 0 || force_draw )
    {
      XSetForeground( HCI_get_display(),
                      hci_control_panel_gc(),
                      background_color );

      XFillRectangle( HCI_get_display(),
                      hci_control_panel_pixmap(),
                      hci_control_panel_gc(),
                      ( int ) x2,
                      ( int ) faa->scanl,
                      ( int ) ( x3 - x2 ),
                      ( int ) ( font_height + 2 ) );

      XSetForeground( HCI_get_display(),
                      hci_control_panel_gc(),
                      foreground_color );

      temp_int = XTextWidth( fontinfo, buf2, strlen( buf2 ) );
      temp_int = x3 - x2 - temp_int;
      temp_int /= 2;

      XDrawString( HCI_get_display(),
                   hci_control_panel_pixmap(),
                   hci_control_panel_gc(),
                   ( int ) ( faa->pixel + temp_int ),
                   ( int ) ( faa->scanl + font_height ),
                   buf2,
                   strlen( buf2 ) );

      XSetForeground( HCI_get_display(),
                      hci_control_panel_gc(),
                      hci_get_read_color( TEXT_FOREGROUND ) );

      XDrawRectangle( HCI_get_display(),
                      hci_control_panel_pixmap(),
                      hci_control_panel_gc(),
                      ( int ) ( x1 - 2 ),
                      ( int ) faa->scanl,
                      ( int ) ( x3 - x1 + 2 ),
                      ( int ) ( font_height + 2 ) );

      strcpy( prev_faa_buf, buf2 );
    }
    temp_height = faa->scanl;
  }
}

/************************************************************************
 *	Description: This function is used to display a verification	*
 *		     popup window to change the VAD update flag state.	*
 *									*
 *	Input:  w           - ID of widget popup window to be child of	*
 *		client_data - YES or NO					*
 *	Output:	NONE							*
 *	Return:	NONE							*
 ************************************************************************/

void verify_vad_update_change( Widget parent_widget, XtPointer client_data )
{
  char buf[HCI_BUF_128];

  User_selected_vad_flag = (int) client_data;

  if( User_selected_vad_flag == HCI_YES_FLAG )
  {
    sprintf( buf, "Do you want to enable VAD Update?" );
  }
  else
  {
    sprintf( buf, "Do you want to disable VAD Update?" );
  }

  hci_confirm_popup( parent_widget, buf, verify_vad_update_change_accept, verify_vad_update_change_cancel );
}

/************************************************************************
 *      Description: This function is used to display a verification    *
 *                   popup window to change the Model EWT update flag	*
 *		     state.  						*
 *                                                                      *
 *      Input:  w           - ID of widget popup window to be child of  *
 *              client_data - YES or NO                                 *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

void verify_model_ewt_update_change( Widget parent_widget, XtPointer client_data )
{
  char buf[HCI_BUF_128];

  User_selected_model_update_flag = (int) client_data;

  if( User_selected_model_update_flag == HCI_YES_FLAG )
  {
    sprintf( buf, "Do you want to enable Model Update?" );
  }
  else
  {
    sprintf( buf, "Do you want to disable Model Update?" );
  }

  hci_confirm_popup( parent_widget, buf, verify_model_ewt_update_change_accept, verify_model_ewt_update_change_cancel );
}


/************************************************************************
 *	Description: This function is called when the user selects the	*
 *		     "Yes" button from the VAD Update confirmation	*
 *		     popup window.					*
 *									*
 *	Input:  w           - ID of yes button				*
 *		client_data - YES or NO					*
 *		call_data   - *unused*					*
 *	Output:	NONE							*
 *	Return:	NONE							*
 ************************************************************************/

void verify_vad_update_change_accept( Widget w,
                                      XtPointer client_data,
                                      XtPointer call_data )
{
  int status = hci_set_vad_update_flag( User_selected_vad_flag );

  if( status <= 0 )
  {
    HCI_LE_error("Error changing VAD Update flag: %d", status);
  }
}

/************************************************************************
 *      Description: This function is called when the user selects the  *
 *                   "Yes" button from the Model Environmental Wind	*
 *		     Update confirmation popup window.                  *
 *                                                                      *
 *      Input:  w           - ID of yes button                          *
 *              client_data - YES or NO                                 *
 *              call_data   - *unused*                                  *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

void verify_model_ewt_update_change_accept( Widget w,
                                      XtPointer client_data,
                                      XtPointer call_data )
{
  int status = hci_set_model_update_flag( User_selected_model_update_flag );

  if( status <= 0 )
  {
    HCI_LE_error("Error changing Model EWT Update flag: %d", status);
  }
}


/************************************************************************
 *	Description: This function is called when the user selects the	*
 *		     "No" button from the VAD Update confirmation	*
 *		     popup window.					*
 *									*
 *	Input:  w           - ID of yes button				*
 *		client_data - YES or NO					*
 *		call_data   - *unused*					*
 *	Output:	NONE							*
 *	Return:	NONE							*
 ************************************************************************/

void verify_vad_update_change_cancel( Widget w,
                                      XtPointer client_data,
                                      XtPointer call_data )
{
}

/************************************************************************
 *      Description: This function is called when the user selects the  *
 *                   "No" button from the Model EWT Update confirmation *
 *                   popup window.                                      *
 *                                                                      *
 *      Input:  w           - ID of yes button                          *
 *              client_data - YES or NO                                 *
 *              call_data   - *unused*                                  *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

void verify_model_ewt_update_change_cancel( Widget w,
                                      XtPointer client_data,
                                      XtPointer call_data )
{
}

