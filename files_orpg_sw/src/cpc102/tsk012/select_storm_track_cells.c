/************************************************************************
 *	Module: select_storm_track_cells.c				*
 *	Description: This module is used by the NEXRAD Product Display	*
 *		     Tool (XPDT) to provide the user with a mechanism	*
 *		     to select the range of storm tracks displayed in	*
 *		     the storm track product.				*
 ************************************************************************/

/*
 * RCS info
 * $Author: davep $
 * $Locker:  $
 * $Date: 2001/05/22 18:14:59 $
 * $Id: select_storm_track_cells.c,v 1.3 2001/05/22 18:14:59 davep Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

/*	Local include file definitions.					*/

#include "rle.h"

/*	Motif & X Windows include file definitions.			*/

#include <Xm/Xm.h>
#include <Xm/DialogS.h>
#include <Xm/Form.h>
#include <Xm/Scale.h>

/*	Various X and windows properties.				*/

extern	Widget	top_widget;
extern	Widget	storm_track_dialog;
extern	int	first_storm;	/* First storm to display */
extern	int	last_storm;	/* Last storm to display */
extern	Pixel	white_color;	/* label foreground color */
extern	Pixel	steelblue_color;	/* label background color */

/************************************************************************
 *	Description: This function is used to create and display a	*
 *		     popup window for defining which storms tracks	*
 *		     to display from the Storm Tracking product.	*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
select_storm_track_cells ()
{

	Widget	form;
	Widget	first_storm_widget;
	Widget	last_storm_widget;

	void	first_storm_callback (Widget w,
			XtPointer client_data, XtPointer call_data);
	void	last_storm_callback (Widget w,
			XtPointer client_data, XtPointer call_data);

	storm_track_dialog = XtVaCreatePopupShell ("Storm Track Display Control",
		xmDialogShellWidgetClass,	top_widget,
		XmNdeleteResponse,		XmDESTROY,
		NULL);

	form = XtVaCreateWidget ("storm_track_form",
		xmFormWidgetClass,	storm_track_dialog,
		XmNforeground,		white_color,
		XmNbackground,		steelblue_color,
		NULL);

	first_storm_widget = XtVaCreateManagedWidget ("First Storm",
		xmScaleWidgetClass,	form,
		XtVaTypedArg, XmNtitleString, XmRString, "First Storm", 11,
		XmNmaximum,		100,
		XmNminimum,		  1,
		XmNwidth,		250,
		XmNdecimalPoints,	  0,
		XmNvalue,		(int) first_storm,
		XmNshowValue,		True,
		XmNorientation,		XmHORIZONTAL,
		XmNprocessingDirection,	XmMAX_ON_RIGHT,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNforeground,		white_color,
		XmNbackground,		steelblue_color,
		NULL);

	XtAddCallback (first_storm_widget,
		XmNvalueChangedCallback, first_storm_callback, NULL);

	last_storm_widget = XtVaCreateManagedWidget ("Last Storm",
		xmScaleWidgetClass,	form,
		XtVaTypedArg, XmNtitleString, XmRString, "Last Storm", 10,
		XmNmaximum,		100,
		XmNminimum,		  1,
		XmNwidth,		250,
		XmNdecimalPoints,	  0,
		XmNvalue,		(int) last_storm,
		XmNshowValue,		True,
		XmNorientation,		XmHORIZONTAL,
		XmNprocessingDirection,	XmMAX_ON_RIGHT,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		first_storm_widget,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNbottomAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNforeground,		white_color,
		XmNbackground,		steelblue_color,
		NULL);

	XtAddCallback (last_storm_widget,
		XmNvalueChangedCallback, last_storm_callback, NULL);

	XtManageChild (form);

	XtPopup (storm_track_dialog, XtGrabNone);

}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the first storm slider bar.  This controls the	*
 *		     number of the first storm displayed from the	*
 *		     product.						*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - scale widget data				*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
first_storm_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	void	resize_callback (Widget w,
			XtPointer client_data, XtPointer call_data);

	XmScaleCallbackStruct *cbs =
		(XmScaleCallbackStruct *) call_data;

	first_storm = (int) cbs->value;

	resize_callback ((Widget) NULL,
		(XtPointer) NULL,
		(XtPointer) NULL);
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the last storm slider bar.  This controls the	*
 *		     number of the last storm displayed from the	*
 *		     product.						*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - scale widget data				*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
last_storm_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	void	resize_callback (Widget w,
			XtPointer client_data, XtPointer call_data);

	XmScaleCallbackStruct *cbs =
		(XmScaleCallbackStruct *) call_data;

	last_storm = (int) cbs->value;

	resize_callback ((Widget) NULL,
		(XtPointer) NULL,
		(XtPointer) NULL);
}
