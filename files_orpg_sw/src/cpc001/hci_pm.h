/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:05:10 $
 * $Id: hci_pm.h,v 1.1 2009/02/27 22:05:10 ccalvert Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */  

#ifndef HCI_PM_H
#define HCI_PM_H


/*  System includes */
#include <Xm/Xm.h>
#include <X11/Intrinsic.h>

#define HCI_PM_CAN_CANCEL    1
#define HCI_PM_NO_CANCEL     2

#ifdef __cplusplus
extern "C"
{
#endif

void HCI_PM( const char* );
void HCI_PM_hide();

#ifdef __cplusplus
}
#endif

#endif
