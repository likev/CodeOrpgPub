/************************************************************************
 *	hci_prf_product.h - This header file defines the		*
 *	functions used to manipulate prf obscuration product data.	*
 ************************************************************************/
/*
 * RCCS info
 * $Author: davep $
 * $Locker:  $
 * $Date: 2002/06/12 22:00:20 $
 * $Id: hci_prf_product.h,v 1.3 2002/06/12 22:00:20 davep Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

#ifndef HCI_PRF_PRODUCT_DEF
#define	HCI_PRF_PRODUCT_DEF

/*	Include files needed.						*/

#include "a309.h"
#include "rdacnt.h"
#include "orpg.h"
#include "orpgda.h"
#include "prfbmap.h"
#include "prod_gen_msg.h"
#include "hci_le.h"
#include "rss_replace.h"

int 	hci_prf_io_status();
int	hci_load_prf_product (int date, int time, float elevation);
int	hci_prf_get_data (int prf, int beam, int bin);
int	hci_prf_get_index (int prf);
int	hci_prf_get_date ();
int	hci_prf_get_time ();
float	hci_prf_get_elevation ();

#endif
