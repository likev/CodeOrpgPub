/************************************************************************
 *									*
 *	Module:  hci_define_bitmaps.c					*
 *									*
 *	Description:  This module is used to set up the bitmap images	*
 *		      for the HCI GUI.					*
 *									*
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/09/10 20:48:03 $
 * $Id: hci_define_bitmaps.c,v 1.20 2012/09/10 20:48:03 ccalvert Exp $
 * $Revision: 1.20 $
 * $State: Exp $
 */

/*	Local include file definitions.					*/

#include <hci_control_panel.h>
#include <alert_threshold_icon.h>
#include <basedata_icon.h>
#include <blockage_icon.h>
#include <clutter_icon.h>
#include <clutter_map_icon.h>
#include <console_message_icon.h>
#include <environmental_winds_icon.h>
#include <misc_icon.h>
#include <prf_control_icon.h>
#include <rda_performance_icon.h>

/************************************************************************
 *	Description: This function defines all of the bitmaps used for	*
 *		     drawn buttons in the RPG Control/Status window.	*
 *		     							*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_define_bitmaps (
)
{
	static	XImage	alert_threshold_ximage;
	static	XImage	basedata_ximage;
	static	XImage	blockage_ximage;
	static	XImage	clutter_ximage;
	static	XImage	clutter_map_ximage;
	static	XImage	console_message_ximage;
	static	XImage	environmental_winds_ximage;
	static	XImage	misc_ximage;
	static	XImage	prf_control_ximage;
	static	XImage	rda_performance_ximage;
	hci_control_panel_object_t	*button;

/*	All of the bitmaps are assumed to e 48x48.  In any event,	*
 *	they all should be a multiple of 8.				*/

/*	Define the pushbutton to open the alert/threshold edit window	*
 *	as an icon in the applications region of the main window.	*/

	alert_threshold_ximage.width            = alert_threshold_icon_width;
	alert_threshold_ximage.height           = alert_threshold_icon_height;
	alert_threshold_ximage.data             = (char *) alert_threshold_icon_bits;
	alert_threshold_ximage.xoffset          = 0;
	alert_threshold_ximage.format           = XYBitmap;
	alert_threshold_ximage.byte_order       = MSBFirst;
	alert_threshold_ximage.bitmap_pad       = 8;
	alert_threshold_ximage.bitmap_bit_order = LSBFirst;
	alert_threshold_ximage.bitmap_unit      = 8;
	alert_threshold_ximage.depth            = 1;
	alert_threshold_ximage.bytes_per_line   = alert_threshold_icon_width/8;
	alert_threshold_ximage.obdata           = NULL;

	XmInstallImage (&alert_threshold_ximage, "alert_threshold_icon");

	button = hci_control_panel_object (ALERTS_BUTTON);

	button->pixel    = 0;
	button->scanl    = 0;
	button->width    = alert_threshold_icon_width;
	button->height   = alert_threshold_icon_height;
	button->fg_color = ICON_FOREGROUND;
	button->bg_color = ICON_BACKGROUND;

/*	Define the pushbutton to open the basedata window as an icon	*
 *	in the applications region of the main window.			*/

	basedata_ximage.width            = basedata_icon_width;
	basedata_ximage.height           = basedata_icon_height;
	basedata_ximage.data             = (char *) basedata_icon_bits;
	basedata_ximage.xoffset          = 0;
	basedata_ximage.format           = XYBitmap;
	basedata_ximage.byte_order       = MSBFirst;
	basedata_ximage.bitmap_pad       = 8;
	basedata_ximage.bitmap_bit_order = LSBFirst;
	basedata_ximage.bitmap_unit      = 8;
	basedata_ximage.depth            = 1;
	basedata_ximage.bytes_per_line   = basedata_icon_width/8;
	basedata_ximage.obdata           = NULL;

	XmInstallImage (&basedata_ximage, "basedata_icon");

	button = hci_control_panel_object (BASEDATA_BUTTON);

	button->pixel    = 0;
	button->scanl    = 0;
	button->width    = basedata_icon_width;
	button->height   = basedata_icon_height;
	button->fg_color = ICON_FOREGROUND;
	button->bg_color = ICON_BACKGROUND;

