/*	hci_rda_adaptation_data.h - This header file defines		*
 *	functions used to access rda adaptation data.			*/

/*
 * RCCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2006/04/13 21:50:22 $
 * $Id: hci_rpg_adaptation_data.h,v 1.1 2006/04/13 21:50:22 steves Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

#ifndef HCI_RPG_ADAPTATION_DATA_DEF
#define	HCI_RPG_ADAPTATION_DATA_DEF

/*  	RPG adaptation data rounding functions */
int hci_rpg_adapt_str_accuracy_round (char *v, double accu, double *tv);
int hci_rpg_adapt_accuracy_round (double v, double accu, double *tv);

#endif
