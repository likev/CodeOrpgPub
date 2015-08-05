/*	basedata_tool.h - This header file defines		*
 *	functions used to access basedata data by the		*
 *	Base Data Display Tool.					*/

/*
 * RCCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2011/03/21 17:03:30 $
 * $Id: basedata_tool.h,v 1.1 2011/03/21 17:03:30 steves Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

#ifndef BASEDATA__TOOL_DEF
#define	BASEDATA_TOOL_DEF

/*	Include files needed.						*/

#define	REFLECTIVITY			0
#define	VELOCITY			1
#define	SPECTRUM_WIDTH			2
#define	DIFF_REFLECTIVITY		3
#define	DIFF_PHASE			4
#define	DIFF_CORRELATION		5
#define MAX_PROD_SHORTS			12640 /* 1840 REF + 9*1200 other */
#define	SIZEOF_BASEDATA			(MAX_PROD_SHORTS*sizeof(short))

#define	UPPER_MOMENT_THRESHOLD		999.9
#define	LOWER_MOMENT_THRESHOLD		-999.9

/* Max/min for legend */
#define	REFLECTIVITY_MIN		 -32.0
#define	REFLECTIVITY_MAX		  95.0
#define	VELOCITY_MIN			-64.0*MPS_TO_KTS /* m/s to knots */
#define	VELOCITY_MAX			 64.0*MPS_TO_KTS /* m/s to knots */
#define	SPECTRUM_WIDTH_MIN		   0.0*MPS_TO_KTS /* m/s to knots */
#define	SPECTRUM_WIDTH_MAX		  10.3*MPS_TO_KTS /* m/s to knots */
#define	DIFF_REFLECTIVITY_MIN		 -4.0
#define	DIFF_REFLECTIVITY_MAX		  8.0
#define	DIFF_PHASE_MIN			  0.0
#define	DIFF_PHASE_MAX			  1080.0
#define	DIFF_CORRELATION_MIN		  0.2
#define	DIFF_CORRELATION_MAX		  1.2

#define	MAX_BINS_ALONG_RADIAL		MAX_BASEDATA_REF_SIZE
#define	MAX_RADIAL_RANGE		460.0

#define	DOPPLER_RESOLUTION_LOW		4
#define	DOPPLER_RESOLUTION_HIGH		2

#define DISPLAY_MODE_STATIC     0 /* single cut mode (not updated) */
#define DISPLAY_MODE_DYNAMIC    1 /* real-time mode (updated) */

#define HCI_BASEDATA_PARTIAL_READ	  1
#define HCI_BASEDATA_COMPLETE_READ	  0

#define PRODUCT_COLORS			16
#define NO_LABEL_FLAG                   -999999

int	hci_basedata_tool_read_radial(int msg_id, int partial_read);
int	hci_basedata_tool_id();
int	hci_basedata_tool_msgid();
int	hci_basedata_tool_time();
int	hci_basedata_tool_date();
float	hci_basedata_tool_unambiguous_range();
float	hci_basedata_tool_nyquist_velocity();
float	hci_basedata_tool_elevation();
float	hci_basedata_tool_target_elevation();
float	hci_basedata_tool_azimuth();
int	hci_basedata_tool_azimuth_number();
int	hci_basedata_tool_elevation_number();
int	hci_basedata_tool_vcp_number();
int	hci_basedata_tool_msg_type();
int	hci_basedata_tool_velocity_resolution();
int	hci_basedata_tool_data_available(int);
int	hci_basedata_tool_radial_status();

int	hci_basedata_tool_seek(int msgid);
int	hci_basedata_tool_range_adjust(int moment);
int	hci_basedata_tool_bin_size(int moment);
int	hci_basedata_tool_number_bins(int moment);

float	hci_basedata_tool_value(int unscaled_value, int moment);
float	hci_basedata_tool_value_index(int indx, int moment);
float	hci_basedata_tool_range(int indx, int moment);
float  *hci_basedata_tool_range_ptr(int moment);
int	hci_basedata_tool_range_index(float range, int moment);
float  *hci_basedata_tool_data_table_ptr(int moment);
float  *hci_basedata_tool_data_thresh_ptr(int moment);
void    hci_basedata_tool_set_data_ptr(char *data, int data_len);
void   *hci_basedata_tool_data_ptr(int moment);
int	hci_basedata_tool_data_word_size(int moment);
int	hci_basedata_tool_azimuth_resolution();
float	hci_basedata_tool_azimuth_width();
void	hci_basedata_tool_set_basedata_LB_id(int LB_id);

float	hci_basedata_tool_interrogate( int mode_flag, float az, float rng,
					int fld, int LB_id, int ptr );

void	hci_basedata_tool_display( char data[], int data_len, Display *display,
		Drawable window, GC gc, float x_offset, float y_offset,
		float scale_x, float scale_y, int cntr_pixel, int cntr_scanl,
		float min_value, float max_value, int *prod_color, int moment);
void	hci_basedata_tool_display_clear( Display *display, Drawable window,
			GC gc, int width, int height, int color );

void	hci_basedata_tool_get_product_colors(int pcode, int *pcolor);

int	hci_basedata_tool_get_lock_state();
void	hci_basedata_tool_set_lock_state(int state);
void	hci_basedata_tool_set_data_feed(int feed_type);

#endif
