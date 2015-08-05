/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/04/15 19:25:11 $
 * $Id: hci_configuration.c,v 1.26 2009/04/15 19:25:11 ccalvert Exp $
 * $Revision: 1.26 $
 * $State: Exp $
 */

/************************************************************************
 *	Description: This module contains a collection of routines	*
 *	concerning HCI state information.  HCI state information	*
 *	is saved in the data store ORPGDAT_HCI_DATA.			*
 ************************************************************************/

/*	Local include file definitions.					*/

#include <hci.h>

/*	The next group of static variables are used to flag whether	*
 *	the specific message has been updated since the last read.	*
 *	These are set in the LB_NOTIFY callback.			*
 *	1 = Not updated since last read; 0 = Updated since last read.	*/

static	int	Hci_gui_init_flag = 0;
static	int	Hci_task_flag     = 0;

/*	The following group of static variables are used to store the	*
 *	last I/O status for each read operation.			*/

static	int	Hci_gui_info_io_status = 0;
static	int	Hci_task_io_status     = 0;

/*	The following group of static variables store the contents of	*
 *	each message in the ORPGDAT_HCI_DATA LB.			*/

static	Hci_gui_t	Hci_gui;
static	Hci_task_t	Hci_task [HCI_MAX_TASK_STATUS_NUM];

/*	The following static variable is used to flag whether the	*
 *	LB notification callback has been registered.			*
 *	0 = Not registered; 1 = Regidtered.				*/

static	int	Hci_info_lb_notify = 0;

static	int	Hci_inhibit_RDA_messages = 0;
static	int	Hci_task_num = 0;

void	hci_state_info_update (int lbfd, LB_id_t msg_id, int msg_info,
			       void *arg);

/************************************************************************
 *	Description: This function reads the specified message from the	*
 *		     HCI state info data store (ORPGDAT_HCI_DATA) 	*
 *									*
 *	Input:	msg_id - The message ID of the state data requested	*
 *			 (use macros defined in hci.h).			*
 *	Output: NONE							*
 *	Return:	On success, a value >= 0 is returned.			*
 ************************************************************************/

