/*	hci_clutter_bypass_map.h - This header file defines		*
 *	functions used to manipulate clutter bypass map data.		*/

/*
 * RCCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:25:50 $
 * $Id: hci_clutter_bypass_map.h,v 1.5 2009/02/27 22:25:50 ccalvert Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

#ifndef HCI_CLUTTER_BYPASS_MAP_DEF
#define	HCI_CLUTTER_BYPASS_MAP_DEF

/*	Include files needed.						*/

#include <math.h>
#include <infr.h>
#include <orpgdat.h>
#include <clutter.h>

#define	AZINT		(360.0/512)
#define	ORDA_AZINT	(256.0/512)
#define	ONE		1

int	hci_clutter_bypass_map_initialize();
int	hci_get_bypass_map_data( float, float, int );
void	hci_set_bypass_map_data( float, float, int, int );
int	hci_clutter_bypass_map_read( int );
int	hci_clutter_bypass_map_segments();
int	hci_get_bypass_map_time ();
int	hci_get_bypass_map_date ();

#endif
