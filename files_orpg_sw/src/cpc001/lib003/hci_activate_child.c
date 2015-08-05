/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:25:46 $
 * $Id: hci_activate_child.c,v 1.7 2009/02/27 22:25:46 ccalvert Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */

/************************************************************************
 * Module:      hci_activate_child.c					*
 * Author:      D. Priegnitz - NSSL					*
 * Description: This function is used to manage HCI child processes.	*
 *		The intent is to prevent multiple instances of the	*
 *		specified task.  The first check is to see if any	*
 *		processes exist matching the process_name.  If not,	*
 *		then the task is started.  If a match is found, then	*
 *		the window heirarchy is checked for a match to the	*
 *		window_name.  If one is found, then a call to the	*
 *		function XtMapRaised() is issued so the window is	*
 *		popped to the top of the window heirarchy.		*
 *									*
 * Input:  display      - connection to an X Server.			*
 *	   top_window   - ID of parent window to query			*
 *	   task_command - pointer to the full command string.		*
 *         process_name - pointer to the process name string		*
 *         window_name  - pointer to the X window name.			*
 * Output: None								*
 * Return: Returns a value indicating whether a new task was activated  *
 *	   (1), the window already exists and has been mapped to the	*
 *	   top (2), or if nothing has been done (either the command	*
 *	   has already been started or no command has been specified.	*
 *	   (0).  A negative value is returned on error.			*
 ************************************************************************/

/* Local include definition files. */

#include <hci.h>

Window hci_window_query (Display *d, Window t, char *n);

int
hci_activate_child (
Display	*display,	/* Connection to an X Server */
Window	top_window,     /* ID of parent window to query */
char	*task_command,	/* Pointer to full command string */
char    *process_name,  /* Pointer to process name */
char	*window_name,   /* Pointer to window name */
int	object_index   /* control panel object index */
)
{
    int		status;
    Window	window_id;
    char	buf [256];
    char	tmp_filename [256];
    struct stat	stat_buf;
    FILE	*fd;
    int		lines;
    static int	busy = 0;

/*  If this function hasn't returned yet from a previous instance	*
 *  do nothing and return.						*/

    if (busy)
	return (0);

    busy = 1;

    if ((task_command != NULL) && (process_name != NULL)) {

/*      Create a temporary filename based on this tasks process ID to	*
 *      write process status information.  The process ID ensures the	*
 *      filenames uniqueness.						*/

        sprintf (tmp_filename, "/tmp/hci.proc.%d", (int)getpid());

/*      Use the "ps" command to gather information about the specified	*
 *      process.							*/

        sprintf (buf, "ps -e -o args | grep \"%s\" >%s", process_name, tmp_filename);

        status = MISC_system (buf);

/*      If there was an error return with an error (-1).		*/

        if (status < 0) {

	    HCI_LE_error("Error - system (%s): %d", buf, status);
	    busy = 0;
            return (-1);

        }

/*      Get the status of the just created file.			*/

        status = stat(tmp_filename,&stat_buf);

/*      If there was an error return with an error (-1).		*/

        if (status != 0) {

	    HCI_LE_error("Error - stat(%s) = %d", tmp_filename, status);
	    busy = 0;
            return (-1);

        }

/*      If we got status, then if the task exists, the file will	*
 *      contain more than 1 line.					*/

/*	Open the file containing the ps command results.		*/

        fd = fopen (tmp_filename,"r");

	if (fd == NULL) {

	    busy = 0;
	    HCI_LE_error("Unable to open file %s (errno=%d)",
			tmp_filename, errno);
	    return (0);

	}

	lines = 0;

/*	Determine how many lines are in the file.			*/

	while (!feof (fd)) {

	    if (fgets (buf,80,fd) != NULL) {

/*		The "grep" commandline may exist so we need to filter	*
 *		it out in case it exists.				*/

		if (strstr (buf,"grep") == NULL) {

		    HCI_LE_log("task [%s] found", buf);
		    lines++;

		}
	    }
	}

	fclose (fd);

        if (lines < 1) { /* The task doesn't exist so start it */

	    HCI_LE_log("task [%s] not found so launching new one", process_name);
            system (task_command);
	    busy = 0;
            return (1);

        }
    }

/*  Lets check to see if the window already exists.  If it is,		*
 *  all we need to do is map the window to the top of the heirarchy.	*/

    if (window_name != NULL) {

        window_id = hci_window_query (display, top_window, window_name );

        if (window_id != (Window) 0) {

	    hci_child_started_event_t	task_data;

	    task_data.child_id = object_index;

	    HCI_LE_log("Raising [%s] to top of window heirarchy",window_name);
	    XMapRaised (display, window_id);
	    EN_post (ORPGEVT_HCI_CHILD_IS_STARTED, &task_data,
		     sizeof (hci_child_started_event_t), 0);
	    busy = 0;
	    return (2);

        } else {

	    busy = 0;
	    return (0);

        }
    }

    busy = 0;
    return (-1);

}

int
hci_child_process_active (
char	*app_name
)
{
    char	buf [256];
    char	tmp_filename [256];
    int		status;
    struct stat stat_buf;
    FILE	*fd;

    status = 0;

    if (app_name != NULL) {

/*      Create a temporary filename based on this tasks process ID to	*
 *      write process status information.  The process ID ensures the	*
 *      filenames uniqueness.						*/

        sprintf (tmp_filename, "/tmp/hci.active.proc.%d", (int)getpid());

/*      Use the "ps" command to gather information about the specified	*
 *      process.							*/

        sprintf (buf, "ps -e -o args | grep \"%s\" >%s", app_name, tmp_filename);

        status = MISC_system (buf);

/*      If there was an error return with an error (-1).		*/

        if (status < 0) {

	    HCI_LE_error("Error - system (%s): %d", buf, status);
            return (-1);

        }

/*      Get the status of the just created file.			*/

        status = stat(tmp_filename,&stat_buf);

/*      If there was an error return with an error (-1).		*/

        if (status != 0) {

	    HCI_LE_error("Error - stat(%s) = %d", tmp_filename, status);
            return (-1);

        }

/*      If we got status, then if the task exists, the file will	*
 *      contain more than 1 line.					*/

/*	Open the file containing the ps command results.		*/

        fd = fopen (tmp_filename,"r");

	if (fd == NULL) {

	    HCI_LE_error("Unable to open file %s (errno=%d)",
			tmp_filename, errno);
	    return (0);

	}

	status = 0;

/*	Determine how many lines are in the file.			*/

	while (!feof (fd)) {

	    if (fgets (buf,80,fd) != NULL) {

/*		The "grep" commandline may exist so we need to filter	*
 *		it out in case it exists.				*/

		if (strstr (buf,"grep") == NULL) {

		    status++;

		}
	    }
	}

	fclose (fd);

    }

    return (status);
}
