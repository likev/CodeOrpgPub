/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:26:26 $
 * $Id: hci_uipm.c,v 1.12 2009/02/27 22:26:26 ccalvert Exp $
 * $Revision: 1.12 $
 * $State: Exp $
 */

/************************************************************************

    The module that implements the UIPM functions.

************************************************************************/

#include <hci.h>
#include <hci_uipm.h>

#define PROGRESS_BAR_HEIGHT 25
#define PROGRESS_BAR_WIDTH 550

/* properties */
static int Cancel_type;		/* UIPM_CAN_CANCEL or UIPM_NO_CANCEL */
static char *Operation = NULL;
static int Simulation_speed;
static int Low_bandwidth;

static XtAppContext App_context;
			/* Application context needed for X event processing
			   while data is being transferred */
static Display *display;
static Widget Operation_label;	
			/* Label showing the users current operation */
static Widget Transfer_rate_label;
			/* Text displayed in the transfer rate label */
static Widget Transfer_op_label;
			/* Label showing the current transfer operation */
static Widget Progress_widget;	
			/* Drawing area that contains the actual progress bar.
			   The progress bar reports the percent complete for a 
			   specific rmt.3 data transfer */
static Widget Main_widget;
static Widget Main_form;/* Main form widget that contains a frame widget */
static Widget Cancel_form;
			/* Form containing a cancel button which may be hidden
			   from the user */
static Widget Cancel_button;

static char *Transfer_rate_label_str = NULL;
			/* Text displayed in the transfer rate label */
static char *Transfer_op_label_str = NULL;
			/* Text displayed in the transfer operation label */
static int Visible;	/* The meter is visible */
static int Trans_bytes;	/* Total bytes transferred since the last call to the 
			   Reset method */
static int Total_bytes;
static GC Progress_gc;	/* graphics context for the Progress_widget */
static double Prev_time;/* Time of last call to the Reset method */
static int Cancelled;	/* 1 - the dialog has been cancelled. 0 otherwise */
static int Bar_width;

static int Initialized = 0;
static int Operate = 0;

static void Create (Widget parent);
static void Listen (int listen_flag);
static void Reset ();
static void SleepForSimulation (rmt_transfer_event_t *event);
static int SimulateLowBandwidth (rmt_transfer_event_t *event);
static int OnTransferEvent (rmt_transfer_event_t *event);
static int TransferListener (rmt_transfer_event_t *event);
static void Show ();
static void RefreshAppearance ();
static void Hide ();
static void Cancel_button_cb (Widget w,
			XtPointer client_data, XtPointer call_data);
static double Get_time ();


/************************************************************************

    Returns true if the progress in working, or false otherwise.

************************************************************************/

int UIPM_is_operate () {

    return (Operate);
}

/************************************************************************

    Sets the user text for display.

************************************************************************/

void UIPM_set_operation (const char* operation) {
    char *p;

    Operation = STR_copy (Operation, operation);
    p = Operation + strlen (Operation) - 1;	/* remove trailing \n */
    while (p >= Operation && *p == '\n') {
	*p = '\0';
	p--;
    }
    Operate = 1;
}

/************************************************************************

    Turns off the progress meter display.

************************************************************************/

void UIPM_destroy () {

    if (Initialized) {
	Listen (0);
	Reset ();
    }
    Operate = 0;
}

/************************************************************************

    Creates a progress meter.

************************************************************************/

void UIPM_create (int cancel_type, int simulation_speed, 
		int low_bandwidth, XtAppContext app_context, Widget parent) {

    if (low_bandwidth || simulation_speed > 0) {

	Cancel_type = cancel_type;
	Simulation_speed = simulation_speed;
	Low_bandwidth = low_bandwidth;
	Operation = STR_copy (Operation, "");
	Trans_bytes = Total_bytes = 0;
	Cancelled = 0;
	Visible = 0;
	if (!Initialized) {
	    display = XtDisplay (parent);
	    App_context = app_context;
	    Create (parent);
	    Initialized = 1;
	}
	Reset ();
	Operate = 1;
	Listen (1);
    }
}

/************************************************************************

    Sets up the progress callback.

************************************************************************/

