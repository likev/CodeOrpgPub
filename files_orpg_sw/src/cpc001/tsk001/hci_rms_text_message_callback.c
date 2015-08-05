/************************************************************************
 *									*
 *	Module:	hci_rms_text_message_callback.c				*
 *									*
 *	Description:	This module handles all functions pertaining	*
 *			to sending rms free text messages.	*
 *									*
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2010/03/10 18:46:58 $
 * $Id: hci_rms_text_message_callback.c,v 1.16 2010/03/10 18:46:58 ccalvert Exp $
 * $Revision: 1.16 $
 * $State: Exp $
 */

/*	Local include file definitions.					*/

#include <hci_control_panel.h>

/*	Macros.								*/

#define MAX_MSG_SIZE	400	/* Maximum RMS message length (bytes) */

/*	File global definitions						*/

static	Widget	Dialog            = (Widget) NULL;
static	Widget	Outgoing_messages = (Widget) NULL;


static char	Cmd [128];	/* Common buffer for feedback message */
static char	Message [1600]; /* Common buffer for RMS message */
XmString	Str;	/* Common compound string for Motif objects */

void	rms_text_message_close_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
void	rms_text_message_send_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
void	rms_text_message_clear_callback (Widget w,
		XtPointer client_data, XtPointer call_data);

/************************************************************************
 *	Description: This function creates a dialog shell for entering	*
 *		     RMS text messages.					*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_rms_text_message ()
{
	Widget	form;
	Widget	rowcol;
	Widget	button;
	Widget	label;
	Widget	outgoing_rowcol;
	int	n;
	Arg	args [16];

/*	Log a message in the hci log file.				*/

	HCI_LE_log("RMS message function selected");
	
/*	If the RMS text message window already defined, do nothing.	*/

	if (Dialog != NULL)
	{
	  HCI_Shell_popup( Dialog );
	  return;
	}
	
/*	Create a dialog shell for RMS messages				*/

	HCI_Shell_init( &Dialog, "RMS Free Text Message" );

/*	Set permission of RMS text msg LB. */

	ORPGDA_write_permission(ORPGDAT_RMS_TEXT_MSG);

/*	Use a form widget to manage window widgets.			*/

	form = XtVaCreateWidget ("rms_text_message_form",
		xmFormWidgetClass,	Dialog,
		NULL);