int
hci_read_info_msg (
int	msg_id
)
{
	int	status = 0;
	int	lbfd;
static	int	read_flag = 0;

/*	Initialize GUI properties to default values.			*/

	LE_set_option ("LE disable", 1);
	if (Hci_info_lb_notify == 0) {

	    Hci_gui.text_fg_color    = BLACK;
	    Hci_gui.text_bg_color    = PEACHPUFF3;
	    Hci_gui.button_fg_color  = WHITE;
	    Hci_gui.button_bg_color  = STEELBLUE;
	    Hci_gui.edit_fg_color    = BLACK;
	    Hci_gui.edit_bg_color    = LIGHTSTEELBLUE;
	    Hci_gui.canvas_bg1_color = PEACHPUFF3;
	    Hci_gui.canvas_bg2_color = GRAY;
	    Hci_gui.normal_color     = GREEN;
	    Hci_gui.warning_color    = YELLOW;
	    Hci_gui.alarm1_color     = RED;
	    Hci_gui.alarm2_color     = ORANGE;
	    Hci_gui.icon_fg_color    = WHITE;
	    Hci_gui.icon_bg_color    = STEELBLUE;
	    Hci_gui.product_fg_color = WHITE;
	    Hci_gui.product_bg_color = BLACK;
	    Hci_gui.loca_fg_color    = WHITE;
	    Hci_gui.loca_bg_color    = PEACHPUFF3;
	    Hci_gui.font_size        = HCI_DEFAULT_FONT_SIZE;
	    Hci_gui.font_point       = HCI_DEFAULT_FONT_POINT;

	    lbfd = ORPGDA_lbfd (ORPGDAT_HCI_DATA);

	    if (lbfd < 0) {

		if (!read_flag) {

		    read_flag++;
		    read_flag++;
		    HCI_LE_error("ORPGDA_lbfd (ORPGDAT_HCI_DATA) (%d) - use default",
			lbfd);

		}

		LE_set_option ("LE disable", 0);
		return (lbfd);

	    }

/*	    Register for updates to GUI info message			*/

	    status = ORPGDA_UN_register (ORPGDAT_HCI_DATA,
			HCI_GUI_INFO_MSG_ID, hci_state_info_update);

	    if (status != LB_SUCCESS) {

		HCI_LE_error("ORPGDA_UN_register (HCI_GUI_INFO_MSG_ID): %d",
			status);

	    }

/*	    Register for updates to Task info message			*/

	    status = ORPGDA_UN_register (ORPGDAT_HCI_DATA,
			HCI_TASK_INFO_MSG_ID, hci_state_info_update);

	    if (status != LB_SUCCESS) {

		HCI_LE_error("ORPGDA_UN_register (HCI_TASK_INFO_MSG_ID): %d",
			status);

	    }

	    Hci_info_lb_notify = 1;

	}

/*	Update the specified message.					*/

	switch (msg_id) {

	    case HCI_GUI_INFO_MSG_ID :

		Hci_gui_info_io_status = ORPGDA_read (ORPGDAT_HCI_DATA,
						(char *) &Hci_gui,
						sizeof (Hci_gui_t),
						HCI_GUI_INFO_MSG_ID);

		if (Hci_gui_info_io_status < 0) {

		    if (read_flag) {

			LE_set_option ("LE disable", 0);
			return (-1);

		    }

		    read_flag++;

/*		If the GUI state info message has not been created, 	*
 *		then lets create it using some hard coded defaults.	*/

		    if (Hci_gui_info_io_status == LB_NOT_FOUND) {

			Hci_gui.text_fg_color    = BLACK;
			Hci_gui.text_bg_color    = PEACHPUFF3;
			Hci_gui.button_fg_color  = WHITE;
			Hci_gui.button_bg_color  = STEELBLUE;
			Hci_gui.edit_fg_color    = BLACK;
			Hci_gui.edit_bg_color    = LIGHTSTEELBLUE;
			Hci_gui.canvas_bg1_color = PEACHPUFF3;
			Hci_gui.canvas_bg2_color = GRAY;
			Hci_gui.normal_color     = GREEN;
			Hci_gui.warning_color    = YELLOW;
			Hci_gui.alarm1_color     = RED;
			Hci_gui.alarm2_color     = ORANGE;
			Hci_gui.icon_fg_color    = WHITE;
			Hci_gui.icon_bg_color    = STEELBLUE;
			Hci_gui.product_fg_color = WHITE;
			Hci_gui.product_bg_color = BLACK;
			Hci_gui.loca_fg_color    = WHITE;
			Hci_gui.loca_bg_color    = PEACHPUFF3;
	    		Hci_gui.font_size        = HCI_DEFAULT_FONT_SIZE;
			Hci_gui.font_point       = HCI_DEFAULT_FONT_POINT;

			Hci_gui_info_io_status = ORPGDA_write (ORPGDAT_HCI_DATA,
						(char *) &Hci_gui,
						sizeof (Hci_gui_t),
						HCI_GUI_INFO_MSG_ID);

			if (Hci_gui_info_io_status < 0) {

			    HCI_LE_error("Error initializing GUI state message (%d)",
				Hci_gui_info_io_status);

			} else {

			    HCI_LE_log("GUI state message initialized");
			    Hci_gui_init_flag = 1;

			}

		    } else {

			HCI_LE_error("Error reading from ORPGDAT_HCI_DATA (%d)",
				Hci_gui_info_io_status);

		    }

		} else {

		    Hci_gui_init_flag = 1;
		    read_flag = 0;

		}

		status = Hci_gui_info_io_status;

		break;

	    case HCI_TASK_INFO_MSG_ID :

		Hci_task_io_status = ORPGDA_read (ORPGDAT_HCI_DATA,
						(char *) &Hci_task,
						sizeof (Hci_task_t)*200,
						HCI_TASK_INFO_MSG_ID);

		Hci_task_flag = 1;

		if (Hci_task_io_status < 0) {

		    Hci_task_num = 0;

		    if (read_flag) {

			LE_set_option ("LE disable", 0);
			return (-1);

		    }

		    read_flag++;

		} else {

		    Hci_task_num = Hci_task_io_status/sizeof (Hci_task_t);
		    read_flag = 0;

		}

		status = Hci_task_io_status;

		break;

	}

	LE_set_option ("LE disable", 0);
	return status;

}

/************************************************************************
 *	Description: This function writes the specified message to the	*
 *		     HCI state info data store (ORPGDAT_HCI_DATA) and	*
 *		     returns a pointer to the data in "data".		*
 *									*
 *	Input:	msg_id - The message ID of the state data requested	*
 *			 (use macros defined in hci.h).			*
 *		data   - Pointer to the state data.  If NULL, then	*
 *			 local library data is written.			*
 *	Output: NONE							*
 *	Return:	On success, a value >= 0 is returned.			*
 ************************************************************************/

