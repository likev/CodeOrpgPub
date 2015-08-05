/*	hci_basedata.h - This header file defines		*
 *	functions used to access basedata data.			*/

/*
 * RCCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:25:48 $
 * $Id: hci_basedata.h,v 1.20 2009/02/27 22:25:48 ccalvert Exp $
 * $Revision: 1.20 $
 * $State: Exp $
 */

#ifndef HCI_BASEDATA_DEF
#define	HCI_BASEDATA_DEF

/*	Include files needed.						*/

#include <hci.h>
#include <basedata.h>

#define	REFLECTIVITY			0
#define	VELOCITY			1
#define	SPECTRUM_WIDTH			2

#define	START_OF_REFLECTIVITY_DATA	(BASEDATA_REF_OFF*sizeof(short))
#define	START_OF_VELOCITY_DATA		(BASEDATA_VEL_OFF*sizeof(short))
#define	START_OF_SPECTRUM_WIDTH_DATA	(BASEDATA_SPW_OFF*sizeof(short))

#define	SIZEOF_BASEDATA			(MAX_GENERIC_BASEDATA_SIZE*sizeof(short))

#define	REFLECTIVITY_MIN		 -32.0
#define	REFLECTIVITY_MAX		  95.0
#define	VELOCITY_MIN			-127.0
#define	VELOCITY_MAX			 126.0
#define	SPECTRUM_WIDTH_MIN		   0.0
#define	SPECTRUM_WIDTH_MAX		  10.3

#define	MAX_BINS_ALONG_RADIAL		MAX_BASEDATA_REF_SIZE
#define	MAX_RADIAL_RANGE		460.0

#define	DOPPLER_RESOLUTION_LOW		4
#define	DOPPLER_RESOLUTION_HIGH		2

#define REFL_QUARTERKM                  250
#define REFL_KM                        1000

#define HCI_BASEDATA_PARTIAL_READ	  1
#define HCI_BASEDATA_COMPLETE_READ	  0

int	hci_basedata_id();
int	hci_basedata_msgid();
int	hci_basedata_seek( int );
int	hci_basedata_read_radial( int, int );
short	*hci_basedata_data( int );
int	hci_basedata_range_adjust( int );
int	hci_basedata_bin_size( int );
int	hci_basedata_number_bins( int );
int	hci_basedata_time();
int	hci_basedata_date();
float	hci_basedata_unambiguous_range();
float	hci_basedata_nyquist_velocity();
float	hci_basedata_elevation();
float	hci_basedata_target_elevation();
int	hci_basedata_azimuth_number();
float	hci_basedata_azimuth();
int	hci_basedata_vcp_number();
int	hci_basedata_msg_type();
int	hci_basedata_elevation_number();
int	hci_basedata_velocity_resolution();
float	hci_basedata_refl_value( int );
float	hci_basedata_width_value( int );
float	hci_basedata_dopl_value( int );
float	hci_basedata_value( int, int );
float	hci_basedata_range( int, int );
float	hci_basedata_value_min( int );
float	hci_basedata_value_max( int );
int	hci_basedata_get_lock_state();
void	hci_basedata_set_lock_state( int );
void	hci_basedata_set_data_feed( int );

#endif