static void Listen (int listen_flag) {

    if (listen_flag) {
	if (Low_bandwidth)
   	   RMT_listen_for_progress (TransferListener, NULL);
	else if (Simulation_speed > 0)
           RMT_listen_for_progress (SimulateLowBandwidth, NULL);
     }
     else {
	RMT_listen_for_progress (NULL, NULL); /* remove callback */
     }
}

/************************************************************************

    Transfer callback.

************************************************************************/

static int TransferListener (rmt_transfer_event_t *event) {
    int ret_value = 0;

    if (Initialized)
	ret_value = OnTransferEvent (event);
    return (ret_value);
}

/************************************************************************

    Transfer actions.

************************************************************************/

static int OnTransferEvent (rmt_transfer_event_t *event) {
    static int rpc_started = 0;
    static double last_update_time = 0.0;
    char sprintf_str[200];
    double new_time, rate;

    if (event->event != RMT_DATA_SENT && event->event != RMT_DATA_RECEIVED)
	return(0);

    if (Cancelled)
	return (Cancelled);

    new_time = Get_time ();
    Trans_bytes += event->no_of_segment_bytes;
    if (event->no_of_bytes == 0) {
	if (!rpc_started) {
	    Total_bytes += event->total_no_of_bytes;
	    rpc_started = 1;
	}
    }
    else 
	rpc_started = 0;
    if (new_time - last_update_time < .2) {
	SleepForSimulation (event);
	return (Cancelled);
    }
    last_update_time = new_time;

    Show ();

    if (Total_bytes > 0.)
	Bar_width = Trans_bytes * PROGRESS_BAR_WIDTH / Total_bytes;
    else
	Bar_width = 0;
    if (Bar_width > PROGRESS_BAR_WIDTH)
	Bar_width = PROGRESS_BAR_WIDTH;

    if (new_time - Prev_time > 0.0)
	rate = ((double)Trans_bytes / 1024.0) / (new_time - Prev_time);
    else
	rate = 0.0;
    sprintf (sprintf_str, "Transferred %d bytes in %1.2f seconds (%1.2f K/s)",
				Trans_bytes, new_time - Prev_time, rate);
    Transfer_rate_label_str = STR_copy (Transfer_rate_label_str, sprintf_str);

    if (event->event == RMT_DATA_RECEIVED)
	sprintf (sprintf_str, "Receiving %d bytes - (%d of %d received)", 
			event->no_of_segment_bytes, event->no_of_bytes, 
			event->total_no_of_bytes);
    else
	sprintf (sprintf_str, "Sending %d bytes - (%d of %d sent)", 
			event->no_of_segment_bytes, event->no_of_bytes, 
			event->total_no_of_bytes);
    Transfer_op_label_str = STR_copy (Transfer_op_label_str, sprintf_str);
    RefreshAppearance ();		/* Refresh the appearance */

    if (Cancelled) {
	Hide ();
	return (Cancelled);
    }
    SleepForSimulation (event);

    return (Cancelled);
}

/************************************************************************

    Simutes low bandwidth callback.

************************************************************************/

static int SimulateLowBandwidth (rmt_transfer_event_t *event) {

    if (Initialized)
	SleepForSimulation (event);
    return (0);
}

/************************************************************************

    Simutes low bandwidth.

************************************************************************/

static void SleepForSimulation (rmt_transfer_event_t *event) {

    if (Simulation_speed > 0) {	/* Sleep for a while to simulate Low BW */
	int sleep_time;
	sleep_time = (int)((double)(event->no_of_segment_bytes << 3) / 
				((double)Simulation_speed / 1000.0));
	msleep (sleep_time);
    }
}

/************************************************************************

    Resets the meter.

************************************************************************/

static void Reset () {
    Trans_bytes = Total_bytes = 0;
    Prev_time = Get_time ();
    Cancelled = 0;
    Operation = STR_copy (Operation, "");
    Hide ();
}

/************************************************************************

    Creates the GUI objects.

************************************************************************/

