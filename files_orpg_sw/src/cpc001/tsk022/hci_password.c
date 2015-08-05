/************************************************************************
 *									*
 *	Module:  hci_password.c						*
 *									*
 *	Description:  This module is used to set up the widgets used	*
 *		      in the HCI Password menu for the HCI GUI.		*
 *									*
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2010/03/10 18:46:54 $
 * $Id: hci_password.c,v 1.6 2010/03/10 18:46:54 ccalvert Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */

/*	Local include file definitions.					*/

#include <hci.h>

/*	Macros used in password change window.	*/

enum	{OLD_PASSWORD=0,
	 NEW_PASSWORD,
	 VERIFY_NEW_PASSWORD
};

#define	HCI_PASSWORD_MAX_LENGTH		64 /* Max length (characters) for a
					      password.	*/

/*	Global widget definitions.	*/

Widget	Top_widget		 = (Widget) NULL;
Widget	Change_password_dialog	 = (Widget) NULL;
Widget	Old_password_text	 = (Widget) NULL;
Widget	New_password_text	 = (Widget) NULL;
Widget	Verify_new_password_text = (Widget) NULL;

Widget	Agency_button	= (Widget) NULL;
Widget	Osf_button	= (Widget) NULL;
Widget	Urc_button	= (Widget) NULL;

Widget	Apply_button	= (Widget) NULL;

static	int	User = HCI_LOCA_NONE; /* lock data for change passwords */
static	char	User_name [10] = {""}; /* Selected user in password window */

int	Update_flag = 0; /* Data change flag (changed = 1) */

char	Buf [128]; /* common buffer for string operations */

static	void	close_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
static	void	select_password_user_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
static	int	validate_password (char *password);
static	void	input_password (Widget w,
		XtPointer client_data, XtPointer call_data);

static	void	timer_proc ();

/************************************************************************
 *	Description: This is the main function for the HCI Password	*
 *		     task.						*
 *									*
 *	Input:  argc - number of commandline arguments			*
 *		argv - pointer to commandline argument data		*
 *	Output: NONE							*
 *	Return: exit_code						*
 ************************************************************************/

