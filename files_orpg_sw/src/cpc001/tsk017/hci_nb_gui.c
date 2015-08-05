/****************************************************************
 *								*
 *	hci_nb_gui.c - GUI routines for hci_nb.			*
 *								*
 ****************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2010/03/10 18:46:53 $
 * $Id: hci_nb_gui.c,v 1.108 2010/03/10 18:46:53 ccalvert Exp $
 * $Revision: 1.108 $
 * $State: Exp $
 */

/*	HCI header files		*/

#include <hci.h>
#include <hci_nb_def.h>

/*	Global variable definitions	*/

#define	PDL_LIST_VISIBLE_LINES	25	/* Number of visible lines in the
					   Product Distribution Lines list */
#define	DIAL_LIST_VISIBLE_LINES	12	/* Number of visible lines in the
					   dial-in users list. */
#define INITIAL_LINE -1

static Widget Top_widget     = (Widget)NULL;
static Widget Line_list;

static Widget Line_list_select_rc    = (Widget) NULL;
static Widget Line_list_select_left  = (Widget) NULL;
static Widget Line_list_select_right = (Widget) NULL;
static Widget Line_list_select_label = (Widget) NULL;
static int    Line_list_min = 0;  /* line entry at top of current page */
static int    Line_list_max = PDL_LIST_VISIBLE_LINES;
				  /* line entry at bottom of current page */

static int    Class_tbl_num = 0; /* number of entries in class table */
static Class_details Class_tbl [MAX_CLASSES]; /* class table */
static int    Dial_tbl_num = 0;  /* number of entries in dial table */
static int    Dial_tbl_min = 0;  /* dial entry at top of current page */
static int    Dial_tbl_max = 0;  /* dial entry at bottom of current page */
static Dial_details Dial_tbl [MAX_DIAL_USERS]; /* dial table */
static int    Dial_change_flag [MAX_DIAL_USERS] = {0}; /* dial table change
				    flags */

static Widget Reset_button;
static Widget Disable_button;
static Widget Enable_button;
static Widget Deselect_button;

static Widget Type_combo;		/* combobox for "type" */
static Widget Type_list;		/* list widget for "type" */
static Widget Type_text;		/* text widget for "type" */
static Widget Packet_combo;		/* combobox for "packet_size" */
static Widget Packet_list;		/* list widget for "type" */
static Widget Packet_text;		/* text widget for "type" */
static unsigned int Ep_type;		/* editing permission bits for "type" */
static unsigned int Ep_packet;		/* editing permission bits for	      *
					 * "packet_size"		      */

/*	The following widgets are used for the dial-in user form.  They	*
	map to entries in the user profile data base.			*/

static Widget Retries_text;
static Widget Timeout_text;
static Widget Alarm_text;
static Widget Warning_text;
static Widget Port_pswd_text;
static Widget Baud_rate_text;
static Widget Line_num_text;
static Widget Pserver_num_text;
static Widget Comm_mgr_num_text;

static Widget Max_conn_time_combo;
static Widget Max_conn_time_text;
static Widget Max_conn_time_list;

static Widget Add_user_id_text     = (Widget) NULL;
static Widget Add_user_name_text   = (Widget) NULL;
static Widget Add_user_pswd_text   = (Widget) NULL;
static Widget Add_user_time_text   = (Widget) NULL;
static Dial_details Add_user;

static int Force_update = 0;		/* != 0 forces line objects to be *
					 * refreshed.			  */
static int N_editable_texts = 0;	/* number of editable text fields */
static Widget Editable_text[2000];	/* list of editable text fields */
static unsigned int Ep_text[2000];	/* editing permission bits */

static int Undo_flag   = 0;		/* Flag to indicate Undo button	*
					 * selected so objects refresh  */
static int Change_flag = 0;		/* any widget edited by the user and
					   not saved */
static int Load_shed_flag = 0;	  	/* If one of the load shed thresholds
					   has changed this is set to 1 */
static int Close_flag = 0;		/* "close" button pushed */
static int Editing_mode = 0;		/* in editing mode */

static int N_list_lines = 0;		/* size of List_lines */
static Line_status *List_lines;		/* Curently displayed contents in 
					   list */
static void *List_tbl = NULL;		/* table id of List_lines */

static Line_details Cr_ld [MAX_LINES];	/* Current displayed line details */
static Line_details Org_ld [MAX_LINES];	/* line details before modification */
static int Cr_alarm   = 0;		/* Current NB alarm threshold */
static int Cr_warning = 0;		/* Current NB warning threshold */
static int Cr_line_num = 1;             /* Current line number */
static int Cr_update [MAX_LINES];	/* Line update flag (one per line) */

enum {SORT_LINE_NUM, SORT_TYPE, SORT_STATUS};
					/* sort selections */
static int Sort_type = SORT_LINE_NUM;	/* current sort selection */
static int List_select_ind = INITIAL_LINE; /* current selection in the list */
static int User_selected_cmd = -1;

static char Buf [512] = {""};		/* general purpose buffer for	*
					 * messages.			*/

typedef struct {
  int status;
  int index;
} line_sort_t;

/*	Function prototypes	*/

static void Close_button_callback (Widget w, XtPointer client_data,
					XtPointer call_data);
static void Control_buttons_callback (Widget w, XtPointer client_data,
					XtPointer call_data);
static void Control_button_yes (Widget w, XtPointer client_data,
					XtPointer call_data);
static void Max_conn_time_callback (Widget w, XtPointer client_data,
					XtPointer call_data);
static char *Get_line_type_str (int type);
static int Get_selected_lines ();
static void timer_proc();
static void Line_list_callback (Widget w, XtPointer client_data,
					XtPointer call_data);
static void Packet_callback (Widget w, XtPointer client_data,
					XtPointer call_data);
static void Set_list_string (Line_status *line);
static void Sort_list ();
static void Sort_rb_callback (Widget w, XtPointer client_data,
					XtPointer call_data);
static char *Text_callback (Widget w, char *text, int status);
static void Type_callback (Widget w, XtPointer client_data,
					XtPointer call_data);
static void Update_change_flag (Widget w, XtPointer client_data,
				XtPointer call_data);
static void Update_class_info ();
static void Update_line_objects (Widget w, XtPointer client_data,
				XtPointer call_data);
static void hci_line_arrow_callback (Widget w, XtPointer client_data,
					XtPointer call_data);
static void hci_sort_dial_user_table (Dial_details *dial_tbl, int table_size);
int  sort_by_status( const void *aa, const void *bb );

/**********************************************************************

    Description: The GUI main function: Creating all GUI objects and
		calling the MOTIF main event loop.

    Input:  argc - number of commandline argiments
	    argv - pointer to commandline argument data
    Output: NONE
    Return: -1 on failure. Otherwise the function will never return.

***********************************************************************/

