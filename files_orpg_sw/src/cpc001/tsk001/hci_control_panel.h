/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/10/03 16:32:50 $
 * $Id: hci_control_panel.h,v 1.77 2014/10/03 16:32:50 steves Exp $
 * $Revision: 1.77 $
 * $State: Exp $
 */

/******************************************************************
 *	hci_control_panel.h is the header file for the HCI	  *
 *	status/control task.					  *
 ******************************************************************/

#ifndef HCI_STATUS_CONTROL_H
#define HCI_STATUS_CONTROL_H

#include <hci.h>
#include <hci_environmental_wind.h>
#include <hci_orda_pmd.h>
#include <hci_precip_status.h>
#include <hci_rda_adaptation_data.h>
#include <hci_rda_performance.h>
#include <hci_rda_control_functions.h>
#include <hci_up_nb.h>
#include <hci_user_info.h>
#include <hci_vcp_data.h>
#include <hci_sails.h>

#define	CONTROL_PANEL_WIDTH		785
#define	CONTROL_PANEL_HEIGHT		713 
#define	MAX_CONTROL_PANEL_WIDTH		1090
#define	MAX_CONTROL_PANEL_HEIGHT	990

#define	RDA_STARTUP		 2
#define	RDA_STANDBY		 4
#define	RDA_RESTART		 8
#define	RDA_OPERATE		16
#define	RDA_PLAYBACK		32
#define	RDA_OFFLINE_OPERATE	64

#define	RDA_IN_CONTROL		 2
#define	RPG_IN_CONTROL		 4
#define	EITHER_IN_CONTROL	 8

#define	UTILITY_POWER		 0
#define	AUXILLIARY_POWER	 1

#define	INTERFERENCE_UNIT_ENABLED	2
#define	INTERFERENCE_UNIT_DISABLED	4

#define	SPOT_BLANKING_NOT_INSTALLED	0
#define	SPOT_BLANKING_ENABLED		2
#define	SPOT_BLANKING_DISABLED		4

#define	WIDEBAND_CONNECT		0
#define	WIDEBAND_DISCONNECT		1

#define	DATA_TYPE_REFLECTIVITY		0
#define	DATA_TYPE_VELOCITY		1
#define	DATA_TYPE_SPECTRUM_WIDTH	2

#define	VOLUME_TIME		0
#define	SYSTEM_TIME		1

#define	OBJECT_SELECTABLE		1

#define	ACTIVE_CURSOR		0
#define	INACTIVE_CURSOR		1
#define BUSY_OBJECT		2

#define	FORCE_REDRAW		1
#define	REDRAW_IF_CHANGED	0

#define HCI_INITIAL_AZIMUTH	-99.9

#define MAX_APPNAME_SIZE 128

/*	The following typedef defines the properties for all control	*
 *	panel objects.							*/
 
typedef struct {

	char		app_name[MAX_APPNAME_SIZE];
				/*  Application to spawn if this
				    object is activated
				    Equals spaces if no application
				    is activated 		*/
	Widget		widget; /*  If the object is a widget,	*
			 	 *  then this field should be	*
				 *  non-NULL (contain the	*
				 *  widget ID).			*/
	int		pixel;	/*  Leftmost pixel of the	*
				 *  object in the window.	*/
	int		scanl;	/*  Top scanline of the object	*
				 *  in the window.		*/
	int		width;	/*  Width (pixels) of the	*
				 *  object.			*/
	int		height;	/*  Height (scanlines) of the	*
				 *  object.			*/
	int		fg_color;
				/*  Foreground color of the	*
				 *  object (index).		*/
	int		bg_color;
				/*  Background color of the	*
				 *  object (index).		*/
	int		flags;	/*  Special flags.  Bit 0 is	*
				 *  used to indicate "hot" spot	*
				 *  for mouse selection.	
				 *  Bit 1 is used to indicate whether
				    an object is "busy" or not
				    An object can not be activated while
				    it is busy
				 */
	int		data;	/*  Data the user wants to	*
				 *  attach to the object for	*
				 *  special processing.		*/

} hci_control_panel_object_t;

/*	The following function prototype deals with manipulating	*
 *	control panel objects.						*/

hci_control_panel_object_t	*hci_control_panel_object (
					int object);

/* The following modules return various X Window properties. */

Window		hci_control_panel_window	();
Cursor		hci_control_panel_cursor      (int cid, hci_control_panel_object_t* object);
Pixmap		hci_control_panel_pixmap      ();
GC		hci_control_panel_gc          ();
int		hci_control_panel_3d          ();
int		hci_control_panel_height      ();
int		hci_control_panel_width       ();
int		hci_control_panel_data_height ();
int		hci_control_panel_data_width  ();
int		hci_control_panel_fields      ();
void		hci_control_panel_new_pixmap (int width, int height);

/* The following function erases the HCI Pixmap. */

void	hci_control_panel_erase();

/* The following function draws the HCI onto the Pixmap. */

void	hci_control_panel_draw( int force_draw );

/* The following function makes an object look 3D by shading at
   the top and left sides of the object. */

