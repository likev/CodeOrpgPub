/************************************************************************
 *									*
 *	Module:  hci_popup_precipitation_status.c			*
 *									*
 *	Description:  This module is used by the ORPG HCI to display,	*
 *		      the latest precipitation detection status data.	*
 *									*
 ************************************************************************/

/* 
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:26:16 $
 * $Id: hci_precip_status.c,v 1.22 2009/02/27 22:26:16 ccalvert Exp $
 * $Revision: 1.22 $
 * $State: Exp $
 */  

/*	Local include file definitions.					*/

#include <hci.h>
#include <hci_precip_status.h>

/*	Global widget definitons					*/

static	Widget		Top_widget = (Widget) NULL;
static	Widget		status_label;
static	Widget		time_last_exceeded_label;
static	Widget		trend_label;
static	Widget		sig_ref_label;
static	Widget		sig_ref_label_units;
static	Widget		sig_area_label;
static	Widget		sig_area_label_units;
static	Widget		total_rain_area_label;
static	Widget		total_rain_area_label_units;
static	Widget		thresh_reset_time_label;
static	Widget		thresh_reset_time_label_units;

/*	Local storage for precip detection adaptation data table	*/

int     hci_activate_child (Display *d, Window w, char *cmd,
                            char *proc, char *win, int object_index);
void	close_precip_status (Widget w,
		XtPointer client_data, XtPointer call_data);
void    hci_modify_precip_status_parameters (Widget w,
                XtPointer client_data, XtPointer call_data);
void	display_precip_status_data();
void	timer_proc ();

/************************************************************************
 *	Description: This is the main function for the Precipitation	*
 *		     Status task.					*
 *									*
 *	Input:  argc - number of commandline arguments			*
 *		argv - pointer to commandline arguments data		*
 *	Output: NONE							*
 *	Return: exit code						*
 ************************************************************************/