/*	Define the pushbutton to open the hci blockage gui as	*
 *	an icon in the applications region of the main window.	*/

	blockage_ximage.width            = blockage_icon_width;
	blockage_ximage.height           = blockage_icon_height;
	blockage_ximage.data             = (char *) blockage_icon_bits;
	blockage_ximage.xoffset          = 0;
	blockage_ximage.format           = XYBitmap;
	blockage_ximage.byte_order       = MSBFirst;
	blockage_ximage.bitmap_pad       = 8;
	blockage_ximage.bitmap_bit_order = LSBFirst;
	blockage_ximage.bitmap_unit      = 8;
	blockage_ximage.depth            = 1;
	blockage_ximage.bytes_per_line   = blockage_icon_width/8;
	blockage_ximage.obdata = NULL;

	XmInstallImage (&blockage_ximage, "blockage_icon");

	button = hci_control_panel_object (MISC_BUTTON);

	button->pixel    = 0;
	button->scanl    = 0;
	button->width    = blockage_icon_width;
	button->height   = blockage_icon_height;
	button->fg_color = ICON_FOREGROUND;
	button->bg_color = ICON_BACKGROUND;

/*	Define the pushbutton to open the clutter regions editor as an	*
 *	icon in the applications region of the main window.		*/

	clutter_ximage.width            = clutter_icon_width;
	clutter_ximage.height           = clutter_icon_height;
	clutter_ximage.data             = (char *) clutter_icon_bits;
	clutter_ximage.xoffset          = 0;
	clutter_ximage.format           = XYBitmap;
	clutter_ximage.byte_order       = MSBFirst;
	clutter_ximage.bitmap_pad       = 8;
	clutter_ximage.bitmap_bit_order = LSBFirst;
	clutter_ximage.bitmap_unit      = 8;
	clutter_ximage.depth            = 1;
	clutter_ximage.bytes_per_line   = clutter_icon_width/8;
	clutter_ximage.obdata           = NULL;

	XmInstallImage (&clutter_ximage, "clutter_icon");

	button = hci_control_panel_object (CENSOR_ZONES_BUTTON);

	button->pixel    = 0;
	button->scanl    = 0;
	button->width    = clutter_icon_width;
	button->height   = clutter_icon_height;
	button->fg_color = ICON_FOREGROUND;
	button->bg_color = ICON_BACKGROUND;

/*	Define the pushbutton to open the environmental winds editor	*
 *	as an icon in the applications region of the main window.	*/

	environmental_winds_ximage.width            = environmental_winds_icon_width;
	environmental_winds_ximage.height           = environmental_winds_icon_height;
	environmental_winds_ximage.data             = (char *) environmental_winds_icon_bits;
	environmental_winds_ximage.xoffset          = 0;
	environmental_winds_ximage.format           = XYBitmap;
	environmental_winds_ximage.byte_order       = MSBFirst;
	environmental_winds_ximage.bitmap_pad       = 8;
	environmental_winds_ximage.bitmap_bit_order = LSBFirst;
	environmental_winds_ximage.bitmap_unit      = 8;
	environmental_winds_ximage.depth            = 1;
	environmental_winds_ximage.bytes_per_line   = environmental_winds_icon_width/8;
	environmental_winds_ximage.obdata = NULL;

	XmInstallImage (&environmental_winds_ximage, "environmental_winds_icon");

	button = hci_control_panel_object (ENVIRONMENTAL_WINDS_BUTTON);

	button->pixel    = 0;
	button->scanl    = 0;
	button->width    = environmental_winds_icon_width;
	button->height   = environmental_winds_icon_height;
	button->fg_color = ICON_FOREGROUND;
	button->bg_color = ICON_BACKGROUND;

/*	Define the pushbutton to open the hci misc gui as an	*
 *	icon in the applications region of the main window.	*/

	misc_ximage.width            = misc_icon_width;
	misc_ximage.height           = misc_icon_height;
	misc_ximage.data             = (char *) misc_icon_bits;
	misc_ximage.xoffset          = 0;
	misc_ximage.format           = XYBitmap;
	misc_ximage.byte_order       = MSBFirst;
	misc_ximage.bitmap_pad       = 8;
	misc_ximage.bitmap_bit_order = LSBFirst;
	misc_ximage.bitmap_unit      = 8;
	misc_ximage.depth            = 1;
	misc_ximage.bytes_per_line   = misc_icon_width/8;
	misc_ximage.obdata = NULL;

	XmInstallImage (&misc_ximage, "misc_icon");

	button = hci_control_panel_object (MISC_BUTTON);

	button->pixel    = 0;
	button->scanl    = 0;
	button->width    = misc_icon_width;
	button->height   = misc_icon_height;
	button->fg_color = ICON_FOREGROUND;
	button->bg_color = ICON_BACKGROUND;