int GUI_main (int argc, char **argv)
{
    Widget form, lform, rform, line_form;
    Widget frame;
    Widget label;
    Widget rc, sort_rc, rc1, lrc;
    Widget trc;
    int n;
    Arg args[16];
    char *labels[16];
    XmString str;
    Dimension	height;

/*  Get reference to top-level widget.					*/

    Top_widget = HCI_get_top_widget();

/*  Create a table to hold line status data for all defined lines.	*/

    List_tbl = MISC_open_table (sizeof (Line_status), 
			22, 0, &N_list_lines, (char **)&List_lines);
    if (List_tbl == NULL) {
	HCI_LE_error("malloc failed");
	return (-1);
    }

/*  Initialize the line number element in the current line details	*/

    Cr_ld [Cr_line_num].line_num = -1;

/*  Clear all elements of the line update array				*/

    memset (Cr_update, 0, sizeof(int)*MAX_LINES);

/*  Initialize the old copy of line details to current.			*/

    memcpy (&Org_ld [Cr_line_num], &Cr_ld [Cr_line_num], sizeof (Line_details));

/*  Read the class information from the user profile data base.		*/

    HCI_LE_log("Initializing class table");
    Class_tbl_num = hci_update_class_table (Class_tbl);
    
/*  Read the dialup user profile information from the user profile	*
 *  data base.								*/

    HCI_LE_log("Initializing dial user table");
    Dial_tbl_num = hci_update_dialup_user_table (Dial_tbl);

/*  Sort the table in increasing order of user ID.			*/

    HCI_LE_log("Sorting dial user table");
    hci_sort_dial_user_table (Dial_tbl, Dial_tbl_num);
    
/*  The displayed dialup table size needs to be controlled in order	*
 *  to minimize the table update time.  If the number of dialup users	*
 *  exceeds the table size, then we need to allow paging.		*/

    Dial_tbl_min = 0;

    if (Dial_tbl_num > DIAL_LIST_SIZE) {

	Dial_tbl_max = DIAL_LIST_SIZE;

    } else {

	Dial_tbl_max = Dial_tbl_num;

    }

/*  Use a form widget as the manager for the entire window.	*/

    form = XtVaCreateWidget ("form",
		xmFormWidgetClass,	Top_widget,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

/*  The main window is to be divided into two parts.  Create a	*
 *  form widget for the left part and one for the right part.	*/

    lform = XtVaCreateWidget ("lform",
		xmFormWidgetClass,	form,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_FORM,
		NULL);

    rform = XtVaCreateWidget ("rform",
		xmFormWidgetClass,	form,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_WIDGET,
		XmNleftWidget,		lform,
		NULL);

/*  Create a rowcolumn widget at the top of the left form to	*
 *  be used to organize window and file control buttons.	*/

    rc = hci_create_rowcolumn (lform, "close_rc", XmHORIZONTAL);
    XtVaSetValues (rc,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

/*  The first button (left) is the Close button.  It is used to	*
 *  end the task.						*/

    hci_create_button (rc, "Close", Close_button_callback);

    XtManageChild (rc);

/*  In the left form and below the top rowcolumn create a frame	*
 *  to hold product distribution line information.		*/

    frame = XtVaCreateManagedWidget ("line_frame",
		xmFrameWidgetClass,	lform,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		rc,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

    hci_create_frame_label (frame, "Product Distribution Lines");

/*  Create a rowcolumn widget to manage the product distribution*
 *  line status table.						*/

    rc = hci_create_rowcolumn (frame, "comms_lines_rc", XmVERTICAL);

/*  Create a set of labels to be displayed above each column of	*
 *  the table.							*/

    label = hci_create_label (rc, 
	"Line  Type  Enabled Proto  ID   User Name    Class    Status   Delay  Rate  ", 
	XmALIGNMENT_BEGINNING);

/*  Create a list widget to manage the product distribution line*
 *  status table.						*/

    XtVaGetValues (label,
	XmNheight,	&height,
	NULL);

    n = 0;
    XtSetArg (args[n], XmNforeground, hci_get_read_color (TEXT_FOREGROUND));  n++;
    XtSetArg (args[n], XmNbackground, hci_get_read_color (BACKGROUND_COLOR2));  n++;

    XtSetArg (args[n], XmNwidth, 300);                         n++;
    XtSetArg (args[n], XmNheight, height*PDL_LIST_VISIBLE_LINES);  n++;
    XtSetArg (args[n], XmNscrollingPolicy, XmAUTOMATIC);       n++;
    XtSetArg (args[n], XmNfontList, hci_get_fontlist (LIST));  n++;

    Line_list = XmCreateList (rc, "link_scroll", args, n);
    XtVaSetValues (Line_list,
		XmNselectionPolicy, XmEXTENDED_SELECT,
		XmNfontList,	hci_get_fontlist (LIST),
		XmNvisibleItemCount, PDL_LIST_VISIBLE_LINES,
		XmNdoubleClickInterval, 400,
		NULL);

    XtAddCallback (Line_list,
		XmNdefaultActionCallback, Line_list_callback, NULL);

    XtManageChild (Line_list);

/*  Beneath the table create a rowcolumn widget containing a	*
 *  set of buttons for controlling the table sort order.	*/

    sort_rc = hci_create_rowcolumn (rc, "sort_rc", XmHORIZONTAL);

    Line_list_select_rc = hci_create_rowcolumn (sort_rc, "line_select_rc", XmHORIZONTAL);

    XtVaCreateManagedWidget ("Prev",
	xmLabelWidgetClass,	Line_list_select_rc,
	XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
	XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
	XmNfontList,		hci_get_fontlist (LIST),
	NULL);

    Line_list_select_left = XtVaCreateManagedWidget ("line_select_left",
	xmArrowButtonWidgetClass,	Line_list_select_rc,
	XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
	XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
	XmNborderWidth,		0,
	XmNarrowDirection,	XmARROW_LEFT,
	XmNsensitive,		False,
	NULL);

    XtAddCallback (Line_list_select_left,
	XmNactivateCallback, hci_line_arrow_callback,
	(XtPointer) XmARROW_LEFT);

    Line_list_select_label = XtVaCreateManagedWidget ("line_select_label",
	xmLabelWidgetClass,	Line_list_select_rc,
	XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
	XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
	XmNfontList,		hci_get_fontlist (LIST),
	NULL);

    sprintf (Buf,"---");

    str = XmStringCreateLocalized (Buf);

    XtVaSetValues (Line_list_select_label,
	XmNlabelString,	str,
	NULL);

    XmStringFree (str);

    Line_list_select_right = XtVaCreateManagedWidget ("line_list_select_left",
	xmArrowButtonWidgetClass,	Line_list_select_rc,
	XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
	XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
	XmNborderWidth,		0,
	XmNarrowDirection,	XmARROW_RIGHT,
	NULL);

    XtAddCallback (Line_list_select_right,
	XmNactivateCallback, hci_line_arrow_callback,
	(XtPointer) XmARROW_RIGHT);

    XtVaCreateManagedWidget ("Next  ",
	xmLabelWidgetClass,	Line_list_select_rc,
	XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
	XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
	XmNfontList,		hci_get_fontlist (LIST),
	NULL);

    XtManageChild (Line_list_select_rc);

    hci_create_label (sort_rc, "Sorted By: ", XmALIGNMENT_CENTER);

/*  Create a set of radio buttons for all sort options.		*/

    labels[0] = "Line";
    labels[1] = "Type";
    labels[2] = "Status";
    hci_create_radio_box (sort_rc, "sort_rb", 3, 0, labels, Sort_rb_callback);

    XtManageChild (sort_rc);
    XtManageChild (rc);

/*  Below the product distribution lines frame create a frame	*
 *  to contain a set of buttons to control line state.		*/

    frame = XtVaCreateManagedWidget ("control_frame",
		xmFrameWidgetClass,	lform,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		frame,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

    hci_create_frame_label (frame, "Line Control");

/*  Inside the frame use a rowcolumn widget to manage the line	*
 *  control buttons.						*/

    rc1 = hci_create_rowcolumn (frame, "control_rc", XmHORIZONTAL);

/*  Create buttons to reset, disable, enable, and deselect	*
 *  lines.  Map their widget IDs to global variables so their	*
 *  sensitivities can be controlled.				*/

    hci_create_label (rc1, "  ", XmALIGNMENT_BEGINNING);
    Reset_button = hci_create_button 
			(rc1, "   Reset    ", Control_buttons_callback);
    hci_create_label (rc1, "  ", XmALIGNMENT_BEGINNING);
				/* insert space between the two buttons */
    Disable_button = hci_create_button 
			(rc1, " Disconnect ", Control_buttons_callback);
    hci_create_label (rc1, "  ", XmALIGNMENT_BEGINNING);
				/* insert space between the two buttons */
    Enable_button = hci_create_button 
			(rc1, "  Connect   ", Control_buttons_callback);
    hci_create_label (rc1, "  ", XmALIGNMENT_BEGINNING);
				/* insert space between the two buttons */
    Deselect_button = hci_create_button 
			(rc1, "  Deselect  ", Control_buttons_callback);

    XtManageChild (rc1);

/*  At the bottom of the left frame create a frame which is	*
 *  used to display/modify general parameters (applied to all	*
 *  lines.							*/

    frame = XtVaCreateManagedWidget ("general_parameter_frame",
		xmFrameWidgetClass,	lform,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		frame,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

    hci_create_frame_label (frame, "General Parameters");

/*  Use two rowcolumn widgets to manage a set of text widgets	*
 *  for displaying/modifying general line parameters.		*/

    rc1 = hci_create_rowcolumn (frame, "management_list",  XmHORIZONTAL);

    hci_set_label_len (8);
    hci_set_editable (False);

/*  Create a text widget for controlling the number of line	*
 *  retries.							*/

    Retries_text =  hci_create_label_text (rc1, "Retries", 3, 
			Text_callback);
    Ep_text[N_editable_texts] = 
	    HCI_LOCA_ROC;
    Editable_text[N_editable_texts++] = Retries_text;

/*  Create a text widget for controlling a global line timeout	*
 *  value.							*/

    Timeout_text =  hci_create_label_text (rc1, "Timeout", 3,
			Text_callback);
    Ep_text[N_editable_texts] = 
	    HCI_LOCA_ROC;
    Editable_text[N_editable_texts++] = Timeout_text;

/*  Create a text widget for controlling the narrowband load	*
 *  shed alarm threshold.					*/

    hci_set_label_len (12);
    Alarm_text =  hci_create_label_text (rc1, "Alarm (%)", 3,
			Text_callback);
    Ep_text[N_editable_texts] = 
	    HCI_LOCA_ROC;
    Editable_text[N_editable_texts++] = Alarm_text;

/*  Create a text widget for controlling the narrowband load	*
 *  shed warning threshold.					*/

    Warning_text =  hci_create_label_text (rc1, "Warning (%)", 3,
			Text_callback);
    Ep_text[N_editable_texts] = 
	    HCI_LOCA_ROC;
    Editable_text[N_editable_texts++] = Warning_text;

    XtManageChild (rc1);

    frame = XtVaCreateManagedWidget ("management_frame",
		xmFrameWidgetClass,	rform,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

    hci_create_frame_label (frame, "Line Info");

/*  Use a form widget to manage widgets in the line management	*
 *  frame.							*/

    line_form = XtVaCreateWidget ("line_management_form",
	xmFormWidgetClass,	frame,
	XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
	NULL);

    hci_set_label_len (12);

    lrc = hci_create_rowcolumn (line_form, "management_list",  XmVERTICAL);

    XtVaSetValues (rc,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNleftAttachment,	XmATTACH_FORM,
	NULL);

/*  Create a text (not editable) widget for the line number	*/

    Line_num_text =  hci_create_label_text (lrc, "Line #", 10, NULL);

/*  Create a dropdown list menu for the line type		*/

    labels[0] = "Dedicated";
    labels[1] = "Dial-in";
    labels[2] = "Wan";
    Type_combo =  
	hci_create_label_combo (lrc, "Type", 3, labels, 120, Type_callback);
    Type_list = XtNameToWidget (Type_combo, "*List");
    Type_text = XtNameToWidget (Type_combo, "*Text");
    XtVaSetValues (Type_list, XmNsensitive, False, NULL);
    XtVaSetValues (Type_text, /* XmNvalue, "       ",*/
		XmNforeground, hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground, hci_get_read_color (BACKGROUND_COLOR1),
		NULL);
    Ep_type = HCI_LOCA_ROC | HCI_LOCA_AGENCY;

/*  Create a text (editable) widget for the line port password	*/

    Port_pswd_text = hci_create_label_text (lrc, "Port Pswd", 4,
			Text_callback);
    Ep_text[N_editable_texts] = 
	    HCI_LOCA_ROC | HCI_LOCA_AGENCY;
    Editable_text[N_editable_texts++] = Port_pswd_text;

/*  Create a text (editable) widget for the line baud rate	*/

    Baud_rate_text = hci_create_label_text (lrc, "Baud Rate", 7,
			Text_callback);
    Ep_text[N_editable_texts] = 
	    HCI_LOCA_ROC | HCI_LOCA_AGENCY;
    Editable_text[N_editable_texts++] = Baud_rate_text;

/*    XtManageChild (lrc);*/
/**/
/*    rrc = hci_create_rowcolumn (line_form, "management_list",  XmVERTICAL);*/
/**/
/*    XtVaSetValues (rrc,*/
/*	XmNtopAttachment,	XmATTACH_FORM,*/
/*	XmNleftAttachment,	XmATTACH_WIDGET,*/
/*	XmNleftWidget,		lrc,*/
/*	NULL);*/

/*  Create a text (not editable) widget for the line comm manager #	*/

    Comm_mgr_num_text = hci_create_label_text (lrc, "Comm Mgr #", 3,
			NULL);

/*  Create a text (not editable) widget for the line p server #	*/

    Pserver_num_text = hci_create_label_text (lrc, "PServer #", 3, NULL);

/*  Create a text (editable) widget for the line time limit	*/

    trc = XtVaCreateWidget ("time_limit_rowcol",
	xmRowColumnWidgetClass,	lrc,
	XmNorientation,	XmHORIZONTAL,
	XmNpacking,	XmPACK_TIGHT,
	XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
	NULL);

    XtVaCreateManagedWidget ("  Time Limit",
	xmLabelWidgetClass,	trc,
	XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
	XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
	XmNfontList,	hci_get_fontlist (LIST),
	NULL);

    Max_conn_time_combo = XtVaCreateWidget ("max_conn_time_combo",
		xmComboBoxWidgetClass,	trc,
		XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,	hci_get_read_color (TEXT_BACKGROUND),
		XmNfontList,	hci_get_fontlist (LIST),
		XmNcomboBoxType,	XmDROP_DOWN_COMBO_BOX,
		XmNwidth,		65,
		XmNborderWidth,		0,
		XmNmarginHeight,	0,
		XmNvisibleItemCount,	2,
		NULL);

    XtVaGetValues (Max_conn_time_combo,
		XmNlist,	&Max_conn_time_list,
		XmNtextField,	&Max_conn_time_text,
		NULL);

    str = XmStringCreateLocalized ("Class");
    XmListAddItemUnselected (Max_conn_time_list, str, 0);
    XmStringFree (str);

    XtVaSetValues (Max_conn_time_text,
		XmNcolumns,	4,
		NULL);
    XtManageChild (Max_conn_time_combo);
    XtManageChild (trc);

    Ep_text[N_editable_texts] = 
	HCI_LOCA_ROC | HCI_LOCA_AGENCY;
    Editable_text[N_editable_texts++] = Max_conn_time_combo;
    Ep_text[N_editable_texts] = 
	HCI_LOCA_ROC | HCI_LOCA_AGENCY;
    Editable_text[N_editable_texts++] = Max_conn_time_list;
    Ep_text[N_editable_texts] = 
	HCI_LOCA_ROC | HCI_LOCA_AGENCY;
    Editable_text[N_editable_texts++] = Max_conn_time_text;

    XtAddCallback (Max_conn_time_combo,
	XmNselectionCallback, Max_conn_time_callback, NULL);
    XtAddCallback (Max_conn_time_text,
	XmNactivateCallback, Max_conn_time_callback, NULL);
    XtAddCallback (Max_conn_time_text,
	XmNmodifyVerifyCallback, hci_verify_unsigned_integer_callback,
	(XtPointer) 4);
    XtAddCallback (Max_conn_time_text,
	XmNlosingFocusCallback, Max_conn_time_callback, NULL);

/*  Create a dropdown list menu for the comms option (packet size) */

    labels[0] = "No";
    labels[1] = "Yes";
    Packet_combo =  
	hci_create_label_combo (lrc, "Comms Option", 2, labels, 55, Packet_callback);
    Packet_list = XtNameToWidget (Packet_combo, "*List");
    Packet_text = XtNameToWidget (Packet_combo, "*Text");
    XtVaSetValues (Packet_list, XmNsensitive, False, NULL);
    XtVaSetValues (Packet_text, /* XmNvalue, "       ",*/
		XmNforeground, hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground, hci_get_read_color (BACKGROUND_COLOR1),
		NULL);
    Ep_packet = HCI_LOCA_ROC | HCI_LOCA_AGENCY;

    XtManageChild (lrc);
    XtManageChild (line_form);


    XtManageChild (lform);
    XtManageChild (rform);
    XtManageChild (form);

    XtPopup (Top_widget, XtGrabNone);
    Update_change_flag (Top_widget, NULL, NULL);

    /* Start HCI loop. */

    HCI_start( timer_proc, HCI_TWO_SECONDS, NO_RESIZE_HCI );

    return 0;
}

/**********************************************************************

    Description: Timer callback function.

    Input:  NONE
    Output: NONE
    Return: NONE

***********************************************************************/

static void timer_proc()
{
	int	comms_relay_state;
static	int	old_comms_relay_state = -1;
static	int	first_time = 1;

	if (first_time)
	{
	   HCI_PM( "Reading narrowband data" );		
	}
	
/*	Call the main housekeeping function to update if needed		*/

	MAIN_housekeeping();
	if (first_time)
	{
	   if (N_list_lines <= PDL_LIST_VISIBLE_LINES) {

		XtVaSetValues (Line_list,
			XmNvisibleItemCount, N_list_lines,
			NULL);

		XtVaSetValues (Line_list_select_left,
			XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
			XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
			XmNsensitive,	False,
			NULL);
		XtVaSetValues (Line_list_select_right,
			XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
			XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
			XmNsensitive,	False,
			NULL);

	   }

	   first_time = 0;	
	}

/*	If this site is FAA redundant, we need to check the state of	*
 *	the comms relay box to see if this channel has it.  If so, the	*
 *	"Reset", "Connect", and "Disconnect" buttons should be		*
 *	sensitized.  Otherwise they should be desensitized.		*/

        if (HCI_get_system() == HCI_FAA_SYSTEM) {

	    comms_relay_state = ORPGRED_comms_relay_state (ORPGRED_MY_CHANNEL);

	    if (comms_relay_state != old_comms_relay_state) {

		if (comms_relay_state == ORPGRED_COMMS_RELAY_ASSIGNED) {

		    XtVaSetValues (Enable_button,
			XmNsensitive,	True,
			NULL);
		    XtVaSetValues (Disable_button,
			XmNsensitive,	True,
			NULL);
		    XtVaSetValues (Reset_button,
			XmNsensitive,	True,
			NULL);

		} else {

		    XtVaSetValues (Enable_button,
			XmNsensitive,	False,
			NULL);
		    XtVaSetValues (Disable_button,
			XmNsensitive,	False,
			NULL);
		    XtVaSetValues (Reset_button,
			XmNsensitive,	False,
			NULL);

		}

		old_comms_relay_state = comms_relay_state;

	    }
	}
}

/**********************************************************************

    Description: Close button callback function.

    Input:  w - Close widget ID
	    client_data - unused
	    call_data - unused
    Output: NONE
    Return: NONE

***********************************************************************/

static void Close_button_callback (Widget	w, XtPointer client_data,
					XtPointer call_data)
{
    HCI_LE_log("Task Status Close selected");
    Close_flag = 1;
    HCI_task_exit( HCI_EXIT_SUCCESS ); 
}

/**********************************************************************

    Description: Control buttons callback function.

    Input:  w - widget ID of button invoking action
	    client_data - unused
	    call_data - unused
    Output: NONE
    Return: NONE

***********************************************************************/

static void Control_buttons_callback (Widget w, XtPointer client_data,
					XtPointer call_data)
{
    int k;
    int n_sel;

/*  Check which button was selected and handle accordingly	*/

    if (w == Deselect_button) {
	XtVaSetValues (Line_list, XmNselectedItemCount, 0, NULL);

        for (k = 0; k < N_list_lines; k++)
	    List_lines[k].selected = 0;

	return;

    }
    
    n_sel = Get_selected_lines ();
    if (n_sel == 0) {
	hci_warning_popup( Top_widget, "No line(s) selected. You must first\nselect one or more lines.", NULL );
	return;
    }
    else {
	char msg[128], *p;
	int cnt, i;

	if (w == Reset_button)
	{
	    User_selected_cmd = HCI_NB_DISCONNECT;
	    if (n_sel < 2)
	        sprintf (msg,"Do you want to reset the selected line?\nLine selected: ");
	    else
	        sprintf (msg,"Do you want to reset the selected %d lines?\nLines selected: ", n_sel);
	}
	else if (w == Disable_button)
	{
	    User_selected_cmd = HCI_NB_DISABLE;
	    if (n_sel < 2)
	        sprintf (msg,"Do you want to disconnect the selected line?\nLine selected: ");
	    else
	        sprintf (msg,"Do you want to disconnect the selected %d lines?\nLines selected: ", n_sel);
	}
	else if (w == Enable_button)
	{
	    User_selected_cmd = HCI_NB_ENABLE;
	    if (n_sel < 2)
	        sprintf (msg,"Do you want to connect the selected line?\nLine selected: ");
	    else
	        sprintf (msg,"Do you want to connect the selected %d lines?\nLines selected: ", n_sel);
	}
	else
	    return;

	p = msg + strlen (msg);
	cnt = 0;
	for (i = 0; i < N_list_lines; i++) {
	    if (List_lines[i].selected) {
		if (cnt >= 8) {
		    strcpy (p, "... ");
		    p += 4;
		    break;
		}
		sprintf (p, "%3d ", List_lines[i].line_num);
		p += 4;
		cnt++;
	    }
	}
	hci_confirm_popup( Top_widget, msg, Control_button_yes, NULL );
    }
}

/**********************************************************************

    Description: Control button "Yes" confirmation callback function.
		A control command is sent to the main module.

    Input:  w - Yes button widget ID
	    client_data - widget ID of control button
	    call_data - unused
    Output: NONE
    Return: NONE

***********************************************************************/

static void Control_button_yes (Widget w, XtPointer client_data,
				XtPointer call_data)
{
    MAIN_nb_control (User_selected_cmd, N_list_lines, List_lines);
}

/**********************************************************************

    Description: Line list widget callback function.

    Input:  w - parent widget ID
	    client_data - unused
	    call_data - list widget data
    Output: NONE
    Return: NONE

***********************************************************************/

static void Line_list_callback (Widget w, XtPointer client_data,
					XtPointer call_data)
{
    XmListCallbackStruct *cbs = (XmListCallbackStruct *) call_data;

    if (List_select_ind != cbs->item_position - 1) {
	List_select_ind = cbs->item_position - 1;
	Update_line_objects (w, client_data, call_data);
    }
}

/**********************************************************************

    Description: Returns the currently selected lines in the line list.
		If the number of selected lines is not zero, it also
		sets the selected field in the List_lines structure.

    Input:  NONE
    Output: NONE
    Return: The number of selected lines in the list.

***********************************************************************/

static int Get_selected_lines ()
{
    int n_selected;
    XmString *str;
    int i, k;

    for (k = Line_list_min; k < Line_list_max; k++)
	List_lines[k].selected = 0;

/*  Get the number of selected items in the list			*/

    XtVaGetValues (Line_list, XmNselectedItemCount, &n_selected, 
		XmNselectedItems, &str, NULL);

    if (n_selected == 0) {

	return (0);

    } else {

        for (k = 0; k < Line_list_min; k++)
	    List_lines[k].selected = 0;
        for (k = Line_list_max; k < N_list_lines; k++)
	    List_lines[k].selected = 0;

    }

/*  Set the select flag for each selected line.				*/

    for (i = 0; i < n_selected; i++) {
	char *text;
	int line_num;

	XmStringGetLtoR (str[i], XmFONTLIST_DEFAULT_TAG, &text);
	if (sscanf (text, "%d", &line_num) != 1) {
	    HCI_LE_error("bad list line text (%s)", text);
	    XtFree (text);
	    return (0);
	}
	for (k = 0; k < N_list_lines; k++) {
	    if (List_lines[k].line_num == line_num) {
		List_lines[k].selected = 1;
		break;
	    }
	}
	XtFree (text);
    }
    return (n_selected);
}

/**********************************************************************

    Description: Sort radio box widget callback function.

    Input:  w - parent button widget ID
	    client_data - sort method
	    call_data - toggle data
    Output: NONE
    Return: NONE

***********************************************************************/

static void Sort_rb_callback (Widget w, XtPointer client_data,
					XtPointer call_data)
{
    XmToggleButtonCallbackStruct *state =
		(XmToggleButtonCallbackStruct *) call_data;
    int i = (int)client_data;

    if (state->set && i != Sort_type) {
	Sort_type = i;
	Sort_list ();
	GUI_update_line_list ();
    }	
	
}

/**********************************************************************

    Description: Type combo box widget callback function.

    Input:  w - parent widget ID
	    client_data - unused
	    call_data - combo data
    Output: NONE
    Return: NONE

***********************************************************************/

static void Type_callback (Widget w, XtPointer client_data,
					XtPointer call_data)
{
    int	i;
    int	class_ok = 0;
    XmComboBoxCallbackStruct *cbs =
		(XmComboBoxCallbackStruct *) call_data;

    if (cbs->event != NULL) {

	Cr_ld [Cr_line_num].type = cbs->item_position;
	if (cbs->item_position == 2)
	    Cr_ld [Cr_line_num].type = WAN_LINE;
	Update_change_flag (w, client_data, call_data);

/*	If the selected line type is dialin, we need to sensitize	*
 *	the port password and time limit items if unlocked.		*/

	if (Cr_ld [Cr_line_num].type == DIAL_IN || Cr_ld [Cr_line_num].type == WAN_LINE) {

/*	    We need to check the line class to see if it is still valid	*
 *	    for dialin users.  If not, then we need to set the line	*
 *	    class to a valid one (the first one found).			*/

	    class_ok = 0;

	    if (Cr_ld [Cr_line_num].uclass > 0) {

	        for (i=0;i<Class_tbl_num;i++) {

		    if (Cr_ld [Cr_line_num].uclass == Class_tbl[i].class) {

			if ((Class_tbl[i].line_ind == CLASS_ALL) ||
			    (Class_tbl[i].line_ind == CLASS_DIAL)) {

			    class_ok = 1;
			    break;

			} else {

			    class_ok = 0;
			    break;

			}
		    }
		}
	    }

	    if (!class_ok) {

		Cr_ld [Cr_line_num].uclass = 0;

	        for (i=0;i<Class_tbl_num;i++) {

		    if ((Class_tbl[i].line_ind == CLASS_ALL) ||
			(Class_tbl[i].line_ind == CLASS_DIAL)) {

			Cr_ld [Cr_line_num].uclass = Class_tbl[i].class;
			break;

		    }
		}
	    }

	    if (Editing_mode) {

		XtVaSetValues (Port_pswd_text,
			XmNbackground,	hci_get_read_color (EDIT_BACKGROUND),
			XmNsensitive,	True,
			NULL);
		XtVaSetValues (Max_conn_time_combo,
			XmNbackground,	hci_get_read_color (EDIT_BACKGROUND),
			XmNsensitive,	True,
			NULL);
		XtVaSetValues (Max_conn_time_text,
			XmNbackground,	hci_get_read_color (EDIT_BACKGROUND),
			XmNsensitive,	True,
			NULL);
		XtVaSetValues (Max_conn_time_list,
			XmNbackground,	hci_get_read_color (BUTTON_BACKGROUND),
			XmNforeground,	hci_get_read_color (BUTTON_FOREGROUND),
			XmNsensitive,	True,
			NULL);

	    } else {

		XtVaSetValues (Port_pswd_text,
			XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
			XmNsensitive,	False,
			NULL);
		XtVaSetValues (Max_conn_time_combo,
			XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
			XmNsensitive,	False,
			NULL);
		XtVaSetValues (Max_conn_time_text,
			XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
			XmNsensitive,	False,
			NULL);
		XtVaSetValues (Max_conn_time_list,
			XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
			XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
			XmNsensitive,	False,
			NULL);

	    }

        } else {

/*	    We need to check the line class to see if it is still valid	*
 *	    for dedicated users.  If not, then we need to set the line	*
 *	    class to a valid one (the first one found).			*/

	    class_ok = 0;

	    if (Cr_ld [Cr_line_num].uclass > 0) {

	        for (i=0;i<Class_tbl_num;i++) {

		    if (Cr_ld [Cr_line_num].uclass == Class_tbl[i].class) {

			if ((Class_tbl[i].line_ind == CLASS_ALL) ||
			    (Class_tbl[i].line_ind == CLASS_DEDICATED)) {

			    class_ok = 1;
			    break;

			} else {

			    class_ok = 0;
			    break;

			}
		    }
		}
	    }

	    if (!class_ok) {

		Cr_ld [Cr_line_num].uclass = 0;

	        for (i=0;i<Class_tbl_num;i++) {

		    if ((Class_tbl[i].line_ind == CLASS_ALL) ||
			(Class_tbl[i].line_ind == CLASS_DEDICATED)) {

			if (strlen (Class_tbl[i].name) && (Class_tbl [i].distri_method == 0)) {

			    if (!strncmp (Class_tbl[i].name,"RPGO",4)) {

				Cr_ld [Cr_line_num].uclass = Class_tbl[i].class;
				break;

			    }
			}
		    }
		}
	    }

	    XtVaSetValues (Port_pswd_text,
		XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
		XmNsensitive,	False,
		NULL);
	    XtVaSetValues (Max_conn_time_combo,
		XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
		XmNsensitive,	False,
		NULL);
	    XtVaSetValues (Max_conn_time_list,
		XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
		XmNsensitive,	False,
		NULL);
	    XtVaSetValues (Max_conn_time_text,
		XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
		XmNsensitive,	False,
		NULL);

	}
    }

    Update_class_info (&Cr_ld [Cr_line_num]);
}

/**********************************************************************

    Description: Packet Size combo box widget callback function.

    Input:  w - parent widget ID
	    client_data - unused
	    call_data - combo data
    Output: NONE
    Return: NONE

***********************************************************************/

static void Packet_callback (Widget w, XtPointer client_data,
					XtPointer call_data)
{
    XmComboBoxCallbackStruct *cbs =
		(XmComboBoxCallbackStruct *) call_data;

    if (cbs->event != NULL) {
	switch (cbs->item_position) {
	    case 0:
		Cr_ld [Cr_line_num].packet_size = PACKET_SIZE_128;
		Update_change_flag (w, client_data, call_data);
		break;
	    case 1:
		Cr_ld [Cr_line_num].packet_size = PACKET_SIZE_512;
		Update_change_flag (w, client_data, call_data);
		break;
	}
    }
}

/**********************************************************************

    Description: Verifies if the user made any changes and sets the
		change flag accordingly. It also updates the save button
		sensitivity.

    Input:  w - widget ID
	    client_data - unused
	    call_data - unused
    Output: NONE
    Return: NONE

***********************************************************************/

static void Update_change_flag (Widget w, XtPointer client_data,
					XtPointer call_data)
{

    if (memcmp (&Cr_ld [Cr_line_num], &Org_ld [Cr_line_num], sizeof (Line_details)) == 0) {
	Cr_update [Cr_line_num] = 0;
    } else {
	Change_flag = 1;
	Cr_update [Cr_line_num] = 1;
    }

}

/**********************************************************************

    Description: Text widgets callback - verifies the text inputs and
		save the completed inputs.

    Input:  w - Text widget ID
	    text - new text widget data
	    done - 1 = data entry completed

***********************************************************************/

static char *Text_callback (Widget w, char *text, int done)
{
    int v;
    char c;
    char buf [128];

    if (Editing_mode) {

	if (!done) {		/* not done */

/*	    Skip validating the string types and the user ID type if	*
 *	    the line is dialin.						*/

	    if ((w != Port_pswd_text) &&
		(w != Add_user_name_text) &&
		(w != Add_user_pswd_text)) {

		if (strlen (text) > 0 && sscanf (text, "%d%c", &v, &c) != 1) {
			return (NULL);
		}
	    }
	}
	else {
	    int v;
	    int found;
	    int i;

/*	    Validate the numeric entry fields first.			*/

	    if ((w != Port_pswd_text)     &&
		(w != Add_user_name_text) &&
		(w != Add_user_pswd_text)) {

		sscanf (text, "%d", &v);

/*		Validate number of connection retries entry		*/

		if (w == Retries_text) {

		    if ((v < CONNECTION_RETRIES_MIN) ||
			(v > CONNECTION_RETRIES_MAX) ||
			(!hci_number_found(text))) {

		 	sprintf (buf,"%d", Cr_ld [Cr_line_num].retries);
			XmTextSetString (w, buf);
			sprintf (Buf,"The value you entered is invalid.\nIt must be in the range %d to %d.",
				CONNECTION_RETRIES_MIN, CONNECTION_RETRIES_MAX);
			return (Buf);

		    } else {

			Cr_ld [Cr_line_num].retries = v;
	    		Update_change_flag (w, NULL, NULL);

		    }

/*		Validate maximum message transmission time entry	*/

		} else if (w == Timeout_text) {

		    if ((v < TRANSMISSION_TIMEOUT_MIN) ||
			(v > TRANSMISSION_TIMEOUT_MAX) ||
			(!hci_number_found(text))) {

		 	sprintf (buf,"%d", Cr_ld [Cr_line_num].timeout);
			XmTextSetString (w, buf);
			sprintf (Buf,"The value you entered is invalid.\nIt must be in the range %d to %d.",
				TRANSMISSION_TIMEOUT_MIN, TRANSMISSION_TIMEOUT_MAX);
			return (Buf);

		    } else {

			Cr_ld [Cr_line_num].timeout = v;
	    		Update_change_flag (w, NULL, NULL);

		    }

/*		Validate maximum connect time entry			*/

		} else if (w == Max_conn_time_text) {

		    if ((v < CONNECT_TIME_MIN) ||
			(v > CONNECT_TIME_MAX) ||
			(!hci_number_found(text))) {

		 	sprintf (buf,"%d", Cr_ld [Cr_line_num].max_conn_time);
			XmTextSetString (w, buf);
			sprintf (Buf,"The value you entered is invalid.\nIt must be in the range %d to %d.",
				CONNECT_TIME_MIN, CONNECT_TIME_MAX);
			return (Buf);

		    } else {

			Cr_ld [Cr_line_num].max_conn_time = v;
	    		Update_change_flag (w, NULL, NULL);

		    }

/*		Validate narrowband load shed alarm threshold entry	*/

		} else if (w == Alarm_text) {

		    if ((v < Cr_warning+1) ||
			(v > PERCENT_MAX)     ||
			(!hci_number_found(text))) {

		 	sprintf (buf,"%d", Cr_alarm);
			XmTextSetString (w, buf);

			sprintf (Buf,"The value you entered is invalid.\nIt must be in the range %d to %d.",
				Cr_warning+1, PERCENT_MAX);
			return (Buf);

		    } else {

			Load_shed_flag = 1;
			Cr_alarm = v;
		    	ORPGLOAD_set_data (LOAD_SHED_CATEGORY_PROD_DIST,
				LOAD_SHED_ALARM_THRESHOLD,
				v);
	    		Update_change_flag (w, NULL, NULL);

		    }

/*		Validate narrowband load shed warning threshold entry	*/

		} else if (w == Warning_text) {

		    if ((v < PERCENT_MIN)   ||
			(v > Cr_alarm-1) ||
			(!hci_number_found(text))) {

		 	sprintf (buf,"%d", Cr_warning);
			XmTextSetString (w, buf);

			sprintf (Buf,"The value you entered is invalid.\nIt must be in the range %d to %d.",
				PERCENT_MIN, Cr_alarm-1);
			return (Buf);

		    } else {

			Load_shed_flag = 1;
			Cr_warning = v;
		    	ORPGLOAD_set_data (LOAD_SHED_CATEGORY_PROD_DIST,
				LOAD_SHED_WARNING_THRESHOLD,
				v);
	    		Update_change_flag (w, NULL, NULL);

		    }

/*		Validate nominal baud rate  entry			*/

		} else if (w == Baud_rate_text) {

		    if ((v < BAUD_RATE_MIN) ||
			(v > BAUD_RATE_MAX) ||
			(!hci_number_found(text))) {

		 	sprintf (buf,"%d", Cr_ld [Cr_line_num].baud_rate);
			XmTextSetString (w, buf);
			sprintf (Buf,"The value you entered is invalid.\nIt must be in the range %d to %d.",
				BAUD_RATE_MIN, BAUD_RATE_MAX);
			return (Buf);

		    } else {

			Cr_ld [Cr_line_num].baud_rate = v;
	    		Update_change_flag (w, NULL, NULL);

		    }

/*		Validate add new user id (dial-in form)			*
 *		This validation is different from the others since	*
 *		we must verify that the new user ID isn't already	*
 *		in use.							*/

		} else if (w == Add_user_id_text) {

		    if ((v < USER_ID_MIN) ||
			(v > USER_ID_MAX) ||
			(!hci_number_found(text))) {

		 	sprintf (buf,"%d", Add_user.user_id);
			XmTextSetString (w, buf);
			sprintf (Buf,"The value you entered is invalid.\nIt must be in the range %d to %d.",
				USER_ID_MIN, USER_ID_MAX);
			return (Buf);

		    } else {

/*			Since the ID is in the allowed range, lets	*
 *			make sure it isn't already used.		*/

			found = 0;

			for (i=0;i<Dial_tbl_num;i++) {

			    if (Dial_tbl [i].user_id == v) {

				found = 1;
				break;

			    }
			}

			if (found) {

			    sprintf (Buf, "The specified user ID is already\nused.  You need to enter another value.");
			    return (Buf);

			} else {

			    Add_user.user_id = v;

			}
		    }

/*		Validate add new user connect time limit (dial-in form)	*/

		} else if (w == Add_user_time_text) {

		    if ((v < CONNECT_TIME_MIN) ||
			(v > CONNECT_TIME_MAX) ||
			(!hci_number_found(text))) {

		 	sprintf (buf,"%d", Add_user.max_connect_time);
			XmTextSetString (w, buf);
			sprintf (Buf,"The value you entered is invalid.\nIt must be in the range %d to %d.",
				CONNECT_TIME_MIN, CONNECT_TIME_MAX);
			return (Buf);

		    } else {

			Add_user.max_connect_time = v;

		    }

		} else {

		    return ("Unexpected text field");

		}

	    } else {

		char	*text;

		text = XmTextGetString (w);

		if (w == Port_pswd_text) {

		    strcpy (Cr_ld [Cr_line_num].port_pswd, text);
		    memset (Cr_ld [Cr_line_num].port_pswd+4,0,1);
	    	    Update_change_flag (w, NULL, NULL);

		} else if (w == Add_user_name_text) {

		    strcpy (Add_user.user_name, text);

		} else if (w == Add_user_pswd_text) {

		    strcpy (Add_user.user_password, text);

		}

		XtFree (text);
	    }

	}
    }

    return (NULL);
}

/**********************************************************************

    Description: Updates info of a NB line.

    Input:  line - the new line info.
    Output: NNE
    Return: NONE

***********************************************************************/

void GUI_update_line_info (Line_status *line)
{
    int i;

    /* the line already displayed? */
    for (i = 0; i < N_list_lines; i++)
	if (line->line_num == List_lines[i].line_num)
	    break;

    if (i < N_list_lines) {		/* already in the list */
	memcpy (List_lines + i, line, sizeof (Line_status));
	Set_list_string (List_lines + i);
	if (i == (List_select_ind+Line_list_min)) {
	    Update_line_objects (Top_widget, NULL, NULL);
	}
    }
    else {				/* add a new line */
	Line_status *new;

	new = (Line_status *)MISC_table_new_entry (List_tbl, NULL);
	if (new == NULL) {
	    HCI_LE_error("malloc failed");
	    return;
	}

	memcpy (new, line, sizeof (Line_status));
	Set_list_string (new);
	line->selected = 0;
	Sort_list ();
    }
}

/**********************************************************************

    Description: Sorts the lines in the line list.

    Input:  NONE
    Output: NONE
    Return: NONE

***********************************************************************/

static void Sort_list ()
{
    int i, j, k;
    int start;
    Line_status temp;
    int line_selected = -1;

    if (N_list_lines <= 1)
	return;

    if (List_select_ind >= 0) {

        line_selected = (int) List_lines [List_select_ind+Line_list_min].line_num;

    }

/*  Regardless of the sort method, we want the sort groups to be in	*
 *  ascending line number order.  So the first thing we do is sort	*
 *  the line status table by line number.  If the sort method chosen	*
 *  is line number, then this is all that we have to do.  Otherwise,	*
 *  we will do another sort based on the chosen method.			*/

    for (i=0;i<N_list_lines-1;i++) {

	for (j=i+1;j<N_list_lines;j++) {

	    if (List_lines [i].line_num > List_lines [j].line_num) {

		temp = List_lines [i];
		List_lines [i] = List_lines [j];
		List_lines [j] = temp;

	    }
	}
    }

    if (Sort_type == SORT_TYPE) { /* Sort by line type */

	for (i=0;i<N_list_lines-1;i++) {

	    for (j=i+1;j<N_list_lines;j++) {

		if (List_lines [i].type > List_lines [j].type) {

		    temp = List_lines [i];
		    List_lines [i] = List_lines [j];
		    List_lines [j] = temp;

		}
	    }
	}

/*      Now that we have sorted by line type, we need to sort within	*
 *      each type by ascending line number.				*/

	start = 0;

        for (i=1;i<N_list_lines;i++) {

	    if ((List_lines[i].type != List_lines[start].type) ||
		(i == N_list_lines-1)) {

		for (j=start;j<i-1;j++) {

		    for (k=j+1;k<i;k++) {

			if (List_lines [j].line_num > List_lines [k].line_num) {

			    temp = List_lines [j];
			    List_lines [j] = List_lines [k];
			    List_lines [k] = temp;

			}
		    }
		}

		start = i;

	    }
	}

    } else if (Sort_type == SORT_STATUS) { /* Sort by line state/status */

	/* Create array of structs that will be sorted. The struct
	   contains the line status and index. Sort by status, then
	   re-order using the index. */

	line_sort_t *sort_array = calloc( N_list_lines, sizeof( line_sort_t ) );
	if( sort_array == NULL )
	{
	  HCI_LE_error("status sort malloc failed");
	  HCI_task_exit( HCI_EXIT_FAIL );
	}

	/* If we only cared about the status values, then there's no
	   need to create an array of structs (simply sort by status).
	   However, we care about the line state if it is US_LINE_FAILED.
	   Therefore, we need a struct to account for all possible
	   values. Fill the struct array with the appropriate values. */

	for (i=0;i<N_list_lines;i++)
	{
	  if( List_lines[ i ].state != US_LINE_FAILED )
	  {
	    sort_array[ i ].status = List_lines[ i ].status;
	  }
	  else
	  {
	    sort_array[ i ].status = List_lines[ i ].state;
	  }
	  sort_array[ i ].index = i;
	}

	/* Use qsort to sort the struct array by status. */

	qsort( sort_array, N_list_lines, sizeof(line_sort_t), sort_by_status );

	/* Re-order the Line_status array using the information garnered
	   by sorting the array of structs. Create a temporary copy of
	   Line_status to accomplish this. */

	Line_status *temp_list_lines = calloc(N_list_lines,sizeof(Line_status));
	memcpy( temp_list_lines, List_lines, N_list_lines*sizeof(Line_status));

	for (i=0;i<N_list_lines;i++)
	{
	  List_lines[ i ] = temp_list_lines[ sort_array[ i ].index ];
	}

	/* Free allocated memory. */

	free( sort_array );
	free( temp_list_lines );
    }

/*  Since we reordered the list, we need to update the position of the	*
 *  selected line.							*/

    if (List_select_ind >= 0) {

	for (i=0;i<N_list_lines-1;i++) {

	    if (List_lines [i+Line_list_min].line_num == line_selected) {

		List_select_ind = i /* $$ -Line_list_min */;
		break;

	    }
	}
    }
}

/**********************************************************************

    Description: Redraws a line in the line list.

    Inputs: line - the line info.
    Output: NONE
    Return: NONE

***********************************************************************/

static void Set_list_string (Line_status *line)
{
    char buf[128];
    char *type, *status, uclass[12], enable[12], protocol[12], id[8], rate[16];
    char user_name[12];
    int	len;
    int i;
    Line_details *ld;

/*  Get the line type (DEDIC or DIALON).			*/

    type = Get_line_type_str (line->type);

/*  Get the line status (CONNECT, DISCON, CON PEND, or N/A).	*/

    if (line->state == US_LINE_FAILED) {

	status = "FAILED  ";

    } else {

	if (line->status == US_CONNECTED) {

	    if (line->state == US_LINE_NOISY) {

		status = "NOISY   ";

	    } else {

		status = "CONNECT ";

	    }

	} else if (line->status == US_CONNECT_PENDING) {

	    status = "CON PEND";

	} else if (line->status == US_DISCONNECTED) {

	    status = "DISCON  ";

	} else {

	    status = "UNKNOWN ";

	}
    }

/*  Get the ID of the user connected to this line.  If nobody	*
 *  connected the ID is "N/A".  If there is a user connected	*
 *  then also get the user name associated with the ID.		*/

    if (line->user_id < 0) {

	strcpy (id, "    ");

    } else {

	sprintf (id, "%4d", line->user_id);

    }

    if ((unsigned short)line->rate == 0xffff)
	strcpy (rate, "  -  ");
    else if ((line->rate & 0xc000) == 0xc000)
	sprintf (rate, "%d", line->rate & ~0xffffc000);
    else if ((line->rate & 0x8000) == 0x8000)
	sprintf (rate, "%dK", line->rate & ~0xffff8000);
    else
	sprintf (rate, "%dM", line->rate);

    ld = MAIN_get_line_details (line);

/*  Build the user name field from two sources.  I no user is connected	*
 *  and a name is defined for the line, then use the line name.  If	*
 *  neither are true then leave blank.					*/

    len = strlen (ld->user_name);

    sprintf (user_name,"           ");

    if (len) {

	if (len > 11)
	    len = 11;

	memcpy ((user_name+11-len), ld->user_name, len);
	memset (user_name+11, 0, 1);

    }

/*  If a user is connected to this line, get its class.		*/

/*  If this is a dedicated line we want to always display the class	*
 *  assigned to the line.  If the line in dial-in, we only want to	*
 *  display the class of the dial user when they are connected.		*/

    if (!strncmp (type,"DEDIC",5)) {

	sprintf (uclass, "%8d", ld->uclass);

	for (i=0;i<Class_tbl_num;i++) {

	    if (ld->uclass == Class_tbl[i].class) {

		if (strlen (Class_tbl[i].name) && (Class_tbl [i].distri_method == 0)) {
		    
		    if (!strncmp (Class_tbl[i].name,"RPGO",4)) {

			sprintf (uclass,"%8.8s",Class_tbl[i].name);
			break;

		    }
		}
	    }
	}

    } else {

        if (line->uclass < 0) {

	    sprintf (uclass, "%8d", ld->uclass);

	    for (i=0;i<Class_tbl_num;i++) {

		if (ld->uclass == Class_tbl[i].class) {

		    if (strlen (Class_tbl[i].name) && (Class_tbl [i].distri_method == 0)) {
		    
			if (!strncmp (Class_tbl[i].name,"RPGO",4)) {

			    sprintf (uclass,"%8.8s",Class_tbl[i].name);
			    break;

			}
		    }
		}
	    }

        } else {

	    sprintf (uclass, "%8d", line->uclass);

	    for (i=0;i<Class_tbl_num;i++) {

		if (line->uclass == Class_tbl[i].class) {

		    if (strlen (Class_tbl[i].name) && (Class_tbl [i].distri_method == 0)) {
		    
			if (!strncmp (Class_tbl[i].name,"RPGO",4)) {

			    sprintf (uclass,"%8.8s",Class_tbl[i].name);
			    break;

			}
		    }
		}
	    }
	}
    }

/*  Get the line enable state (yes or no).			*/

    if (line->enable)
	strcpy (enable, "yes");
    else
	strcpy (enable, " no");

/*  Get the line protocol (TCP or X25).				*/

    if (line->protocol == PROTO_TCP)
	strcpy (protocol, "TCP");
    else
	strcpy (protocol, "X25");

/*  Format the string for display.				*/


    sprintf (buf, 
	"%3d   %s  %s    %s %4.4s %11.11s  %8.8s   %s  %3d%c %6s", line->line_num, type, 
		enable, protocol, id, user_name, uclass, status, 
		line->util, 045, rate);

    strcpy (line->str,buf);
}

/**********************************************************************

    Description: This function updates the line status objects according
		to the currently selected line. In non-editing mode
		all fields are treated as status fields. In editing
		mode, the modifiable fields are treated as adaptation
		data. They are assumed not changing (only this screen
		can update them). Thus they are not updated when status
		changes. 

		If the selected line changed, we have to update all
		objects. If any of them has beem updated, the user is
		prompted for save the changes.

    Inputs: w - widget ID 
	    client_data - unused
	    call_data - unused
    Output: NONE
    Return: NONE

**********************************************************************/

static void Update_line_objects (Widget w, XtPointer client_data,
				XtPointer call_data)
{
    static int line_num_displayed = -1;
    Line_details *ld;
    char buf[128];
    int new_line = 0;
    int	i;
    static int busy = 0;

/*  Check to see if we are entering this module before a previous	*
 *  call was completed.  If so, return and do nothing.			*/

    if (busy) {

	return;

    }

    busy = 1;

    if ((line_num_displayed != (List_select_ind+Line_list_min)) || Force_update) {

	new_line = 1;

    }
/*
    if (Change_flag && (new_line && !Force_update)) {
	Ask_save (w, client_data, call_data);
	busy = 0;
	return;
    }
*/
    ld = MAIN_get_line_details (List_lines + List_select_ind + Line_list_min);

    Cr_line_num = ld->line_num;

/*  If the selected line has been edited already and the edits not	*
 *  saved, then do not update it.					*/

    if (Cr_update [Cr_line_num]) {

/*	Line has been updated so not reading new data from LB.	*/

	ld = &Cr_ld [Cr_line_num];
/*
    } else {

	if (memcmp (ld, &Org_ld [Cr_line_num], sizeof (Line_details)) == 0 && (!Force_update)) {
	    busy = 0;
	    return;

	}
*/
    }

    Force_update = 0;
    line_num_displayed = List_select_ind + Line_list_min;

    /* draw the objects */

    if (!Editing_mode || new_line || Undo_flag) {

	int	alarm_status, warn_status;
	int	warn_thresh, alarm_thresh, pos;

	sprintf (buf, "%d", ld->retries);

	XmTextSetString (Retries_text, buf);

	Cr_ld [Cr_line_num].retries = ld->retries;

	sprintf (buf, "%d", ld->timeout);

	XmTextSetString (Timeout_text, buf);

	Cr_ld [Cr_line_num].timeout = ld->timeout;

/*	Get the product distribution load shed warning and alarm	*
 *	thresholds from load shed adaptation data.			*/

	warn_status = ORPGLOAD_get_data (LOAD_SHED_CATEGORY_PROD_DIST,
				LOAD_SHED_WARNING_THRESHOLD,
				&warn_thresh);

	alarm_status = ORPGLOAD_get_data (LOAD_SHED_CATEGORY_PROD_DIST,
				LOAD_SHED_ALARM_THRESHOLD,
				&alarm_thresh);

	sprintf (buf, "%d", alarm_thresh);

	XmTextSetString (Alarm_text, buf);

	Cr_alarm = alarm_thresh;

	sprintf (buf, "%d", warn_thresh);

	XmTextSetString (Warning_text, buf);

	Cr_warning = warn_thresh;

	pos = ld->type;
	if (pos == WAN_LINE)
	    pos = 2;
        XtVaSetValues (Type_combo,
		XmNselectedPosition, pos,
		NULL);
	Cr_ld [Cr_line_num].type = ld->type;

	switch (ld->packet_size) {
	    case PACKET_SIZE_128:
        	XtVaSetValues (Packet_combo,
			XmNselectedPosition, 0,
			NULL);
		Cr_ld [Cr_line_num].packet_size = ld->packet_size;
		break;
	    case PACKET_SIZE_512:
        	XtVaSetValues (Packet_combo,
			XmNselectedPosition, 1,
			NULL);
		Cr_ld [Cr_line_num].packet_size = ld->packet_size;
		break;
	}

	Cr_ld [Cr_line_num].distri_method = ld->distri_method;
	strcpy (Cr_ld [Cr_line_num].port_pswd, ld->port_pswd);
	memset (Cr_ld [Cr_line_num].port_pswd+4,0,1);
	sprintf (buf, "%d", ld->baud_rate);

	XmTextSetString (Baud_rate_text, buf);

	Cr_ld [Cr_line_num].baud_rate = ld->baud_rate;

        if (ld->protocol == PROTO_TCP)
	  sprintf (buf, "%d [TCP]", ld->line_num);
        else
	  sprintf (buf, "%d [X25]", ld->line_num);
	if (ld->protocol == PROTO_TCP || !Editing_mode) {
	    XtVaSetValues (Packet_text, XmNsensitive, False, 
			XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
			NULL);
	    XtVaSetValues (Packet_list, XmNsensitive, False, 
			XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
			XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
			NULL);
	    XtVaSetValues (Packet_combo, XmNsensitive, False, 
			XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
			NULL);
	}
	else {
	    XtVaSetValues (Packet_text, XmNsensitive, True, 
			XmNbackground,	hci_get_read_color (EDIT_BACKGROUND),
			NULL);
	    XtVaSetValues (Packet_list, XmNsensitive, True, 
			XmNbackground,	hci_get_read_color (BUTTON_BACKGROUND),
			XmNforeground,	hci_get_read_color (BUTTON_FOREGROUND),
			NULL);
	    XtVaSetValues (Packet_combo, XmNsensitive, True, 
			XmNbackground,	hci_get_read_color (EDIT_BACKGROUND),
			NULL);
	}
	Cr_ld [Cr_line_num].protocol = ld->protocol;

	XmTextSetString (Line_num_text, buf);
	Cr_ld [Cr_line_num].line_num = ld->line_num;

	sprintf (buf, "%d", ld->pserver_num);
	XmTextSetString (Pserver_num_text, buf);
	Cr_ld [Cr_line_num].pserver_num = ld->pserver_num;

	sprintf (buf, "%d", ld->comms_mgr_num);
	XmTextSetString (Comm_mgr_num_text, buf);
	Cr_ld [Cr_line_num].comms_mgr_num = ld->comms_mgr_num;

	Cr_ld [Cr_line_num].max_conn_time = ld->max_conn_time;

	if (ld->user_id >= 0) {
	    sprintf (buf, "%d", ld->user_id);
	} else {
	    sprintf (buf, "   ");
	}

	Cr_ld [Cr_line_num].user_id = ld->user_id;

	if (strlen (ld->user_name)) {
	    sprintf (buf, "%s", ld->user_name);
	} else {
	    sprintf (buf, "   ");
	}

	strcpy (Cr_ld [Cr_line_num].user_name, ld->user_name);

	Cr_ld [Cr_line_num].uclass        = ld->uclass;
	Cr_ld [Cr_line_num].distri_method = ld->distri_method;
	Cr_ld [Cr_line_num].defined       = ld->defined;

	if (ld->type == DIAL_IN || ld->type == WAN_LINE) {

/*	If max connect time defined in line user record then display	*
 *	the value.							*/

	    if (ld->defined & UP_DEFINED_MAX_CONNECT_TIME) {

		sprintf (buf,"%d ", ld->max_conn_time);

	    } else {

/*	Else, get the max connect time value from the class record and	*
 *	display it.							*/

		sprintf (buf,"0");

		for (i=0;i<Class_tbl_num;i++) {

		    if (Class_tbl[i].class == ld->uclass) {

			sprintf (buf,"%d ", Class_tbl [i].max_connect_time);
			break;

		    }
		}
	    }

	    XmTextSetString (Max_conn_time_text, buf);

	    if (Editing_mode) {

		strcpy (buf,Cr_ld [Cr_line_num].port_pswd);
		XmTextSetString (Port_pswd_text, buf);

		XtVaSetValues (Max_conn_time_combo,
			XmNbackground,	hci_get_read_color (EDIT_BACKGROUND),
			XmNsensitive,	True,
			NULL);
		XtVaSetValues (Max_conn_time_list,
			XmNbackground,	hci_get_read_color (BUTTON_BACKGROUND),
			XmNforeground,	hci_get_read_color (BUTTON_FOREGROUND),
			XmNsensitive,	True,
			NULL);
		XtVaSetValues (Max_conn_time_text,
			XmNbackground,	hci_get_read_color (EDIT_BACKGROUND),
			XmNsensitive,	True,
			NULL);
		XtVaSetValues (Port_pswd_text,
			XmNbackground,	hci_get_read_color (EDIT_BACKGROUND),
			XmNsensitive,	True,
			NULL);

	    } else {

		XtVaSetValues (Max_conn_time_combo,
			XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
			XmNsensitive,	False,
			NULL);
		XtVaSetValues (Max_conn_time_text,
			XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
			XmNsensitive,	False,
			NULL);
		XtVaSetValues (Max_conn_time_list,
			XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
			XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
			XmNsensitive,	False,
			NULL);
		XtVaSetValues (Port_pswd_text,
			XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
			XmNsensitive,	False,
			NULL);

		XmTextSetString (Port_pswd_text, "****");


	    }

	} else {

	    XtVaSetValues (Max_conn_time_combo,
		XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
		XmNsensitive,	False,
		NULL);
	    XtVaSetValues (Max_conn_time_text,
		XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
		XmNsensitive,	False,
		NULL);
	    XtVaSetValues (Max_conn_time_list,
		XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
		XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
		XmNsensitive,	False,
		NULL);

	    sprintf (buf, "N/A");
	    XmTextSetString (Max_conn_time_text, buf);

	    XtVaSetValues (Port_pswd_text,
		XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
		XmNsensitive,	False,
		NULL);

	    memset (buf,0,1);
	    XmTextSetString (Port_pswd_text, buf);

	}

	Update_class_info (&Cr_ld [Cr_line_num]);

    }

    memcpy (&Org_ld [Cr_line_num], ld, sizeof (Line_details));
    busy = 0;
}

/**********************************************************************

    Description: Returns the line type string.

    Input:  type - the line type value.
    Output: NONE
    Return: pointer to line type string

***********************************************************************/

static char *Get_line_type_str (int type)
{
    if (type == DEDICATED)
	return ("DEDIC ");
    else if (type == WAN_LINE)
	return ("WAN   ");
    else
	return ("DIALIN");
}

/**********************************************************************

    Description: Updates the entire text on the line list without
		 changing the selection status.

    Input:  NONE
    Output: NONE
    Return: NONE

***********************************************************************/

void GUI_update_line_list ()
{
    XmString buf[128], *s;
    int i, cnt;

    if (List_select_ind == INITIAL_LINE && N_list_lines > 0) {	/* init selection */
	List_select_ind = 0;

	if (N_list_lines < Line_list_max)
	    Line_list_max = N_list_lines;

	Update_line_objects (Top_widget, NULL, NULL);
    }

    Get_selected_lines ();

    if (N_list_lines <= 128)
	s = buf;
    else {
	s = malloc (N_list_lines * sizeof (XmString));
	if (s == NULL) {
	    HCI_LE_error("malloc failed");
	    return;
	}
    }

/*  Resort the list in case status changed.			*/

    Sort_list ();

/*  Build the list status list and display it.			*/

    for (i = Line_list_min; i < Line_list_max; i++) {
	s[i] = XmStringCreateLocalized (List_lines[i].str);
    }
    XtVaSetValues (Line_list, XmNitemCount, Line_list_max - Line_list_min, XmNitems, &s[Line_list_min], NULL);

/*  Determine the selected lines and display them as selected.	*/

    cnt = 0;
    for (i = Line_list_min; i < Line_list_max; i++) {
	if (List_lines[i].selected) {
	    s[cnt] = XmStringCreateLocalized (List_lines[i].str);
	    cnt++;
	}
    }

    XtVaSetValues (Line_list, XmNselectedItemCount, cnt, 
					XmNselectedItems, s, NULL);

    if (s != buf) {
	free (s);
    } else {
	for (i=Line_list_min;i<Line_list_max;i++) {
	    XmStringFree (buf[i]);
	}
    }
}

void
Update_class_info (Line_details	*ld)

{
    char buf[128];
    int	i;
    int	j;
    int	found;
    int cnt = 0;

    if (ld->uclass >= 0) {

/*	Now lets update the class dropdown list.			*/


	cnt = 0;

	for (i=0;i<Class_tbl_num;i++) {

/*	    We need to build the list based on line type.		*/

	    if (ld->type == DIAL_IN || ld->type == WAN_LINE) {

		if (Class_tbl[i].line_ind == CLASS_DEDICATED) {

		    continue;

	        }

	    } else {

		if (Class_tbl[i].line_ind == CLASS_DIAL) {

		    continue;

	        }
	    }

	    found = 0;

	    for (j=0;j<i;j++) {

		if (Class_tbl[i].class == Class_tbl[j].class) {

		    found = 1;

		}
	    }

/*	    If this class has already been found, do not create a new	*
 *	    list entry.							*/

	    if (found) {

		continue;

	    }


	    if (strlen (Class_tbl[i].name) && (Class_tbl [i].distri_method == 0)) {

		sprintf (Buf,"%7s", Class_tbl[i].name);

	    } else {

		sprintf (Buf,"%7d", Class_tbl[i].class);

	    }

	    cnt++;

        }

/*	Next, we need to update the currently selected class item.	*/

	sprintf (buf, "%7d", ld->uclass);

	if (ld->distri_method == 0) {

	    for (i=0;i<Class_tbl_num;i++) {

		if (Class_tbl [i].class == ld->uclass) {

		    if (strlen (Class_tbl [i].name)) {
		    
			sprintf (buf,"%7s", Class_tbl [i].name);

		    }

		    break;

		}
	    }
	}


/*	We need to build the distribution method list based on		*
 *	the class which is defined for the line.  First, we need	*
 *	to delete any previous list entries.				*/


	cnt = 0;

	for (i=0;i<Class_tbl_num;i++) {

	    if (Class_tbl[i].class == ld->uclass) {

		if (Class_tbl[i].distri_method != 0) {

		    sprintf (buf,"%s [%d]", Class_tbl [i].name, Class_tbl [i].distri_method);
		    cnt++;

		}
	    }
	}


    } 

}

/**********************************************************************

    Description: Go to a new line list page.

    Inputs: w - Up or Down arrow button 
	    client_data - XmARROW_LEFT, XmARROW_RIGHT
	    client_data - unused
    Output: NONE
    Return: NONE

***********************************************************************/

void
hci_line_arrow_callback (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{

    GUI_update_line_list ();

    switch ((int) client_data) {

	case XmARROW_LEFT :

	    Line_list_min = Line_list_min - PDL_LIST_VISIBLE_LINES;

	    if (Line_list_min < 0) {

		Line_list_min = 0;

	    }

	    Line_list_max = Line_list_min + PDL_LIST_VISIBLE_LINES;

	    if (Line_list_min == 0) {

		XtVaSetValues (Line_list_select_left,
			XmNsensitive,	False,
			XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
			XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
			NULL);

	    }

	    if (Line_list_max > N_list_lines) {

		Line_list_max = N_list_lines;

		XtVaSetValues (Line_list_select_right,
			XmNsensitive,	False,
			XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
			XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
			NULL);

	    } else {

		XtVaSetValues (Line_list_select_right,
			XmNsensitive,	True,
			XmNforeground,	hci_get_read_color (BUTTON_FOREGROUND),
			XmNbackground,	hci_get_read_color (BUTTON_BACKGROUND),
			NULL);

	    }

	    break;

	case XmARROW_RIGHT :

	    if (Line_list_min >= N_list_lines) {

		break;

	    }

	    Line_list_min = Line_list_min + PDL_LIST_VISIBLE_LINES;
	    Line_list_max = Line_list_min + PDL_LIST_VISIBLE_LINES;

	    if (Line_list_min != 0) {

		XtVaSetValues (Line_list_select_left,
			XmNsensitive,	True,
			XmNforeground,	hci_get_read_color (BUTTON_FOREGROUND),
			XmNbackground,	hci_get_read_color (BUTTON_BACKGROUND),
			NULL);

	    }

	    if (Line_list_max > N_list_lines) {

		Line_list_max = N_list_lines;

		XtVaSetValues (Line_list_select_right,
			XmNsensitive,	False,
			XmNforeground,	hci_get_read_color (TEXT_FOREGROUND),
			XmNbackground,	hci_get_read_color (BACKGROUND_COLOR1),
			NULL);

	    } else {

		XtVaSetValues (Line_list_select_right,
			XmNsensitive,	True,
			XmNforeground,	hci_get_read_color (BUTTON_FOREGROUND),
			XmNbackground,	hci_get_read_color (BUTTON_BACKGROUND),
			NULL);

	    }

	default:

	    break;

    }

/*  Determine the selected lines and display them as selected.	*/

    {
	int cnt;
	int i;
	XmString s[128];

        cnt = 0;
        for (i = Line_list_min; i < Line_list_max; i++) {
	    if (List_lines[i].selected) {
	        s[cnt] = XmStringCreateLocalized (List_lines[i].str);
	        cnt++;
	    }
        }

        if (cnt) {

            XtVaSetValues (Line_list, XmNselectedItemCount, cnt, 
					XmNselectedItems, s, NULL);

	    for (i=0;i<cnt;i++) {

		XmStringFree (s[i]);

	    }
        }
    }

    GUI_update_line_list ();

}

/**********************************************************************

    Description: This function sorts the dial users table in order of
		 increasing user ID.  Entries with user IDs less than
 		 0 are considered marked for deletion and are ignored.

    Inputs: dial_tbl - pointer to dial table data
	    table_size - number of items in table
    Output: NONE
    Return: NONE

***********************************************************************/

void
hci_sort_dial_user_table (
Dial_details	*dial_tbl,
int		table_size
)
{
    int	i, j;
    Dial_details	temp;
    int			itemp;

    for (i=0;i<table_size-1;i++) {

	for (j=i+1;j<table_size;j++) {

	    if (dial_tbl [i].user_id > dial_tbl [j].user_id) {

		temp = Dial_tbl [i];
		Dial_tbl [i] = Dial_tbl [j];
		Dial_tbl [j] = temp;

		itemp = Dial_change_flag [i];
		Dial_change_flag [i] = Dial_change_flag [j];
		Dial_change_flag [j] = itemp;;

	    }
	}
    }
}

/**********************************************************************

    Description: Maximum connect time combo box widget callback function.

    Input:  w - parent widget ID
	    client_data - unused
	    call_data - unused
    Output: NONE
    Return: NONE

***********************************************************************/

static void Max_conn_time_callback (Widget w, XtPointer client_data,
					XtPointer call_data)
{
    char	*text;
    int		val;
    int		i;
    int		class_val;
static int	busy = 0;
    char	buf [16];
    XmComboBoxCallbackStruct *cbs =
		(XmComboBoxCallbackStruct *) call_data;

    if (busy)
	return;

    busy = 1;
    class_val = 0;

    if (cbs->event != NULL) {

	text = XmTextGetString (Max_conn_time_text);

/*	Retreive the max connect time for the class associated with	*
 *	this dial user (if one is defined).				*/

	for (i=0;i<Class_tbl_num;i++) {

	    if (Class_tbl[i].class == Cr_ld [Cr_line_num].uclass) {

		class_val = Class_tbl [i].max_connect_time;
		break;

	    }
	}

	if (strlen (text)) {

/*	    If class is selected then we want to clear the max connect	*
 *	    time defined bit so we use the class value.			*/

	    if (strcmp (text,"Clas") != 0) {

		sscanf (text,"%d",&val);

/*		If the value hasn't changed then do nothing and return.	*/

		if ((val == class_val) ||
		    (val == Cr_ld [Cr_line_num].max_conn_time)) {

		    XtFree (text);
		    busy = 0;
		    return;

		}

	    } else {

		val = class_val;

	    }

	    if ((val < CONNECT_TIME_MIN) ||
		(val > CONNECT_TIME_MAX)) {

		sprintf (Buf,"The value you entered is invalid.\nIt must be in the range %d to %d.",
			CONNECT_TIME_MIN, CONNECT_TIME_MAX);

		if (!(Cr_ld [Cr_line_num].defined & UP_DEFINED_MAX_CONNECT_TIME)) {

		    sprintf (buf,"%d ", class_val);

		} else {

		    sprintf (buf,"%d ", Cr_ld [Cr_line_num].max_conn_time);

		}

		XmTextSetString (Max_conn_time_text,buf);
		hci_warning_popup( Top_widget, Buf, NULL );

	    } else {

		if (strcmp (text,"Clas") == 0) {

		    Cr_ld [Cr_line_num].defined =
				Cr_ld [Cr_line_num].defined &
				~UP_DEFINED_MAX_CONNECT_TIME;
		    val = class_val;

		} else {

		    Cr_ld [Cr_line_num].defined =
				Cr_ld [Cr_line_num].defined |
				UP_DEFINED_MAX_CONNECT_TIME;

		}

		Cr_ld [Cr_line_num].max_conn_time = val;

		sprintf (buf,"%d ", Cr_ld [Cr_line_num].max_conn_time);
		XmTextSetString (Max_conn_time_text,buf);
		Update_change_flag (Top_widget, NULL, NULL);

	    }

	    XtFree (text);

	}
    }

    busy = 0;

}

/**********************************************************************

    Description: Sorting function used with qsort. Assumes status values
		 are (in ascending order): US_LINE_FAILED, US_CONNECTED,
		 US_DISCONNECTED, or US_CONNECT_PENDING.

***********************************************************************/

int sort_by_status( const void *aa, const void *bb )
{
  const line_sort_t *a = aa;
  const line_sort_t *b = bb;

  if( a->status == US_CONNECT_PENDING )
  {
    if( b->status == US_CONNECT_PENDING ){ return 0; }
    else{ return 1; }
  }
  else if( a->status == US_DISCONNECTED )
  {
    if( b->status == US_CONNECT_PENDING ){ return -1; }
    else if( b->status == US_DISCONNECTED ){ return 0; }
    else{ return 1; }
  }
  else if( a->status == US_CONNECTED )
  {
    if( b->status == US_LINE_FAILED ){ return 1; }
    else if( b->status == US_CONNECTED ){ return 0; }
    else{ return -1; }
  }
  else
  {
    if( b->status == US_LINE_FAILED ){ return 0; }
    else{ return -1; }
  }
}

