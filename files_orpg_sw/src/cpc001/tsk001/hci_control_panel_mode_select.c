/************************************************************************
 *									*
 *	Module:  hci_control_panel_mode_select.c			*
 *									*
 *	Description:  This module is used to display the status of	*
 *	Mode Status variables.  These include:				*
 *									*
 *		Mode Conflict	 - YES/NO				*
 *		Clear Air Switch - AUTO/MANUAL				*
 *		Precip Switch	 - AUTO/MANUAL				*
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/11/16 15:25:39 $
 * $Id: hci_control_panel_mode_select.c,v 1.12 2012/11/16 15:25:39 ccalvert Exp $
 * $Revision: 1.12 $
 * $State: Exp $
 */

/* Local include file definitions. */

#include <hci_control_panel.h>
#include <hci_wx_status.h>

/* Global/static variables. */

static	char	prev_conflict_buf[ 10 ] = "";
static	char	prev_timer_buf[ 10 ] = "";
static	char	prev_mode_A_switch_buf[ 10 ] = "";
static	char	prev_mode_B_switch_buf[ 10 ] = "";
static	hci_control_panel_object_t *Top_object = NULL;

/* Function Prototypes. */

void deau_failure_popup();

/************************************************************************
 *	Description: This function positions and displays objects in	*
 *		     the mode select region which is displayed to the	*
 *		     right of the radome.				*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void hci_control_panel_mode_select( int force_draw )
{
  int temp_int;
  int x1;
  int x2;
  int x3;
  int y1;
  Pixel background_color;
  Pixel foreground_color;
  int height;
  char buf1[ 24 ];
  char buf2[ 24 ];
  char buf3[ 8 ] = "NA";
  XFontStruct *fontinfo;
  hci_control_panel_object_t *radome;
  hci_control_panel_object_t *mode_stat;
  hci_control_panel_object_t *clear_air_switch;
  hci_control_panel_object_t *precip_switch;
  int current_mode = -1;
  int recommended_mode = -1;
  int mode_deselect = -1;
  int mode_A_switch = -1;
  int switch_flag = -1;
  int time_diff = -1;
  int rain_area = -1;
  int rain_area_threshold = -1;
  time_t clr_air_time;
  time_t curr_vol_time;

  /* Get pointers to the attributes for the status objects. */

  Top_object = hci_control_panel_object( TOP_WIDGET );
  radome = hci_control_panel_object( RADOME_OBJECT );
  mode_stat = hci_control_panel_object( MODE_STATUS_OBJECT );
  clear_air_switch = hci_control_panel_object( CLEAR_AIR_SWITCH_OBJECT );
  precip_switch = hci_control_panel_object( PRECIP_SWITCH_OBJECT );

  /* If the top widget does not exist, do nothing. */

  if( Top_object->widget == ( Widget ) NULL )
  {
    return;
  }

  /* We want to use a scaled font based on the size of the RPG
     Control/Status window. */

  XSetFont( HCI_get_display(),
            hci_control_panel_gc(),
            hci_get_font( SCALED ) );

  fontinfo = hci_get_fontinfo( SCALED );

  /* Get the height of a label using set font. */

  height = fontinfo->ascent + fontinfo->descent;

  /* The Mode Conflict label is the largest so lets use 
     it to determine label width so value fields line up. */

  sprintf( buf1, "Clear Air Switch: " );
  sprintf( buf2, " MANUAL " );

  /* Calculate the coordinate of the upper left corner of 
     the mode select area. */

  x1 = radome->pixel + 0.38*Top_object->width;
  x2 = x1 + XTextWidth( fontinfo, buf1, strlen( buf1 ) );
  x3 = x2 + XTextWidth( fontinfo, buf2, strlen( buf2 ) );
  y1 = radome->scanl + 3*radome->height/4;

  /* For now, this variable will always be text foreground color. */

  foreground_color = hci_get_read_color( TEXT_FOREGROUND );

  /* Row #1 - Display Mode Conflict. */

  /* Get coordinates for 1st row. */

  mode_stat->pixel  = x2;
  mode_stat->scanl  = y1 - height;
  mode_stat->height = height;
  mode_stat->width  = x3 - x2;

  /* Build/draw left side of 1st row. */

  sprintf( buf1, "Mode Conflict:" );

  if( force_draw )
  {
    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( BACKGROUND_COLOR2 ) );

    XFillRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    ( int ) ( x1 - 2 ),
                    ( int ) mode_stat->scanl,
                    ( int ) ( x2 - x1 + 2 ),
                    ( int ) ( height + 2 ) );

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( TEXT_FOREGROUND ) );

    XDrawString( HCI_get_display(),
                 hci_control_panel_pixmap(),
                 hci_control_panel_gc(),
                 ( int ) x1,
                 ( int ) ( mode_stat->scanl + height ),
                 buf1,
                 strlen( buf1 ) );
  }

  /* Build/draw right side of 1st row. */

  current_mode = hci_get_wx_status().current_wxstatus;
  recommended_mode = hci_get_wx_status().recommended_wxstatus;
  mode_deselect = hci_get_wx_status().wxstatus_deselect;
  mode_A_switch = hci_get_wx_status().mode_select_adapt.auto_mode_A;
  clr_air_time = hci_get_wx_status().a3052t.time_to_cla;
  curr_vol_time = hci_get_wx_status().a3052t.curr_time;
  rain_area = ( int ) hci_get_wx_status().precip_area;
  rain_area_threshold = hci_get_wx_status().mode_select_adapt.precip_mode_area_thresh;

  if( current_mode != WX_STATUS_UNDEFINED  &&
      recommended_mode != WX_STATUS_UNDEFINED )
  {
    if( current_mode == recommended_mode )
    {
      sprintf( buf2, "NO" );
      background_color = hci_get_read_color( NORMAL_COLOR );

      /* If countdown timer is applicable, draw it. */

      if( mode_A_switch == AUTO_SWITCH &&
          current_mode == MODE_A &&
          ( rain_area - rain_area_threshold <= 0 ) )
      {
        time_diff = ( clr_air_time - curr_vol_time + 30 )/60;
        if( time_diff < 0 ){ time_diff = 0; }
        sprintf( buf3, "  %2d  ", time_diff );
      }
    }
    else
    {
      if( mode_deselect )
      {
        sprintf( buf2, "TRANS" );
      }
      else
      {
        sprintf (buf2,"YES");
      }
      background_color = hci_get_read_color (WARNING_COLOR);
    }
  }
  else
  {
    sprintf( buf2, "????" );
    background_color = hci_get_read_color( WARNING_COLOR );
  }

  if( strcpy( prev_conflict_buf, buf2 ) !=0 ||
      strcpy( prev_timer_buf, buf3 ) != 0 || force_draw )
  {
    if( strcmp( buf3, "NA" ) == 0 )
    {
      XSetForeground( HCI_get_display(),
                      hci_control_panel_gc(),
                      background_color );

      XFillRectangle( HCI_get_display(),
                      hci_control_panel_pixmap(),
                      hci_control_panel_gc(),
                      ( int ) x2,
                      ( int ) mode_stat->scanl,
                      ( int ) ( x3 - x2 ),
                      ( int ) ( height + 2 ) );

      XSetForeground( HCI_get_display(),
                      hci_control_panel_gc(),
                      foreground_color );

      temp_int = XTextWidth( fontinfo, buf2, strlen( buf2 ) );
      temp_int = x3 - x2 - temp_int;
      temp_int /= 2;

      XDrawString( HCI_get_display(),
                   hci_control_panel_pixmap(),
                   hci_control_panel_gc(),
                   ( int ) ( mode_stat->pixel + temp_int ),
                   ( int ) ( mode_stat->scanl + height ),
                   buf2,
                  strlen( buf2 ) );
    }
    else
    {
      /* Maximum width of timer label (according to XTextWidth). */

      int timer_width = XTextWidth( fontinfo, "  20  ", strlen( "  20  " ) ); 

      /* Actual width of timer label. */

      int wid = XTextWidth( fontinfo, buf3, strlen( buf3 ) ); 


      XSetForeground( HCI_get_display(),
                      hci_control_panel_gc(),
                      background_color );

      XFillRectangle( HCI_get_display(),
                      hci_control_panel_pixmap(),
                      hci_control_panel_gc(),
                      ( int ) x2,
                      ( int ) mode_stat->scanl,
                      ( int ) ( ( x3 - timer_width ) - x2 ),
                      ( int ) ( height + 2 ) );

      XSetForeground( HCI_get_display(),
                      hci_control_panel_gc(),
                      foreground_color );

      temp_int = XTextWidth( fontinfo, buf2, strlen( buf2 ) );
      temp_int = ( x3 - timer_width ) - x2 - temp_int;
      temp_int /= 2;

      XDrawString( HCI_get_display(),
                   hci_control_panel_pixmap(),
                   hci_control_panel_gc(),
                   ( int ) ( mode_stat->pixel + temp_int ),
                   ( int ) ( mode_stat->scanl + height ),
                   buf2,
                   strlen( buf2 ) );

      XSetForeground( HCI_get_display(),
                      hci_control_panel_gc(),
                      hci_get_read_color( WHITE ) );

      XFillRectangle( HCI_get_display(),
                      hci_control_panel_pixmap(),
                      hci_control_panel_gc(),
                      ( int ) ( x3 - timer_width ),
                      ( int ) mode_stat->scanl,
                      ( int ) timer_width,
                      ( int ) ( height + 2 ) );

      XSetForeground( HCI_get_display(),
                      hci_control_panel_gc(),
                      foreground_color );

      temp_int = ( timer_width - wid )/2;

      XDrawString( HCI_get_display(),
                   hci_control_panel_pixmap(),
                   hci_control_panel_gc(),
                   ( int ) ( x3 - timer_width + temp_int ),
                   ( int ) ( mode_stat->scanl +height ),
                   buf3,
                   strlen( buf3 ) );
    }

    /* Draw rectangle around 1st row. */

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( TEXT_FOREGROUND ) );

    XDrawRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    ( int ) ( x1 - 2 ),
                    ( int ) mode_stat->scanl,
                    ( int ) ( x3 - x1 + 2 ),
                    ( int ) ( height + 2 ) );

    strcpy( prev_conflict_buf, buf2 );
    strcpy( prev_timer_buf, buf3 );
  }

  /* Row #2 - Display Clear Air Switching. */

  /* Get coordinates for 2nd row. */

  clear_air_switch->pixel  = x2;
  clear_air_switch->scanl  = mode_stat->scanl + height + 4;
  clear_air_switch->height = height;
  clear_air_switch->width  = x3 - x2;

  /* Build/draw left side of 2nd row. */

  sprintf( buf1, "Clear Air Switch: " );

  if( force_draw )
  {
    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( BACKGROUND_COLOR2 ) );

    XFillRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    ( int ) ( x1 - 2 ),
                    ( int ) clear_air_switch->scanl,
                    ( int ) ( x2 - x1 + 2 ),
                    ( int ) ( height + 2 ) );

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    foreground_color );

    XDrawString( HCI_get_display(),
                 hci_control_panel_pixmap(),
                 hci_control_panel_gc(),
                 ( int ) x1,
                 ( int ) ( clear_air_switch->scanl + height ),
                 buf1,
                 strlen( buf1 ) );
  }

  /* Build/draw right side of 2nd row. */

  switch_flag = hci_get_mode_B_auto_switch_flag();

  if( switch_flag == AUTO_SWITCH )
  {
    sprintf( buf2, " AUTO " );
    background_color = hci_get_read_color( NORMAL_COLOR );
    foreground_color = hci_get_read_color( TEXT_FOREGROUND );
  }
  else if( switch_flag == MANUAL_SWITCH )
  {
    sprintf( buf2, " MANUAL " );
    background_color = hci_get_read_color( WARNING_COLOR );
    foreground_color = hci_get_read_color( TEXT_FOREGROUND );
  }
  else
  {
    sprintf( buf2, " ???? " );
    background_color = hci_get_read_color( WARNING_COLOR );
    foreground_color = hci_get_read_color( TEXT_FOREGROUND );
  }

  if( strcmp( prev_mode_B_switch_buf, buf2 ) != 0 || force_draw )
  {
    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    background_color );

    XFillRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    ( int ) x2,
                    ( int ) clear_air_switch->scanl,
                    ( int ) ( x3 -x2 ),
                    ( int ) ( height+2 ) );

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    foreground_color );

    temp_int = XTextWidth( fontinfo, buf2, strlen( buf2 ) );
    temp_int = x3 - x2 - temp_int;
    temp_int /= 2;

    XDrawString( HCI_get_display(),
                 hci_control_panel_pixmap(),
                 hci_control_panel_gc(),
                 ( int ) ( clear_air_switch->pixel + temp_int ),
                 ( int ) ( clear_air_switch->scanl + height ),
                 buf2,
                 strlen( buf2 ) );

    /* Draw rectangle around 2nd row. */

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( TEXT_FOREGROUND ) );

    XDrawRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    ( int ) ( x1 - 2 ),
                    ( int ) clear_air_switch->scanl,
                    ( int ) ( x3 - x1 + 2 ),
                    ( int ) ( height + 2 ) );

    strcpy( prev_mode_B_switch_buf, buf2 );
  }

  /* Row #3 - Display Precip Switching. */

  /* Get coordinates for 3rd row. */

  precip_switch->pixel  = x2;
  precip_switch->scanl  = clear_air_switch->scanl + height + 4;
  precip_switch->height = height;
  precip_switch->width  = x3 - x2;

  /* Build/draw left side of 3rd row. */

  sprintf( buf1, "Precip Switch: " );

  if( force_draw )
  {
    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( BACKGROUND_COLOR2 ) );

    XFillRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    ( int ) ( x1 - 2 ),
                    ( int ) precip_switch->scanl,
                    ( int ) ( x2 - x1 + 2 ),
                    ( int ) ( height + 2 ) );

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    foreground_color );

    XDrawString( HCI_get_display(),
                 hci_control_panel_pixmap(),
                 hci_control_panel_gc(),
                 ( int ) x1,
                 ( int ) ( precip_switch->scanl + height ),
                 buf1,
                 strlen( buf1 ) );
  }

  /* Build/draw right side of 3rd row. */

  switch_flag = hci_get_mode_A_auto_switch_flag();

  if( switch_flag == AUTO_SWITCH )
  {
    sprintf( buf2, " AUTO " );
    background_color = hci_get_read_color( NORMAL_COLOR );
    foreground_color = hci_get_read_color( TEXT_FOREGROUND );
  }
  else if( switch_flag == MANUAL_SWITCH )
  {
    sprintf( buf2, " MANUAL " );
    background_color = hci_get_read_color( WARNING_COLOR );
    foreground_color = hci_get_read_color( TEXT_FOREGROUND );
  }
  else
  {
    sprintf( buf2, " ???? " );
    background_color = hci_get_read_color( WARNING_COLOR );
    foreground_color = hci_get_read_color( TEXT_FOREGROUND );
  }

  if( strcmp( prev_mode_A_switch_buf, buf2 ) != 0 || force_draw )
  {
    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    background_color );

    XFillRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    ( int ) x2,
                    ( int ) precip_switch->scanl,
                    ( int ) ( x3 - x2 ),
                    ( int ) ( height + 2 ) );

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    foreground_color );

    temp_int = XTextWidth( fontinfo, buf2, strlen( buf2 ) );
    temp_int = x3 - x2 - temp_int;
    temp_int /= 2;

    XDrawString( HCI_get_display(),
                 hci_control_panel_pixmap(),
                 hci_control_panel_gc(),
                 ( int ) ( precip_switch->pixel + temp_int ),
                 ( int ) ( precip_switch->scanl + height ),
                  buf2,
                 strlen( buf2 ) );

    /* Draw rectangle around 3rd row. */

    XSetForeground( HCI_get_display(),
                    hci_control_panel_gc(),
                    hci_get_read_color( BLACK ) );

    XDrawRectangle( HCI_get_display(),
                    hci_control_panel_pixmap(),
                    hci_control_panel_gc(),
                    ( int ) ( x1 - 2 ),
                    ( int ) precip_switch->scanl,
                    ( int ) ( x3 - x1 + 2 ),
                    ( int ) ( height + 2 ) );

    strcpy( prev_mode_A_switch_buf, buf2 );
  }
}

