/************************************************************************
*									*
*    Module: hci_lock.h							*
*									*
*    Description: This is the header file to support security		*
*		 operations in the HCI.					*
*									*
*************************************************************************/

/*
 * RCS info 
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:26:09 $
 * $Id: hci_lock.h,v 1.9 2009/02/27 22:26:09 ccalvert Exp $
 * $Revision: 1.9 $
 * $State: Exp $
 * $Log: hci_lock.h,v $
 * Revision 1.9  2009/02/27 22:26:09  ccalvert
 * consolidate HCI code
 *
 * Revision 1.8  2004/06/16 23:03:21  steves
 * issue 2-447
 *
*/

#ifndef HCI_LOCK_H
#define HCI_LOCK_H

#ifdef __cplusplus
extern "C"
{
#endif

#define	HCI_LOCA_NONE		0x00000000
#define	HCI_LOCA_URC		0x00000001
#define	HCI_LOCA_AGENCY		0x00000002
#define	HCI_LOCA_ROC		0x00000004
#define	HCI_LOCA_NONOP		0x0000000f
#define	HCI_LOCA_FAA_OVERRIDE	0x00000010
#define	HCI_LOCK_CANCEL		1
#define	HCI_LOCK_PROCEED	0

Widget	hci_lock_widget( Widget, int (*)(), int );
int	hci_lock_open();
int	hci_lock_close();
int	hci_lock_loca_selected();
int	hci_lock_loca_unlocked();
int	hci_lock_ROC_selected();
int	hci_lock_URC_selected();
int	hci_lock_AGENCY_selected();
int	hci_lock_ROC_unlocked();
int	hci_lock_URC_unlocked();
int	hci_lock_AGENCY_unlocked();

#ifdef __cplusplus
}
#endif

#endif

