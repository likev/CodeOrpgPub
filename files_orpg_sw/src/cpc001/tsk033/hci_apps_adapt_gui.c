
/************************************************************************

    The module that implements the HCI application adaptation screen.

************************************************************************/

/*
 * RCS info
 * $Author: jing $
 * $Locker:  $
 * $Date: 2012/10/15 20:59:43 $
 * $Id: hci_apps_adapt_gui.c,v 1.35 2012/10/15 20:59:43 jing Exp $
 * $Revision: 1.35 $
 * $State: Exp $
 */

/*	Local file definitions */

#include <hci.h>
#include <hci_apps_adapt_def.h>
#include <hci_rpg_adaptation_data.h>

/* Widgets */
static Widget Top_widget = NULL, Parent = NULL;
static Widget Data_scroll = NULL, Data_form = NULL;
static Widget Top_rowcol, Select_rowcol;

static Widget Save_button = NULL, Undo_button = NULL;
static Widget Restore_button = NULL, Update_button = NULL;

static Widget Active_row = NULL, Lock_widget = NULL;
static Widget List_title, Label_rowcol;

static int Edit_mode = 0; 	/* current edit mode for editable fields */
static int Change_flag = HCI_NOT_CHANGED_FLAG;		/* Data edited flag */
static int Close_flag = 0;	/* Flag indicating "Close" button selected */
static int Unlocked_urc = HCI_NO_FLAG; /* URC LOCA lock flag */
static int Unlocked_roc = HCI_NO_FLAG; /* URC LOCA lock flag */
static int Unlocked_agency = HCI_NO_FLAG; /* URC LOCA lock flag */
static int Selected_ind = 0;	/* current selected application index */
static Widget Select_combo;	/* combobox for "Select Application" */
static int Operational;		/* RPG is running in operational site */

static Widget Description_button = NULL;	/* description button */
static Widget Description_window = NULL;	/* description window */
static Widget Description_text = NULL;		/* description text window */

#define TEXT_WIDTH 20		/* value text widget width */

typedef struct {		/* for a row of the DE list */
    Widget rowcol;		/* Row column manager parent */
    Widget name_w;		/* name widget of the data element */
    Widget value_w;		/* value widget of the data element */
    Widget desc_w;		/* description widget of the data element */
    Dea_t *de;			/* the DEA for this row */
    int array_ind;		/* the array index in the values in "de" */
    int is_combo;		/* the widget is a combo instead of text */
} List_row_t;

extern char *Sb;		/* string buffer */
extern Apps_list_t *Apps;	/* application table. */
extern int N_apps;		/* size of application table */

static List_row_t *List_rows = NULL;
				/* the rows of the current editing list */
static int N_rows = 0;		/* number of rows, the current editing list */
static int List_row_height = -1;

/* variables for continuing the calls due to Save_callback */
enum {FROM_NONE, FROM_SECURITY_CB, FROM_SELECT_CALLBACK}; /* for Save_from */
static int Cbs_item_position = 0;
static int Locked_by_myself = 0;
static int Save_from = FROM_NONE;
static int Db_updated = 0;		/* DEA database updated */
static int In_modification = 0;
static char *Init_app_name = "";

static void Show_ac_table ();
static void Adjust_window_size ();
static int  Security_cb ();
static void Save_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
static void Close_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
static void Undo_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
static void Restore_baseline_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
static void Restore_baseline_yes_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
static void Restore_baseline_no_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
static void Update_baseline_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
static void Update_baseline_yes_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
static void Update_baseline_no_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
static void Select_callback (Widget w, XtPointer client_data,
					XtPointer call_data);
static void Description_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
static void Edit_select_callback (Widget w, XtPointer client_data,
					XtPointer call_data);
static void Save_yes_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
static void Save_no_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
static void Edit_modify_cb (Widget w, XtPointer client_data, 
					XtPointer call_data);
static void Edit_verify_cb (Widget w, 
		XtPointer client_data, XtPointer call_data);
static void Edit_gain_focus_cb (Widget w, 
		XtPointer client_data, XtPointer call_data);
static void Description_close_callback (Widget w,
		XtPointer client_data, XtPointer call_data);
static void Resize_callback (Widget, XtPointer, XtPointer);

static Widget Create_combo (Widget parent, int n_columns, 
	int visible_item_cnt, int n_items, char **labels, int init_position);
static Widget Create_button (char *label, Widget parent, 
					int sens, void (*cb) ());
static Widget Create_label (char *label, Widget parent);
static Widget Create_edit_area_label (char *label, Widget parent);
static int Create_edit_area ();
static void Set_label_string (Widget label, char *string, int extend);
static void Decorate_active_row (Widget w, 
		XtPointer client_data, XtPointer call_data);
static int Check_permission (char *perm);
static char *Entends_string (char *str, int width);
static void Update_value (Dea_t *de, char *text, int array_ind);
static void *Duplicate_values (Dea_t *de, int baseline);
static int Get_value_index (Dea_t *de, int array_ind);
static int Convert_to_double (char *tv, double *dp);
static void Set_unchanged ();
static void Set_changed ();
static int Is_current_equals_baseline (Dea_t *de);
static void timer_proc ();
static void Continue_security_cb ();
static void Continue_select_callback ();
static void Db_update_notify_func 
		(EN_id_t event, char *msg, int msg_len, void *arg);
static void Set_raise ();
static void Update_description_text ();


/************************************************************************

    Sets the initial application name.	
	
************************************************************************/

void HAA_set_init_app_name (char *optarg) {
    char *p;

    Init_app_name = malloc (strlen (optarg) + 1);
    if (Init_app_name == NULL) {
	HCI_LE_error("malloc failed");
	HCI_task_exit (HCI_EXIT_FAIL);
    }
    strcpy (Init_app_name, optarg);
    p = Init_app_name;
    while (*p != '\0') {
	if (*p == ' ')
	    *p = '_';
	p++;
    }
}

/************************************************************************

    The GUI main function for the RPG application adaptation data task.		
************************************************************************/

