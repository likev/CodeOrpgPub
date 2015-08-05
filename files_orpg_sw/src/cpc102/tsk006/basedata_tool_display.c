/************************************************************************
 *									*
 *	Module:  basedata_tool_display.c				*
 *									*
 *	Description: This function is used by the RPG Base Data		*
 *		     Display Tool task to display base data.		*
 *									*
 ************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2011/03/21 17:03:31 $
 * $Id: basedata_tool_display.c,v 1.1 2011/03/21 17:03:31 steves Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

/* Local include file definitions. */

#include <hci.h>
#include <basedata_tool.h>

extern int BD_ignore_scale_offset;

/* Static variables. */

static Display *Display_info = NULL;
static Drawable Window_info;
static GC GC_info;
static XPoint Xpt[12];
static float Min_value = 0.0;
static float Max_value = 0.0;
static float Sin [730];
static float Cos [730];
static float Color_scale = 1.0;
static float Sin1 = 0.0;
static float Sin2 = 0.0;
static float Cos1 = 0.0;
static float Cos2 = 0.0;
static float Scale_x = 0.0;
static float Scale_y = 0.0;
static float Offset_x = 0.0;
static float Offset_y = 0.0;
static int *Product_color = NULL;
static int Init_flag = 0;
static int Doppler_data_flag = 0;
static int Max_color_adjust = 1;
static int Previous_color = -1;
static int Center_pixel = -1;
static int Center_scanl = -1;

/* Function prototypes. */

void hci_basedata_tool_draw_bin( float range, float value );
void hci_basedata_tool_draw_last_bin( float range );

/************************************************************************
 *	Description: This function is used to display a base data	*
 *		     radial in the specified window.  The base data	*
 *		     can be either passed directly when reading from	*
 *		     the replay database or indirectly using the	*
 *		     basedata functions in libhci.			*
 *									*
 *	Input:  data          - pointer to radial data.  If NULL, data	*
 *				are read using basedata functions in	*
 *				libhci					*
 *		data_len      - length of data				*
 *		*display      - Display information for X windows	*
 *		window        - Drawable				*
 *		gc            - Graphics context to use			*
 *		x_offset      - X offset of radar to drawable center	*
 *		y_offset      - Y offset of radar to drawable center	*
 *		scale_x       - pixels/km scale factor			*
 *		scale_y       - scanlines/km scale factor		*
 *		center_pixel  - center pixel in drawable		*
 *		center_scanl  - center scanline in drawable		*
 *		min_value     - minimum data value to display		*
 *		max_value     - maximum data value to display		*
 *		product_color - pointer to color table for data display	*
 *		moment        - moment to display			*
 *	Return: NONE							*
 ************************************************************************/

