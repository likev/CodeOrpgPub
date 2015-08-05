/************************************************************************
 *	Module:	 rdasim_send_command_popup.c				*
 *									*
 *	Description:  This module is used by the rda simulator gui to	*
 *		      define and display a popup that will allow the	*
 *		      the user to interact with the rda simulator by	*
 *		      sending commands.					*
 ************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/11/01 18:17:27 $
 * $Id: rdasim_gui_send_command_popup.c,v 1.5 2013/11/01 18:17:27 steves Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

/*	Local include files.						*/

#include <hci.h>
#include <rdasim_simulator.h>

/*	Local definitions.	*/

#define	NUMBER_OF_COMMANDS	6

/*	Global static variables.	*/

static	Widget	Send_command_popup_dialog = (Widget) NULL;
static	Widget	term_cut_user_input = (Widget) NULL;
static	int	lbfd = -1;
static	char	parameters_to_send[ LB_PARAMETER_LENGTH ];

/*	Function prototypes.	*/

void	send_command_popup_close (Widget w,
		XtPointer client_data, XtPointer call_data);
void	send_command (Widget w,
		XtPointer client_data, XtPointer call_data);
void	send_command_term_cut(Widget w,
		XtPointer client_data, XtPointer call_data);
void	error_with_LB( char *lb_fx_name, int error_code );
void	bad_input( char *section_name  );

/************************************************************************
 *	Description: This is the callback for the "send command"	*
 *		     pushbutton in the rda simulator gui.		*
 *									*
 *	Input:  w - Widget ID of "send command" pushbutton.		*
 *	        client_data - file descriptor of LB			*
 *	        call_data   - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
send_command_popup(
Widget		w,
XtPointer	client_data,
int		rda_redundant_mode,
XtPointer	call_data
)
{
	Widget		control_rowcol; /* Holds close button. */
	Widget		command_frame;  /* Frame to hold command_rowcol. */
	Widget		command_rowcol; /* Rowcol to hold commands. */
	Widget		row1_rowcol;	/* One rowcol per command so */
	Widget		row2_rowcol;    /* the spacing works out.    */
	Widget		row3_rowcol;    /* " */
	Widget		row4_rowcol;    /* " */
	Widget		row5_rowcol;    /* " */
	Widget		row6_rowcol;    /* " */
	Widget		button;
	Widget		form;
	Widget		label;

/*	Set LB file descriptor.		*/

	lbfd = ( int ) client_data;

/*	Do not allow more than one send command popup to exist at a	*
 *	time.								*/

	if (Send_command_popup_dialog != NULL)
	{
/*	  The send command window already exist so bring it to the	*
 *	  top of the window heirarchy.				*/

	  HCI_Shell_popup( Send_command_popup_dialog );
	  return;
	}

/*	Create the top-level widget.					*/

	HCI_Shell_init( &Send_command_popup_dialog, "Send Command" );

