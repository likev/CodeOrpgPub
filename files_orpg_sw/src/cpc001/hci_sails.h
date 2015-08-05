/*
 * RCCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2014/08/04 16:52:22 $
 * $Id: hci_sails.h,v 1.3 2014/08/04 16:52:22 ccalvert Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

#ifndef HCI_SAILS_H
#define	HCI_SAILS_H

/* Local include files */

/* Enums/Definitions */

#define	HCI_SAILS_STATUS_DISABLED	GS_SAILS_DISABLED
#define	HCI_SAILS_STATUS_ACTIVE		GS_SAILS_ACTIVE
#define	HCI_SAILS_STATUS_INACTIVE	GS_SAILS_INACTIVE

/* Function prototypes */

int  hci_sails_get_status();
int  hci_sails_allowed();
int  hci_sails_get_num_cuts();
int  hci_sails_get_req_num_cuts();
int  hci_sails_get_max_num_cuts();
int  hci_sails_get_site_max_num_cuts();
int  hci_sails_set_num_cuts( int );

#endif