int HAA_gui_main (int operational) {
    Widget button, lock_form;
    int i;
    int retval;

    Operational = operational;

    retval = DEAU_UN_register (NULL, (void *) Db_update_notify_func);
    if (retval < 0) {
	HCI_LE_error("DEAU_UN_register failed: %d", retval);
	HCI_task_exit (HCI_EXIT_FAIL) ;
    }

    Top_widget = HCI_get_top_widget();

    HCI_PM ("Read application names");
    HAA_read_app_names ();	/* reads application names */

    /* Define a form widget to be used as the manager for widgets in the a/c
       window. */
    Parent = XtVaCreateWidget ("parent",
		xmFormWidgetClass,	Top_widget,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);        
                
    if (N_apps <= 0) {
	HCI_LE_log("No application adaptation data element found");
	HCI_task_exit (HCI_EXIT_SUCCESS) ;
    }

    /* The following widget definitions define the pushbuttons for the various
       control panel items.  The exact placement of these widgets is done in 
       the resize callback procedure. */

    Top_rowcol = XtVaCreateWidget ("Top_rowcol",
		xmRowColumnWidgetClass,	Parent,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNpacking,		XmPACK_TIGHT,
		XmNorientation,		XmHORIZONTAL,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNentryAlignment,	XmALIGNMENT_CENTER,
		NULL);

    button = Create_button (" Close ", Top_rowcol, True, Close_callback);
    Save_button = Create_button ("  Save ", Top_rowcol, False, Save_callback);

    /* The Undo button is used to reinitialize the generation table display by 
       throwing away all current edits and re-reading the table from the LB. */
    Undo_button = Create_button (" Undo ", Top_rowcol, False, Undo_callback);

    Create_label ("     Baseline: ", Top_rowcol);
    Restore_button = Create_button ("Restore", Top_rowcol, False, 
					Restore_baseline_callback);
    Update_button = Create_button ("Update", Top_rowcol, False, 
					Update_baseline_callback);

    XtManageChild (Top_rowcol);

    /* Create a pixmap and drawn button for the window lock. */
    lock_form = XtVaCreateWidget ("lock_form",
		xmFormWidgetClass,	Parent,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

    /* which LOCA's apply to this window? */

    Lock_widget = hci_lock_widget (lock_form,
			Security_cb,
			HCI_LOCA_AGENCY | HCI_LOCA_URC | HCI_LOCA_ROC );

    XtManageChild (lock_form);

    /* The application selection row */
    Select_rowcol = XtVaCreateWidget ("Select_rowcol",
		xmRowColumnWidgetClass,	Parent,
		XmNorientation,		XmHORIZONTAL,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		Top_rowcol,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNmarginHeight,	1,
		XmNmarginWidth,		1,
		XmNborderColor,		hci_get_read_color (BLACK),
		XmNborderWidth,		1,
		NULL);

    Create_label ("  Adaptation Item  ", Select_rowcol);

    {
	char **labels;
	int combo_w;
	combo_w = 0;		/* find combo list width */
	for (i = 0; i < N_apps; i++) {
	    int len = strlen (Sb + Apps[i].display_name_o);
	    if (len > combo_w)
		combo_w = len;
	}
	Selected_ind = 0;
	labels = (char **)MISC_malloc (sizeof (char *) * N_apps);
	for (i = 0; i < N_apps; i++) {
	    labels[i] = Sb + Apps[i].display_name_o;
	    if (strcmp (Init_app_name, labels[i]) == 0)
		Selected_ind = i;
	}
	Select_combo = Create_combo (Select_rowcol, combo_w + 2, 
		(N_apps > 20 ? 20 : N_apps), N_apps, labels, Selected_ind);
	free (labels);
	Create_label ("    ", Select_rowcol);
	Description_button = Create_button (" Descriptions ", Select_rowcol, 
						True, Description_callback);
    }
    XtAddCallback (Select_combo, XmNselectionCallback, Select_callback, NULL);

    XtManageChild (Select_rowcol);

    /* Display a label to go above the adaptation adapt data list to identify
       the contents of each column in the table. */
    Label_rowcol = XtVaCreateWidget ("label_rowcol",
		xmRowColumnWidgetClass,	Parent,
		XmNorientation,		XmHORIZONTAL,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		Select_rowcol,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNmarginHeight,	1,
		XmNmarginWidth,		1,
		NULL);

    List_title = Create_label ("   ", Label_rowcol);
    XtManageChild (Label_rowcol);

    /* Update the main data editing area */
    Show_ac_table ();

    XtManageChild (Parent);
    XtRealizeWidget (Top_widget);
    Adjust_window_size ();

    HCI_start( timer_proc, HCI_ONE_SECOND, RESIZE_HCI );

    return 0;
}


/************************************************************************

    This function is used to display data in the editing area.
	
************************************************************************/

static void Show_ac_table () {
    int i;

    Create_edit_area ();

    for (i = 0; i < N_rows; i++) {
	Dea_t *de;
	List_row_t *row;
	int fg_color, bg_color, e_mode;

	row = List_rows + i;
	de = row->de;

	e_mode = 0;
	fg_color = EDIT_FOREGROUND;
	bg_color = BACKGROUND_COLOR1;
	if( hci_lock_loca_selected() )
	{
	    /* LOCA selected from Lock */
	    if (strstr(Sb + de->perm_o, "ROC") != NULL) {
		if( hci_lock_ROC_selected() )
		    fg_color = LOCA_FOREGROUND;
	    }
	    if (strstr(Sb + de->perm_o, "URC") != NULL) {
		if( hci_lock_URC_selected() )
		    fg_color = LOCA_FOREGROUND;
	    }
	    if (strstr(Sb + de->perm_o, "AGENCY") != NULL) {
		if( hci_lock_AGENCY_selected() )
		    fg_color = LOCA_FOREGROUND;
	    }
	}
	else if (Check_permission (Sb + de->perm_o)) {
	    /* LOCA unlocked */
	    if (Edit_mode) {
		e_mode = 1;
		bg_color = EDIT_BACKGROUND;
	    }
	}

	if (row->is_combo) {
	    Widget list, text;
	    XtVaGetValues (row->value_w, 
		XmNlist,	&list,
		XmNtextField,	&text,
		NULL);
	    XtVaSetValues (row->value_w,
		XmNforeground,		hci_get_read_color (fg_color),
		XmNbackground,		hci_get_read_color (bg_color),
		XmNselectedPosition,	Get_value_index (de, row->array_ind),
		NULL);
	    XtVaSetValues (text,
		XmNforeground,		hci_get_read_color (fg_color),
		XmNbackground,		hci_get_read_color (bg_color),
		NULL);
	}
	else {
	    XtVaSetValues (row->value_w,
		XmNforeground,		hci_get_read_color (fg_color),
		XmNbackground,		hci_get_read_color (bg_color),
		XmNtraversalOn,		e_mode,
		XmNeditable,		e_mode,
		XmNcursorPositionVisible,		False,
		NULL);

	    XmTextSetString (row->value_w, 
			HAA_get_text_from_value (de, row->array_ind));
	}
    }

    if (N_rows > 0 && Active_row != NULL)
	XtVaSetValues (Active_row,
		XmNbackground,	hci_get_read_color (WHITE),
		NULL);
}

/************************************************************************

    Creates the data edit area GUI objects.
	
************************************************************************/

static int Create_edit_area () {
    static int prev_ind = -1, prev_has_combo = -1;
    int i, n_des, ind, k, size, has_combo, data_read;
    Apps_list_t *apps;
    Dea_t *des;
    char buf[512];
    Widget clip;

    if (prev_ind != Selected_ind)
	Active_row = NULL;
    data_read = 0;
    apps = Apps + Selected_ind;
    if (apps->n_des < 0) {
	HAA_read_data_elements (Selected_ind);
	data_read = 1;
    }
    des = apps->des;
    n_des = apps->n_des;
    if (n_des == 0) {
	N_rows = 0;
	return (0);
    }
    has_combo = 0;
    if (Edit_mode && apps->has_selection) {
	for (i = 0; i < n_des; i++) {
	    if (des[i].n_sel_values > 0 && 
				Check_permission (Sb + des[i].perm_o))
		has_combo = 1;
	}
    }
    if (!data_read && prev_ind == Selected_ind && has_combo == prev_has_combo)
	return (0);

    if (Data_form != NULL) {
	XtDestroyWidget (Data_form);
	XtDestroyWidget (Data_scroll);
    }

    /* create List_rows */
    if (List_rows != NULL)
	free (List_rows);
    List_row_height = -1;
    N_rows = 0;
    for (i = 0; i < n_des; i++)
	N_rows += des[i].n_values;
    List_rows = (List_row_t *)MISC_malloc (N_rows * sizeof (List_row_t));
    ind = 0;
    for (i = 0; i < n_des; i++) {
	for (k = 0; k < des[i].n_values; k++) {
	    List_rows[ind].de = des + i;
	    List_rows[ind].array_ind = k;
	    ind++;
	}
    }

    /* update the editing area lable */
    size = apps->name_field_width + TEXT_WIDTH + 24;
    for (i = 0; i < size; i++)
	buf[i] = ' ';
    buf[size - 1] = '\0';
    strncpy (buf + 2, "Name", 4);
    strncpy (buf + apps->name_field_width + 4, "Value", 5);
    strncpy (buf + apps->name_field_width + TEXT_WIDTH + 6, "Range", 5);
    Set_label_string (List_title, buf, apps->name_field_width);

    XtManageChild (Label_rowcol);

    /* create the editing area */
    Data_scroll = XtVaCreateManagedWidget ("data_scroll",
		xmScrolledWindowWidgetClass,	Parent,
		XmNscrollingPolicy,	XmAUTOMATIC,
		XmNforeground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		Label_rowcol,
		XmNbottomAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

    /* Get reference to clip window */
    XtVaGetValues (Data_scroll, XmNclipWindow, &clip, NULL );

    /* Register resize callback */
    XtAddCallback (clip, XmNresizeCallback, Resize_callback, NULL);

    Data_form = XtVaCreateWidget ("Data_form",
	    xmFormWidgetClass,	Data_scroll,
	    XmNbackground,		hci_get_read_color (BLACK),
	    XmNverticalSpacing,	1,
	    NULL);

    for (i = 0; i < N_rows; i++) {	/* create the rows */
	List_row_t *row;
	Dea_t *de;

	row = List_rows + i;
	de = row->de;
	if (i == 0)
	    row->rowcol = XtVaCreateWidget ("data_rowcol",
		xmRowColumnWidgetClass,	Data_form,
		XmNorientation,		XmHORIZONTAL,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNmarginHeight,	1,
		XmNmarginWidth,		0,
		NULL);
	else
	    row->rowcol = XtVaCreateWidget ("data_rowcol",
		xmRowColumnWidgetClass,	Data_form,
		XmNorientation,		XmHORIZONTAL,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		List_rows[i - 1].rowcol,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNmarginHeight,	1,
		XmNmarginWidth,		0,
		NULL);

	if (de->n_values == 1)
	    sprintf (buf, "%s", Sb + de->name_o);
	else
	    sprintf (buf, "%s [%d]", Sb + de->name_o, row->array_ind);
	row->name_w = Create_edit_area_label (
		Entends_string (buf, apps->name_field_width), row->rowcol);

	if (has_combo && de->n_sel_values > 0 && 
				Check_permission (Sb + de->perm_o)) {
	    char **labels, *p;
	    int k;
	    labels = (char **)MISC_malloc (sizeof (char *) * de->n_sel_values);
	    p = Sb + de->sel_values_o;
	    for (k = 0; k < de->n_sel_values; k++) {
		labels[k] = p;
		p += strlen (p) + 1;
	    }
	    row->value_w = Create_combo (row->rowcol, TEXT_WIDTH - 2, 
			(de->n_sel_values > 8 ? 8 : de->n_sel_values), 
			de->n_sel_values, labels, 
			Get_value_index (de, row->array_ind));
	    row->is_combo = 1;
	    free (labels);
	    XtAddCallback (row->value_w, XmNselectionCallback, 
					Edit_select_callback, (XtPointer)row);
	}
	else {

	    row->value_w = XtVaCreateManagedWidget ("tbl_value",
		xmTextFieldWidgetClass,	row->rowcol,
		XmNforeground,		hci_get_read_color (EDIT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNcolumns,		TEXT_WIDTH,
		XmNtraversalOn,		0,
		XmNeditable,		0,
		XmNtopShadowColor,	hci_get_read_color (EDIT_BACKGROUND),
		XmNbottomShadowColor,	hci_get_read_color (EDIT_FOREGROUND),
		XmNborderColor,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNmarginHeight,	0,
		XmNborderWidth,		0,
		XmNcursorPositionVisible,		False,
		NULL);
	    row->is_combo = 0;

	    XtAddCallback (row->value_w,
		XmNfocusCallback, Edit_gain_focus_cb, (XtPointer)row);
	    XtAddCallback (row->value_w,
		XmNmodifyVerifyCallback, Edit_verify_cb, (XtPointer)row);
	    XtAddCallback (row->value_w,
		XmNlosingFocusCallback, Edit_modify_cb, (XtPointer)row);
	    XtAddCallback (row->value_w,
		XmNactivateCallback, Edit_modify_cb, (XtPointer)row);
	}

	row->desc_w = Create_edit_area_label (
		Entends_string (Sb + de->desc_o, apps->desc_field_width), 
							row->rowcol);
	XtManageChild (row->rowcol);
    }
    XtManageChild (Data_form);
    Adjust_window_size ();
    prev_has_combo = has_combo;
    prev_ind = Selected_ind;

    return (0);
}

/************************************************************************

    Adjust the main window size.
	
************************************************************************/

static void Adjust_window_size () {
    Dimension width, height;
    int w, h, ht;

    if (N_rows == 0)
	return;

    w = h = 8;			/* window margin */
    XtVaGetValues (Data_form, XmNwidth, &width, XmNheight, &height, NULL);
    w += width;
    h += height;
    XtVaGetValues (Top_rowcol, XmNwidth, &width, XmNheight, &height, NULL);
    ht = height;
    XtVaGetValues (Select_rowcol, XmNwidth, &width, XmNheight, &height, NULL);
    ht += height;
    XtVaGetValues (Label_rowcol, XmNwidth, &width, XmNheight, &height, NULL);
    ht += height;

    h += ht;
    if (w > 1004) {
	w = 1000;
	h += 24;		/* scroll bar size */
    }
    if (h > 604) {
	h = 600;
	w += 24;		/* scroll bar size */
    }
    XtVaSetValues (Top_widget, XmNwidth, w, XmNheight, h, NULL);
    XtVaSetValues (Data_scroll, XmNwidth, w, XmNheight, h - ht, NULL);

    XtVaGetValues (List_rows[0].rowcol,
		XmNheight,	&height,
		NULL);
    List_row_height = height;
    Resize_callback (NULL, 0, 0);
}

/************************************************************************

    Creates a label in the data edit area GUI objects.
	
************************************************************************/

static Widget Create_edit_area_label (char *label, Widget parent) {
    Widget w;

    w = XtVaCreateManagedWidget (label,
		xmLabelWidgetClass,	parent,
		XmNforeground,		hci_get_read_color (EDIT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNhighlightColor,	hci_get_read_color (WHITE),
		XmNtopShadowColor,	hci_get_read_color (EDIT_BACKGROUND),
		XmNbottomShadowColor,	hci_get_read_color (EDIT_FOREGROUND),
		XmNborderColor,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNmarginWidth,		0,
		XmNmarginHeight,	0,
		XmNborderWidth,		0,
		NULL);
    return (w);
}

/************************************************************************

    Creates a combo widget.
	
************************************************************************/

static Widget Create_combo (Widget parent, int n_columns, 
	int visible_item_cnt, int n_items, char **labels, int init_position) {
    Widget w, combo_list;
    int i;

    w = XtVaCreateWidget ("combo",
		xmComboBoxWidgetClass,	parent,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNcomboBoxType,	XmDROP_DOWN_LIST,
		XmNcolumns,		n_columns,
		XmNvisibleItemCount,	visible_item_cnt,
		XmNmarginHeight,	0,
		NULL);

    XtVaGetValues (w, XmNlist, &combo_list, NULL);

    for (i = 0; i < n_items; i++) {
	XmString str;
	char buf[MAX_NAME_SIZE], *p;
	strncpy (buf, labels[i], MAX_NAME_SIZE);
	buf[MAX_NAME_SIZE - 1] = '\0';
	p = buf;
	while (*p != '\0') {
	    if (*p == '_')
		*p = ' ';
	    p++;
	}
	str = XmStringCreateLocalized (buf);
	XmListAddItemUnselected (combo_list, str, 0);
	XmStringFree (str);
    }
    XtVaSetValues (w,
		XmNselectedPosition,	init_position,
		NULL);

    XtManageChild (w);
    return (w);
}

/************************************************************************

    Creates a button with "label", sensitivity "sens", callback function 
    "cb" and parent "parent".
		
************************************************************************/

static Widget Create_button (char *label, Widget parent, 
					int sens, void (*cb) ()) {
    Widget w;

    w = XtVaCreateManagedWidget (label,
		xmPushButtonWidgetClass,parent,
		XmNforeground,		hci_get_read_color (BUTTON_FOREGROUND),
		XmNbackground,		hci_get_read_color (BUTTON_BACKGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNsensitive,		sens,
		NULL);
    XtAddCallback (w,
		XmNactivateCallback, cb, NULL);
    return (w);
}

/************************************************************************

    Creates a label with "label" and parent "parent".
		
************************************************************************/

static Widget Create_label (char *label, Widget parent) {
    Widget w;

    w = XtVaCreateManagedWidget (label,
		xmLabelWidgetClass,	parent,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);
    return (w);
}

/************************************************************************

    This function is activated when the user select the "Close" button.		
************************************************************************/

static void Close_callback (Widget w, 
			XtPointer client_data, XtPointer call_data) {

    Close_flag = 1;
    if (Change_flag == HCI_CHANGED_FLAG)
	/* prompt the user about saving changes */
	Save_callback (w, (XtPointer) NULL, (XtPointer) NULL);
    else
	HCI_task_exit (HCI_EXIT_SUCCESS);
}

/************************************************************************

    This function is activated when the user select the "Close" button
    in the "Description" popup.	
	
************************************************************************/

static void Description_close_callback (Widget w, 
			XtPointer client_data, XtPointer call_data) {

    HCI_Shell_popdown( Description_window );
}

/************************************************************************

    This function is the resizing callback function. In this function we
    resets the scroll bar increments.	
	
************************************************************************/

static void Resize_callback (Widget l, XtPointer y, XtPointer z) {
    Widget vsb;			/* Widget ID of vertical scroll bar */
    int slider_size;

    if (List_row_height < 0)
	return;

    XtVaGetValues (Data_scroll,
		XmNverticalScrollBar, &vsb,
		NULL);

    XtVaGetValues (vsb,
		XmNsliderSize, &slider_size,
		NULL);

    XtVaSetValues (vsb,
		XmNincrement,		List_row_height,
		XmNpageIncrement,	slider_size,
		NULL);
}

/************************************************************************

    This function is activated when the user selects the "Save" button.		
************************************************************************/

static void Save_callback (Widget w,
		XtPointer client_data, XtPointer call_data) {
    char buf[HCI_BUF_128];

    if (Change_flag != HCI_CHANGED_FLAG)
	return;
			
    sprintf( buf, "Do you want to save changes you made to\nthe database?" );
    hci_confirm_popup (Top_widget, buf, Save_yes_callback, Save_no_callback);
}

/************************************************************************

    This function is activated when the user selects the "Yes" button
    in the "Save" confirmation window.
	
************************************************************************/

static void Save_yes_callback (Widget w,
		XtPointer client_data, XtPointer call_data) {
    int a, d, i, updated, file_updated;
    char feedback_buf[256];

    updated = 0;
    for (a = 0; a < N_apps; a++) {
	Dea_t *des, *de;

	des = Apps[a].des;
	file_updated = 0;
	for (d = 0; d < Apps[a].n_des; d++) {
	    int str_type, ret;

	    de = des + d;
	    de->value_upd = 0;
	    if (de->back_values == NULL)	/* not updated */
		continue;
/* printf ("save id %s to the DB\n", Sb + de->id_o); */
	    str_type = 0;
	    if (de->type == DEAU_T_STRING)
		str_type = 1;
	    ret = HAA_set_values (Sb + de->id_o, str_type, 
					de->values, de->n_values, 0);
	    if (ret < 0)
		HCI_LE_error("HAA_set_values failed (%d)", ret);
	    free (de->back_values);		/* discard backup */
	    de->back_values = NULL;
	    de->value_upd = 1;
	    file_updated = updated = 1;
	    /* Create feedback message. Remove underscores ("_") from name. */
	    sprintf (feedback_buf, "%s adaptation data updated", Sb+Apps[a].display_name_o);
            for(i = 0; i < strlen(feedback_buf); i++ ) {
	      if( feedback_buf[i] == '_' ) {
	        feedback_buf[i] = ' ';
	      }
	    }
	}
	if (file_updated)
	    HAA_update_apps_dea_file (a);
    }
    if (updated) {
	HAA_set_values (NULL, 0, NULL, 0, 0);
	/* Post feedback message to main HCI. */
	HCI_display_feedback( feedback_buf );
    }

    Set_unchanged ();

    if (Close_flag == 1)
	HCI_task_exit (HCI_EXIT_SUCCESS);
    else if (Save_from == FROM_SECURITY_CB)
	Continue_security_cb ();
    else if (Save_from == FROM_SELECT_CALLBACK)
	Continue_select_callback ();
    Save_from = FROM_NONE;
}

/************************************************************************

    This function is activated when the user selects the "No" button
    in the "Save" confirmation window.
	
************************************************************************/

static void Save_no_callback (Widget w,
		XtPointer client_data, XtPointer call_data) {

    if (Close_flag == 1)
	HCI_task_exit (HCI_EXIT_SUCCESS);
    else if (Save_from == FROM_SECURITY_CB) {
	Undo_callback (w, client_data, call_data);
	Continue_security_cb ();
    }
    else if (Save_from == FROM_SELECT_CALLBACK) {
	Undo_callback (w, client_data, call_data);
	Continue_select_callback ();
    }
    Save_from = FROM_NONE;
}

/************************************************************************

    This function is activated when the user selects the "Undo" button.		
************************************************************************/

static void Undo_callback (Widget w,
		XtPointer client_data, XtPointer call_data) {
    int a, d;

    for (a = 0; a < N_apps; a++) {
	Dea_t *des, *de;

	des = Apps[a].des;
	for (d = 0; d < Apps[a].n_des; d++) {
	    de = des + d;
	    if (de->back_values == NULL)	/* not updated */
		continue;
/* printf ("undo id %s to the DB\n", Sb + de->id_o); */
	    free (de->values);
	    de->values = de->back_values;
	    de->back_values = NULL;
	}
    }

    Show_ac_table ();
    Set_unchanged ();
}

/************************************************************************

    This function is activated when the user selects the "Restore" button.		
************************************************************************/

static void Restore_baseline_callback (Widget w,
		XtPointer client_data, XtPointer call_data) {
    char buf[HCI_BUF_128];

    sprintf( buf, "You are about to restore the applications\nadaptation data to baseline values.\nDo you want to continue?" );
    hci_confirm_popup (Top_widget, buf, Restore_baseline_yes_callback, Restore_baseline_no_callback);
}

/************************************************************************

    This function is activated when the user selects the "Yes" button
    in the "Restore" confirmation dialog.
	
************************************************************************/

static void Restore_baseline_yes_callback (Widget w,
		XtPointer client_data, XtPointer call_data) {
    int d, updated;
    Dea_t *des, *de;

    updated = 0;
    des = Apps[Selected_ind].des;
    for (d = 0; d < Apps[Selected_ind].n_des; d++) {
	int ret, str_type;
	de = des + d;
	if (de->back_values != NULL) {	/* discard backups */
	    free (de->back_values);
	    de->back_values = NULL;
	}
	if (de->baseline == NULL || Is_current_equals_baseline (de))
	    continue;			/* identical */
/* printf ("use baseline id %s to the DB\n", Sb + de->id_o); */
	if (de->values != NULL)
	    free (de->values);
	de->values = Duplicate_values (de, 1);
	str_type = 0;
	if (de->type == DEAU_T_STRING)
	    str_type = 1;
	ret = HAA_set_values (Sb + de->id_o, str_type, 
				    de->values, de->n_values, 0);
	if (ret < 0)
	    HCI_LE_error("HAA_set_values failed (%d)", ret);
	if (de->back_values != NULL) {
	    free (de->back_values);		/* discard backup */
	    de->back_values = NULL;
	}
	de->value_upd = 1;
	updated = 1;
    }
    if (updated) {
	HAA_set_values (NULL, 0, NULL, 0, 0);
    }

    Change_flag = HCI_NOT_CHANGED_FLAG;
    XtVaSetValues (Save_button, XmNsensitive, False, NULL);
    XtVaSetValues (Undo_button, XmNsensitive, False, NULL);
    HCI_LE_log("Application adaptation data restored from baseline");
    Show_ac_table ();
}

/************************************************************************

    This function is activated when the user selects the "No" button
    in the "Restore" confirmation dialog.
	
************************************************************************/

static void Restore_baseline_no_callback (Widget w,
		XtPointer client_data, XtPointer call_data) {
}

/************************************************************************

    This function is activated when the user selects the "Update" button.		
************************************************************************/

static void Update_baseline_callback (Widget w,
		XtPointer client_data, XtPointer call_data) {
    char buf[HCI_BUF_128];
 
    sprintf( buf, "You are about to update the baseline\napplication adaptation data values.\nDo you want to continue?" );
    hci_confirm_popup (Top_widget, buf, Update_baseline_yes_callback, Update_baseline_no_callback);
}

/************************************************************************

    This function is activated when the user selects the "Yes" button
    in the "Update" confirmation dialog.
	
************************************************************************/

static void Update_baseline_yes_callback (Widget w,
		XtPointer client_data, XtPointer call_data) {
    int d, i, updated;
    Dea_t *des, *de;
    char feedback_buf[256];

    updated = 0;
    des = Apps[Selected_ind].des;
    for (d = 0; d < Apps[Selected_ind].n_des; d++) {
	int str_type, ret;

	de = des + d;
	de->baseline_upd = 0;
	if (de->baseline == NULL || Is_current_equals_baseline (de))
	    continue;		/* identical */
/* printf ("write baseline id %s to the DB\n", Sb + de->id_o); */
	str_type = 0;
	if (de->type == DEAU_T_STRING)
	    str_type = 1;
	ret = HAA_set_values (Sb + de->id_o, str_type, 
				    de->values, de->n_values, 1);
	if (ret < 0)
	    HCI_LE_error("HAA_set_values failed (%d)", ret);
	if (de->baseline != NULL)
	    free (de->baseline);
	de->baseline = Duplicate_values (de, 0);
	de->baseline_upd = 1;
	updated = 1;
	/* Create feedback message. Remove underscores ("_") from name. */
	sprintf (feedback_buf, "%s baseline adaptation data updated", Sb+Apps[Selected_ind].display_name_o);
        for(i = 0; i < strlen(feedback_buf); i++ ) {
	  if( feedback_buf[i] == '_' ) {
	    feedback_buf[i] = ' ';
	  }
	}
    }
    if (updated) {
	HAA_update_apps_dea_file (Selected_ind);
	HAA_set_values (NULL, 0, NULL, 0, 0);
	/* Post feedback message to main HCI. */
	HCI_display_feedback( feedback_buf );
    }
}

/************************************************************************

    This function is activated when the user selects the "No" button
    in the "Update" confirmation dialog.
	
************************************************************************/

static void Update_baseline_no_callback (Widget w,
		XtPointer client_data, XtPointer call_data) {
}

/************************************************************************

    This function is activated when the user selects the lock icon button 
    and changes the LOCA or password state.
				
************************************************************************/

static int Security_cb () {

    if( hci_lock_open() )
    {
	  if (!Locked_by_myself) {
		int ret = DEAU_remote_lock_de ("alg", LB_EXCLUSIVE_LOCK);
		if (ret == 0) {
		    Locked_by_myself = 1;
		}
		else {
		    if (ret != LB_HAS_BEEN_LOCKED)
			HCI_LE_error("DEAU_remote_lock_de failed (%d)", ret);
		    hci_warning_popup( Top_widget, "Another user has locked the\nadaptation data. Try again later.", NULL );
                    return HCI_LOCK_CANCEL;
		}
	  }
    }
    else if( hci_lock_loca_selected() ) {
	/* LOCA button selected */
	if( hci_lock_URC_selected() || hci_lock_ROC_selected() || hci_lock_AGENCY_selected() )
	{
	  Edit_mode = 0;
	  Active_row = NULL;
	}
    }
    else if( hci_lock_loca_unlocked() ) {
	/* the lock has been unlocked */
	if( hci_lock_URC_unlocked() ){ Unlocked_urc = HCI_YES_FLAG; }
	if( hci_lock_ROC_unlocked() ){ Unlocked_roc = HCI_YES_FLAG; }
	if( hci_lock_AGENCY_unlocked() ){ Unlocked_agency = HCI_YES_FLAG; }
	Edit_mode = 1;
	HAA_delete_des ();		/* reread all DEA */
    }
    else if( hci_lock_close() &&
		( Unlocked_urc == HCI_YES_FLAG ||
		  Unlocked_roc == HCI_YES_FLAG ||
		  Unlocked_agency == HCI_YES_FLAG ) ) {

	/* the lock has been closed */
        Unlocked_urc = HCI_NO_FLAG; 
        Unlocked_roc = HCI_NO_FLAG; 
        Unlocked_agency = HCI_NO_FLAG; 
	Edit_mode = 0;
	Active_row = NULL;
	if (Change_flag == HCI_CHANGED_FLAG) { 
				/* prompt the user about saving changes */
		Save_from = FROM_SECURITY_CB;
		Save_callback (Top_widget, (XtPointer) NULL, (XtPointer) NULL);
		return HCI_LOCK_CANCEL;
	}
	DEAU_remote_lock_de ("alg", LB_UNLOCK);
	Locked_by_myself = 0;
    }

    Show_ac_table ();
    if (Edit_mode)
	Set_unchanged ();
    else {
	XtVaSetValues (Restore_button, XmNsensitive, False, NULL);
	XtVaSetValues (Update_button, XmNsensitive, False, NULL);
    }

    return HCI_LOCK_PROCEED;
}

/**********************************************************************

    Continues function Security_cb.

***********************************************************************/

static void Continue_security_cb () {

    DEAU_remote_lock_de ("alg", LB_UNLOCK);
    Locked_by_myself = 0;
    Show_ac_table ();
    if (Edit_mode)
	Set_unchanged ();
    else {
	XtVaSetValues (Restore_button, XmNsensitive, False, NULL);
	XtVaSetValues (Update_button, XmNsensitive, False, NULL);
    }
}

/**********************************************************************

    Type combo box widget callback function. Refer to Type_callback, 
    hci_nb_gui.c

***********************************************************************/

static void Select_callback (Widget w, XtPointer client_data,
						XtPointer call_data) {
    XmComboBoxCallbackStruct *cbs =
			(XmComboBoxCallbackStruct *) call_data;

    if (Selected_ind == cbs->item_position) {
	Set_raise ();
	return;
    }
    if (Change_flag == HCI_CHANGED_FLAG) {
	/* prompt the user about saving changes */
	Save_from = FROM_SELECT_CALLBACK;
	Cbs_item_position = cbs->item_position;
	Save_callback (w, (XtPointer) NULL, (XtPointer) NULL);
	Set_raise ();
	return;
    }
    Selected_ind = cbs->item_position;
    Show_ac_table ();
    Set_raise ();
    Update_description_text ();
}

/**********************************************************************

    Continues function Select_callback.

***********************************************************************/

static void Continue_select_callback () {
    Selected_ind = Cbs_item_position;
    Show_ac_table ();
    Set_raise ();
    Update_description_text ();
}

/************************************************************************

    This function is activated when the user select the "Desription" button.		
************************************************************************/

static void Description_callback (Widget w, 
			XtPointer client_data, XtPointer call_data) {

    Arg args[20];
    Widget form, button, rowcol1;

    if( Description_window != NULL )
    {
      HCI_Shell_popup( Description_window );
      return;
    }

    HCI_Shell_init( &Description_window, "Adaptation Data Descriptions" );

    form = XtVaCreateWidget ("form",
		xmFormWidgetClass,	Description_window,
		XmNautoUnmanage,	False,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		NULL);

    rowcol1 = XtVaCreateWidget ("Rowcol1",
		xmRowColumnWidgetClass,	form,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNpacking,		XmPACK_TIGHT,
		XmNorientation,		XmHORIZONTAL,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNentryAlignment,	XmALIGNMENT_CENTER,
		NULL);

    button = Create_button (" Close ", rowcol1, True, 
					Description_close_callback);
    XtManageChild (rowcol1);

    XtSetArg(args[0], XmNrows,      30);
    XtSetArg(args[1], XmNcolumns,   80);
    XtSetArg(args[2], XmNeditable,  False);
    XtSetArg(args[3], XmNeditMode,  XmMULTI_LINE_EDIT);
    XtSetArg(args[4], XmNtopAttachment,	XmATTACH_WIDGET);
    XtSetArg(args[5], XmNtopWidget,		rowcol1);
    XtSetArg(args[6], XmNleftAttachment,	XmATTACH_FORM);
    XtSetArg(args[7], XmNrightAttachment,	XmATTACH_FORM);
    XtSetArg(args[8], XmNbottomAttachment,	XmATTACH_FORM);
    XtSetArg(args[9], XmNforeground, hci_get_read_color (TEXT_FOREGROUND));
    XtSetArg(args[10], XmNbackground, hci_get_read_color (BACKGROUND_COLOR1));
    XtSetArg(args[11], XmNfontList, hci_get_fontlist (LIST));
    XtSetArg(args[12], XmNscrollHorizontal, False);
    XtSetArg(args[13], XmNwordWrap, True);
    XtSetArg(args[14], XmNcursorPositionVisible, False);
    Description_text = XmCreateScrolledText(form, "Description_text", args, 15);
    XtManageChild(Description_text);

    XtManageChild (form);

    Update_description_text ();
    HCI_Shell_start( Description_window, RESIZE_HCI );
}

/**********************************************************************

    Update text in the Description window.

***********************************************************************/

static void Update_description_text () {
    static char *buf = NULL;
    int i;

    if (Description_text == NULL)
	return;

    buf = STR_reset (buf, 1024);
    for (i = 0; i < N_rows; i++) {
	List_row_t *row;
	Dea_t *de;

	row = List_rows + i;
	de = row->de;
	buf = STR_append (buf, "\n", strlen ("\n"));
	buf = STR_append (buf, Sb + de->name_o, strlen (Sb + de->name_o));
	buf = STR_append (buf, ": ", strlen (": "));
	buf = STR_append (buf, Sb + de->description_o, 
					strlen (Sb + de->description_o));
	buf = STR_append (buf, "\n", strlen ("\n"));
    }
    buf = STR_append (buf, "\0", 1);
    XmTextSetString (Description_text, buf);
}

/**********************************************************************

    Type combo box widget callback function. Refer to Type_callback, 
    hci_nb_gui.c

***********************************************************************/

static void Edit_select_callback (Widget w, XtPointer client_data,
						XtPointer call_data) {
    List_row_t *row;
    Dea_t *de;
    char *p;
    int i;
    XmComboBoxCallbackStruct *cbs =
			(XmComboBoxCallbackStruct *) call_data;

    row = (List_row_t *)client_data;
    de = row->de;
    Decorate_active_row (w, client_data, call_data);

    p = Sb + de->sel_values_o;
    for (i = 0; i < cbs->item_position; i++)
	p += strlen (p) + 1;

    if (de->type != DEAU_T_STRING) {
	double d;
	if (Convert_to_double (p, &d) >= 0 &&
	    d == ((double *)de->values)[row->array_ind]) { /* not changed */
	    Set_raise ();
	    return;
	}
    }
    else {
	if (strcmp (p, HAA_get_text_from_value (de, row->array_ind)) == 0) {
	    Set_raise ();
	    return;
	}
    }
/* printf ("accept (select) input %s\n", p); */
    Update_value (de, p, row->array_ind);
    Set_changed ();
    Set_raise ();
}

/************************************************************************

    This function is activated when the user modifies an editable item 
    in the table.

************************************************************************/

static void Edit_modify_cb (Widget w, XtPointer client_data, 
						XtPointer call_data) {
    Dea_t *de;
    List_row_t *row;
    char *text, buf[512];
    double d;
    XmAnyCallbackStruct *cb = (XmAnyCallbackStruct *)call_data;

    if (cb->reason == XmCR_LOSING_FOCUS)
	In_modification = 0;

    row = (List_row_t *)client_data;
    text = XmTextGetString (w);
    if (text == NULL)
	return;
    de = row->de;

    if (cb->reason == XmCR_LOSING_FOCUS) {
	XmTextSetString (row->value_w, text);
	XtVaSetValues (row->value_w, XmNcursorPositionVisible, False, NULL);
    }

    if (de->type != DEAU_T_STRING) {
	if (de->accuracy > 0.) {
	    double tv;
	    if (hci_rpg_adapt_str_accuracy_round (text, de->accuracy, &tv) == 0) {
		HAA_form_num_string (tv, de->type, buf);
		XmTextSetString (row->value_w, buf);
		text = XmTextGetString (w);
	    }
	}
	if (Convert_to_double (text, &d) < 0)
	    goto Failed;
	if (de->type == DEAU_T_DOUBLE || de->type == DEAU_T_FLOAT) {
	    HAA_form_num_string (d, de->type, buf);
	    if (strcmp (buf, text) != 0)
		XmTextSetString (row->value_w, buf);
	}
	if (d == ((double *)de->values)[row->array_ind]) /* not changed */
	    return;
	if (DEAU_check_data_range (Sb + de->id_o, 
				DEAU_T_DOUBLE, 1, (char *)&d) < 0)
	    goto Failed;
    }
    else {
	if (strcmp (text, HAA_get_text_from_value (de, row->array_ind)) == 0)
	    return;
	if (DEAU_check_data_range (Sb + de->id_o, 
				DEAU_T_STRING, 1, text) < 0)
	    goto Failed;
    }
/* printf ("accept input %s\n", text); */
    Update_value (de, text, row->array_ind);
    Set_changed ();

    return;

    Failed:
    /* If an error occurred, inform the user and reset the field to it's 
      prior state. */
    sprintf (buf,"An invalid value (%s) was\nentered - not accepted", text);
    hci_warning_popup (Top_widget, buf, NULL);
    XmTextSetString (w, HAA_get_text_from_value (de, row->array_ind));
}

/************************************************************************

    This function is activated when the user types something in the editing
    area.

************************************************************************/

static void Edit_verify_cb (Widget w, 
		XtPointer client_data, XtPointer call_data) {
    Dea_t *de;
    List_row_t *row;
    XmTextVerifyCallbackStruct	*cbs =
			(XmTextVerifyCallbackStruct *) call_data;

    if (cbs->event != NULL && cbs->event->type == KeyPress)
	In_modification = 1;

    row = (List_row_t *)client_data;
    de = row->de;

    if (de->type == DEAU_T_STRING)
	hci_verify_text_callback (w, (XtPointer)30, call_data);
    else if (de->type == DEAU_T_FLOAT || de->type == DEAU_T_DOUBLE) {
#ifdef ON_THE_FLY_ACCURACY_CHECK
   This and the next commented parts implement the while-typing accuracy check
	char *text;

	text = XmTextGetString (w);
#endif
	hci_verify_float_callback (w, (XtPointer)18, call_data);
#ifdef ON_THE_FLY_ACCURACY_CHECK
	if (text != NULL && cbs->text->ptr != NULL &&
	    de->accuracy > 0.) {		/* check accuracy */
	    char buf[256];
	    strcpy (buf, text);
	    if (cbs->text->length > 0) {	/* include newly typed chars */
		memcpy (buf + strlen (buf), cbs->text->ptr, cbs->text->length);
		buf[strlen (text) + cbs->text->length] = '\0';
	    }
	    if (hci_rpg_adapt_str_accuracy_round (buf, de->accuracy, NULL) == 0) {
		char *p;
		int cnt;
		p = text;
		while (*p == ' ' || *p == '\t')
		    p++;
		cnt = 0;			/* number of chars in text */
		while (*p != '\0') {
		    p++;
		    cnt++;
		}
		/* disallow the new chars beyond cnt chars */
		hci_verify_float_callback (w, (XtPointer)cnt, call_data);
	    }
	}
#endif

    }
    else 
	hci_verify_signed_integer_callback (w, (XtPointer)10, call_data);
}

/************************************************************************

    This function is activated when an editable table item receives input 
    focus.

************************************************************************/

static void Edit_gain_focus_cb (Widget w, 
			XtPointer client_data, XtPointer call_data) {
    List_row_t *row;

    row = (List_row_t *)client_data;
    XtVaSetValues (row->value_w, XmNcursorPositionVisible, True, NULL);

    Decorate_active_row (w, client_data, call_data);
 }

/************************************************************************

    Enters in value changed mode.

************************************************************************/

static void Set_changed () {

    if (Change_flag == HCI_NOT_CHANGED_FLAG) {
	Change_flag = HCI_CHANGED_FLAG;
	XtVaSetValues (Save_button, XmNsensitive, True, NULL);
	XtVaSetValues (Undo_button, XmNsensitive, True, NULL);
	XtVaSetValues (Restore_button, XmNsensitive, False, NULL);
	XtVaSetValues (Update_button, XmNsensitive, False, NULL);
    }
}

/************************************************************************

    Enters in value not-changed mode.

************************************************************************/

static void Set_unchanged () {

    Change_flag = HCI_NOT_CHANGED_FLAG;
    XtVaSetValues (Save_button, XmNsensitive, False, NULL);
    XtVaSetValues (Undo_button, XmNsensitive, False, NULL);
    XtVaSetValues (Restore_button, XmNsensitive, True, NULL);
    XtVaSetValues (Update_button, XmNsensitive, True, NULL);
}

/************************************************************************

    Decorates (highlight) the current active row and remove decoration 
    for the previous active row.

************************************************************************/

static void Decorate_active_row (Widget w, 
			XtPointer client_data, XtPointer call_data) {
    List_row_t *row;

    if (Active_row == XtParent (w))
	return;		/* If the row hasn't changed then do nothing. */

    row = (List_row_t *)client_data;

    if (Active_row != NULL)
	XtVaSetValues (Active_row,
	    XmNbackground,	    	hci_get_read_color (BACKGROUND_COLOR1),
	    XmNborderColor,		hci_get_read_color (BACKGROUND_COLOR1),
	    XmNhighlightColor,		hci_get_read_color (BACKGROUND_COLOR1),
	    XmNtopShadowColor,		hci_get_read_color (BACKGROUND_COLOR1),
	    XmNbottomShadowColor,	hci_get_read_color (BACKGROUND_COLOR1),
	    NULL);

    Active_row = XtParent (w);

    XtVaSetValues (Active_row,
	    XmNbackground,		hci_get_read_color (WHITE),
	    XmNborderColor,		hci_get_read_color (WHITE),
	    XmNhighlightColor,		hci_get_read_color (WHITE),
	    XmNbottomShadowColor,	hci_get_read_color (WHITE),
	    XmNtopShadowColor,		hci_get_read_color (WHITE),
	    NULL);
}

/**************************************************************************

    Sets the text on label "label" to "string". If "extend" > 0, additinal
    spaces are appended to the string to extend it to "extend" bytes.

**************************************************************************/

static void Set_label_string (Widget label, char *string, int extend) {
    XmString str;

    if (extend == 0)
	str = XmStringCreateLocalized (string);
    else
	str = XmStringCreateLocalized (Entends_string (string, extend));
    XtVaSetValues (label, XmNlabelString, str, NULL);
    XmStringFree (str);
}

/************************************************************************

    Checks if "Current_security" permits an DE of permission spec "perm".
    Returns 1 if permission is granted or 0 otherwise.
	
************************************************************************/

static int Check_permission (char *perm) {
    char *key;
    DEAU_attr_t at;

    if (!Operational)
	return (1);

    if (Unlocked_agency == HCI_YES_FLAG)
	key = "AGENCY";
    else if (Unlocked_roc == HCI_YES_FLAG)
	key = "ROC";
    else if (Unlocked_urc == HCI_YES_FLAG)
	key = "URC";
    else 
	key = "";
    at.ats[DEAU_AT_PERMISSION] = perm;
    if (DEAU_check_permission (&at, key) <= 0)
	return (0);
    return (1);
}

/**************************************************************************

    Extends string "str" to "width" characters by adding " ".

**************************************************************************/

static char *Entends_string (char *str, int width) {
    static char buf[512];
    int len, i;

    if (width >= 512)
	width = 511;
    len = strlen (str);
    if (width - len <= 0)
	return (str);
    strcpy (buf, str);
    for (i = len; i < width; i++)
	buf[i] = ' ';
    buf[width] = '\0';
    return (buf);    
}

/************************************************************************

    Updates the "array_ind"-th value in "de" with "text". If the value
    has not backedup, it is backed up first.

************************************************************************/

static void Update_value (Dea_t *de, char *text, int array_ind) {
    int new_size, len, i;
    char *p, *pnew, *pold, *sp;

    if (de->values == NULL)
	return;
    if (de->back_values == NULL)
	de->back_values = Duplicate_values (de, 0);

    if (de->type == DEAU_T_STRING) {
	p = de->values;
	new_size = 0;
	for (i = 0; i < de->n_values; i++) {
	    len = strlen (p) + 1;
	    if (i != array_ind)
	       new_size += len;
	    p += len;
	}
	new_size += strlen (text) + 1;
	pnew = MISC_malloc (new_size);
	p = pold = de->values;
	de->values = pnew;
	for (i = 0; i < de->n_values; i++) {
	    if (i != array_ind)
		sp = p;
	    else
		sp = text;
	    strcpy (pnew, sp);
	    pnew += strlen (sp) + 1;
	    p += strlen (p) + 1;
	}
	free (pold);
    }
    else {
	double d;
	Convert_to_double (text, &d);
	((double *)de->values)[array_ind] = d;
    }
}

/************************************************************************

    Returns a duplicated copy of the current value of "de". The user must
    free the returned pointer. If "baseline" is non-zero, the baseline 
    value is duplicated.

************************************************************************/

static void *Duplicate_values (Dea_t *de, int baseline) {
    char *v, *p;
    int s;

    v = de->values;
    if (baseline)
	v = de->baseline;
    if (v == NULL)
	return (NULL);
    s = HAA_get_size_of_value_field (de->type, de->n_values, v);
    p = MISC_malloc (s);
    memcpy (p, v, s);
    return (p);
}

/************************************************************************

    Returns the index of the current "array_ind"-th value in the list of 
    all allowable values. Returns 0 in case the index cannot be found.
	
************************************************************************/

static int Get_value_index (Dea_t *de, int array_ind) {
    char *v, *p;
    int i;

    v = HAA_get_text_from_value (de, array_ind);
    if (v[0] == '\0')
	return (0);
    p = Sb + de->sel_values_o;
    for (i = 0; i < de->n_sel_values; i++) {
	if (de->type == DEAU_T_STRING) {
	    if (strcmp (v, p) == 0)
		return (i);
	}
	else {
	    double d1, d2;
	    if (Convert_to_double (v, &d1) < 0 ||
		Convert_to_double (p, &d2) < 0)
		return (0);
	    if (d1 == d2)
		return (i);
	}
	p += strlen (p) + 1;
    }
    return (0);
}

/************************************************************************

    Converts "tv", a numerical value in text format, to double and returns
    it with "dp". Returns 0 on success or -1 on failure.

************************************************************************/

static int Convert_to_double (char *tv, double *dp) {
    double d;
    char c;
    if (sscanf (tv, "%lf%c", &d, &c) != 1)
	return (-1);
    *dp = d;
    return (0);
}

/************************************************************************

    Returns 1 if the current values are identical to the baseline values,
    or 0 otherwise. No value or no baseline is considered as identical.
	
************************************************************************/

static int Is_current_equals_baseline (Dea_t *de) {
    int i;

    if (de->n_values == 0 || de->baseline == NULL)
	return (1);
    if (de->type == DEAU_T_STRING) {
	char *p1, *p2;
	p1 = de->values;
	p2 = de->baseline;
	for (i = 0; i < de->n_values; i++) {
	    if (strcmp (p1, p2) != 0)
		return (0);
	    p1 += strlen (p1) + 1;
	    p2 += strlen (p2) + 1;
	}
    }
    else {
	double *d1, *d2;
	d1 = (double *)de->values;
	d2 = (double *)de->baseline;
	for (i = 0; i < de->n_values; i++) {
	    if (d1[i] != d2[i])
		return (0);
	}
    }
    return (1);
}

/***************************************************************************

    This is to work around a problem (window goes back) in the Motif combo 
    widget.

***************************************************************************/

static void timer_proc () {

    if (Db_updated && !In_modification && Change_flag == HCI_NOT_CHANGED_FLAG) {
	Active_row = NULL;
	HAA_delete_des ();		/* reread all DEA */
	Show_ac_table ();
	Db_updated = 0;
    }
}

/**************************************************************************

    Sets the flag to raise the window later.

**************************************************************************/

static void Set_raise () {

    XSetInputFocus (HCI_get_display(), HCI_get_window(), RevertToParent, CurrentTime);

}

/**************************************************************************

    Database update notification function.

**************************************************************************/

static void Db_update_notify_func 
		(EN_id_t event, char *msg, int msg_len, void *arg) {

    Db_updated = 1;
}



