/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/06/04 16:12:38 $
 * $Id: hci_pm.c,v 1.7 2014/06/04 16:12:38 steves Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */

/************************************************************************

    The module that implements the Progress meter functions.

************************************************************************/

/* Local include definition files. */

#include <hci.h>
#include <hci_pm.h>

/* Macros. */

#define PM_TITLE		"Data Transfer Meter"
#define PM_THICKNESS		5
#define PM_MARGIN		5
#define PM_MAX_LABEL_SIZE	HCI_BUF_128
#define PM_DEFAULT_OPERATION	"Default Operation"
#define PM_UPDATE_INTERVAL	0.1 /* 0.1 second = 100 milliseconds */
#define PM_WIDGET_MIN_WIDTH	400
#define PM_WIDGET_MARGIN_WIDTH	30

/* Static/global variables. */

static Display	*PM_display;
static Widget	PM_widget;
static Widget	PM_form;
static Widget	Operation_label;
static Widget	Transfer_bytes_label;

static int	PM_initialized_flag = HCI_NO_FLAG;
static int	PM_listening_flag = HCI_NO_FLAG;
static int	PM_active_flag = HCI_NO_FLAG;
static int	PM_skip_flag = HCI_NO_FLAG;
static int	Running_sum_of_bytes = 0;
static double	Previous_update_time = 0.0;
static char	Operation_label_buf[PM_MAX_LABEL_SIZE] = "";
static char	Transfer_bytes_label_buf[PM_MAX_LABEL_SIZE] = "";
static int	Satellite_connection_flag = HCI_YES_FLAG;
static int	Simulation_flag = HCI_YES_FLAG;
static int	Simulation_speed = 0;
static int	Debug_flag = HCI_NO_FLAG;

/* Function prototypes. */

static void PM_initialize();
static void PM_create_gui();
static void PM_popup();
static void PM_popdown();
static void PM_add_listener_callback();
static void PM_remove_listener_callback();
static void PM_adjust_size();
static int  PM_RMT_transfer_callback( rmt_transfer_event_t * );
static void PM_simulation_sleep( rmt_transfer_event_t *event );
static void Print_event( rmt_transfer_event_t *event );

/************************************************************************
    Public function to initialize/change progress meter.
************************************************************************/

void HCI_PM( const char* operation )
{
  char op_label_buf[PM_MAX_LABEL_SIZE] = "";
  XmString label;

  if( PM_skip_flag == HCI_YES_FLAG )
  {
    /* Return, if previously decided progress meters aren't needed. */
    return;
  }
  else if( PM_active_flag == HCI_YES_FLAG )
  {
    /* Return, if progress meter previously initialized and active. */
    return;
  }

  /* Set active flag to prevent re-entry. */

  PM_active_flag = HCI_YES_FLAG;

  /* Ensure operation label is valid. */

  if( operation != NULL )
  {
    if( strlen( operation ) < PM_MAX_LABEL_SIZE )
    {
      strcpy( op_label_buf, operation );
    }
    else
    {
      strncpy( op_label_buf, operation, PM_MAX_LABEL_SIZE );
      op_label_buf[PM_MAX_LABEL_SIZE-1] = '\0';
    }
  }
  else
  {
    strcpy( op_label_buf, PM_DEFAULT_OPERATION );
  }

  /* If operation label has changed, recreate widgets for correct sizing. */

  if( strcmp( op_label_buf, Operation_label_buf ) != 0 )
  {
    strcpy( Operation_label_buf, op_label_buf );
  }

  /* If first time, initialize progress meter. */

  if( PM_initialized_flag == HCI_NO_FLAG )
  {
    Satellite_connection_flag = HCI_is_satellite_connection();
    Simulation_speed = HCI_simulation_speed();
    if( Simulation_speed < 1 ){ Simulation_flag = HCI_NO_FLAG; }
    if( Satellite_connection_flag == HCI_NO_FLAG && Simulation_flag == HCI_NO_FLAG )
    {
      /* Skip popups, if not low bandwidth or not simulating low bandwidth. */
      PM_skip_flag = HCI_YES_FLAG;
      PM_active_flag = HCI_NO_FLAG;
      return;
    }
    PM_initialized_flag = HCI_YES_FLAG;
    PM_initialize();
    PM_display = HCI_get_display();
  }
  else
  {
    XtUnmanageChild( Operation_label );
    label = XmStringCreateLocalized( Operation_label_buf );
    XtVaSetValues( Operation_label, XmNlabelString, label, NULL );
    XmStringFree( label );
    XtManageChild( Operation_label );
    PM_adjust_size();
  }

  /* Add RMT listener callbacks which update the progress meter. */

  PM_add_listener_callback();

  /* Reset flags. */

  Running_sum_of_bytes = 0;
  Previous_update_time = 0.0;
}

