/****************************************************************
 *								*
 *	hci_nb_funcs.c - shared hci_nb gui routines.		*
 *								*
 ****************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:26:11 $
 * $Id: hci_nb_funcs.c,v 1.19 2009/02/27 22:26:11 ccalvert Exp $
 * $Revision: 1.19 $
 * $State: Exp $
 */

/*	Local include file definitions.					*/

#include <hci.h>
#include <hci_nb_def.h>

static Widget Cr_rc;
static int Label_len = 0; /* Defines label length */
static int Editable = 0;  /* Defines widget editability */

static char Text_org[HCI_BUF_128]; /* text widget previous label state */
static Widget Current_w = NULL;
static int Text_verified = 1; /* flag to indicate text entry completed */
static int Text_password = 0; /* flag indicating text field is a password */
static int Text_start_offset = 0; /* start position of text data */
static int Text_insert_len = 0;   /* length of text data */

static void Text_verify_callback( Widget, XtPointer, XtPointer );
static void Add_space_in_front_of_string( char *, char * );
void hci_text_callback( Widget, char *(*)() );


/**********************************************************************

    Description: Sets global variable used by hci_create_label_text,  
		 hci_crete_label_toggle, and hci_create_label_combo.

    Input:  label_len - global label length (characters)
    Output: NONE
    Return: NONE

***********************************************************************/

void hci_set_label_len (int label_len)
{
    Label_len = label_len;
}

/**********************************************************************

    Description: Sets global variable used by hci_create_label_text.

    Input:  editable - if 0, widget not editable; if 1, widget editable
    Output: NONE
    Return: NONE

***********************************************************************/

void hci_set_editable (int editable)
{
    Editable = editable;
}

/**********************************************************************

    Description: Returns additional info from hci_create_label_text.

    Input:  NONE
    Output: NONE
    Return: Widget ID of current label/text rowcolumn parent

***********************************************************************/

Widget hci_get_current_rc ()
{
    return (Cr_rc);
}

/**********************************************************************

    Description: Function used for additional control of the widget 
		 created by hci_create_label_text for processing password
		 type text widget.

    Input:  insert_len - text insert length
    Output: NONE
    Return: text start position

***********************************************************************/

int hci_text_password (int *insert_len)
{
    Text_password = 1;
    *insert_len = Text_insert_len;
    return (Text_start_offset);
}

/**********************************************************************

    Description: Creates a custom rowcolumn widget.

    Input:  parent - widget parent of rowcolumn widget
	    name   - internal name of rowcolumn widget
	    orientation - XmHORIZONTAL or XmVERTICAL
    Output: NONE
    Return: rowcolumn widget ID

***********************************************************************/

Widget hci_create_rowcolumn (Widget parent, char *name, int orientation)
{
    Widget l;

    l = XtVaCreateWidget (name,
		xmRowColumnWidgetClass,	parent,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,		orientation,
		XmNpacking,		XmPACK_TIGHT,
		NULL);
    return (l);
}

/**********************************************************************

    Description: Creates a custom label widget.

    Input: parent - widget ID of label parent
	   label  - text label for label widget
	   align  - XmALIGNMENT_BEGINNING, XmALIGNMENT_CENTER,
		    XmALIGNMENT_END
    Output: NONE
    Return: label widget ID

***********************************************************************/

Widget hci_create_label (Widget parent, char *label, int align)
{
    Widget l;

    l = XtVaCreateManagedWidget (label,
		xmLabelWidgetClass,	parent,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNalignment,		align,
		NULL);
    return (l);
}

/**********************************************************************

    Description: Creates a custom frame_label widget.

    Input:  parent - parent widget for new frame widget
	    label  - frame title string
    Output: NONE
    Return: frame widget ID

***********************************************************************/

Widget hci_create_frame_label (Widget parent, char *label)
{
    Widget l;

    l = XtVaCreateManagedWidget (label,
		xmLabelWidgetClass,	parent,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);
    return (l);
}

/**********************************************************************

    Description: Creates a custom button widget.

    Input:  parent - parent widget ID
	    label  - button label string
	    callback - user callback when button selected
    Output: NONE
    Return: pushbutton widget ID

***********************************************************************/

