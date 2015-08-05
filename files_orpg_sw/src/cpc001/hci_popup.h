/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:05:10 $
 * $Id: hci_popup.h,v 1.1 2009/02/27 22:05:10 ccalvert Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */  

#ifndef HCI_POPUP_H
#define HCI_POPUP_H

#include <Xm/DialogS.h>

Widget hci_custom_popup( Widget, char *, char *, char *, void (*)(),
                         char *, void (*)(), char *, void (*)(), int, int ); 
void hci_info_popup( Widget, char *, void (*)() );
void hci_warning_popup( Widget, char *, void (*)() );
void hci_error_popup( Widget, char *, void (*)() );
void hci_confirm_popup( Widget, char *, void (*)(), void (*)() );
void hci_custom_confirm_popup( Widget, char *, char *, void (*)(), char *, void (*)() );
void hci_rda_config_change_popup();

#endif