int
hci_write_info_msg (
int	msg_id,
void	*data
)
{
	int	status = 0;
	Hci_gui_t	*gui;
	Hci_task_t	*task;
static	int	old_num_tasks = -1;

	LE_set_option ("LE disable", 1);
	switch (msg_id) {

	    case HCI_GUI_INFO_MSG_ID :

		if (data != NULL) {

		    gui = (Hci_gui_t *) data;

		    Hci_gui = *gui;

		} else if (!Hci_info_lb_notify) {

		    Hci_gui.text_fg_color    = BLACK;
		    Hci_gui.text_bg_color    = PEACHPUFF3;
		    Hci_gui.button_fg_color  = WHITE;
		    Hci_gui.button_bg_color  = STEELBLUE;
		    Hci_gui.edit_fg_color    = BLACK;
		    Hci_gui.edit_bg_color    = LIGHTSTEELBLUE;
		    Hci_gui.canvas_bg1_color = PEACHPUFF3;
		    Hci_gui.canvas_bg2_color = GRAY;
		    Hci_gui.normal_color     = GREEN;
		    Hci_gui.warning_color    = YELLOW;
		    Hci_gui.alarm1_color     = RED;
		    Hci_gui.alarm2_color     = ORANGE;
		    Hci_gui.icon_fg_color    = WHITE;
		    Hci_gui.icon_bg_color    = STEELBLUE;
		    Hci_gui.product_fg_color = WHITE;
		    Hci_gui.product_bg_color = BLACK;
		    Hci_gui.loca_fg_color    = WHITE;
		    Hci_gui.loca_bg_color    = PEACHPUFF3;
	    	    Hci_gui.font_size        = HCI_DEFAULT_FONT_SIZE;
		    Hci_gui.font_point       = HCI_DEFAULT_FONT_POINT;

		}

		Hci_gui_info_io_status = ORPGDA_write (ORPGDAT_HCI_DATA,
						(char *) &Hci_gui,
						sizeof (Hci_gui_t),
						HCI_GUI_INFO_MSG_ID);

		if (Hci_gui_info_io_status < 0) {

		    HCI_LE_error("Error writing GUI state message (%d)",
			Hci_gui_info_io_status);

		} else {

		    HCI_LE_log("GUI state message written");

		}

		status = Hci_gui_info_io_status;

		break;

	    case HCI_TASK_INFO_MSG_ID :

/*		If task info data is being passed from an extrnal	*
 *		source then replace the internal data.			*/

		if (data != NULL) {

		    int	offset;
		    int	i;
		    char	*buf;

		    offset = 0;
		    buf = (char *) data;

		    for (i=0;i<HCI_MAX_TASK_STATUS_NUM;i++) {

		        task = (Hci_task_t *) (buf+offset);

			if (strlen( task->name ) == 0) {

			    Hci_task_num = i;
			    break;

			}

			offset = offset + sizeof (Hci_task_t);
			strcpy (Hci_task [i].name, task->name);

		    }
		}

/*		If the number of failed tasks is 0 and if previously	*
 *		there were failed tasks, we need to clear the message	*
 *		so it is zero length.  Otherwise, we need to write out	*
 *		info for each failed task.				*/

		if (((Hci_task_num == 0) && (old_num_tasks != 0)) ||
		    (Hci_task_num != 0)) {

		    Hci_task_io_status = ORPGDA_write (ORPGDAT_HCI_DATA,
						(char *) &Hci_task,
						sizeof (Hci_task_t)*Hci_task_num,
						HCI_TASK_INFO_MSG_ID);

		}

		old_num_tasks = Hci_task_num;

		if (Hci_task_io_status < 0) {

		    HCI_LE_error("Error writing task info data (%d)",
			Hci_task_io_status);

		} else {

		    HCI_LE_log("Task info data updated");

		}

		status = Hci_task_io_status;

		break;

	}

	LE_set_option ("LE disable", 0);
	return status;

}

/************************************************************************
 *	Description: This function returns the last known I/O status	*
 *		     for the specified message.				*
 *									*
 *	Input:	msg_id - The message identifier				*
 *	Output: NONE							*
 *	Return: The last known I/O status				*
 ************************************************************************/

int
Hci_info_io_status (
int	msg_id
)
{
	switch (msg_id) {

	    case HCI_GUI_INFO_MSG_ID :

		return Hci_gui_info_io_status;

	    case HCI_TASK_INFO_MSG_ID :

		return Hci_task_io_status;

	    default:

		break;

	}

	return (-1);

}