Widget hci_create_button (Widget parent, 
			char *label, XtCallbackProc callback)
{
    Widget button;

    button = XtVaCreateManagedWidget (label,
		xmPushButtonWidgetClass,	parent,
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNalignment,		XmALIGNMENT_CENTER,
		XmNmarginHeight,	2,
		NULL);

    XtAddCallback (button,
		XmNactivateCallback, callback, NULL);
    return (button);
}

/**********************************************************************

    Description: Creates a custom radio_box widget.

    Input:  parent - Widget ID of parent
	    name   - name for radio button group
	    n      - number of radio buttons
	    ind_set - button which is set by default
	    label  - button label strings
	    callback - user callback when radio button set/unset
    Output: NONE
    Return: Widget ID of radio button parent

***********************************************************************/

Widget hci_create_radio_box (Widget parent, char *name,
		int n, int ind_set, char **label, XtCallbackProc callback)
{
    int na, i;
    Arg args[16];
    Widget rb, button;

    na = 0;
    XtSetArg (args[na], XmNforeground, hci_get_read_color (TEXT_FOREGROUND));  na++;
    XtSetArg (args[na], XmNbackground, hci_get_read_color (BACKGROUND_COLOR1));  na++;
    XtSetArg (args[na], XmNorientation, XmHORIZONTAL);          na++;
    XtSetArg (args[na], XmNfontList, hci_get_fontlist (LIST));  na++;

    rb = XmCreateRadioBox (parent, name, args, na);
    for (i = 0; i < n; i++) {
	int set;

	if (i == ind_set)
	    set = True;
	else
	    set = False;
	button = XtVaCreateManagedWidget (label[i],
		xmToggleButtonWidgetClass, rb,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNselectColor,		hci_get_read_color (WHITE),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNalignment,		XmALIGNMENT_CENTER,
		XmNset,			set,
		NULL);

	XtAddCallback (button,
		XmNvalueChangedCallback, callback, (XtPointer) i);
    }

    XtManageChild (rb);
    return (rb);
}

/**********************************************************************

    Description: Creates a custom label_toggle widget.

    Input:  parent - widget ID of parent
	    label  - toggle button label string
	    callback - user callback when toggle state changes
    Output: NONE
    Return: Widget ID of toggle button

***********************************************************************/

Widget hci_create_label_toggle (Widget parent, char *label, 
			void (*callback) ())
{
    Widget rc, toggle, l;
    int background_color;
    char buf[128];

    rc = hci_create_rowcolumn (parent, "", XmHORIZONTAL);

    Add_space_in_front_of_string (label, buf);
    l = hci_create_label (rc, buf, XmALIGNMENT_END);

    if (Editable == True)
	background_color = EDIT_BACKGROUND;
    else
	background_color = BACKGROUND_COLOR1;
    toggle = XtVaCreateManagedWidget (" ",
		xmToggleButtonWidgetClass, rc,
		XmNforeground,		hci_get_read_color (EDIT_FOREGROUND),
		XmNbackground,		hci_get_read_color (background_color),
		XmNborderColor,		hci_get_read_color (background_color),
		XmNselectColor,		hci_get_read_color (WHITE),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		Editable,
		XmNmarginHeight,	2,
		XmNmarginWidth,		2,
		XmNborderWidth,		1,
		XmNset,			False,
		NULL);

    if (callback != NULL) {

	XtAddCallback (toggle,
		XmNvalueChangedCallback, callback, NULL);

    }

    XtManageChild (rc);
    return (toggle);
}

/**********************************************************************

    Description: Creates a custom label_text widget.

    Input:  parent - widget ID of parent
	    label  - label string for associated label widget
	    text_size - width of text edit box
	    callback - user callback when text data updated
    Output: NONE
    Return: text widget ID

***********************************************************************/

