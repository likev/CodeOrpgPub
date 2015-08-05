
/***********************************************************************

    Description: Internal include file for hci_nb.

***********************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:26:10 $
 * $Id: hci_nb_def.h,v 1.20 2009/02/27 22:26:10 ccalvert Exp $
 * $Revision: 1.20 $
 * $State: Exp $
 */

#ifndef HCI_NB_DEF_H
#define HCI_NB_DEF_H

#include <hci_up_nb.h>

#define	PACKET_SIZE_128	128
#define	PACKET_SIZE_512	512
#define	MAX_DIAL_USERS	500
#define	MAX_CLASSES	100
#define	DIAL_LIST_SIZE   20
#define	LINE_LIST_SIZE   24
#define MAX_LINES       100

#define	CONNECTION_RETRIES_MIN	 	  1
#define	CONNECTION_RETRIES_MAX		999
#define	TRANSMISSION_TIMEOUT_MIN	 60
#define TRANSMISSION_TIMEOUT_MAX	999
#define	CONNECT_TIME_MIN		  1
#define	CONNECT_TIME_MAX	       1440
#define	PERCENT_MIN			  0
#define	PERCENT_MAX			100
#define	BAUD_RATE_MIN		       9600
#define	BAUD_RATE_MAX		    9999999
#define	USER_ID_MIN			  0
#define	USER_ID_MAX		       9999

/* values for argument "cmd" of MAIN_nb_control */
enum {HCI_NB_DISCONNECT, HCI_NB_DISABLE, HCI_NB_ENABLE};

void MAIN_housekeeping ();
Line_details *MAIN_get_line_details (Line_status *ls);
void MAIN_nb_control (int cmd, int n_list_lines, Line_status *list_lines);

int GUI_main (int argc, char **argv);
void GUI_update_line_info (Line_status *line);
void GUI_update_line_list ();

int hci_text_password (int *insert_len);
Widget hci_create_rowcolumn (Widget parent, char *name,
				int orientation);
Widget hci_create_label (Widget parent, char *label, int align);
Widget hci_create_label_text (Widget parent, char *label, int text_size, 
				char *(*callback) ());
Widget hci_create_label_toggle (Widget parent, char *label, 
				void (*callback) ());
Widget hci_create_label_combo (Widget parent, char *label, int n, 
	char **list, int width, XtCallbackProc callback);
Widget hci_create_frame_label (Widget parent, char *label);
Widget hci_create_button (Widget parent, 
			char *label, XtCallbackProc callback);
Widget hci_create_radio_box (Widget parent, char *name, 
		int n, int ind_set, char **label, XtCallbackProc callback);
void hci_freeze_window_size (Widget w);
void hci_set_label_len (int label_len);
void hci_set_editable (int editable);
Widget hci_get_current_rc ();
void Hci_info_dialog (Widget w, char *msg);
void Hci_confirmation_dialog (Widget w, XtPointer client_data, char *text, 
		XtCallbackProc yes_callback, XtCallbackProc no_callback);
int  hci_update_class_table (Class_details *tbl);
int  hci_update_dialup_user_table (Dial_details *tbl);
int  hci_update_dedicated_user_table (Dial_details *tbl);

#endif		/* #ifndef HCI_NB_DEF_H */