int
main (
int	argc,
char	*argv[]
)
{
	Widget		form;
	Widget		detection_frame;
	Widget		detection_rowcol;
	Widget		precip_table_frame;
	Widget		precip_table_rowcol;
	Widget		button_rowcol;
	Widget		button;
	Widget		label;
	Widget		rowcol;
	
/*	Initialize HCI.							*/

	HCI_init( argc, argv, HCI_PRECIP_STATUS_TASK );

/*	Check to see if the parent widget has been defined.  It must	*
 *	be defined first so that variaous X properties are defined.	*/

	Top_widget = HCI_get_top_widget();

	form = XtVaCreateManagedWidget ("precip_list_form",
		xmFormWidgetClass,	Top_widget,
		NULL);
		
/*	If low bandwidth, display a progress meter.			*/

	HCI_PM( "Reading Precipitation Data" );		
	
/*	Display a row at the top of the window for the Close button/label.*/

	button_rowcol = XtVaCreateManagedWidget ("button_rowcol",
		xmRowColumnWidgetClass, form,
		XmNorientation,		XmHORIZONTAL,
		XmNnumColumns,		1,
		XmNforeground,		hci_get_read_color (BLACK),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

	button = XtVaCreateManagedWidget( "Close",
		xmPushButtonWidgetClass,	button_rowcol,
		XmNforeground,	hci_get_read_color( BUTTON_FOREGROUND ),
		XmNbackground,	hci_get_read_color( BUTTON_BACKGROUND ),
		XmNfontList,	hci_get_fontlist( LIST ),
		NULL );

	XtAddCallback( button,
		XmNactivateCallback,	close_precip_status,
		NULL );

	button = XtVaCreateManagedWidget( "Modify Parameters",
		xmPushButtonWidgetClass,	button_rowcol,
		XmNforeground,	hci_get_read_color( BUTTON_FOREGROUND ),
		XmNbackground,	hci_get_read_color( BUTTON_BACKGROUND ),
		XmNfontList,	hci_get_fontlist( LIST ),
		NULL );

	XtAddCallback( button,
		XmNactivateCallback,	hci_modify_precip_status_parameters,
		NULL );

/*	Put space after top row of button(s).	*/

	label = XtVaCreateManagedWidget (" ",
		xmLabelWidgetClass,	form,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		button_rowcol,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

/*	Next row contains a frame to hold the detection info.	*/

	detection_frame = XtVaCreateManagedWidget( "detection_frame",
		xmFrameWidgetClass,	form,
		XmNforeground,		hci_get_read_color (BLACK),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		label,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

	label = XtVaCreateManagedWidget ("Precipitation Status",
		xmLabelWidgetClass,	detection_frame,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,		XmALIGNMENT_BEGINNING,
		XmNchildVerticalAlignment,		XmALIGNMENT_CENTER,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	detection_rowcol = XtVaCreateManagedWidget ("detection_rowcol",
		xmRowColumnWidgetClass, detection_frame,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,		XmVERTICAL,
		XmNnumColumns,		1,
		NULL);

	rowcol = XtVaCreateManagedWidget ("rowcol",
		xmRowColumnWidgetClass, detection_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,		XmHORIZONTAL,
		XmNnumColumns,		1,
		NULL);

	label = XtVaCreateManagedWidget ("Current Status:",
		xmLabelWidgetClass,	rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	label = XtVaCreateManagedWidget ("                                        ",
		xmLabelWidgetClass,	rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	status_label = XtVaCreateManagedWidget ("",
		xmLabelWidgetClass,	rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	rowcol = XtVaCreateManagedWidget ("rowcol",
		xmRowColumnWidgetClass, detection_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,		XmHORIZONTAL,
		XmNnumColumns,		1,
		NULL);

	label = XtVaCreateManagedWidget ("Significant Rain Area Threshold [RAINA] Last Exceeded:",
		xmLabelWidgetClass,	rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	label = XtVaCreateManagedWidget (" ",
		xmLabelWidgetClass,	rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	time_last_exceeded_label = XtVaCreateManagedWidget ("",
		xmLabelWidgetClass,	rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	rowcol = XtVaCreateManagedWidget ("rowcol",
		xmRowColumnWidgetClass, detection_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,		XmHORIZONTAL,
		XmNnumColumns,		1,
		NULL);

	label = XtVaCreateManagedWidget ("Significant Rain Area Trend:",
		xmLabelWidgetClass,	rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	label = XtVaCreateManagedWidget ("                           ",
		xmLabelWidgetClass,	rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	trend_label = XtVaCreateManagedWidget ("",
		xmLabelWidgetClass,	rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

/*	Put space after detection info.	*/

	label = XtVaCreateManagedWidget (" ",
		xmLabelWidgetClass,	form,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		detection_frame,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

/*	Next row contains a frame to hold the precip status table.	*/

	precip_table_frame = XtVaCreateManagedWidget( "precip_table_frame",
		xmFrameWidgetClass,	form,
		XmNforeground,		hci_get_read_color (BLACK),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		label,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL);

	label = XtVaCreateManagedWidget ("Precipitation Detection",
		xmLabelWidgetClass,	precip_table_frame,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,		XmALIGNMENT_BEGINNING,
		XmNchildVerticalAlignment,		XmALIGNMENT_CENTER,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	precip_table_rowcol = XtVaCreateManagedWidget ("precip_table_rowcol",
		xmRowColumnWidgetClass, precip_table_frame,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,		XmVERTICAL,
		XmNnumColumns,		1,
		NULL);

	rowcol = XtVaCreateManagedWidget ("rowcol",
		xmRowColumnWidgetClass, precip_table_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,		XmHORIZONTAL,
		XmNnumColumns,		1,
		NULL);

	label = XtVaCreateManagedWidget ("Reflectivity (dBZ) Representing Significant Rain [RAINZ]:",
		xmLabelWidgetClass,	rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	label = XtVaCreateManagedWidget ("             ",
		xmLabelWidgetClass,	rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	sig_ref_label = XtVaCreateManagedWidget ("",
		xmLabelWidgetClass,	rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	sig_ref_label_units = XtVaCreateManagedWidget (" dBZ",
		xmLabelWidgetClass,	rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	rowcol = XtVaCreateManagedWidget ("rowcol",
		xmRowColumnWidgetClass, precip_table_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,		XmHORIZONTAL,
		XmNnumColumns,		1,
		NULL);

	label = XtVaCreateManagedWidget ("Significant Rain Area Threshold [RAINA]:",
		xmLabelWidgetClass,	rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	label = XtVaCreateManagedWidget ("                              ",
		xmLabelWidgetClass,	rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	sig_area_label = XtVaCreateManagedWidget ("",
		xmLabelWidgetClass,	rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	sig_area_label_units = XtVaCreateManagedWidget (" km^2",
		xmLabelWidgetClass,	rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	rowcol = XtVaCreateManagedWidget ("rowcol",
		xmRowColumnWidgetClass, precip_table_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,		XmHORIZONTAL,
		XmNnumColumns,		1,
		NULL);

	label = XtVaCreateManagedWidget ("Significant Rain Area Detected:",
		xmLabelWidgetClass,	rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	label = XtVaCreateManagedWidget ("                                       ",
		xmLabelWidgetClass,	rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	total_rain_area_label = XtVaCreateManagedWidget ("",
		xmLabelWidgetClass,	rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	total_rain_area_label_units = XtVaCreateManagedWidget (" km^2",
		xmLabelWidgetClass,	rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	rowcol = XtVaCreateManagedWidget ("rowcol",
		xmRowColumnWidgetClass, precip_table_rowcol,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,		XmHORIZONTAL,
		XmNnumColumns,		1,
		NULL);

	label = XtVaCreateManagedWidget ("Threshold Time Without Accumulation Before Resetting STP [RAINT]:",
		xmLabelWidgetClass,	rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	label = XtVaCreateManagedWidget ("     ",
		xmLabelWidgetClass,	rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	thresh_reset_time_label = XtVaCreateManagedWidget ("",
		xmLabelWidgetClass,	rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

	thresh_reset_time_label_units = XtVaCreateManagedWidget (" min",
		xmLabelWidgetClass,	rowcol,
		XmNforeground,		hci_get_read_color (TEXT_FOREGROUND),
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist (LIST),
		NULL);

/*	Popupate the table with real data.                              */

	display_precip_status_data ();

        XtRealizeWidget (Top_widget);

/*	Start HCI loop.							*/

	HCI_start( timer_proc, HCI_TWO_SECONDS, NO_RESIZE_HCI );

	return 0;
}

/************************************************************************
 *	Description: This function is used to display precip		*
 *                   status data in the Precipitation Status window.	*
 *									*
 *	Input:  NONE							*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
display_precip_status_data (
)
{
  Precip_status_t	precip_status;
  char			label_buf[ 50 ];
  XmString		string;
  int			temp_int;
  float			temp_float;
  time_t		temp_time_t;

  /* Get latest precip status values. */
 
  precip_status = hci_get_precip_status();

  /* Fill hci labels with latest values. */

  temp_int = precip_status.current_precip_status;
  if( temp_int == PRECIP_NOT_ACCUM )
  {
    sprintf( label_buf, "      NOT ACCUMULATING      " );
    XtVaSetValues( status_label, XmNbackground,
                   hci_get_read_color( NORMAL_COLOR ), NULL );
  }
  else if( temp_int == PRECIP_ACCUM )
  {
    sprintf( label_buf, "        ACCUMULATING        " );
    XtVaSetValues( status_label, XmNbackground,
                   hci_get_read_color( NORMAL_COLOR ), NULL );
  }
  else if( temp_int == PRECIP_STATUS_UNKNOWN )
  {
    sprintf( label_buf, "             NA             " );
    XtVaSetValues( status_label, XmNbackground,
                   hci_get_read_color( BACKGROUND_COLOR2 ), NULL );
  }
  string = XmStringCreateLocalized( label_buf );
  XtVaSetValues( status_label, XmNlabelString, string, NULL );
  XmStringFree( string );

  temp_time_t = precip_status.time_last_exceeded_raina;

  if( temp_time_t == TIME_LAST_EXC_RAINA_UNKNOWN )
  {
    sprintf( label_buf, "             NA             " );
    XtVaSetValues( time_last_exceeded_label, XmNbackground,
                   hci_get_read_color( BACKGROUND_COLOR2 ), NULL );
  }
  else
  {
    strftime( label_buf, 50, " %b %d, %Y - %X UT ", gmtime( &temp_time_t ) ); 
    XtVaSetValues( time_last_exceeded_label, XmNbackground,
                   hci_get_read_color( WHITE ), NULL );
  }
  string = XmStringCreateLocalized( label_buf );
  XtVaSetValues( time_last_exceeded_label, XmNlabelString, string, NULL );
  XmStringFree( string );

  temp_int = precip_status.rain_area_diff;
  if( precip_status.rain_area_trend == TREND_INCREASING )
  {
    if( temp_int == PRECIP_AREA_DIFF_UNKNOWN )
    {
      sprintf( label_buf, "             NA             " );
      XtVaSetValues( trend_label, XmNbackground,
                     hci_get_read_color( BACKGROUND_COLOR2 ), NULL );
    }
    else
    {
      if( temp_int < 10 )
      {
        sprintf( label_buf, "    %s by %d km^2    ", "INCREASING", temp_int );
      }
      else if( temp_int < 100 )
      {
        sprintf( label_buf, "    %s by %d km^2   ", "INCREASING", temp_int );
      }
      else if( temp_int < 1000 )
      {
        sprintf( label_buf, "   %s by %d km^2   ", "INCREASING", temp_int );
      }
      else if( temp_int < 10000 )
      {
        sprintf( label_buf, "   %s by %d km^2  ", "INCREASING", temp_int );
      }
      else if( temp_int < 100000 )
      {
        sprintf( label_buf, "  %s by %d km^2  ", "INCREASING", temp_int );
      }
      else
      {
        sprintf( label_buf, "  %s by %d km^2 ", "INCREASING", temp_int );
      }
      XtVaSetValues( trend_label, XmNbackground,
                     hci_get_read_color( WHITE ), NULL );
    }
  }
  else if( precip_status.rain_area_trend == TREND_DECREASING )
  {
    if( temp_int == PRECIP_AREA_DIFF_UNKNOWN )
    {
      sprintf( label_buf, "             NA             " );
      XtVaSetValues( trend_label, XmNbackground,
                     hci_get_read_color( BACKGROUND_COLOR2 ), NULL );
    }
    else
    {
      if( temp_int < 10 )
      {
        sprintf( label_buf, "    %s by %d km^2    ", "DECREASING", temp_int );
      }
      else if( temp_int < 100 )
      {
        sprintf( label_buf, "    %s by %d km^2   ", "DECREASING", temp_int );
      }
      else if( temp_int < 1000 )
      {
        sprintf( label_buf, "   %s by %d km^2   ", "DECREASING", temp_int );
      }
      else if( temp_int < 10000 )
      {
        sprintf( label_buf, "   %s by %d km^2  ", "DECREASING", temp_int );
      }
      else if( temp_int < 100000 )
      {
        sprintf( label_buf, "  %s by %d km^2  ", "DECREASING", temp_int );
      }
      else
      {
        sprintf( label_buf, "  %s by %d km^2 ", "DECREASING", temp_int );
      }
      XtVaSetValues( trend_label, XmNbackground,
                     hci_get_read_color( WHITE ), NULL );
    }
  }
  else if( precip_status.rain_area_trend == TREND_STEADY )
  {
    sprintf( label_buf, "           STEADY           " );
    XtVaSetValues( trend_label, XmNbackground,
                   hci_get_read_color( WHITE ), NULL );
  }
  else if( precip_status.rain_area_trend == TREND_UNKNOWN )
  {
    sprintf( label_buf, "             NA             " );
    XtVaSetValues( trend_label, XmNbackground,
                   hci_get_read_color( BACKGROUND_COLOR2 ), NULL );
  }
  string = XmStringCreateLocalized( label_buf );
  XtVaSetValues( trend_label, XmNlabelString, string, NULL );
  XmStringFree( string );

  if( precip_status.rain_dbz_thresh_rainz == RAIN_DBZ_THRESH_UNKNOWN )
  {
    sprintf( label_buf, "  NA  " );
    XtVaSetValues( sig_ref_label, XmNbackground,
                   hci_get_read_color( BACKGROUND_COLOR2 ), NULL );
    XtVaSetValues( sig_ref_label_units, XmNforeground,
                   hci_get_read_color( BACKGROUND_COLOR1 ), NULL );
  }
  else 
  {
    temp_float = precip_status.rain_dbz_thresh_rainz;
    if( temp_float <= -10.0 )
    {
      sprintf( label_buf, " %5.1f", temp_float );
    }
    else if( temp_float < 0.0 )
    {
      sprintf( label_buf, " %4.1f ", temp_float );
    }
    else if( temp_float < 10.0 )
    {
      sprintf( label_buf, "  %3.1f ", temp_float );
    }
    else
    {
      sprintf( label_buf, " %4.1f ", temp_float );
    }
    XtVaSetValues( sig_ref_label, XmNbackground,
                   hci_get_read_color( WHITE ), NULL );
    XtVaSetValues( sig_ref_label_units, XmNforeground,
                   hci_get_read_color( TEXT_FOREGROUND ), NULL );
  }
  string = XmStringCreateLocalized( label_buf );
  XtVaSetValues( sig_ref_label, XmNlabelString, string, NULL );
  XmStringFree( string );

  if( precip_status.rain_area_thresh_raina == RAIN_AREA_THRESH_UNKNOWN )
  {
    sprintf( label_buf, "  NA  " );
    XtVaSetValues( sig_area_label, XmNbackground,
                   hci_get_read_color( BACKGROUND_COLOR2 ), NULL );
    XtVaSetValues( sig_area_label_units, XmNforeground,
                   hci_get_read_color( BACKGROUND_COLOR1 ), NULL );
  }
  else 
  {
    temp_int = precip_status.rain_area_thresh_raina;
    if( temp_int < 10 )
    {
      sprintf( label_buf, "   %1d  ", temp_int );
    }
    else if( temp_int < 100 )
    {
      sprintf( label_buf, "  %2d  ", temp_int );
    }
    else if( temp_int < 1000 )
    {
      sprintf( label_buf, "  %3d ", temp_int );
    }
    else if( temp_int < 10000 )
    {
      sprintf( label_buf, " %4d ", temp_int );
    }
    else if( temp_int < 100000 )
    {
      sprintf( label_buf, " %5d", temp_int );
    }
    else
    {
      sprintf( label_buf, "%6d", temp_int );
    }
    XtVaSetValues( sig_area_label, XmNbackground,
                   hci_get_read_color( WHITE ), NULL );
    XtVaSetValues( sig_area_label_units, XmNforeground,
                   hci_get_read_color( TEXT_FOREGROUND ), NULL );
  }
  string = XmStringCreateLocalized( label_buf );
  XtVaSetValues( sig_area_label, XmNlabelString, string, NULL );
  XmStringFree( string );

  if( precip_status.rain_area == PRECIP_AREA_UNKNOWN )
  {
    sprintf( label_buf, "  NA  " );
    XtVaSetValues( total_rain_area_label, XmNbackground,
                   hci_get_read_color( BACKGROUND_COLOR2 ), NULL );
    XtVaSetValues( total_rain_area_label_units, XmNforeground,
                   hci_get_read_color( BACKGROUND_COLOR1 ), NULL );
  }
  else
  {
    temp_int = precip_status.rain_area;
    if( temp_int < 10 )
    {
      sprintf( label_buf, "   %1d  ", temp_int );
    }
    else if( temp_int < 100 )
    {
      sprintf( label_buf, "  %2d  ", temp_int );
    }
    else if( temp_int < 1000 )
    {
      sprintf( label_buf, "  %3d ", temp_int );
    }
    else if( temp_int < 10000 )
    {
      sprintf( label_buf, " %4d ", temp_int );
    }
    else if( temp_int < 100000 )
    {
      sprintf( label_buf, " %5d", temp_int );
    }
    else
    {
      sprintf( label_buf, "%6d", temp_int );
    }
    XtVaSetValues( total_rain_area_label, XmNbackground,
                   hci_get_read_color( WHITE ), NULL );
    XtVaSetValues( total_rain_area_label_units, XmNforeground,
                   hci_get_read_color( TEXT_FOREGROUND ), NULL );
  }
  string = XmStringCreateLocalized( label_buf );
  XtVaSetValues( total_rain_area_label, XmNlabelString, string, NULL );
  XmStringFree( string );

  if( precip_status.rain_time_thresh_raint == RAIN_TIME_THRESH_UNKNOWN )
  {
    sprintf( label_buf, "  NA  " );
    XtVaSetValues( thresh_reset_time_label, XmNbackground,
                   hci_get_read_color( BACKGROUND_COLOR2 ), NULL );
    XtVaSetValues( thresh_reset_time_label_units, XmNforeground,
                   hci_get_read_color( BACKGROUND_COLOR1 ), NULL );
  }
  else
  {
    temp_int = precip_status.rain_time_thresh_raint;
    if( temp_int < 10 )
    {
      sprintf( label_buf, "   %1d  ", temp_int );
    }
    else if( temp_int < 100 )
    {
      sprintf( label_buf, "  %2d  ", temp_int );
    }
    else if( temp_int < 1000 )
    {
      sprintf( label_buf, "  %3d ", temp_int );
    }
    else
    {
      sprintf( label_buf, " %4d ", temp_int );
    }
    XtVaSetValues( thresh_reset_time_label, XmNbackground,
                   hci_get_read_color( WHITE ), NULL );
    XtVaSetValues( thresh_reset_time_label_units, XmNforeground,
                   hci_get_read_color( TEXT_FOREGROUND ), NULL );
  }
  string = XmStringCreateLocalized( label_buf );
  XtVaSetValues( thresh_reset_time_label, XmNlabelString, string, NULL );
  XmStringFree( string );
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Close" button in the Precipitation Status	*
 *		     window.						*
 *									*
 *	Input:  w - widget ID						*
 *		client_data - unused					*
 *		call_data - ununsed					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
close_precip_status (
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
	    XtDestroyWidget (Top_widget);
}

/************************************************************************
 *      Description: This function is activated when the user selects	*
 *                   the apps adapt button.				*
 *                                                                      *
 *      Input:  w - widget ID                                           *
 *      client_data - unused                                            *
 *      call_data - unused                                              *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

void
hci_modify_precip_status_parameters (
Widget          w,
XtPointer       client_data,
XtPointer       call_data
)
{
  char window_name[256];
  char task_name[256];
  char process_name[50];
  char channel_number[32];

  sprintf (window_name,"Algorithms");

  sprintf (channel_number,"-A 0");
  if( HCI_get_system() == HCI_FAA_SYSTEM )
  {
    sprintf( channel_number, "-A %1d",
             ORPGRED_channel_num( ORPGRED_MY_CHANNEL ) );
  }

  sprintf( process_name, "hci_apps_adapt %s", channel_number );
  sprintf( task_name, "%s -name \"Algorithms\" -I \"Hydromet Preprocessing\"", process_name );

  strcat( task_name, HCI_child_options_string() );

  HCI_LE_log( "Spawning %s", task_name );

  hci_activate_child( HCI_get_display(),
                      RootWindowOfScreen( HCI_get_screen() ),
                      task_name,
                      process_name,
                      window_name,
                      -1 );
}

/************************************************************************
 *	Description: This function is the timer procedure for the	*
 *		     Precipitation Status task.  Its main function is	*
 *		     to force a window update when adaptation data	*
 *		     changes are detected.				*
 *									*
 *	Input:  w - widget ID						*
 *		id - timer interval ID					*
 *	Output: NONE							*
 *	Return: NONE							*
 ************************************************************************/

void
timer_proc ()
{
  /*	Re-popupate the table with data.	*/
	display_precip_status_data ();
}

