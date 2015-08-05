/************************************************************************
 *	Module: display_tabular_data.c					*
 *	Description: This function is used to disply the tabular text	*
 *		     information in an RPG product in the NEXRAD	*
 *		     Product Display Tool (XPDT) display window.	*
 ************************************************************************/

/*
 * RCS info
 * $Author: jeffs $
 * $Locker:  $
 * $Date: 2014/03/18 19:00:43 $
 * $Id: display_tabular_data.c,v 1.7 2014/03/18 19:00:43 jeffs Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */

/*	Local include file definitions.					*/

#include "rle.h"

/*	Motif & X Windows include file definitions.			*/

#include <Xm/List.h>
#include <Xm/DialogS.h>
#include <Xm/Form.h>
#include <Xm/RowColumn.h>
#include <Xm/PushB.h>

static	Widget	ttab_list;	/* widget ID for tabular list */
static	int	current_page = 0; /* current displayed page */

extern	Widget	ttab_dialog; /* shell widget ID for tabular data window */
extern	Widget	draw_widget; /* drawing area widget ID */
extern	XmFontList	fontlist; /* font information for labels */
extern	Pixel	white_color; /* foreground text color */
extern	Pixel	seagreen_color; /* background color */
extern	Pixel	steelblue_color; /* button background color */

/************************************************************************
 *	Description: This function is used to display ptoduct tabular	*
 *		     data in the XPDT display window.			*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void display_tabular_data ()
{
	Widget		form;
	Widget		rowcol;
	Widget		page;
	Arg		args [10];
	XmString	str;
	int		i;

	void	ttab_page_callback (Widget w, XtPointer client_data,
			XtPointer call_data);

	if (ttab_dialog != (Widget) NULL) {

	    XmListDeleteAllItems (ttab_list);

	} else {

	    ttab_dialog = XtVaCreatePopupShell ("Product Tabular Data",
		xmDialogShellWidgetClass,	draw_widget,
		XmNdeleteResponse,		XmDESTROY,
		NULL);

	    form = XtVaCreateWidget ("tabular_form",
		xmFormWidgetClass,	ttab_dialog,
		XmNforeground,		white_color,
		XmNbackground,		steelblue_color,
		NULL);

	    rowcol = XtVaCreateWidget ("tabular_rowcol",
		xmRowColumnWidgetClass,	form,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNorientation,		XmHORIZONTAL,
		XmNforeground,		white_color,
		XmNbackground,		steelblue_color,
		NULL);

	    page = XtVaCreateManagedWidget ("Previous Page",
		xmPushButtonWidgetClass,	rowcol,
		XmNforeground,		white_color,
		XmNbackground,		steelblue_color,
		NULL);

	    XtAddCallback (page,
		XmNactivateCallback, ttab_page_callback, (XtPointer) -1);

	    page = XtVaCreateManagedWidget ("Next Page",
		xmPushButtonWidgetClass,	rowcol,
		XmNforeground,		white_color,
		XmNbackground,		steelblue_color,
		NULL);

	    XtAddCallback (page,
		XmNactivateCallback, ttab_page_callback, (XtPointer) 1);

	    XtManageChild (rowcol);

	    XtSetArg (args [0], XmNvisibleItemCount, ttab->number_of_lines [0]);
	    XtSetArg (args [1], XmNtopWidget,        rowcol);
	    XtSetArg (args [2], XmNtopAttachment,    XmATTACH_WIDGET);
	    XtSetArg (args [3], XmNleftAttachment,   XmATTACH_WIDGET);
	    XtSetArg (args [4], XmNrightAttachment,  XmATTACH_WIDGET);
	    XtSetArg (args [5], XmNbottomAttachment, XmATTACH_WIDGET);
	    XtSetArg (args [6], XmNfontList,	 fontlist);
	    XtSetArg (args [7], XmNforeground,	 white_color);
	    XtSetArg (args [8], XmNbackground,	 seagreen_color);
	    XtSetArg (args [9], XmNwidth, 		 720);

	    ttab_list = XmCreateScrolledList (form,
		"ttab_scrolled_list",
		args,
		10);

	    XtManageChild (ttab_list);

	    XtManageChild (form);

	    XtPopup (ttab_dialog, XtGrabNone);

	}

	current_page = 0;

	for (i=0;i<=ttab->number_of_lines [current_page];i++) {

	    str = XmStringCreate (ttab->text [current_page][i], "ttab_font");

	    XmListAddItemUnselected (ttab_list, str, i+1);

	    XmStringFree (str);

	}
}

/************************************************************************
 *	Description: This function is activated when the used selects	*
 *		     the "Next Page" button in the tabular data popup	*
 *		     window.						*
 *									*
 *	Input:  widget - widget ID					*
 *		client_data - page index				*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
ttab_page_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	int		i;
	XmString	str;

	current_page = current_page + (int) client_data;

	if (current_page < 0) {

	    current_page = 0;
	    return;

	} else if (current_page >= ttab->number_of_pages) {

	    current_page = ttab->number_of_pages-1;
	    return;

	}

	XmListDeleteAllItems (ttab_list);

	for (i=0;i<=ttab->number_of_lines [current_page];i++) {

	    str = XmStringCreate (ttab->text [current_page][i], "ttab_font");

	    XmListAddItemUnselected (ttab_list, str, i+1);

	    XmStringFree (str);

	}

}
