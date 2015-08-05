/************************************************************************
 *	Module: select_hail_thresholds.c				*
 *	Description: This module is used by the NEXRAD Product Display	*
 *		     Tool (XPDT) to define and popup a dialog window	*
 *		     for selecting the threshold values for different	*
 *		     hail categories for the Hail product.		*
 ************************************************************************/

/*
 * RCS info
 * $Author: davep $
 * $Locker:  $
 * $Date: 2001/05/22 18:14:53 $
 * $Id: select_hail_thresholds.c,v 1.3 2001/05/22 18:14:53 davep Exp $
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
extern	Widget	hail_dialog;
extern	int	hail_threshold1;	/* Lower hail threshold */
extern	int	hail_threshold2;	/* Upper hail threshold */
extern	int	svr_hail_threshold1;	/* Lower severe hail threshold */
extern	int	svr_hail_threshold2;	/* Upper severe hail threshold */
extern	Pixel	white_color;		/* label foreground color */
extern	Pixel	steelblue_color;	/* label background color */

/************************************************************************
 *	Description: This function is used to create and display a	*
 *		     popup shell for defining the threshold ciriteria 	*
 *		     for displaying hail product data.			*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
select_hail_thresholds ()
{

	Widget	form;
	Widget	first_threshold_widget;
	Widget	second_threshold_widget;
	Widget	first_svr_threshold_widget;
	Widget	second_svr_threshold_widget;

	void	first_threshold_callback (Widget w,
			XtPointer client_data, XtPointer call_data);
	void	second_threshold_callback (Widget w,
			XtPointer client_data, XtPointer call_data);
	void	first_svr_threshold_callback (Widget w,
			XtPointer client_data, XtPointer call_data);
	void	second_svr_threshold_callback (Widget w,
			XtPointer client_data, XtPointer call_data);

	hail_dialog = XtVaCreatePopupShell ("Hail Display Control",
		xmDialogShellWidgetClass,	top_widget,
		XmNdeleteResponse,		XmDESTROY,
		NULL);

	form = XtVaCreateWidget ("hail_form",
		xmFormWidgetClass,	hail_dialog,
		XmNforeground,		white_color,
		XmNbackground,		steelblue_color,
		NULL);

	first_threshold_widget = XtVaCreateManagedWidget ("First Threshold",
		xmScaleWidgetClass,	form,
		XtVaTypedArg, XmNtitleString, XmRString, "Low Threshold", 13,
		XmNmaximum,		100,
		XmNminimum,		  1,
		XmNwidth,		250,
		XmNdecimalPoints,	  0,
		XmNvalue,		(int) hail_threshold1,
		XmNshowValue,		True,
		XmNorientation,		XmHORIZONTAL,
		XmNprocessingDirection,	XmMAX_ON_RIGHT,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNforeground,		white_color,
		XmNbackground,		steelblue_color,
		NULL);

	XtAddCallback (first_threshold_widget,
		XmNvalueChangedCallback, first_threshold_callback, NULL);

	second_threshold_widget = XtVaCreateManagedWidget ("Second Threshold",
		xmScaleWidgetClass,	form,
		XtVaTypedArg, XmNtitleString, XmRString, "High Threshold", 14,
		XmNmaximum,		100,
		XmNminimum,		  1,
		XmNwidth,		250,
		XmNdecimalPoints,	  0,
		XmNvalue,		(int) hail_threshold2,
		XmNshowValue,		True,
		XmNorientation,		XmHORIZONTAL,
		XmNprocessingDirection,	XmMAX_ON_RIGHT,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		first_threshold_widget,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNforeground,		white_color,
		XmNbackground,		steelblue_color,
		NULL);

	XtAddCallback (second_threshold_widget,
		XmNvalueChangedCallback, second_threshold_callback, NULL);

	first_svr_threshold_widget = XtVaCreateManagedWidget ("First SVR Threshold",
		xmScaleWidgetClass,	form,
		XtVaTypedArg, XmNtitleString, XmRString, "Low SVR Threshold", 17,
		XmNmaximum,		100,
		XmNminimum,		  1,
		XmNwidth,		250,
		XmNdecimalPoints,	  0,
		XmNvalue,		(int) svr_hail_threshold1,
		XmNshowValue,		True,
		XmNorientation,		XmHORIZONTAL,
		XmNprocessingDirection,	XmMAX_ON_RIGHT,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		second_threshold_widget,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNforeground,		white_color,
		XmNbackground,		steelblue_color,
		NULL);

	XtAddCallback (first_svr_threshold_widget,
		XmNvalueChangedCallback, first_svr_threshold_callback, NULL);

	second_svr_threshold_widget = XtVaCreateManagedWidget ("Second SVR Threshold",
		xmScaleWidgetClass,	form,
		XtVaTypedArg, XmNtitleString, XmRString, "High SVR Threshold", 18,
		XmNmaximum,		100,
		XmNminimum,		  1,
		XmNwidth,		250,
		XmNdecimalPoints,	  0,
		XmNvalue,		(int) svr_hail_threshold2,
		XmNshowValue,		True,
		XmNorientation,		XmHORIZONTAL,
		XmNprocessingDirection,	XmMAX_ON_RIGHT,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		first_svr_threshold_widget,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNbottomAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNforeground,		white_color,
		XmNbackground,		steelblue_color,
		NULL);

	XtAddCallback (second_svr_threshold_widget,
		XmNvalueChangedCallback, second_svr_threshold_callback, NULL);

	XtManageChild (form);

	XtPopup (hail_dialog, XtGrabNone);

}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     changes the lower hail threshold value.		*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - scale widget data				*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
first_threshold_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	void	resize_callback (Widget w,
			XtPointer client_data, XtPointer call_data);

	XmScaleCallbackStruct *cbs =
		(XmScaleCallbackStruct *) call_data;

	hail_threshold1 = (int) cbs->value;

	resize_callback ((Widget) NULL,
		(XtPointer) NULL,
		(XtPointer) NULL);
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     changes the upper hail threshold value.		*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - scale widget data				*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
second_threshold_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	void	resize_callback (Widget w,
			XtPointer client_data, XtPointer call_data);

	XmScaleCallbackStruct *cbs =
		(XmScaleCallbackStruct *) call_data;

	hail_threshold2 = (int) cbs->value;

	resize_callback ((Widget) NULL,
		(XtPointer) NULL,
		(XtPointer) NULL);
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     changes the lower severe hail threshold value.	*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - scale widget data				*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
first_svr_threshold_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	void	resize_callback (Widget w,
			XtPointer client_data, XtPointer call_data);

	XmScaleCallbackStruct *cbs =
		(XmScaleCallbackStruct *) call_data;

	svr_hail_threshold1 = (int) cbs->value;

	resize_callback ((Widget) NULL,
		(XtPointer) NULL,
		(XtPointer) NULL);
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     changes the upper severe hail threshold value.	*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - scale widget data				*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
second_svr_threshold_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	void	resize_callback (Widget w,
			XtPointer client_data, XtPointer call_data);

	XmScaleCallbackStruct *cbs =
		(XmScaleCallbackStruct *) call_data;

	svr_hail_threshold2 = (int) cbs->value;

	resize_callback ((Widget) NULL,
		(XtPointer) NULL,
		(XtPointer) NULL);
}