/************************************************************************
 *	Description: This function is used to display a verification	*
 *		     popup window to change the Clear Air switch state.	*
 *									*
 *	Input:  w - ID of widget popup window to be child of		*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void verify_clear_air_switch( Widget parent_widget,
                              XtPointer client_data,
                              XtPointer call_data )
{
  int switch_flag = -1;
  char buf[ HCI_BUF_128 ];

  void verify_clear_air_switch_accept( Widget, XtPointer, XtPointer );
  void verify_clear_air_switch_cancel( Widget, XtPointer, XtPointer );

  switch_flag = hci_get_mode_B_auto_switch_flag();

  sprintf( buf, "You are about to " );

  if( switch_flag == AUTO_SWITCH )
  {
    strcat( buf, "disable the auto-switch to Clear Air mode." );
  }
  else
  {
    strcat( buf, "enable the auto-switch to Clear Air mode." );
  }

  strcat( buf, "\nDo you want to continue?" );

  hci_confirm_popup( parent_widget, buf, verify_clear_air_switch_accept, verify_clear_air_switch_cancel );
}

/************************************************************************
 *	Description: This function is called when the user selects the	*
 *		     "Yes" button from the Clear Air Switch		*
 *		     confirmation popup window.				*
 *									*
 *	Input:  w           - ID of yes button				*
 *		client_data - unused					*
 *		call_data   - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void verify_clear_air_switch_accept( Widget w,
                                     XtPointer client_data,
                                     XtPointer call_data )
{
  if( hci_get_mode_B_auto_switch_flag() == AUTO_SWITCH )
  {
    if( hci_set_mode_B_auto_switch_flag( MANUAL_SWITCH ) < 0 )
    {
      HCI_LE_error( "hci_set_mode_B_auto_switch_flag(MANUAL_SWITCH): failed" );
      deau_failure_popup();
    }
  }
  else
  {
    if( hci_set_mode_B_auto_switch_flag( AUTO_SWITCH ) < 0 )
    {
      HCI_LE_error( "hci_set_mode_B_auto_switch_flag(AUTO_SWITCH): failed" );
      deau_failure_popup();
    }
  }
}

/************************************************************************
 *	Description: This function is called when the user selects the	*
 *		     "No" button from the Clear Air Switch		*
 *		     confirmation popup window.				*
 *									*
 *	Input:  w - ID of yes button					*
 *		client_data - unused					*
 *		call_data   - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void verify_clear_air_switch_cancel( Widget w,
                                     XtPointer client_data,
                                     XtPointer call_data )
{
}

/************************************************************************
 *	Description: This function is used to display a verification	*
 *		     popup window to change the Precip switch state.	*
 *									*
 *	Input:  w - ID of widget popup window to be child of		*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void verify_precip_switch( Widget parent_widget,
                           XtPointer client_data,
                           XtPointer call_data )
{
  int switch_flag = -1;
  char buf[ HCI_BUF_128 ];

  void verify_precip_switch_accept( Widget, XtPointer, XtPointer );

  switch_flag = hci_get_mode_A_auto_switch_flag();

  sprintf( buf, "You are about to " );

  if( switch_flag == AUTO_SWITCH )
  {
    strcat( buf, "disable the auto-switch to Precipitation mode." );
  }
  else
  {
    strcat( buf, "enable the auto-switch to Precipitation mode." );
  }

  strcat( buf, "\nDo you want to continue?" );

  hci_confirm_popup( parent_widget, buf, verify_precip_switch_accept, NULL );
}

/************************************************************************
 *	Description: This function is called when the user selects the	*
 *		     "Yes" button from the Precip Switch confirmation	*
 *		     popup window.					*
 *									*
 *	Input:  w - ID of yes button					*
 *		client_data - unused					*
 *		call_data   - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void verify_precip_switch_accept( Widget w,
                                  XtPointer client_data,
                                  XtPointer call_data )
{
  if( hci_get_mode_A_auto_switch_flag() == AUTO_SWITCH )
  {
    if( hci_set_mode_A_auto_switch_flag( MANUAL_SWITCH ) < 0 )
    {
      HCI_LE_error( "hci_set_mode_A_auto_switch_flag(MANUAL_SWITCH): failed" );
      deau_failure_popup();
    }
  }
  else
  {
    if( hci_set_mode_A_auto_switch_flag( AUTO_SWITCH ) < 0 )
    {
      HCI_LE_error( "hci_set_mode_A_auto_switch_flag(AUTO_SWITCH): failed" );
      deau_failure_popup();
    }
  }
}

/************************************************************************
 *	Description: This function is called when setting a DEAU	*
 *		     variable fails. A popup displays warning the user.	*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void deau_failure_popup()
{
  char msg_buf[ HCI_BUF_256 ];

  sprintf( msg_buf, "Failure accessing DEAU database.\n\n" );
  strcat( msg_buf, "This could be due to loss of network connectivity,\n" );
  strcat( msg_buf, "rebooting the RPG box, or DEAU database corruption.\n\n" );
  strcat( msg_buf, "Try again later.\n\n" );
  strcat( msg_buf, "If this error continues, contact the WSR-88D Hotline.\n" );

  hci_error_popup( Top_object->widget, msg_buf, NULL );
}

