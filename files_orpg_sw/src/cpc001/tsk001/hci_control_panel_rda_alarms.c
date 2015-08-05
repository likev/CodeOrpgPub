/************************************************************************
 *									*
 *	Module:  hci_control_panel_rda_alarms.c				*
 *									*
 *	Description:  This module is used to display a active RDA	*
 *		      alarms, by component, to the left of the RDA	*
 *		      radome/tower.					*
 *									*
 ************************************************************************/

/*
 * RCS info
 * $Author: jeffs $
 * $Locker:  $
 * $Date: 2014/03/18 16:55:31 $
 * $Id: hci_control_panel_rda_alarms.c,v 1.23 2014/03/18 16:55:31 jeffs Exp $
 * $Revision: 1.23 $
 * $State: Exp $
 */

/* Local include file definitions. */

#include <hci_control_panel.h>

/* Macros. */

#define	NUM_LEGACY_ALARMS	9

/* Global/static variables. */

static	int	prev_alarm = -1;

/************************************************************************
 *	Description: This function checks the RDA alarm summary in the	*
 *		     latest RDA status data message and creates a	*
 *		     selectable label to the left of the radome/tower	*
 *		     for each device with an active alarm.		*
 *									*
 *	Input:  force_draw 1 - force a redraw				*
 *		           0 - redraw only if anything changes		*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void hci_control_panel_rda_alarms( int force_draw )
{
  int top_scanl = -1;
  int left_pixel = -1;
  int height = -1;
  int alarm = -1;
  int wbstat = -1;
  int blanking = -1;
  int indx = 0;
  int i = -1;
  int x_offset = -1;
  int label_width = -1;
  int font_asc = -1;
  int font_des = -1;
  char buf1[ 6 ];
  XFontStruct *fontinfo;
  hci_control_panel_object_t *top;
  hci_control_panel_object_t *rda;
  hci_control_panel_object_t *object;
  Pixel alarm_bg, alarm_fg;

  /* Get reference to top-level and RDA gui objects. */

  top = hci_control_panel_object( TOP_WIDGET );
  rda = hci_control_panel_object( RDA_BUTTON );

  if( rda->widget == ( Widget ) NULL )
  {
    return;
  }

  /* Get the wideband and display blanking status. */

  wbstat = ORPGRDA_get_wb_status( ORPGRDA_WBLNSTAT );
  blanking = ORPGRDA_get_wb_status( ORPGRDA_DISPLAY_BLANKING );

  /* If the wideband is connected and there is no display blanking,
     display the alarm information. */

  if( wbstat == RS_CONNECTED ||
      wbstat == RS_DISCONNECT_PENDING ||
      ( blanking && RS_RDA_ALARM_SUMMARY ) )
  {
    /* Read alarms. */

    alarm = ORPGRDA_get_status( RS_RDA_ALARM_SUMMARY );

    if( alarm != prev_alarm || force_draw )
    {
      /* Determine the screen coordinates for the first alarm label. */

      left_pixel = 10;
      top_scanl = rda->scanl - top->height/4 - rda->width;

      /* Set font information. */

      fontinfo = hci_get_fontinfo( SCALED );
      font_asc = fontinfo->ascent;
      font_des = fontinfo->descent;
      height = font_asc + font_des;

      /* Reset y-coordinate for first label. */

      top_scanl = rda->scanl - top->height/4 - rda->width;

      /* The " XMT " label is the longest (with respect to XTextWidth()),
         so use it to determine label width for all alarms. */

      sprintf( buf1, " COM " );
      label_width = XTextWidth( fontinfo, buf1, strlen( buf1 ) );

      alarm_fg = hci_get_read_color( BACKGROUND_COLOR1 );
      alarm_bg = hci_get_read_color( BACKGROUND_COLOR1 );

      /* Clear previous alarms from screen. Do this by drawing alarms
         in background color. */

      for( i = 1; i < NUM_LEGACY_ALARMS; i++ )
      {
        XSetBackground( HCI_get_display(),
                        hci_control_panel_gc(),
                        alarm_bg );

        XSetForeground( HCI_get_display(),
                        hci_control_panel_gc(),
                        alarm_fg );

        XFillRectangle( HCI_get_display(),
                        hci_control_panel_pixmap(),
                        hci_control_panel_gc(),
                        left_pixel,
                        top_scanl,
                        label_width,
                        height + font_des );

        /* Determine x_offset to center label. */

        x_offset = label_width - XTextWidth( fontinfo, buf1, strlen( buf1 ) );
        x_offset /= 2;

        XDrawImageString( HCI_get_display(),
                          hci_control_panel_pixmap(),
                          hci_control_panel_gc(),
                          left_pixel + x_offset,
                          top_scanl + height,
                          buf1,
                          strlen( buf1 ) );

        XDrawRectangle( HCI_get_display(),
                        hci_control_panel_pixmap(),
                        hci_control_panel_gc(),
                        left_pixel,
                        top_scanl,
                        label_width,
                        height + font_des );

        top_scanl = top_scanl + 3*height/2;
      }

      /* Set background color for new alarms. */

      alarm_fg = hci_get_read_color( TEXT_FOREGROUND );
      alarm_bg = hci_get_read_color( WHITE );

      /* Reset y-coordinate for first label. */

      top_scanl = rda->scanl - top->height/4 - rda->width;

      /* If the Tower/Utility category has an active alarm, display
         a selectable label. */

      if( alarm & BIT_1_MASK )
      {
        sprintf( buf1, " UTL " );

        XSetForeground( HCI_get_display(),
                        hci_control_panel_gc(),
                        alarm_bg );

        XSetBackground( HCI_get_display(),
                        hci_control_panel_gc(),
                        alarm_bg );

        XFillRectangle( HCI_get_display(),
                        hci_control_panel_pixmap(),
                        hci_control_panel_gc(),
                        left_pixel,
                        top_scanl,
                        label_width,
                        height + font_des );

        XSetForeground( HCI_get_display(),
                        hci_control_panel_gc(),
                        alarm_fg );

        /* Determine x_offset to center label. */

        x_offset = label_width - XTextWidth( fontinfo, buf1, strlen( buf1 ) );
        x_offset /= 2;

        XDrawImageString( HCI_get_display(),
                          hci_control_panel_pixmap(),
                          hci_control_panel_gc(),
                          left_pixel + x_offset,
                          top_scanl + height,
                          buf1,
                          strlen( buf1 ) );

        XDrawRectangle( HCI_get_display(),
                        hci_control_panel_pixmap(),
                        hci_control_panel_gc(),
                        left_pixel,
                        top_scanl,
                        label_width,
                        height + font_des );

        object = hci_control_panel_object( RDA_ALARM1_OBJECT +indx );

        object->pixel = left_pixel;
        object->scanl = top_scanl;
        object->width = label_width;
        object->height = height;
        object->flags = OBJECT_SELECTABLE;
        object->data = ( unsigned int ) UTL_MASK;
        indx++;

        top_scanl = top_scanl + 3*height/2;
      }

      /* If the Antenna/Pedestal category has an active alarm,
         display a selectable label. */

      if( alarm & BIT_2_MASK )
      {
        sprintf( buf1, " PED " );

        XSetForeground( HCI_get_display(),
                        hci_control_panel_gc(),
                        alarm_bg );

        XSetBackground( HCI_get_display(),
                        hci_control_panel_gc(),
                        alarm_bg );

        XFillRectangle( HCI_get_display(),
                        hci_control_panel_pixmap(),
                        hci_control_panel_gc(),
                        left_pixel,
                        top_scanl,
                        label_width,
                        height + font_des );

        XSetForeground( HCI_get_display(),
                        hci_control_panel_gc(),
                        alarm_fg );

        /* Determine x_offset to center label. */

        x_offset = label_width - XTextWidth( fontinfo, buf1, strlen( buf1 ) );
        x_offset /= 2;

        XDrawImageString( HCI_get_display(),
                          hci_control_panel_pixmap(),
                          hci_control_panel_gc(),
                          left_pixel + x_offset,
                          top_scanl + height,
                          buf1,
                          strlen( buf1 ) );

        XDrawRectangle( HCI_get_display(),
                        hci_control_panel_pixmap(),
                        hci_control_panel_gc(),
                        left_pixel,
                        top_scanl,
                        label_width,
                        height + font_des );

        object = hci_control_panel_object( RDA_ALARM1_OBJECT +indx );

        object->pixel = left_pixel;
        object->scanl = top_scanl;
        object->width = label_width;
        object->height = height;
        object->flags = OBJECT_SELECTABLE;
        object->data = ( unsigned int ) PED_MASK;
        indx++;

        top_scanl = top_scanl + 3*height/2;
      }

      /* If the Transmitter category has an active alarm, display
         a selectable label. */

      if( alarm & BIT_3_MASK )
      {
        sprintf( buf1, " XMT " );

        XSetForeground( HCI_get_display(),
                        hci_control_panel_gc(),
                        alarm_bg );

        XSetBackground( HCI_get_display(),
                        hci_control_panel_gc(),
                        alarm_bg );

        XFillRectangle( HCI_get_display(),
                        hci_control_panel_pixmap(),
                        hci_control_panel_gc(),
                        left_pixel,
                        top_scanl,
                        label_width,
                        height + font_des );

        XSetForeground( HCI_get_display(),
                        hci_control_panel_gc(),
                        alarm_fg );

        /* Determine x_offset to center label. */

        x_offset = label_width - XTextWidth( fontinfo, buf1, strlen( buf1 ) );
        x_offset /= 2;

        XDrawImageString( HCI_get_display(),
                          hci_control_panel_pixmap(),
                          hci_control_panel_gc(),
                          left_pixel + x_offset,
                          top_scanl + height,
                          buf1,
                          strlen( buf1 ) );

        XDrawRectangle( HCI_get_display(),
                        hci_control_panel_pixmap(),
                        hci_control_panel_gc(),
                        left_pixel,
                        top_scanl,
                        label_width,
                        height + font_des );

        object = hci_control_panel_object( RDA_ALARM1_OBJECT +indx );

        object->pixel = left_pixel;
        object->scanl = top_scanl;
        object->width = label_width;
        object->height = height;
        object->flags = OBJECT_SELECTABLE;
        object->data = ( unsigned int ) XMT_MASK;
        indx++;

        top_scanl = top_scanl + 3*height/2;
      }

      /* If the Receiver category has an active alarm, display
         a selectable label. */

      if( alarm & BIT_4_MASK )
      {
        sprintf( buf1, " RSP " );

        XSetForeground( HCI_get_display(),
                        hci_control_panel_gc(),
                        alarm_bg );

        XSetBackground( HCI_get_display(),
                        hci_control_panel_gc(),
                        alarm_bg );

        XFillRectangle( HCI_get_display(),
                        hci_control_panel_pixmap(),
                        hci_control_panel_gc(),
                        left_pixel,
                        top_scanl,
                        label_width,
                        height + font_des );

        XSetForeground( HCI_get_display(),
                        hci_control_panel_gc(),
                        alarm_fg );

        /* Determine x_offset to center label. */

        x_offset = label_width - XTextWidth( fontinfo, buf1, strlen( buf1 ) );
        x_offset /= 2;

        XDrawImageString( HCI_get_display(),
                          hci_control_panel_pixmap(),
                          hci_control_panel_gc(),
                          left_pixel + x_offset,
                          top_scanl + height,
                          buf1,
                          strlen( buf1 ) );

        XDrawRectangle( HCI_get_display(),
                        hci_control_panel_pixmap(),
                        hci_control_panel_gc(),
                        left_pixel,
                        top_scanl,
                        label_width,
                        height + font_des );

        object = hci_control_panel_object( RDA_ALARM1_OBJECT +indx );

        object->pixel = left_pixel;
        object->scanl = top_scanl;
        object->width = label_width;
        object->height = height;
        object->flags = OBJECT_SELECTABLE;
        object->data = ( unsigned int ) RSP_MASK;
        indx++;

        top_scanl = top_scanl + 3*height/2;
      }

      /* If the RDA Control category has an active alarm, display
         a selectable label. */

      if( alarm & BIT_5_MASK )
      {
        sprintf( buf1, " CTR " );

        XSetForeground( HCI_get_display(),
                        hci_control_panel_gc(),
                        alarm_bg );

        XSetBackground( HCI_get_display(),
                        hci_control_panel_gc(),
                        alarm_bg );

        XFillRectangle( HCI_get_display(),
                        hci_control_panel_pixmap(),
                        hci_control_panel_gc(),
                        left_pixel,
                        top_scanl,
                        label_width,
                        height + font_des );

        XSetForeground( HCI_get_display(),
                        hci_control_panel_gc(),
                        alarm_fg );

        /* Determine x_offset to center label. */

        x_offset = label_width - XTextWidth( fontinfo, buf1, strlen( buf1 ) );
        x_offset /= 2;

        XDrawImageString( HCI_get_display(),
                          hci_control_panel_pixmap(),
                          hci_control_panel_gc(),
                          left_pixel + x_offset,
                          top_scanl + height,
                          buf1,
                          strlen( buf1 ) );

        XDrawRectangle( HCI_get_display(),
                        hci_control_panel_pixmap(),
                        hci_control_panel_gc(),
                        left_pixel,
                        top_scanl,
                        label_width,
                        height + font_des );

        object = hci_control_panel_object( RDA_ALARM1_OBJECT +indx );

        object->pixel = left_pixel;
        object->scanl = top_scanl;
        object->width = label_width;
        object->height = height;
        object->flags = OBJECT_SELECTABLE;
        object->data = ( unsigned int ) CTR_MASK;
        indx++;

        top_scanl = top_scanl + 3*height/2;
      }

      /* If the RPG Communications category has an active alarm, display
         a selectable label. */

      if( alarm & BIT_6_MASK )
      {
        sprintf( buf1, " WID " );

        XSetForeground( HCI_get_display(),
                        hci_control_panel_gc(),
                        alarm_bg );

        XSetBackground( HCI_get_display(),
                        hci_control_panel_gc(),
                        alarm_bg );

        XFillRectangle( HCI_get_display(),
                        hci_control_panel_pixmap(),
                        hci_control_panel_gc(),
                        left_pixel,
                        top_scanl,
                        label_width,
                        height + font_des );

        XSetForeground( HCI_get_display(),
                        hci_control_panel_gc(),
                        alarm_fg );

        /* Determine x_offset to center label. */

        x_offset = label_width - XTextWidth( fontinfo, buf1, strlen( buf1 ) );
        x_offset /= 2;

        XDrawImageString( HCI_get_display(),
                          hci_control_panel_pixmap(),
                          hci_control_panel_gc(),
                          left_pixel + x_offset,
                          top_scanl + height,
                          buf1,
                          strlen( buf1 ) );

        XDrawRectangle( HCI_get_display(),
                        hci_control_panel_pixmap(),
                        hci_control_panel_gc(),
                        left_pixel,
                        top_scanl,
                        label_width,
                        height + font_des );

        object = hci_control_panel_object( RDA_ALARM1_OBJECT +indx );

        object->pixel = left_pixel;
        object->scanl = top_scanl;
        object->width = label_width;
        object->height = height;
        object->flags = OBJECT_SELECTABLE;
        object->data = ( unsigned int ) WID_MASK;
        indx++;

        top_scanl = top_scanl + 3*height/2;
      }

      /* If the Wideband User category has an active alarm, display
         a selectable label. */

      if( alarm & BIT_7_MASK )
      {
        sprintf( buf1, " USR " );

        XSetForeground( HCI_get_display(),
                        hci_control_panel_gc(),
                        alarm_bg );

        XSetBackground( HCI_get_display(),
                        hci_control_panel_gc(),
                        alarm_bg );

        XFillRectangle( HCI_get_display(),
                        hci_control_panel_pixmap(),
                        hci_control_panel_gc(),
                        left_pixel,
                        top_scanl,
                        label_width,
                        height + font_des );

        XSetForeground( HCI_get_display(),
                        hci_control_panel_gc(),
                        alarm_fg );

        /* Determine x_offset to center label. */

        x_offset = label_width - XTextWidth( fontinfo, buf1, strlen( buf1 ) );
        x_offset /= 2;

        XDrawImageString( HCI_get_display(),
                          hci_control_panel_pixmap(),
                          hci_control_panel_gc(),
                          left_pixel + x_offset,
                          top_scanl + height,
                          buf1,
                          strlen( buf1 ) );

        XDrawRectangle( HCI_get_display(),
                        hci_control_panel_pixmap(),
                        hci_control_panel_gc(),
                        left_pixel,
                        top_scanl,
                        label_width,
                        height + font_des );

        object = hci_control_panel_object( RDA_ALARM1_OBJECT +indx );

        object->pixel = left_pixel;
        object->scanl = top_scanl;
        object->width = label_width;
        object->height = height;
        object->flags = OBJECT_SELECTABLE;
        object->data = ( unsigned int ) USR_MASK;
        indx++;

        top_scanl = top_scanl + 3*height/2;
      }

      /* If the Archive II category has an active alarm, display
         a selectable label. */

      if( alarm & BIT_8_MASK )
      {
        sprintf( buf1, " ARC " );

        XSetForeground( HCI_get_display(),
                        hci_control_panel_gc(),
                        alarm_bg );

        XSetBackground( HCI_get_display(),
                        hci_control_panel_gc(),
                        alarm_bg );

        XFillRectangle( HCI_get_display(),
                        hci_control_panel_pixmap(),
                        hci_control_panel_gc(),
                        left_pixel,
                        top_scanl,
                        label_width,
                        height + font_des );

        XSetForeground( HCI_get_display(),
                        hci_control_panel_gc(),
                        alarm_fg );

        /* Determine x_offset to center label. */

        x_offset = label_width - XTextWidth( fontinfo, buf1, strlen( buf1 ) );
        x_offset /= 2;

        XDrawImageString( HCI_get_display(),
                          hci_control_panel_pixmap(),
                          hci_control_panel_gc(),
                          left_pixel + x_offset,
                          top_scanl + height,
                          buf1,
                          strlen( buf1 ) );

        XDrawRectangle( HCI_get_display(),
                        hci_control_panel_pixmap(),
                        hci_control_panel_gc(),
                        left_pixel,
                        top_scanl,
                        label_width,
                        height + font_des );

        object = hci_control_panel_object( RDA_ALARM1_OBJECT +indx );

        object->pixel = left_pixel;
        object->scanl = top_scanl;
        object->width = label_width;
        object->height = height;
        object->flags = OBJECT_SELECTABLE;
        object->data = ( unsigned int ) ARC_MASK;
        indx++;

        top_scanl = top_scanl + 3*height/2;
      }

      /* For all alarm categories without an active alarm, set them
         to unselectable. */

      for( i = indx; i < 8; i++ )
      {
        object = hci_control_panel_object( RDA_ALARM1_OBJECT + indx );
        object->flags  = 0;
      }

      prev_alarm = alarm;
    }
  }
}
