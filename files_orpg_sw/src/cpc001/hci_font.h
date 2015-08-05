/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/09/21 16:53:01 $
 * $Id: hci_font.h,v 1.12 2009/09/21 16:53:01 ccalvert Exp $
 * $Revision: 1.12 $
 * $State: Exp $
*/

/*	hci_font.h - This header file defines a standard set	*
 *	of fonts to use in hci displays.			*/

#ifndef HCI_FONT_DEF
#define	HCI_FONT_DEF

#include <Xm/Xm.h>

/*	Font index variables					*/

/*  C++ extern statement for C++ compiles */
#ifdef __cplusplus
extern "C"
{
#endif

enum {LIST, SMALL, MEDIUM, LARGE, EXTRA_LARGE, EXTRA_EXTRA_LARGE, SCALED};

enum {HCI_FONT_SIZE, HCI_FONT_POINT};

#define	HCI_DEFAULT_FONT_POINT	100
#define	HCI_DEFAULT_FONT_SIZE	100
#define	HCI_MIN_FONT_POINT	 80
#define	HCI_MIN_FONT_SIZE	 80
#define	HCI_MAX_FONT_POINT	100
#define	HCI_MAX_FONT_SIZE	100

void		hci_initialize_fonts (Display *display);
XFontStruct	*hci_get_fontinfo (int font);
XFontStruct	*hci_get_fontinfo_adj (int font, float adjustment);
void		hci_free_fontinfo_adj (XFontStruct *str);
XmFontList	hci_get_fontlist (int font);
Font		hci_get_font (int font);
Font		hci_get_font_adj (int font, float adjustment);
void		hci_set_font (int font, int size);

#ifdef __cplusplus
}
#endif

#endif