Widget hci_create_label_text (Widget parent, char *label, int text_size, 
			char *(*callback) ())
{
    Widget rc, text, l;
    int background_color;
    char buf[128];

    rc = hci_create_rowcolumn (parent, "", XmHORIZONTAL);

    Add_space_in_front_of_string (label, buf);
    l = hci_create_label (rc, buf, XmALIGNMENT_END);

    if (Editable == True)
	background_color = EDIT_BACKGROUND;
    else
	background_color = BACKGROUND_COLOR1;
    text = XtVaCreateManagedWidget ("text",
		xmTextFieldWidgetClass, rc,
		XmNforeground,		hci_get_read_color (EDIT_FOREGROUND),
		XmNbackground,		hci_get_read_color (background_color),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNresizeWidth,		True,
		XmNcolumns,		text_size,
		XmNmaxLength,		text_size+1,
		XmNeditable,		Editable,
		XmNmarginHeight,	2,
		XmNresizeWidth,		False,
		XmNuserData,		(XtPointer) text_size,
		NULL);
    if (callback != NULL)
	hci_text_callback (text, callback);

    XtManageChild (rc);
    Cr_rc = rc;
    return (text);
}

/**********************************************************************

    Description: Creates a custom label_combo widget.

    Input:  parent - widget ID of parent
	    label  - label string of associated label widget
	    n      - number of items in combo list
	    list   - combo list labels
	    width  - width of combo box
	    callback - user callback when list item selected
    Output: NONE
    Return: ID of combo box widget

***********************************************************************/

Widget hci_create_label_combo (Widget parent, char *label, int n, 
	char **list, int width, XtCallbackProc callback)
{
    Widget rc, l, combo, combo_list;
    int i;
    char buf[128];
    XmString	str;

    rc = hci_create_rowcolumn (parent, "", XmHORIZONTAL);

    Add_space_in_front_of_string (label, buf);
    l = hci_create_label (rc, buf, XmALIGNMENT_END);

    combo = XtVaCreateWidget ("combo",
		xmComboBoxWidgetClass,	rc,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNcomboBoxType,	XmDROP_DOWN_LIST,
		XmNwidth,		width,
		XmNvisibleItemCount,	n,
		XmNmarginHeight,	1,
		NULL);
    XtVaGetValues (combo,
		XmNlist,	&combo_list,
		NULL);

    for (i = 0; i < n; i++) {
	str = XmStringCreateLocalized (list[i]);
	XmListAddItemUnselected (combo_list, str, 0);
	XmStringFree (str);
    }

    XtManageChild (combo);
    XtVaSetValues (combo,
		XmNselectedPosition,	0,
		NULL);
    XtAddCallback (combo, XmNselectionCallback, callback, NULL);

    XtManageChild (rc);
    Cr_rc = rc;
    return (combo);
}

/**********************************************************************

    Description: Adding spaces in front of a string to fit common length.

    Input:  str - input string
    Output: buf - padded string
    Return: NONE

***********************************************************************/

static void Add_space_in_front_of_string (char *str, char *buf)
{
    int n_spaces;

    n_spaces = Label_len - strlen (str);
    if (n_spaces >= 0) {
	int i;

	for (i = 0; i < n_spaces; i++)
	    buf[i] = ' ';
	strcpy (buf + n_spaces, str);
    }
    else {
	strncpy (buf, str, Label_len);
	buf[Label_len] = '\0';
    }
}

/**********************************************************************

    Description: Make the window fixed size.

    Input:  w - ID of shell widget
    Output: NONE
    Return: NONE

***********************************************************************/

void hci_freeze_window_size (Widget w)
{
    Dimension width, height;

    XtVaGetValues (w,
		XmNwidth,	&width,
		XmNheight,	&height,
		NULL);

    XtVaSetValues (w,
		XmNminWidth,	width,
		XmNmaxWidth,	width,
		XmNminHeight,	height,
		XmNmaxHeight,	height,
		NULL);
    return;
}

/**********************************************************************

    Description: Internal text widget callback.

    Input:  w - text widget ID
	    callback - user registered callback
    Output: NONE
    Return: NONE

***********************************************************************/

void hci_text_callback (Widget w, char *(*callback) ())
{
    XtAddCallback (w,
	XmNmodifyVerifyCallback, Text_verify_callback, callback);

    XtAddCallback (w,
	XmNlosingFocusCallback, Text_verify_callback, callback);

    XtAddCallback (w, 
	XmNactivateCallback, Text_verify_callback, callback);
}

/**********************************************************************

    Description: Internal text widget event callback.

    Input:  w - text widget ID
	    client_data - client data
	    call_data - text widget data
    Output: NONE
    Return: NONE

***********************************************************************/