int
main (
int	argc,
char	*argv []
)
{
	Widget		form;
	Widget		control_rowcol;
	Widget		loca_rowcol;
	Widget		loca_list;
	Widget		loca_frame;
	Widget		password_rowcol;
	Widget		button;
	Arg		args[ 10 ];
	int		n;
        
	
/*	Initialize HCI.						*/

	HCI_init( argc, argv, HCI_PASSWD_TASK );

	Top_widget = HCI_get_top_widget();

/*      Add redundancy information if site FAA redundant        */

	if (HCI_get_system() == HCI_FAA_SYSTEM)
	{
	  /* The password data can't be changed if the other	*
	   * channel is active.					*/

	  if( ( ORPGRED_channel_state( ORPGRED_MY_CHANNEL ) ==
                ORPGRED_CHANNEL_INACTIVE ) &&
              ( ORPGRED_channel_state( ORPGRED_OTHER_CHANNEL ) ==
	        ORPGRED_CHANNEL_ACTIVE ) &&
              ( ORPGRED_rpg_rpg_link_state () ==
	        ORPGRED_CHANNEL_LINK_UP ) )
	  {
            hci_warning_popup( Top_widget, "You cannot edit this data because the\nother channel is Active.", NULL);
	  }
	}

/*	Define a form widget to be used as the manager for widgets in	*
 *	the rda performance window.					*/

	form = XtVaCreateManagedWidget ("password_form",
		xmFormWidgetClass,	Top_widget,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	control_rowcol = XtVaCreateManagedWidget ("control_rowcol",
		xmRowColumnWidgetClass,	form,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,		XmHORIZONTAL,
		XmNpacking,		XmPACK_TIGHT,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

	button = XtVaCreateManagedWidget ("Close",
		xmPushButtonWidgetClass, control_rowcol,
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (button,
		XmNactivateCallback, close_callback, NULL);

	loca_frame = XtVaCreateManagedWidget ("loca_frame",
		xmFrameWidgetClass,     form,
		XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,       XmATTACH_WIDGET,
		XmNtopWidget,           control_rowcol,
		XmNleftAttachment,      XmATTACH_FORM,
		XmNrightAttachment,     XmATTACH_FORM,
		NULL);

	XtVaCreateManagedWidget ("Select User (LOCA)",
		xmLabelWidgetClass,	loca_frame,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNforeground,		hci_get_read_color (BLACK),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	loca_rowcol = XtVaCreateManagedWidget ("loca_rowcol",
		xmRowColumnWidgetClass,	loca_frame,
		XmNforeground,		hci_get_read_color (BLACK),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNpacking,		XmPACK_TIGHT,
		XmNorientation,		XmHORIZONTAL,
		XmNmarginHeight,	0,
		NULL);

	n = 0;

	XtSetArg (args [n], XmNforeground,
		hci_get_read_color (TEXT_FOREGROUND)); n++;
	XtSetArg (args [n], XmNbackground,
		hci_get_read_color (BACKGROUND_COLOR1)); n++;
	XtSetArg (args [n], XmNfontList,
		hci_get_fontlist (LIST)); n++;
	XtSetArg (args [n], XmNorientation,	XmHORIZONTAL); n++;

	loca_list = XmCreateRadioBox( loca_rowcol, "loca_list", args, n);

	Agency_button = XtVaCreateManagedWidget ("Agency",
		xmToggleButtonWidgetClass,loca_list,
		XmNselectColor,		hci_get_read_color (WHITE),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNset,			False,
		NULL);

	XtAddCallback (Agency_button,
		XmNvalueChangedCallback, select_password_user_callback,
		(XtPointer) HCI_LOCA_AGENCY);

	Osf_button = XtVaCreateManagedWidget ("ROC",
		xmToggleButtonWidgetClass,loca_list,
		XmNselectColor,		hci_get_read_color (WHITE),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNset,			False,
		NULL);

	XtAddCallback (Osf_button,
		XmNvalueChangedCallback, select_password_user_callback,
		(XtPointer) HCI_LOCA_ROC);

	Urc_button = XtVaCreateManagedWidget ("URC",
		xmToggleButtonWidgetClass,loca_list,
		XmNselectColor,		hci_get_read_color (WHITE),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNset,			False,
		NULL);

	XtAddCallback (Urc_button,
		XmNvalueChangedCallback, select_password_user_callback,
		(XtPointer) HCI_LOCA_URC);

	XtManageChild (loca_list);

/*	Create a label and text widget which can be used to prompt	*
 *	for the password.  As the password is entered, the text is	*
 *	to be overwritten by "*".					*/

	password_rowcol = XtVaCreateManagedWidget ("password_rowcol",
		xmRowColumnWidgetClass,	form,
		XmNorientation,		XmHORIZONTAL,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		loca_frame,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

	XtVaCreateManagedWidget ("Old Password:        ",
		xmLabelWidgetClass,	password_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	Old_password_text = XtVaCreateManagedWidget ("password_text",
		xmTextFieldWidgetClass,	password_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		False,
		XmNsensitive,		False,
		NULL);

	XtAddCallback (Old_password_text,
		XmNmodifyVerifyCallback, input_password,
		(XtPointer) OLD_PASSWORD);

	XtAddCallback (Old_password_text,
		XmNactivateCallback, input_password,
		(XtPointer) OLD_PASSWORD);

	password_rowcol = XtVaCreateManagedWidget ("password_rowcol",
		xmRowColumnWidgetClass,	form,
		XmNorientation,		XmHORIZONTAL,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		password_rowcol,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

	XtVaCreateManagedWidget ("New Password:        ",
		xmLabelWidgetClass,	password_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	New_password_text = XtVaCreateManagedWidget ("password_text",
		xmTextFieldWidgetClass,	password_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		False,
		XmNsensitive,		False,
		NULL);

	XtAddCallback (New_password_text,
		XmNmodifyVerifyCallback, input_password,
		(XtPointer) NEW_PASSWORD);

	XtAddCallback (New_password_text,
		XmNactivateCallback, input_password,
		(XtPointer) NEW_PASSWORD);

	password_rowcol = XtVaCreateManagedWidget ("password_rowcol",
		xmRowColumnWidgetClass,	form,
		XmNorientation,		XmHORIZONTAL,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		password_rowcol,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

	XtVaCreateManagedWidget ("Verify New Password: ",
		xmLabelWidgetClass,	password_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	Verify_new_password_text = XtVaCreateManagedWidget ("password_text",
		xmTextFieldWidgetClass,	password_rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNeditable,		False,
		XmNsensitive,		False,
		NULL);

	XtAddCallback (Verify_new_password_text,
		XmNmodifyVerifyCallback, input_password,
		(XtPointer) VERIFY_NEW_PASSWORD);

	XtAddCallback (Verify_new_password_text,
		XmNactivateCallback, input_password,
		(XtPointer) VERIFY_NEW_PASSWORD);

	XtRealizeWidget (Top_widget);

/*	Start HCI loop.							*/

	HCI_start( timer_proc, HCI_TWO_SECONDS, NO_RESIZE_HCI );	

	return 0;
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Close" button in the HCI Password window.	*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

static	void
close_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	XtDestroyWidget (Top_widget);
}

/************************************************************************
 *	Description: This function is the timer procedure for the	*
 *		     HCI Password task.					*
 *									*
 *	Input:  w - timer widget ID					*
 *		id - timer ID						*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

static	void
timer_proc ()
{
  /* Nothing to do in event timer as of yet. */
}

/************************************************************************
 *	Description: This function is activated when the user changes	*
 *		     one of the password text items in the Change	*
 *		     Passwords window.					*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - OLD_PASSWORD				*
 *			      NEW_PASSWORD				*
 *			      VERIFY_NEW_PASSWORD			*
 *		call_data - text data					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

static	void
input_password (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	char	*new;
	int	len;
static	char	*passwd = (char *) NULL;
static	char	*verify_passwd = (char *) NULL;
	char	buf[HCI_BUF_128];

	XmTextVerifyCallbackStruct	*cbs =
		(XmTextVerifyCallbackStruct *) call_data;

	if (cbs->reason == XmCR_ACTIVATE) {

	    switch ((int) client_data) {

		case OLD_PASSWORD :

		    if (validate_password (passwd)) {

			XtVaSetValues (Old_password_text,
				XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
				XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
				XmNsensitive,	False,
				XmNeditable,	False,
				NULL);

			XtVaSetValues (New_password_text,
				XmNforeground,	hci_get_read_color (EDIT_FOREGROUND),
				XmNbackground,	hci_get_read_color (EDIT_BACKGROUND),
				XmNsensitive,	True,
				XmNeditable,	True,
				NULL);

			XtVaSetValues (Agency_button,
				XmNsensitive,	False,
				NULL);
			XtVaSetValues (Osf_button,
				XmNsensitive,	False,
				NULL);
			XtVaSetValues (Urc_button,
				XmNsensitive,	False,
				NULL);

			XmProcessTraversal (New_password_text, XmTRAVERSE_CURRENT);

		    } else {

			sprintf( buf, "You entered an invalid password!" );
		        XmTextSetString (Old_password_text,"");

		    }

		    break;

		case NEW_PASSWORD :

		    XtVaSetValues (New_password_text,
			XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
			XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
			XmNsensitive,	False,
			XmNeditable,	False,
			NULL);

		    XtVaSetValues (Verify_new_password_text,
			XmNforeground,	hci_get_read_color (EDIT_FOREGROUND),
			XmNbackground,	hci_get_read_color (EDIT_BACKGROUND),
			XmNsensitive,	True,
			XmNeditable,	True,
			NULL);

		    verify_passwd = (char *) calloc (strlen (passwd)+1, 1);
		    strcpy (verify_passwd, passwd);

		    XmProcessTraversal (Verify_new_password_text, XmTRAVERSE_CURRENT);
		    break;

		case VERIFY_NEW_PASSWORD :

		    if (!strcmp (passwd, verify_passwd)) {

		    	{
			    sprintf( buf, "The password has been changed!" );
			    hci_warning_popup( Top_widget, buf, NULL );
			}

/*			We need to update the permanent password record	*/

			{
			    char	*p;
			    int		status;

/*			    Since passwords are stored encrypted, we	*
 *			    must encrypt the input password before	*
 *			    saving it to file.				*/

			    p = ORPGMISC_crypt (passwd);
			    if (p == NULL) {
				HCI_LE_error( "ORPGMISC_crypt %s failed", passwd);
			    }
			    else {
			        status = 0;
			        switch (User) {

				case HCI_LOCA_AGENCY :

				    status = HCI_set_agency_password(p);
				    break;

				case HCI_LOCA_ROC :

				    status = HCI_set_roc_password(p);
				    break;

				case HCI_LOCA_URC :

				    status = HCI_set_urc_password(p);
				    break;
				}
			    
			        if (status < 0) {

				    sprintf (Buf, "Unable to change password for user %s (%d)", User_name, status);
				    HCI_LE_error( Buf );

			        } else {

				    sprintf (Buf, "Password has been changed for user %s", User_name);
				    HCI_LE_log( Buf );

			        }
			    }
			}

		    } else {

			sprintf( buf, "You entered an invalid password!" );
			hci_warning_popup( Top_widget, buf, NULL );
		        sprintf (Buf, "Password update for user %s failed!",
				User_name);

		    }

		    free (verify_passwd);

		    XmTextSetString (Old_password_text,"");
		    XmTextSetString (New_password_text,"");
		    XmTextSetString (Verify_new_password_text,"");

		    XtVaSetValues (Verify_new_password_text,
			XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
			XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
			XmNsensitive,	False,
			XmNeditable,	False,
			NULL);

		    XtVaSetValues (Agency_button,
			XmNsensitive,	True,
			NULL);
		    XtVaSetValues (Osf_button,
			XmNsensitive,	True,
			NULL);
		    XtVaSetValues (Urc_button,
			XmNsensitive,	True,
			NULL);
		    XmToggleButtonSetState (Agency_button, False, False);
		    XmToggleButtonSetState (Osf_button, False, False);
		    XmToggleButtonSetState (Urc_button, False, False);
		    User = HCI_LOCA_NONE;

		    HCI_display_feedback( Buf );

		    break;

	    }

	    XtFree (passwd);
	    passwd = (char *) NULL;
	    return;

	} else {

	    if (cbs->startPos < cbs->currInsert) { /* backspace */

	        if (passwd != (char *) NULL) {

		    cbs->endPos = strlen (passwd); /* delete from here to end */
	            passwd [cbs->startPos] = 0;
	        }

	        return;

	    }

	    if (cbs->text->length > 1) {

	        cbs->doit = False;		/* don't allow paste ops */
	        return;

	    }

/*	    If the password has reached the maximum allowed length,	*
 *	    reject any additional characters.				*/

	    if (cbs->endPos >= HCI_PASSWORD_MAX_LENGTH) {

		cbs->doit = False;
		return;

	    }

	    new = XtMalloc (cbs->endPos+2);	/* new char + NULL term */

	    if (passwd) {
	        strcpy (new, passwd);
	        XtFree (passwd);

	    } else {
		memset (new,0,1);
	    }

	    passwd = new;
	    strncat (passwd, cbs->text->ptr, cbs->text->length);
	    passwd [cbs->endPos + cbs->text->length] = 0;

	    for (len = 0; len < cbs->text->length; len++) {

	        cbs->text->ptr [len] = '*';

	    }
	}
}

/************************************************************************
 *	Description: This function is used to validate the old password	*
 *		     in the Change Passwords window.			*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

static	int
validate_password (
char	*password
)
{
	char	*check_pwd;
	char	*official_pwd;

/*	Since the passwords are encrypted, we must first set the	*
 *	encryption key and then encrypt the user input before		*
 *	matching passwords.						*/

	check_pwd = ORPGMISC_crypt (password);

	if (check_pwd == NULL)
	    return HCI_LOCA_NONE;

	if (XmToggleButtonGetState (Agency_button)) {

	    if( HCI_agency_password( &official_pwd ) && official_pwd != NULL )
	    {
	       if (!strcmp (check_pwd, official_pwd)) {

		return HCI_LOCA_AGENCY;

	       }
	    }

	} else if (XmToggleButtonGetState (Osf_button)) {

	    if( HCI_roc_password( &official_pwd ) && official_pwd != NULL )
	    {
	       if (!strcmp (check_pwd, official_pwd)) {

		return HCI_LOCA_ROC;

	       }
	    }

	} else if (XmToggleButtonGetState (Urc_button)) {

	    if( HCI_urc_password( &official_pwd ) && official_pwd != NULL )
	    {
	       if (!strcmp (check_pwd, official_pwd)) {

		return HCI_LOCA_URC;

	       }
	    }
	}

	return HCI_LOCA_NONE;

}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     one of the "Select User" radio buttons in the	*
 *		     Change Passwords window.				*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - user type					*
 *		call_data - toggle data					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

static	void
select_password_user_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	XmToggleButtonCallbackStruct	*state =
		(XmToggleButtonCallbackStruct *) call_data;

	if (state->set) {

	    User = (int) client_data;
	    XtVaSetValues (Old_password_text,
		XmNforeground,	hci_get_read_color (EDIT_FOREGROUND),
		XmNbackground,	hci_get_read_color (EDIT_BACKGROUND),
		XmNsensitive,	True,
		XmNeditable,	True,
		NULL);

	    switch ((int) client_data) {

		case HCI_LOCA_AGENCY :

		    sprintf (User_name,"AGENCY");
		    break;

		case HCI_LOCA_ROC :

		    sprintf (User_name,"ROC");
		    break;

		case HCI_LOCA_URC :

		    sprintf (User_name,"URC");
		    break;

		default :

		    sprintf (User_name,"Unknown");
		    break;

	    }
	}
}