void hci_basedata_tool_display( char data[], int data_len, Display *display,
        Drawable window, GC gc, float x_offset, float y_offset,
        float scale_x, float scale_y, int center_pixel, int center_scanl,
        float min_value, float max_value, int *product_color, int moment )
{
  int i;
  float azimuth, azimuth1, azimuth2, azimuth_width;
  int num_gates;
  int data_word_size;
  float *ranges = NULL;
  float *values = NULL;
  unsigned char *data_c = NULL;
  unsigned short *data_s = NULL;
  char temp_buf[ 64 ];

  Display_info = display;
  Window_info = window;
  GC_info = gc;
  Offset_x = x_offset;
  Offset_y = y_offset;
  Scale_x = scale_x;
  Scale_y = scale_y;
  Center_pixel = center_pixel;
  Center_scanl = center_scanl;
  Min_value = min_value;
  Max_value = max_value;
  Product_color = product_color;

  /* If this is first time, then initialize Sin/Cos tables.  This
     saves processing time. */

  if( !Init_flag )
  {
    Init_flag = 1;
    for( i=0; i<730; i++ )
    {
      Sin[i] = sin( (double) (i/2.0+90.0)*HCI_DEG_TO_RAD );
      Cos[i] = cos( (double) (i/2.0-90.0)*HCI_DEG_TO_RAD );
    }
  }

  /* If this is Doppler data, then set flag to be reused later. */

  if( (moment == VELOCITY)
              || 
      (moment == SPECTRUM_WIDTH)
              ||
      (moment == DIFF_REFLECTIVITY) )
  {
    Doppler_data_flag = 1;
  }

  /* If data was passed in, pass to library which allows
     standardized access to data via convenience routines. */

  if( data != NULL ){ hci_basedata_tool_set_data_ptr( &data[0], data_len );}

  /* If the active moment isn't enabled, then display a message
     at the window center indicating that the data for the moment
     is not available. */

  if( !hci_basedata_tool_data_available( moment ) )
  {
    strcpy( temp_buf, "Data Not Available For This Field" );

    XSetForeground( display, gc,
                    hci_get_read_color( PRODUCT_FOREGROUND_COLOR ) );

    XDrawImageString( display, window, gc,
                      (center_pixel - 4*strlen(temp_buf)),
                      center_scanl + 4,
                      temp_buf,
                      strlen(temp_buf) );

    return;
  }

  /* Define sin/cos values intrinsic to this radial. */

  azimuth = hci_basedata_tool_azimuth();
  azimuth_width = hci_basedata_tool_azimuth_width();

  azimuth1 = azimuth - azimuth_width;
  if( azimuth1 < 0 ){ azimuth1 = azimuth1+360.0; }
  azimuth2 = azimuth + 3*azimuth_width/2;

  Sin1  = Sin [(int) (azimuth1*2)];
  Sin2  = Sin [(int) (azimuth2*2)];
  Cos1  = Cos [(int) (azimuth1*2)];
  Cos2  = Cos [(int) (azimuth2*2)];

  /* Determine the color scale. Adjust by 1 to account for
     the background color. Adjust Doppler data by 2 to also
     account for range folding. */

  if( Doppler_data_flag ){ Max_color_adjust = 2; }

  Color_scale = (Max_value - Min_value)/(PRODUCT_COLORS-Max_color_adjust);

  /* Unpack the data at each gate and draw to the display. */

  num_gates = hci_basedata_tool_number_bins( moment );
  ranges = hci_basedata_tool_range_ptr( moment );
  values = hci_basedata_tool_data_table_ptr( moment );

  data_word_size = hci_basedata_tool_data_word_size( moment );

  Previous_color = -1;

  if( data_word_size == 16 )
  {
    data_s = (unsigned short *)hci_basedata_tool_data_ptr( moment );
    if( data_s == NULL ){ return; }
    for( i=0; i<num_gates; i++)
    {
      hci_basedata_tool_draw_bin( ranges[i]/HCI_METERS_PER_KM, values[data_s[i]] );
    }
    hci_basedata_tool_draw_last_bin( ranges[i-1]/HCI_METERS_PER_KM );
  }
  else
  {
    data_c = (unsigned char *)hci_basedata_tool_data_ptr( moment );
    if( data_c == NULL ){ return; }
    for( i=0; i<num_gates; i++)
    {
      hci_basedata_tool_draw_bin( ranges[i]/HCI_METERS_PER_KM, values[(int)data_c[i]] );
    }
    hci_basedata_tool_draw_last_bin( ranges[i-1]/HCI_METERS_PER_KM );
  }
}

/************************************************************************
 *	Description: This function draws/fills a polygon corresponding	*
 *		     to a bin of radar data. To reduce the number of	*
 *		     XPolygonFill operations, only paint when a color	*
 *		     change has occurred.				*
 *									*
 *	Input:	range - distance to front of bin			*
 *		value - value of bin					*
 *	Return: NONE							*
 ************************************************************************/

