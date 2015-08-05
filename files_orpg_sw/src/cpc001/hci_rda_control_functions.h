/*
 * RCCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/09/10 20:14:06 $
 * $Id: hci_rda_control_functions.h,v 1.1 2012/09/10 20:14:06 ccalvert Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

#ifndef HCI_RDA_CONTROL_FXS
#define	HCI_RDA_CONTROL_FXS

/* Local include files */

#include <prfselect_buf.h>

/* Definitions */

#define	HCI_PRF_MODE_MANUAL		PRF_COMMAND_MANUAL_PRF
#define	HCI_PRF_MODE_AUTO_ELEVATION	PRF_COMMAND_AUTO_PRF
#define	HCI_PRF_MODE_AUTO_STORM		PRF_COMMAND_STORM_BASED
#define	HCI_PRF_MODE_AUTO_CELL		PRF_COMMAND_CELL_BASED

/* Function prototypes */

void Verify_power_source_change( Widget parent_widget, XtPointer y );
void Verify_super_res_change( Widget parent_widget, XtPointer y );
void Verify_cmd_change( Widget parent_widget, XtPointer y );
void Verify_avset_change( Widget parent_widget, XtPointer y );
int  hci_get_PRF_Mode_state();
int  hci_write_PRF_command();
Prf_status_t hci_get_PRF_Mode_status_msg();
void hci_change_velocity_resolution( int );

#endif
