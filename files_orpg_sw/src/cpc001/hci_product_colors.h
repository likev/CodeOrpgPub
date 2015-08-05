/*	This header file is used by the HCI for setting up product	*
 *	color attributes.						*/

/*
 * RCS info
 * $Author: priegni $
 * $Locker:  $
 * $Date: 1997/12/17 15:38:49 $
 * $Id: hci_product_colors.h,v 1.2 1997/12/17 15:38:49 priegni Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

#ifndef HCI_PRODUCT_COLORS_DEF
#define HCI_PRODUCT_COLORS_DEF

#include <Xm/Xm.h>

#define	PRODUCT_COLORS			16
#define DIGITAL_HYBRID_PRODUCT_COLORS	64

typedef struct {
	short	red   [PRODUCT_COLORS];
	short	green [PRODUCT_COLORS];
	short	blue  [PRODUCT_COLORS];
} hci_product_color_t;
	
typedef struct {
	short	red   [DIGITAL_HYBRID_PRODUCT_COLORS];
	short	green [DIGITAL_HYBRID_PRODUCT_COLORS];
	short	blue  [DIGITAL_HYBRID_PRODUCT_COLORS];
} hci_dhr_product_color_t;

int	hci_initialize_product_colors (Display *display, Colormap cmap);
void	hci_get_product_colors (int pcode, int *pcolor);

#endif
