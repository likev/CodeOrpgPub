/*	hci_scan_info.h - This header file defines			*
 *	functions used to access scan info data.			*/

/*
 * RCCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:26:25 $
 * $Id: hci_scan_info.h,v 1.7 2009/02/27 22:26:25 ccalvert Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */

#ifndef HCI_SCAN_INFO_DEF
#define	HCI_SCAN_INFO_DEF

/*	Include files needed.						*/

#include <infr.h>
#include <orpgdat.h>
#include <gen_stat_msg.h>
#include <a309.h>

int 	hci_scan_info_io_status();
short	hci_get_scan_info_key();
int	hci_get_scan_mode_operation();
int	hci_get_scan_vcp_number();
int	hci_get_scan_julian_date();
int	hci_get_scan_time();
int	hci_get_scan_number_elevation_cuts();
int	hci_get_scan_info_flag();
int	hci_get_scan_active_flag();
void	hci_read_scan_info_data();
unsigned long	hci_get_scan_volume_number();
int	hci_get_scan_status();
float	hci_get_scan_elevation_angle( int );
int	hci_get_scan_elevation_number( int );
void	hci_set_scan_info_key( short );
void	hci_set_scan_info_flag( int );
void	hci_set_scan_active_flag( int );

#endif