/*	Use a form widget to organize the various menu widgets.		*/

	form   = XtVaCreateWidget ("form",
		xmFormWidgetClass,		Send_command_popup_dialog,
		XmNautoUnmanage,		False,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	control_rowcol = XtVaCreateWidget ("control_rowcol",
		xmRowColumnWidgetClass,		form,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,			XmHORIZONTAL,
		XmNpacking,			XmPACK_TIGHT,
		XmNnumColumns,			1,
		XmNisAligned,			False,
		XmNtopAttachment,		XmATTACH_FORM,
		XmNleftAttachment,		XmATTACH_FORM,
		XmNrightAttachment,		XmATTACH_FORM,
		NULL);

	button = XtVaCreateManagedWidget ("Close",
		xmPushButtonWidgetClass,	control_rowcol,
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNfontList,			hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (button,
		XmNactivateCallback, send_command_popup_close, NULL);

	XtManageChild (control_rowcol);

/*	Create frame to hold command rowcols.	*/

	command_frame   = XtVaCreateManagedWidget ("command_frame",
		xmFrameWidgetClass,		form,
		XmNtopAttachment,		XmATTACH_WIDGET,
		XmNtopWidget,			control_rowcol,
		XmNleftAttachment,		XmATTACH_FORM,
		XmNrightAttachment,		XmATTACH_FORM,
		XmNbackground,			hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	label = XtVaCreateManagedWidget ("Commands to Send",
		xmLabelWidgetClass,	command_frame,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	command_rowcol = XtVaCreateWidget ("products_rowcol",
		xmRowColumnWidgetClass,	command_frame,
		XmNorientation,		XmHORIZONTAL,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNpacking,		XmPACK_COLUMN,
		XmNnumColumns,		NUMBER_OF_COMMANDS,
		XmNentryAlignment,	XmALIGNMENT_CENTER,
		NULL);

/*	Create a rowcol for each command, add button/label	*
 *	to each rowcol.						*/

	row1_rowcol = XtVaCreateManagedWidget( "row1_rowcol",
		xmRowColumnWidgetClass,	command_rowcol,
		XmNorientation,		XmHORIZONTAL,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		XmNentryAlignment,	XmALIGNMENT_CENTER,
		NULL);

	button = XtVaCreateManagedWidget ("Send",
		xmPushButtonWidgetClass,	row1_rowcol,
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (button,
		XmNactivateCallback, send_command,
		(XtPointer) COMMAND_RDA_TO_LOCAL );

	label = XtVaCreateManagedWidget ("RDA Control to Local",
		xmLabelWidgetClass,	row1_rowcol,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	XtManageChild (row1_rowcol);

	row2_rowcol = XtVaCreateManagedWidget( "row2_rowcol",
		xmRowColumnWidgetClass,	command_rowcol,
		XmNorientation,		XmHORIZONTAL,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		XmNentryAlignment,	XmALIGNMENT_CENTER,
		NULL);

	button = XtVaCreateManagedWidget ("Send",
		xmPushButtonWidgetClass,	row2_rowcol,
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (button,
		XmNactivateCallback, send_command,
		(XtPointer) COMMAND_RDA_TO_REMOTE );

	label = XtVaCreateManagedWidget ("RDA Control to Remote",
		xmLabelWidgetClass,	row2_rowcol,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	XtManageChild (row2_rowcol);

	row3_rowcol = XtVaCreateManagedWidget( "row3_rowcol",
		xmRowColumnWidgetClass,	command_rowcol,
		XmNorientation,		XmHORIZONTAL,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		XmNentryAlignment,	XmALIGNMENT_CENTER,
		NULL);

	button = XtVaCreateManagedWidget ("Send",
		xmPushButtonWidgetClass,	row3_rowcol,
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	if( rda_redundant_mode == 0 )
	{
	  XtVaSetValues (button,
		XmNsensitive,	False,
		NULL);
	}

	XtAddCallback (button,
		XmNactivateCallback, send_command,
		(XtPointer) TOGGLE_CHANNEL_NUMBER );

	label = XtVaCreateManagedWidget ("Toggle RDA Channel Number",
		xmLabelWidgetClass,	row3_rowcol,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	if( rda_redundant_mode == 0 )
	{
	  XtVaSetValues (label,
		XmNsensitive,	False,
		NULL);
	}

	XtManageChild (row3_rowcol);

	row4_rowcol = XtVaCreateManagedWidget( "row4_rowcol",
		xmRowColumnWidgetClass,	command_rowcol,
		XmNorientation,		XmHORIZONTAL,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		XmNentryAlignment,	XmALIGNMENT_CENTER,
		NULL);

	button = XtVaCreateManagedWidget ("Send",
		xmPushButtonWidgetClass,	row4_rowcol,
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	if( rda_redundant_mode == 0 )
	{
	  XtVaSetValues (button,
		XmNsensitive,	False,
		NULL);
	}

	XtAddCallback (button,
		XmNactivateCallback, send_command,
		(XtPointer) CHANGE_CHANNEL_CONTROL_STATUS );

	label = XtVaCreateManagedWidget ("Change RDA Channel Control State",
		xmLabelWidgetClass,	row4_rowcol,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	if( rda_redundant_mode == 0 )
	{
	  XtVaSetValues (label,
		XmNsensitive,	False,
		NULL);
	}

	XtManageChild (row4_rowcol);

	row5_rowcol = XtVaCreateManagedWidget( "row5_rowcol",
		xmRowColumnWidgetClass,	command_rowcol,
		XmNorientation,		XmHORIZONTAL,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		XmNentryAlignment,	XmALIGNMENT_CENTER,
		NULL);

	button = XtVaCreateManagedWidget ("Send",
		xmPushButtonWidgetClass,	row5_rowcol,
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (button,
		XmNactivateCallback, send_command_term_cut,
		(XtPointer) AVSET_TERMINATION_CUT );

	label = XtVaCreateManagedWidget ("Change VCP Termination Cut #",
		xmLabelWidgetClass,	row5_rowcol,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	term_cut_user_input = XtVaCreateManagedWidget( "term_cut",
		xmTextFieldWidgetClass, row5_rowcol,
		XmNforeground,          hci_get_read_color (EDIT_FOREGROUND),
		XmNbackground,          hci_get_read_color (EDIT_BACKGROUND),
		XmNfontList,            hci_get_fontlist (LIST),
		XmNcolumns,             5,
		XmNmaxLength,           5,
		XmNmarginHeight,        2,
		XmNshadowThickness,     1,
		NULL);

	XtManageChild (row5_rowcol);

	row6_rowcol = XtVaCreateManagedWidget( "row6_rowcol",
		xmRowColumnWidgetClass,	command_rowcol,
		XmNorientation,		XmHORIZONTAL,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNpacking,		XmPACK_TIGHT,
		XmNnumColumns,		1,
		XmNentryAlignment,	XmALIGNMENT_CENTER,
		NULL);

	button = XtVaCreateManagedWidget ("Send",
		xmPushButtonWidgetClass,	row6_rowcol,
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	XtAddCallback (button,
		XmNactivateCallback, send_command,
		(XtPointer) TOGGLE_MAINTENANCE_MODE );

	label = XtVaCreateManagedWidget ("Toggle Offline Maintenance Mode",
		xmLabelWidgetClass,	row6_rowcol,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNfontList,		hci_get_fontlist (LIST),
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

	XtManageChild (row6_rowcol);

	XtManageChild (command_rowcol);

	XtManageChild (form);

	HCI_Shell_start( Send_command_popup_dialog, NO_RESIZE_HCI );
}

/************************************************************************
 *	Description: This callback is activated when the send command 	*
 *		     "Close" button is selected.			*
 *									*
 *	Input:  w - Widget ID of "Close" button.			*
 *	        client_data - unused					*
 *	        call_data   - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
send_command_popup_close (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	HCI_Shell_popdown( Send_command_popup_dialog );
}

/************************************************************************
 *	Description: This callback is activated when one of the "send"	*
 *		      buttons is selected.				*
 *									*
 *	Input:  w - Widget ID of "send" button.				*
 *	        client_data - unused					*
 *	        call_data   - unused					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
send_command (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
  int status = -1;
  Rdasim_gui_t msg;

  /* Assemble message. */

  msg.cmd_type = COMMAND;
  msg.command = ( int )client_data;
  memcpy( msg.parameters, parameters_to_send, LB_PARAMETER_LENGTH );

  /* Write message to LB. */

  status = LB_write( lbfd, ( char * ) &msg,
                     sizeof( Rdasim_gui_t ), RDASIM_GUI_MSG_ID );
  
  if( status < 0 )
  {
    error_with_LB( "LB_write", status );
    return;
  }
}

/************************************************************************
 *      Description: This callback is activated when the "send"		*
 *                   buttons is selected for the AVSET termination cut 	*
 *		     command. The user input for the cut number must be	*
 *		     handled.						*
 *                                                                      *
 *      Input:  w - Widget ID of "send" button.                         *
 *              client_data - unused                                    *
 *              call_data   - unused                                    *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

void
send_command_term_cut (
Widget          w,
XtPointer       client_data,
XtPointer       call_data
)
{
  char *input;
  int len;
  int tmp_int;
  int *param_list;

  input = XmTextGetString( term_cut_user_input );
  len = strlen( input );
  if( len < 1 )
  {
    bad_input( "Command: Termination Cut #" );
    return;
  }
  else
  {
    tmp_int = atoi( input );
  }
  XtFree( input );

  param_list = ( int * ) parameters_to_send;
  param_list[ 0 ] = tmp_int;

  send_command( w, client_data, call_data );
}