static void Text_verify_callback (Widget w, XtPointer client_data,
					XtPointer call_data)
{
    XmTextVerifyCallbackStruct	*cbs =
		(XmTextVerifyCallbackStruct *) call_data;
    char *text, *msg;
    char *selection;
    XtPointer data;
    int	i;
    int	len;
    static int busy = 0;

    if (busy) {
	return;
    }

    busy = 1;

    if (w != Current_w) {

	Current_w = w;
	text = XmTextGetString (w);
	len = strlen (text);

	for (i=0;i<len;i++) {
	    if (strncmp ((text+i)," ",1)) {
		len--;
	    }
	}

	XtVaGetValues (w, XmNuserData, &data, NULL);

	if (len > (int) data) {
	    if ((cbs->startPos > 0) &&
		(cbs->startPos == cbs->endPos)) {

		cbs->text->length--;
		if (cbs->text->length == 0) {
		    cbs->doit = False;
		}
		XtFree (text);
    		busy = 0;
		return;
	    }
	}

	strcpy (Text_org, text);
	Text_org[strlen (text)] = '\0';
	Text_verified = 1;
    }
    else
	text = NULL;

    msg = NULL;
    if (cbs->reason == XmCR_MODIFYING_TEXT_VALUE) {
	char buf[HCI_BUF_128];
	int len, l;
	int text_len, selection_len;

	/* prepare the text to be sent to the verification func */
	text = XmTextGetString (w);
	if (text == NULL) {
	    text_len = 0;
	} else {
	    text_len = strlen (text);
	}
	selection = XmTextGetSelection (w);
	if (selection == NULL) {
	    selection_len = 0;
	} else {
	    selection_len = strlen (selection);
	    XtFree (selection);
	}
	XtVaGetValues (w, XmNuserData, &data, NULL);

	if (((text_len-selection_len) >= (int) data) &&
	     (cbs->startPos == cbs->endPos)) {
	    if (cbs->text->length > 0) {
	        cbs->doit = False;
		XtFree (text);
		busy = 0;
		return;
	    }
	}
	l = text_len - cbs->endPos;
	if (cbs->startPos + cbs->text->length + l >= HCI_BUF_128) {
	    len = cbs->text->length;
	    if (len >= HCI_BUF_128)
		len = HCI_BUF_128 - 1;
	    strncpy (buf, cbs->text->ptr, len);
	}
	else {		/* construct the new text */
	    if (cbs->startPos) {
		strncpy (buf, text, (int) cbs->startPos);
		memset (buf+cbs->startPos,0,1);
	    }
	    len = cbs->startPos;
	    if (cbs->text->length) {
	        strncpy (buf + len, cbs->text->ptr, (int) cbs->text->length);
		memset (buf+len+cbs->text->length,0,1);
	    }
	    len += cbs->text->length;
	    if (l) {
	        strncpy (buf + len, text + (int) cbs->endPos, l);
		memset (buf+len+l,0,1);
	    }
	    len += l;
	}
	memset (buf+len,0,1);
	Text_start_offset = cbs->startPos;
	Text_insert_len = cbs->text->length;
	cbs->doit = True;
	if (client_data != NULL) {
	    msg = ((char *(*)())client_data) (w, buf, 0);
	    if (msg != NULL)
		cbs->doit = False;
	}
	if (cbs->doit) {
	    Text_verified = 0;
	    if (Text_password) {
		int i;
		for (i = 0; i < cbs->text->length; i++)
		    cbs->text->ptr[i] = '*';
		Text_password = 0;
	    }
	}
    }
    else {
	if (client_data != NULL && !Text_verified) {
	    text = XmTextGetString (w);
	    msg = ((char *(*)())client_data) (w, text, 1);
	    if (msg != NULL) {
		XmTextSetString (w, Text_org);
	    } else {
		strncpy (Text_org, text, HCI_BUF_128);
		Text_org[HCI_BUF_128-1] = '\0';
	    }
	}
	Text_verified = 1;
    }

    /* popup a dialog to tell user the error */
    if (msg != NULL && msg[0] != '\0'){ hci_info_popup( w, msg, NULL ); }

    if (text != NULL){ XtFree(text);}

    busy = 0;
}



