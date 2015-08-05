/*	hci_precip_status.h - This header file defines			*
 *	functions used to access current precip status data.		*/

/*
 * RCCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:26:16 $
 * $Id: hci_precip_status.h,v 1.3 2009/02/27 22:26:16 ccalvert Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

#ifndef HCI_PRECIP_STATUS_DEF
#define	HCI_PRECIP_STATUS_DEF

/*	Include files needed.						*/

#include <hci.h>
#include <precip_status.h>

/*	Macros for various precipitation detection elements.		*/

Precip_status_t	hci_get_precip_status();

#endif