/************************************************************************
 *	Description: This function is the LB_NOTIFY callback for	*
 *		     updates to the HCI state info messages.		*
 *									*
 *	Input:	fd       - File descriptor				*
 *		msgid    - Message ID					*
 *		msg_info - Length (bytes) of the new message		*
 *		*arg     - user registered argument (unused)		*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_state_info_update (
int	fd,
LB_id_t	msgid,
int	msg_info,
void	*arg
)
{

	switch (msgid) {
	
	    case HCI_GUI_INFO_MSG_ID :
	    
		Hci_gui_init_flag = 0;
		break;
	
	    case HCI_TASK_INFO_MSG_ID :
	    
		Hci_task_flag = 0;
		break;

	}
}

/************************************************************************
 *	Description: This function returns a pointer to the specified	*
 *		     HCI state info data.  If the data does not exist,	*
 *		     NULL is returned.					*
 *									*
 *	Input:  msg_id - message ID					*
 *	Output: NONE							*
 *	Return: pointer to specified message local data			*
 ************************************************************************/

void
*hci_get_info_msg (
int	msg_id
)
{
	int	status = 1;

	switch (msg_id) {

	    case HCI_GUI_INFO_MSG_ID :

		if (Hci_gui_init_flag == 0) {

		    status = hci_read_info_msg (HCI_GUI_INFO_MSG_ID);
		    
		}

		return (void *) &Hci_gui;

	    case HCI_TASK_INFO_MSG_ID :

		if (Hci_task_flag == 0) {

		    status = hci_read_info_msg (HCI_TASK_INFO_MSG_ID);
		    
		}

		return (void *) &Hci_task[0];

	}

	return (void *) NULL;
}

/************************************************************************
 *	Description: This function returns the update status for the	*
 *		     specified message.					*
 *									*
 *	Input:	msg_id - The message identifier				*
 *	Output: NONE							*
 *	Return: 0 = Message has been updated and needs to be read.	*
 *		1 = Message has not been updated since last read.	*
 ************************************************************************/

int
hci_info_update_status (
int	msg_id
)
{
	switch (msg_id) {

	    case HCI_GUI_INFO_MSG_ID :

		return Hci_gui_init_flag;

	    case HCI_TASK_INFO_MSG_ID :

		return Hci_task_flag;

	    default:

		break;

	}

	return (-1);
}

/************************************************************************
 *	Description: This function returns the current value of the	*
 *		     RDA messages inhibit flag.				*
 *									*
 *	Input:  NONE							*
 *	Output:	NONE							*
 *	Return: Value of the inhibit flag				*
 ************************************************************************/

int
hci_info_inhibit_RDA_messages ()
{
	return Hci_inhibit_RDA_messages;
}

/************************************************************************
 *	Description: This function sets the state of the RDA status	*
 *		    and alarm message inhibit flag.			*
 *									*
 *	Input:  state -  0 = ENABLED or 1 = DISABLED			*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
hci_info_set_inhibit_RDA_messages (
int	state
)
{
	Hci_inhibit_RDA_messages = state;
}

/************************************************************************
 *	Description: This function returns the number of currently	*
 *		     failed RPG tasks.					*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: The number of failed RPG tasks				*
 ************************************************************************/

int
hci_info_failed_task_num ()
{
	int	status;

	if (!Hci_task_flag) {

	    status = hci_read_info_msg (HCI_TASK_INFO_MSG_ID);

	}

	return (Hci_task_num);
}

/***********************************************************************
 *	Description: This function returns the task type of the		*
 *		     specified failed task table item.			*
 *									*
 *	Input:  indx - table index number (should be in range		*
 *		       0 < Hci_task_num)				*
 *	Output: NONE							*
 *	Return: A 0 or 1 on success, otherwise -1.			*
 ************************************************************************/

int
hci_info_failed_task_type (
int	indx
)
{
	int	ret;

/*	Check to see if a valid index specified.  If not, then return	*
 *	-1.								*/

	if ((indx < 0) || (indx >= Hci_task_num)) {

	    ret = -1;

	} else {

	    if (!Hci_task_flag) {

		ret = hci_read_info_msg (HCI_TASK_INFO_MSG_ID);

	    }

	    ret = Hci_task [indx].control_task;

	}

	return (ret);
}

/***********************************************************************
 *	Description: This function returns a pointer to the name of the	*
 *		     specified failed task table item.			*
 *									*
 *	Input:  indx - table index number (should be in range		*
 *		       0 < Hci_task_num)				*
 *	Output: NONE							*
 *	Return: A pointer to the task name on success, otherwise NULL.	*
 ************************************************************************/

char
*hci_info_failed_task_name (
int	indx
)
{
	
/*	Check to see if a valid index specified.  If not, then return	*
 *	NULL.								*/

	if ((indx < 0) || (indx >= Hci_task_num)) {

	    return ((char *) NULL);

	} else {

	    if (!Hci_task_flag) {

		hci_read_info_msg (HCI_TASK_INFO_MSG_ID);

	    }

	    return (Hci_task [indx].name);

	}
}