void hci_basedata_tool_draw_bin( float range, float value )
{
  int color = -1;

  color = (int) value;

  if( color > 0 )
  {
    if( color != Previous_color )
    {
      if( Previous_color <= 0 )
      {
        Xpt[0].x = (range*Cos1 + Offset_x) * Scale_x + Center_pixel;
        Xpt[0].y = (range*Sin1 + Offset_y) * Scale_y + Center_scanl;
        Xpt[1].x = (range*Cos2 + Offset_x) * Scale_x + Center_pixel;
        Xpt[1].y = (range*Sin2 + Offset_y) * Scale_y + Center_scanl;
        Xpt[4].x = Xpt[0].x;
        Xpt[4].y = Xpt[0].y;
      }
      else
      {
        Xpt[2].x = (range*Cos2 + Offset_x) * Scale_x + Center_pixel;
        Xpt[2].y = (range*Sin2 + Offset_y) * Scale_y + Center_scanl;
        Xpt[3].x = (range*Cos1 + Offset_x) * Scale_x + Center_pixel;
        Xpt[3].y = (range*Sin1 + Offset_y) * Scale_y + Center_scanl;

        XSetForeground( Display_info, GC_info, Product_color[Previous_color] );
        XFillPolygon( Display_info, Window_info, GC_info,
                      Xpt, 4, Convex, CoordModeOrigin );

        Xpt[0].x = Xpt[3].x;
        Xpt[0].y = Xpt[3].y;
        Xpt[1].x = Xpt[2].x;
        Xpt[1].y = Xpt[2].y;
        Xpt[4].x = Xpt[0].x;
        Xpt[4].y = Xpt[0].y;
      }
    }
  }
  else if( Previous_color > 0 )
  {
    Xpt[2].x = (range*Cos2 + Offset_x) * Scale_x + Center_pixel;
    Xpt[2].y = (range*Sin2 + Offset_y) * Scale_y + Center_scanl;
    Xpt[3].x = (range*Cos1 + Offset_x) * Scale_x + Center_pixel;
    Xpt[3].y = (range*Sin1 + Offset_y) * Scale_y + Center_scanl;

    XSetForeground( Display_info, GC_info, Product_color[Previous_color] );
    XFillPolygon( Display_info, Window_info, GC_info,
                  Xpt, 4, Convex, CoordModeOrigin );

    Xpt[0].x = Xpt[3].x;
    Xpt[0].y = Xpt[3].y;
    Xpt[1].x = Xpt[2].x;
    Xpt[1].y = Xpt[2].y;
    Xpt[4].x = Xpt[0].x;
    Xpt[4].y = Xpt[0].y;
  }

  Previous_color = color;
}

/************************************************************************
 *	Description: This function draws/fills the last polygon		*
 *		     corresponding to a radial of radar data. To reduce	*
 *		     the number of XPolygonFill operations, only paint	*
 *		     when a color change has occurred.			*
 *									*
 *	Input:	range - distance to front of bin			*
 *	Return: NONE							*
 ************************************************************************/

void hci_basedata_tool_draw_last_bin( float range )
{
  if( Previous_color > 0 )
  {
    Xpt[2].x = (range*Cos2 + Offset_x) * Scale_x + Center_pixel;
    Xpt[2].y = (range*Sin2 + Offset_y) * Scale_y + Center_scanl;
    Xpt[3].x = (range*Cos1 + Offset_x) * Scale_x + Center_pixel;
    Xpt[3].y = (range*Sin1 + Offset_y) * Scale_y + Center_scanl;

    XSetForeground( Display_info, GC_info, Product_color[Previous_color] );
    XFillPolygon( Display_info, Window_info, GC_info,
                  Xpt, 4, Convex, CoordModeOrigin );
  }
}

/************************************************************************
 *	Description: This function clears the contents of the specified	*
 *	window by filling in region with specified color.		*
 *									*
 *	Input:	*display      - Display information for X windows	*
 *		window        - Drawable				*
 *		gc            - Graphics context to use			*
 *		width         - Width of region (pixels) to clear	*
 *		height        - Height of region (scanlines) to clear	*
 *		color         - Color index to fill with		*
 *	Return: NONE							*
 ************************************************************************/

void hci_basedata_tool_display_clear( Display *display, Drawable window,
                          GC gc, int width, int height, int color )
{
  XSetForeground( display, gc, color );
  XFillRectangle( display, window, gc, 0, 0, width, height );
}