/*	Define the pushbutton to open the clutter bypass map display 	*
 *	as an icon in the applications region of the main window.	*/

	clutter_map_ximage.width            = clutter_map_icon_width;
	clutter_map_ximage.height           = clutter_map_icon_height;
	clutter_map_ximage.data             = (char *) clutter_map_icon_bits;
	clutter_map_ximage.xoffset          = 0;
	clutter_map_ximage.format           = XYBitmap;
	clutter_map_ximage.byte_order       = MSBFirst;
	clutter_map_ximage.bitmap_pad       = 8;
	clutter_map_ximage.bitmap_bit_order = LSBFirst;
	clutter_map_ximage.bitmap_unit      = 8;
	clutter_map_ximage.depth            = 1;
	clutter_map_ximage.bytes_per_line   = clutter_map_icon_width/8;
	clutter_map_ximage.obdata           = NULL;

	XmInstallImage (&clutter_map_ximage, "clutter_map_icon");

	button = hci_control_panel_object (BYPASS_MAP_BUTTON);

	button->pixel    = 0;
	button->scanl    = 0;
	button->width    = clutter_map_icon_width;
	button->height   = clutter_map_icon_height;
	button->fg_color = ICON_FOREGROUND;
	button->bg_color = ICON_BACKGROUND;

/*	Define the pushbutton to open the rda performance data menu as 	*
 *	an icon in the applications region of the main window.		*/

	rda_performance_ximage.width            = rda_performance_icon_width;
	rda_performance_ximage.height           = rda_performance_icon_height;
	rda_performance_ximage.data             = (char *) rda_performance_icon_bits;
	rda_performance_ximage.xoffset          = 0;
	rda_performance_ximage.format           = XYBitmap;
	rda_performance_ximage.byte_order       = MSBFirst;
	rda_performance_ximage.bitmap_pad       = 8;
	rda_performance_ximage.bitmap_bit_order = LSBFirst;
	rda_performance_ximage.bitmap_unit      = 8;
	rda_performance_ximage.depth            = 1;
	rda_performance_ximage.bytes_per_line   = rda_performance_icon_width/8;
	rda_performance_ximage.obdata           = NULL;

	XmInstallImage (&rda_performance_ximage, "rda_performance_icon");

	button = hci_control_panel_object (RDA_PERFORMANCE_BUTTON);

	button->pixel    = 0;
	button->scanl    = 0;
	button->width    = rda_performance_icon_width;
	button->height   = rda_performance_icon_height;
	button->fg_color = ICON_FOREGROUND;
	button->bg_color = ICON_BACKGROUND;

/*	Define the pushbutton to open the rda console message window	*
 *	as an icon in the applications region of the main window.	*/

	console_message_ximage.width            = console_message_icon_width;
	console_message_ximage.height           = console_message_icon_height;
	console_message_ximage.data             = (char *) console_message_icon_bits;
	console_message_ximage.xoffset          = 0;
	console_message_ximage.format           = XYBitmap;
	console_message_ximage.byte_order       = MSBFirst;
	console_message_ximage.bitmap_pad       = 8;
	console_message_ximage.bitmap_bit_order = LSBFirst;
	console_message_ximage.bitmap_unit      = 8;
	console_message_ximage.depth            = 1;
	console_message_ximage.bytes_per_line   = console_message_icon_width/8;
	console_message_ximage.obdata           = NULL;

	XmInstallImage (&console_message_ximage, "console_message_icon");

	button = hci_control_panel_object (CONSOLE_MESSAGE_BUTTON);

	button->pixel    = 0;
	button->scanl    = 0;
	button->width    = console_message_icon_width;
	button->height   = console_message_icon_height;
	button->fg_color = ICON_FOREGROUND;
	button->bg_color = ICON_BACKGROUND;

/*	Define the pushbutton to open the prf control window		*
 *	as an icon in the applications region of the main window.	*/

	prf_control_ximage.width            = prf_control_icon_width;
	prf_control_ximage.height           = prf_control_icon_height;
	prf_control_ximage.data             = (char *) prf_control_icon_bits;
	prf_control_ximage.xoffset          = 0;
	prf_control_ximage.format           = XYBitmap;
	prf_control_ximage.byte_order       = MSBFirst;
	prf_control_ximage.bitmap_pad       = 8;
	prf_control_ximage.bitmap_bit_order = LSBFirst;
	prf_control_ximage.bitmap_unit      = 8;
	prf_control_ximage.depth            = 1;
	prf_control_ximage.bytes_per_line   = prf_control_icon_width/8;
	prf_control_ximage.obdata           = NULL;

	XmInstallImage (&prf_control_ximage, "prf_control_icon");

	button = hci_control_panel_object (PRF_CONTROL_BUTTON);

	button->pixel    = 0;
	button->scanl    = 0;
	button->width    = prf_control_icon_width;
	button->height   = prf_control_icon_height;
	button->fg_color = ICON_FOREGROUND;
	button->bg_color = ICON_BACKGROUND;

}
