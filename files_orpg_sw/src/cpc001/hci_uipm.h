/*
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2004/10/18 15:30:21 $
 * $Id: hci_uipm.h,v 1.1 2004/10/18 15:30:21 jing Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */  

#ifndef HCI_UIPM_H
#define HCI_UIPM_H


/*  System includes */
#include <Xm/Xm.h>
#include <X11/Intrinsic.h>

#define UIPM_CAN_CANCEL    1
#define UIPM_NO_CANCEL     2

#ifdef __cplusplus
extern "C"
{
#endif

void UIPM_create(int cancel_type, int simulation_speed, int low_bandwidth, XtAppContext app_context, Widget parent);
void UIPM_set_operation(const char* operation);
void UIPM_destroy();
int UIPM_is_operate ();

#ifdef __cplusplus
}
#endif

#endif