void	hci_control_panel_draw_3d (int pixel, int scanl,
		int width, int height, int color);

/* The following handle mouse clicks, resizing, exposing events, etc. */

void	hci_control_panel_button_input     (Widget    w,
				     XEvent    *event,
				     String    *args,
				     int       *num_args);

void	hci_control_panel_input     (Widget    w,
				     XEvent    *event,
				     String    *args,
				     int       *num_args);

void	hci_control_panel_expose (Widget w,
		XtPointer client_data, XtPointer call_data);

void	hci_control_panel_resize (Widget w,
		XtPointer client_data, XtPointer call_data);

void    hci_control_panel_force_resize( int flag );

/* The following launches an HCI task. */

void	hci_activate_cmd (Widget w,
		XtPointer client_data, XtPointer call_data);

/* The following are callbacks from buttons on the HCI. */

void	hci_faa_redundant_force_update (Widget w,
		XtPointer client_data, XtPointer call_data);

void	hci_RMS_control_button (Widget w,
		XtPointer client_data, XtPointer call_data);

void	hci_RPG_products_button (Widget w,
		XtPointer client_data, XtPointer call_data);
				     
void	hci_RPG_status_button (Widget w,
		XtPointer client_data, XtPointer call_data);

/* The following is the timer function for the HCI. */

void	hci_timer_proc ();

/* The following draw differenct components of the control panel. */

void	hci_control_panel_set_system_time( int force_draw );
void	hci_control_panel_set_volume_time( int force_draw );
void	hci_control_panel_tower( int force_draw );
void	hci_control_panel_power( int force_draw );
void	hci_control_panel_rda_alarms( int force_draw );
void	hci_control_panel_orda_alarms( int force_draw );
void	hci_control_panel_radome( int force_draw );
void	hci_control_panel_radome_data( int elev_num, int super_res,
                                       float elevation, int n_sails_cuts,
                                       int sails_cut_seq, int last_elev_flag );
void	hci_update_antenna_position();
void	hci_control_panel_elevation_lines( int force_draw );
void	hci_control_panel_vcp( int force_draw );
void	hci_control_panel_mode_select( int force_draw );
void	hci_control_panel_rpg_rda_connection( int force_draw );
void	hci_control_panel_rpg_rms_connection( int force_draw );
void	hci_control_panel_rpg_users_connection( int force_draw );
void	hci_control_panel_MLOS_button( int force_draw );
void	hci_control_panel_RDA_button( int force_draw );
void	hci_control_panel_RPG_button( int force_draw );
void	hci_control_panel_RMS_button( int force_draw );
void	hci_control_panel_USERS_button( int force_draw );
void	hci_control_panel_RPG_buttons_background();
void	hci_control_panel_applications( int force_draw );
void	hci_control_panel_environmental_winds( int force_draw );
void	hci_control_panel_control_status( int force_draw );
void	hci_control_panel_status( int force_draw );
void	hci_control_panel_rda_status( int force_draw );
void	hci_control_panel_rpg_status( int force_draw );
void	hci_control_panel_users_status( int force_draw );
void	hci_control_panel_system_log_messages( int force_draw );
void	hci_control_panel_window_title();
void	hci_display_feedback_string( int force_draw );
void	hci_set_display_feedback_string( char *msg );

/* The following are misc support functions for the HCI. */

void	hci_rms_text_message();
void	hci_rms_inhibit_message();

float	hci_control_panel_azimuth();
float	hci_control_panel_elevation();
int	hci_control_panel_azimuth_num();
int	hci_control_panel_elevation_num();
int	hci_control_panel_super_res();
int	hci_control_panel_moments();
int	hci_control_panel_last_ele_flag();
int	hci_control_panel_n_sails_cuts();
int	hci_control_panel_sails_cut_seq();
float	hci_control_panel_first_azimuth();
void	hci_control_panel_clear_HCI();

void	hci_set_latest_radial_info();

void	get_julian_date_time   (int *julian,
				int *seconds);

int	hci_get_prod_info_update_flag();
void	hci_set_prod_info_update_flag(int flag);

void    hci_get_console_message();
int     hci_get_RDA_message_flag();
void    hci_set_RDA_message_flag(int flag);

int     hci_get_env_wind_update_flag();
void    hci_set_env_wind_update_flag(int flag);

/* Add the following since f(x) calls will take place across different files. */

void	update_rda_control_menu_properties ();
void	rda_config_change_orda( Widget w );
void	rda_config_change_legacy( Widget w );
int     hci_get_system_log_update_flag();
void    hci_set_system_log_update_flag (int flag);
int     hci_get_pd_line_info_update_flag();
void    hci_set_pd_line_info_update_flag (int flag);
int     hci_get_prod_user_status_update_flag();
void    hci_set_prod_user_status_update_flag (int flag);

void	hci_control_panel( int argc, char *argv[] );
void	hci_force_adapt_load( int argc, char *argv[] );
void	hci_force_dev_configure( int argc, char *argv[] );
void	hci_register_callbacks();

void	hci_set_rms_down_flag( int );
int	hci_get_rms_down_flag();
#endif
