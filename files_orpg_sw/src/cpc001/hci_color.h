/************************************************************************
 *									*
 *	Module: hci_colors.h - This header file defines a standard set	*
 *	of read only colors to use in hci displays.			*
 *									*
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:25:53 $
 * $ID: $
 * $Revision: 1.13 $
 * $State: Exp $
 */

#ifndef HCI_COLORS_DEF
#define	HCI_COLORS_DEF

/*	Motif & X window include file definitions.			*/

#include <Xm/Xm.h>

/*	Color variables							*/

/**     NOTE: Currently, the fixed color portion of this enumerated type
	matches the enumerated type in "uipalette.h".  If this enumerated
	type chages, the enumerated type in uipalette.h must change also  */

enum {
      BLACK=0,
      CYAN, STEELBLUE, BLUE,
      LIGHTGREEN, GREEN, GREEN1, GREEN2, GREEN3, GREEN4, SEAGREEN, DARKGREEN,
      YELLOW, GOLD, ORANGE,
      RED, RED1, RED2, RED3, RED4, INDIANRED1, PINK,
      LIGHTGRAY, GRAY, DARKGRAY,
      MAGENTA, MAGENTA3,
      WHITE,
      PURPLE, /* Really PURPLE3 */
      PEACHPUFF3,
      BROWN,
      LIGHTSTEELBLUE,
      DARKSEAGREEN,
      BACKGROUND_COLOR1, BACKGROUND_COLOR2,
      BUTTON_BACKGROUND, BUTTON_FOREGROUND,
      ICON_BACKGROUND,   ICON_FOREGROUND,
      EDIT_BACKGROUND,   EDIT_FOREGROUND,
      TEXT_FOREGROUND,   TEXT_BACKGROUND,
      LOCA_FOREGROUND,    LOCA_BACKGROUND,
      NORMAL_COLOR,      WARNING_COLOR,
      ALARM_COLOR1,      ALARM_COLOR2,
      PRODUCT_BACKGROUND_COLOR,
      PRODUCT_FOREGROUND_COLOR
   };
#ifdef __cplusplus
extern "C"
{
#endif

int	hci_initialize_read_colors( Display *, Colormap );
int	hci_initialize_read_colors_r( Display*, Colormap, int, Pixel* );
int	hci_get_color_index( int );
int	hci_find_best_color( Display *, Colormap, XColor * );
Pixel	hci_get_read_color( int );

#ifdef __cplusplus
}
#endif

/*   Macro for C++ clients that use the UIPalette class and hci color routines */
#define HCI_PALETTE(color) ((UIPalette::Color)hci_get_color_index(color))

#endif