/*	Create a row of buttons across the top of the window.		*/

	rowcol = XtVaCreateWidget ("rowcol",
		xmRowColumnWidgetClass,		form,
		XmNpacking,		XmPACK_COLUMN,
		XmNorientation,		XmHORIZONTAL,
		XmNcolumns,		1,
		XmNforeground,		hci_get_read_color (BLACK),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNalignment,		XmALIGNMENT_CENTER,
		NULL);

	button = XtVaCreateManagedWidget ("Close",
		xmPushButtonWidgetClass, rowcol,
		XmNbackground,		 hci_get_read_color (BUTTON_BACKGROUND),
		XmNforeground,		 hci_get_read_color (BUTTON_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (button,
		XmNactivateCallback, rms_text_message_close_callback,
		NULL);

	button = XtVaCreateManagedWidget ("Send",
		xmPushButtonWidgetClass, rowcol,
		XmNbackground,		 hci_get_read_color (BUTTON_BACKGROUND),
		XmNforeground,		 hci_get_read_color (BUTTON_FOREGROUND),
		XmNtraversalOn,		False,
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (button,
		XmNactivateCallback, rms_text_message_send_callback,
		NULL);
	
	button = XtVaCreateManagedWidget ("Clear",
		xmPushButtonWidgetClass, rowcol,
		XmNbackground,		 hci_get_read_color (BUTTON_BACKGROUND),
		XmNforeground,		 hci_get_read_color (BUTTON_FOREGROUND),
		XmNtraversalOn,		False,
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (button,
		XmNactivateCallback, rms_text_message_clear_callback,
		NULL);
	
	XtManageChild (rowcol);

/*	Below the button row create a list of outgoing messages.	*/

	outgoing_rowcol = XtVaCreateWidget ("outgoing_rowcol",
		xmRowColumnWidgetClass,	form,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,		XmHORIZONTAL,
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		XmNentryAlignment,	XmALIGNMENT_CENTER,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		rowcol,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

	

	label = XtVaCreateManagedWidget ("                               Free Text Message:",
		xmLabelWidgetClass,	outgoing_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtManageChild (outgoing_rowcol);

	n = 0;
	XtSetArg (args [n], XmNrows,	5); n++;	
	XtSetArg (args [n], XmNcolumns,	80); n++;	
	XtSetArg (args [n], XmNforeground,	hci_get_read_color (EDIT_FOREGROUND)); n++;
	XtSetArg (args [n], XmNbackground,	hci_get_read_color (EDIT_BACKGROUND)); n++;
	XtSetArg (args [n], XmNeditMode,	XmMULTI_LINE_EDIT); n++;	
	XtSetArg (args [n], XmNtopAttachment,	XmATTACH_WIDGET); n++;	
	XtSetArg (args [n], XmNtopWidget,	outgoing_rowcol); n++;	
	XtSetArg (args [n], XmNfontList,	hci_get_fontlist (LIST)); n++;	
	XtSetArg (args [n], XmNleftAttachment,	XmATTACH_FORM); n++;	
	XtSetArg (args [n], XmNrightAttachment,	XmATTACH_FORM); n++;	
	XtSetArg (args [n], XmNbottomAttachment,	XmATTACH_FORM); n++;	

	Outgoing_messages = XmCreateScrolledText (form, "outgoing_messages",
				args, n);
	XtManageChild (Outgoing_messages);
	
	XtManageChild (form);

	HCI_Shell_start( Dialog, NO_RESIZE_HCI );
}

/************************************************************************
 *	Description: This function is called when the user selects the	*
 *		     "Close" button from the RMS Free Text Messages	*
 *		     window.						*
 *									*
 *	Input:  w	    - widget ID of the Close button		*
 *		client_data - *unused*					*
 *		call_data   - *unused*					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
rms_text_message_close_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	HCI_LE_log("RMS message Close selected");
	HCI_Shell_popdown( Dialog );
}

/************************************************************************
 *	Description: This function is called when the user selects the	*
 *		     "Clear" button from the RMS Free Text Messages	*
 *		     window.						*
 *									*
 *	Input:  w	    - widget ID of the Clear button		*
 *		client_data - *unused*					*
 *		call_data   - *unused*					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
rms_text_message_clear_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	XmTextSetString (Outgoing_messages,"");
}

/************************************************************************
 *	Description: This function is called when the user selects the	*
 *		     "Send" button from the RMS Free Text Messages	*
 *		     window.						*
 *									*
 *	Input:  w	    - widget ID of the Send button		*
 *		client_data - *unused*					*
 *		call_data   - *unused*					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
rms_text_message_send_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	char	*string;
	int 	ret;
	char 	buf[128];
	
/*	Extract the outgoing message from the widget and copy it to	*
 *	the message buffer.						*/

	string = XmTextGetString (Outgoing_messages);

	strcpy (Message, string);

/*	Log a message in the hci log file.				*/

	HCI_LE_log("RMS message [%d] bytes selected", strlen(Message));
	
/*	If the message size is valid, display a message in the feedback	*
 *	line and write the RMS free text message to the RMS text LB so	*
 *	the RMS manager can send it.					*/

	if(strlen(Message) <= MAX_MSG_SIZE) {

	    sprintf (Cmd,"Sending RMS Message");

	    HCI_display_feedback( Cmd );
		
	    ORPGDA_write (ORPGDAT_RMS_TEXT_MSG,
			(char*) &Message,
			strlen(Message),
			LB_ANY);
		
	    if ((ret = EN_post (ORPGEVT_RMS_TEXT_MSG,
			      NULL, 0, 0)) < 0){ 
				       
     		 HCI_LE_log("Failed to Post ORPGEVT_RMS_TEXT_MSG (%d).", ret); 
		     
	    } else {
		
		 HCI_LE_log("Message sent to RMS");
			
	    } 
								
	} else {	

/*	Else, the message is too large so popup an informative message	*
 *	so the user can decide what to do next.				*/
		
	    sprintf (buf,"RMS message cannot exceed 400 characters.\nMessage is %d characters long.\nDo you want to change it?",strlen(Message));
            hci_confirm_popup( Dialog, buf, NULL, rms_text_message_close_callback ); 
				
	}/*End else*/		
		
}