/************************************************************************
    Initialize progress meter.
************************************************************************/

static void PM_initialize()
{
  HCI_Shell_init( &PM_widget, PM_TITLE );
  PM_create_gui();
  HCI_Shell_start( PM_widget, RESIZE_HCI );
  HCI_Shell_popdown( PM_widget );
}

/************************************************************************
    Create progress meter gui widgets.
************************************************************************/

static void PM_create_gui()
{
  Widget frame;
  Widget progress_row_column;

  PM_form = XtVaCreateWidget( "form_widget",
		xmFormWidgetClass, PM_widget,
		XmNmarginWidth, PM_MARGIN,
		XmNmarginHeight, PM_MARGIN,
		XmNdialogStyle, XmDIALOG_PRIMARY_APPLICATION_MODAL,
		XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ), 
		XmNforeground, hci_get_read_color( TEXT_FOREGROUND ), 
		NULL );

  frame = XtVaCreateManagedWidget( "frame_widget",
		xmFrameWidgetClass, PM_form,
		XmNshadowType, XmSHADOW_ETCHED_IN,
		XmNshadowThickness, PM_THICKNESS,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ), 
		XmNforeground, hci_get_read_color( TEXT_FOREGROUND ), 
		NULL );

  progress_row_column = XtVaCreateManagedWidget( "progress_rowcol",
		xmRowColumnWidgetClass, frame,
		XmNorientation, XmVERTICAL,
		XmNmarginWidth, PM_MARGIN,
		XmNmarginHeight, PM_MARGIN,
		XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ), 
		XmNforeground, hci_get_read_color( TEXT_FOREGROUND ), 
		NULL );

  Operation_label = XtVaCreateManagedWidget( Operation_label_buf, 
		xmLabelWidgetClass, progress_row_column,
		XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ), 
		XmNforeground, hci_get_read_color( TEXT_FOREGROUND ), 
		XmNfontList, hci_get_fontlist( LIST ),
		XmNrecomputeSize, True,
		NULL );

  Transfer_bytes_label = XtVaCreateManagedWidget( " ",
		xmLabelWidgetClass, progress_row_column, 
		XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ), 
		XmNforeground, hci_get_read_color( TEXT_FOREGROUND ), 
		XmNfontList, hci_get_fontlist( LIST ),
		XmNrecomputeSize, True,
		NULL );

  XtManageChild( PM_form );

  PM_adjust_size();
}

/************************************************************************
    Pop up progress meter.
************************************************************************/

static void PM_popup()
{
  HCI_Shell_popup( PM_widget );
}

/************************************************************************
    Pop down progress meter.
************************************************************************/

static void PM_popdown()
{
  HCI_Shell_popdown( PM_widget );
}

/************************************************************************
    Public function to hide the progress meter.
************************************************************************/

void HCI_PM_hide()
{
  if( PM_initialized_flag == HCI_NO_FLAG || PM_active_flag == HCI_NO_FLAG )
  {
    return;
  }
  PM_remove_listener_callback();
  PM_popdown();
  /* Make sure this is last to prevent re-entry. */
  PM_active_flag = HCI_NO_FLAG;
}

/************************************************************************
    Add RMT transfer event listener callback.
************************************************************************/

static void PM_add_listener_callback()
{
  if( PM_listening_flag == HCI_NO_FLAG )
  {
    PM_listening_flag = HCI_YES_FLAG;
    RMT_listen_for_progress( PM_RMT_transfer_callback, NULL );
  }
}

/************************************************************************
    Remove RMT transfer event listener callback.
************************************************************************/

