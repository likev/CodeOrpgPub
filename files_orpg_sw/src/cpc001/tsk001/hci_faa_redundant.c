/************************************************************************
 *									*
 *	Module:  hci_faa_redundant.c					*
 *									*
 *	Description:  This module handles FAA redundancy requirements	*
 *		      in the RPG Control/Status task.			*
 *									*
 ************************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2010/03/10 18:46:53 $
 * $Id: hci_faa_redundant.c,v 1.18 2010/03/10 18:46:53 ccalvert Exp $
 * $Revision: 1.18 $
 * $State: Exp $
 */

/*	Local include file definitions.					*/

#include <hci_control_panel.h>

char	Cmd [128];		/* Local buffer for building strings */
Redundant_cmd_t	Red_cmd;	/* Common Redundant command buffer */

void	faa_redundant_force_update (Widget w,
		XtPointer client_data, XtPointer call_data);
void	accept_faa_redundant_force_update (Widget w,
		XtPointer client_data, XtPointer call_data);
void	reject_faa_redundant_force_update (Widget w,
		XtPointer client_data, XtPointer call_data);

/************************************************************************
 *	Description: The following function is the front end to the	*
 *		     FAA redundant adaptation data update.  It is	*
 *		     usually activated when one wants to force a copy	*
 *		     of adaptation data on the local channel to the	*
 *		     redundant channel.					*
 *									*
 *	Input:  w           - ID of widget to be parent of popup	*
 *		client_data - *unused*					*
 *		call_data   - *unused*					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_faa_redundant_force_update (
Widget		parent_widget,
XtPointer	client_data,
XtPointer	call_data
)
{
	char buf[HCI_BUF_128];

	if( !ORPGRED_version_numbers_match() )
	{
	  sprintf( buf, "Adaptation data versions do not match.\nAre you sure you want to force an\nupdate of adaptation data on the\nother channel?" );
	}
	else
	{
	  sprintf( buf, "Are you sure you want to force an\nupdate of adaptation data on the\nother channel?" );
	}

        hci_confirm_popup( parent_widget, buf, accept_faa_redundant_force_update, reject_faa_redundant_force_update );
}

/************************************************************************
 *	Description: The following function is called when the "Yes"	*
 *		     button is selected from the FAA redundant force	*
 *		     adaptation data update confirmation window.	*
 *									*
 *	Input:  w           - ID of Yes button				*
 *		client_data - *unused*					*
 *		call_data   - *unused*					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
accept_faa_redundant_force_update (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	int	status;

/*	Build and send message for feedback line on RPG Control/Status	*
 *	window.								*/

	sprintf (Cmd,"Force Adaptation Data Update Commanded by Operator");
	HCI_display_feedback( Cmd );

/*	Log message in hci log file.					*/

	HCI_LE_log(Cmd);

/*	Build force adaptation data copy command			*/

	Red_cmd.cmd        = ORPGRED_UPDATE_ALL_MESSAGES;
	Red_cmd.lb_id      = 0;
	Red_cmd.msg_id     = 0;
	Red_cmd.parameter1 = 0;
	Red_cmd.parameter2 = 0;
	Red_cmd.parameter3 = 0;
	Red_cmd.parameter4 = 0;
	Red_cmd.parameter5 = 0;

/*	Send command for redundant manager to process.			*/

	status = ORPGRED_send_msg (Red_cmd);

	if (status != 0) {

	    sprintf (Cmd,"Force Adaptation Data Update Failed");
	    HCI_display_feedback( Cmd );
	    HCI_LE_error("Force Adaptation Data Update Command failed: %d",
		 status);

	}
}

/************************************************************************
 *	Description: The following function is called when the "No"	*
 *		     button is selected from the FAA redundant force	*
 *		     adaptation data update confirmation window.	*
 *									*
 *	Input:  w           - ID of Yes button				*
 *		client_data - *unused*					*
 *		call_data   - *unused*					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
reject_faa_redundant_force_update (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{

/*	Build and send message for feedback line on RPG Control/Status	*
 *	window.								*/

	sprintf (Cmd,"Force Adaptation Data Update Rejected by Operator");
	HCI_display_feedback( Cmd );

/*	Log message in hci log file.					*/

	HCI_LE_log(Cmd);

}
