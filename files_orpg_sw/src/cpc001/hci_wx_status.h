/*	hci_wx_status.h - This header file defines			*
 *	functions used to access current wx status data.		*/

/*
 * RCCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:26:28 $
 * $Id: hci_wx_status.h,v 1.7 2009/02/27 22:26:28 ccalvert Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */

#ifndef HCI_WX_STATUS_DEF
#define	HCI_WX_STATUS_DEF

/*	Include files needed.						*/

#include <hci.h>
#include <gen_stat_msg.h>

#define DEA_AUTO_SWITCH_NODE    "alg.mode_select"
#define DEA_AUTO_SWITCH_MODE_A  "alg.mode_select.auto_mode_A"
#define DEA_AUTO_SWITCH_MODE_B  "alg.mode_select.auto_mode_B"
#define	AUTO_SWITCH	1
#define	MANUAL_SWITCH	0
#define	MODE_A		2
#define	MODE_B		1	

/*	Macros for various precipitation detection elements.		*/

Wx_status_t	hci_get_wx_status();
int		hci_get_mode_A_auto_switch_flag();
int		hci_set_mode_A_auto_switch_flag(int flag);
int		hci_get_mode_B_auto_switch_flag();
int		hci_set_mode_B_auto_switch_flag(int flag);

#endif