static void PM_remove_listener_callback()
{
  if( PM_listening_flag == HCI_YES_FLAG )
  {
    PM_listening_flag = HCI_NO_FLAG;
    RMT_listen_for_progress( NULL, NULL );
  }
}

/************************************************************************
    Callback when RMT transfer event is received.
************************************************************************/

static int PM_RMT_transfer_callback( rmt_transfer_event_t *event )
{
  int bytes_transferred;
  char temp_buf[PM_MAX_LABEL_SIZE];
  double new_time;
  struct timeval tv;
  XmString label;

  /* Get time info and convert to seconds. Use gettimeofday() in lieu
     of time() to get finer resolution. Convert microseconds to fractional
     seconds. */

  gettimeofday( &tv, NULL );
  new_time = tv.tv_sec + tv.tv_usec/1000000.0;

  /* If debugging, print event info. */

  if( Debug_flag == HCI_YES_FLAG )
  {
    Print_event( event );
  }

  if( event->event != RMT_DATA_SENT && event->event != RMT_DATA_RECEIVED )
  {
    /* Ignore events that aren't send or receive. */
    return 0;
  }

  if( ( bytes_transferred = event->no_of_bytes ) > 0 )
  {
    Running_sum_of_bytes += bytes_transferred;

    if( ( new_time - Previous_update_time ) > PM_UPDATE_INTERVAL )
    {
      Previous_update_time = new_time;
      if( HCI_is_minimized() == HCI_NO_FLAG )
      {
        sprintf( temp_buf, "Transferred %d bytes", Running_sum_of_bytes );
        if( strlen( temp_buf ) < PM_MAX_LABEL_SIZE )
        {
          strcpy( Transfer_bytes_label_buf, temp_buf );
        }
        else
        {
          strncpy( Transfer_bytes_label_buf, temp_buf, PM_MAX_LABEL_SIZE );
          Transfer_bytes_label_buf[PM_MAX_LABEL_SIZE-1] = '\0';
        }
        label = XmStringCreateLocalized( Transfer_bytes_label_buf );
        XtVaSetValues( Transfer_bytes_label, XmNlabelString, label, NULL );
        XmStringFree( label );
        PM_popup();
      }
    }
  }

  if( Simulation_flag == HCI_YES_FLAG )
  {
    PM_simulation_sleep( event );
  }

  return 0;
}

/************************************************************************
    Adjust size of popup GUI.
************************************************************************/

static void PM_adjust_size()
{
  Dimension w1, w2, w3, max_width, new_width;

  XtVaGetValues( PM_widget, XmNwidth, &w1, NULL );
  XtVaGetValues( Operation_label, XmNwidth, &w2, NULL );
  XtVaGetValues( Transfer_bytes_label, XmNwidth, &w3, NULL );

  if( w2 > w3 ){ max_width = w2; }
  else{ max_width = w3; }

  if( ( new_width = max_width + PM_WIDGET_MARGIN_WIDTH ) < PM_WIDGET_MIN_WIDTH )
  {
    new_width = PM_WIDGET_MIN_WIDTH;
  }

  XtVaSetValues( PM_widget, XmNwidth, new_width, NULL );
}

/************************************************************************
    Simutes low bandwidth.
************************************************************************/

static void PM_simulation_sleep( rmt_transfer_event_t *event )
{
  /* Sleep to simulate Low BW */
  int sleep_time = (int)((double)(event->no_of_segment_bytes << 3) / 
		((double)Simulation_speed / 1000.0));
  msleep( sleep_time );
}

/************************************************************************
    Print event info.
************************************************************************/

static void Print_event( rmt_transfer_event_t *event )
{
  if(event->event == RMT_DATA_SENT){printf(" SENT ");}
  else if(event->event == RMT_DATA_RECEIVED){printf(" RCVD ");}
  if(event->segment_flags == RMT_LAST_SEGMENT){printf(" LAST ");}
  else{printf(" %1d ",event->segment_flags);}
  printf(" %6d ",event->no_of_segment_bytes);
  printf(" %6d ",event->no_of_bytes);
  printf(" %6d ",event->total_no_of_bytes);
  printf(" %f\n",event->no_of_seconds);
}