static void Create (Widget parent) {
    XGCValues gcv;
    Widget frame;
    Widget progress_row_column;

    HCI_Shell_init( &Main_widget, "Data Transfer Meter" );

    Main_form = XtVaCreateWidget ("Main_form",
			xmFormWidgetClass,	Main_widget,
			XmNmarginWidth, 5,
			XmNmarginHeight, 5,
			XmNdialogStyle, XmDIALOG_PRIMARY_APPLICATION_MODAL,
			XmNbackground, hci_get_read_color (BACKGROUND_COLOR1), 
			XmNforeground, hci_get_read_color (TEXT_FOREGROUND), 
			NULL);

    frame = XtVaCreateManagedWidget ("frame_widget",
			xmFrameWidgetClass, Main_form,
			XmNshadowType, XmSHADOW_ETCHED_IN,
			XmNshadowThickness, 5,
			XmNtopAttachment, XmATTACH_FORM,
			XmNleftAttachment, XmATTACH_FORM,
			XmNrightAttachment, XmATTACH_FORM,
			XmNbackground, hci_get_read_color (BACKGROUND_COLOR1), 
			XmNforeground, hci_get_read_color (TEXT_FOREGROUND), 
			NULL);

    progress_row_column = XtVaCreateWidget ("form",
			xmRowColumnWidgetClass, frame,
			XmNorientation, 	XmVERTICAL,
			XmNmarginWidth, 5,
			XmNmarginHeight, 5,
			XmNbackground, hci_get_read_color (BACKGROUND_COLOR1), 
			XmNforeground, hci_get_read_color (TEXT_FOREGROUND), 
			NULL);

    Operation_label = XtVaCreateManagedWidget (" ", 
			xmLabelWidgetClass, progress_row_column,
			XmNbackground, hci_get_read_color (BACKGROUND_COLOR1), 
			XmNforeground, hci_get_read_color (TEXT_FOREGROUND), 
			XmNfontList, hci_get_fontlist (LIST),
			NULL);

    Progress_widget = XtVaCreateManagedWidget ("meter", 
			xmDrawingAreaWidgetClass, progress_row_column,
			XmNwidth, PROGRESS_BAR_WIDTH,
			XmNheight, PROGRESS_BAR_HEIGHT,
			NULL);

    gcv.foreground = BlackPixelOfScreen (XtScreen (Progress_widget));
    Progress_gc = XCreateGC (XtDisplay (Progress_widget), 
			RootWindowOfScreen (XtScreen (Progress_widget)), 
			GCForeground, &gcv);

    Transfer_op_label_str = STR_copy (Transfer_op_label_str, 
					"No data received or sent");
    Transfer_op_label = XtVaCreateManagedWidget ("bottom_label", 
			xmLabelWidgetClass, progress_row_column, 
			XmNbackground, hci_get_read_color (BACKGROUND_COLOR1), 
			XmNforeground, hci_get_read_color (TEXT_FOREGROUND), 
			XmNfontList, hci_get_fontlist (LIST),
			NULL);

    Transfer_rate_label = XtVaCreateManagedWidget ("Transfer_rate_label", 
			xmLabelWidgetClass, progress_row_column, 
			XmNbackground, hci_get_read_color (BACKGROUND_COLOR1), 
			XmNforeground, hci_get_read_color (TEXT_FOREGROUND), 
			XmNfontList, hci_get_fontlist (LIST),
			NULL);
    Transfer_rate_label_str = STR_copy (Transfer_rate_label_str, 
			"Transferred 0 bytes in 0.0 seconds (0.0 K/s)");
    XtManageChild (progress_row_column);

    /*  Create cancel button as unmanaged */
    Cancel_form = XtVaCreateWidget ("Cancel_form", 
			xmFormWidgetClass, Main_form,
			XmNfractionBase, 3,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, progress_row_column,
			XmNleftAttachment, XmATTACH_FORM,
			XmNrightAttachment, XmATTACH_FORM,
			XmNmarginWidth, 5,
			XmNmarginHeight, 5,
			XmNbackground, hci_get_read_color (BACKGROUND_COLOR1), 
			XmNforeground, hci_get_read_color (TEXT_FOREGROUND), 
			NULL);

    Cancel_button = XtVaCreateManagedWidget ("Cancel", 
			xmPushButtonWidgetClass, Cancel_form,
			XmNleftAttachment, XmATTACH_POSITION,
			XmNleftPosition, 1,
			XmNrightAttachment, XmATTACH_POSITION,
			XmNrightPosition, 2,
			XmNtopAttachment, XmATTACH_FORM,
			XmNbottomAttachment, XmATTACH_FORM,
			XmNbackground, hci_get_read_color (BACKGROUND_COLOR1), 
			XmNforeground, hci_get_read_color (TEXT_FOREGROUND), 
			XmNfontList, hci_get_fontlist (LIST),
			NULL);
    XtAddCallback (Cancel_button, XmNactivateCallback, Cancel_button_cb, NULL);

    XtManageChild (Cancel_form);

    HCI_Shell_start( Main_widget, RESIZE_HCI );
}

