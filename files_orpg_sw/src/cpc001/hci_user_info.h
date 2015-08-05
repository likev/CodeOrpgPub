/************************************************************************
 *	hci_userhci_user_info.h - This header file functions used	*
 *	to access user information data.				*
 ************************************************************************/

/*
 * RCCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:26:26 $
 * $Id: hci_user_info.h,v 1.5 2009/02/27 22:26:26 ccalvert Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

#ifndef HCI_USER_INFO_DEF
#define	HCI_USER_INFO_DEF

/*	Include files needed.						*/

void *hci_get_next_user_rec ();
int hci_search_users (char *sql_text);

#endif