/************************************************************************

    Cancel button pushed callback function.

************************************************************************/

static void Cancel_button_cb (Widget w,
			XtPointer client_data, XtPointer call_data) {
    Cancelled = 1;
}

/************************************************************************

    Displays the GUI.

************************************************************************/

static void Show () {

    if (!Visible) {
	Dimension width, height;
	int i;

	XtManageChild (Main_form);
	HCI_Shell_popup( Main_widget );

	/* Map this window in the case that it is iconified */
	if (XtIsRealized (Main_widget))
	    XMapRaised (XtDisplay (Main_widget), XtWindow (Main_widget));

	if (Cancel_type == 1)
	    XtVaSetValues (Cancel_button, XmNsensitive, True, NULL);
	else
	    XtVaSetValues (Cancel_button, XmNsensitive, False, NULL);

	/* Do not allow resizes */
	XtVaGetValues (Main_widget, XmNwidth, &width, NULL);
	XtVaGetValues (Main_widget, XmNheight, &height, NULL);
	XtVaSetValues (Main_widget, XmNmaxWidth, width, 
			XmNminWidth, width,
			XmNmaxHeight, height, 
			XmNminHeight, height, NULL);
	Visible = 1;
	for (i = 0; i < 2; i++) {	/* to make sure the screen updates */
	    RefreshAppearance ();
	    msleep (10);
	}
    }
}

/************************************************************************

    Displays the GUI.

************************************************************************/

static void RefreshAppearance () {
    static char *operation_text = NULL;
    XmString label;
    Window window;

    window = XtWindow (Progress_widget);
    XSetForeground (display, Progress_gc, hci_get_read_color (WHITE));
    XSetBackground (display, Progress_gc, hci_get_read_color (WHITE));
    XFillRectangle (display, window, Progress_gc, 
				0, 0, PROGRESS_BAR_WIDTH, PROGRESS_BAR_HEIGHT);
    XSetForeground (display, Progress_gc, hci_get_read_color (BLACK));
    XFillRectangle (display, window, Progress_gc, 
				0, 0, Bar_width, PROGRESS_BAR_HEIGHT);

    operation_text = STR_copy (operation_text, "Operation:\t");
    operation_text = STR_cat (operation_text, Operation);

    label = XmStringCreateLocalized (operation_text);
    XtVaSetValues (Operation_label, XmNlabelString, label, NULL);
    XmStringFree (label);

    label = XmStringCreateLocalized (Transfer_op_label_str);
    XtVaSetValues (Transfer_op_label, XmNlabelString, label, NULL);
    XmStringFree (label);

    label = XmStringCreateLocalized (Transfer_rate_label_str);
    XtVaSetValues (Transfer_rate_label, XmNlabelString, label, NULL);
    XmStringFree (label);

    if( Visible ){ XmUpdateDisplay( Main_widget ); }
}

/************************************************************************

    Turns off the progress meter GUI.

************************************************************************/

static void Hide () {

    if (Visible) {
	HCI_Shell_popdown( Main_widget );
	Visible = 0;
    }
}

/************************************************************************

    Returns the current time in double precision.

************************************************************************/

static double Get_time () {
    struct timeval tv;
    gettimeofday (&tv, NULL);
    return ((double)tv.tv_usec * .000001 + (double)tv.tv_sec);
}

