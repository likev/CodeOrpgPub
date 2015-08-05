/****************************************************************
 *								*
 *	hci_console_msg.c - This task is used to send a console	*
 *	message to the RPG.					*
 *								*
 ****************************************************************/

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/11/13 14:04:16 $
 * $Id: hci_console_msg.c,v 1.3 2012/11/13 14:04:16 ccalvert Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

/*	Local include file definitions.					*/

#include <hci.h>

/*	Macros.								*/

#define	MAX_MSG_LENGTH		128

/*	Global widget definitions					*/

Widget		Top_widget     = (Widget) NULL;
Widget		Console_msg_box = (Widget) NULL;

static void hci_close_console_msg_callback( Widget, XtPointer, XtPointer );
static void hci_send_console_msg_callback( Widget, XtPointer, XtPointer );
static void hci_clear_console_msg_callback( Widget, XtPointer, XtPointer );
static int  convert_date_to_julian( struct tm * );

/************************************************************************
 *	Description: This is the main function for the RPG Status task.	*
 *									*
 *	Input:  argc - number of commandline arguments			*
 *		argv - pointer to commandline argument data		*
 *	Output: NONE							*
 *	Return: exit code						*
 ************************************************************************/

int main( int argc, char *argv[] )
{
  Widget form;
  Widget button;
  Widget label;
  Widget rowcol;
  Widget frame;
  Arg args[16];
  int n;
	
  /*  Initialize HCI. */

  HCI_partial_init( argc, argv, -1 );

  Top_widget = HCI_get_top_widget();

  /* Need to reset the GUI title. */

  XtVaSetValues( Top_widget, XmNtitle, "RPG Console Message Tool", NULL );

  /* Set write permission to prevent premature exit. */

  ORPGDA_write_permission( ORPGDAT_RDA_CONSOLE_MSG );

  /* Build wigets. */

  form = XtVaCreateWidget( "console_msg_form",
		xmFormWidgetClass, Top_widget,
		XmNforeground, hci_get_read_color( BACKGROUND_COLOR1 ),
		XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
		NULL );

  rowcol = XtVaCreateManagedWidget( "control_rowcol",
		xmRowColumnWidgetClass, form,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
		XmNorientation, XmHORIZONTAL,
		XmNpacking, XmPACK_TIGHT,
		NULL );

  button = XtVaCreateManagedWidget( "Close",
		xmPushButtonWidgetClass, rowcol,
		XmNforeground, hci_get_read_color( BUTTON_FOREGROUND ),
		XmNbackground, hci_get_read_color( BUTTON_BACKGROUND ),
		XmNfontList, hci_get_fontlist( LIST ),
		XmNalignment, XmALIGNMENT_CENTER,
		NULL );

  XtAddCallback( button,
		XmNactivateCallback, hci_close_console_msg_callback,
		NULL );

  button = XtVaCreateManagedWidget( "Send Console Message",
		xmPushButtonWidgetClass, rowcol,
		XmNforeground, hci_get_read_color( BUTTON_FOREGROUND ),
		XmNbackground, hci_get_read_color( BUTTON_BACKGROUND ),
		XmNfontList, hci_get_fontlist( LIST ),
		XmNalignment, XmALIGNMENT_CENTER,
		NULL );

  XtAddCallback( button,
		XmNactivateCallback, hci_send_console_msg_callback,
		NULL );

  button = XtVaCreateManagedWidget( "Clear Console Message",
		xmPushButtonWidgetClass, rowcol,
		XmNforeground, hci_get_read_color( BUTTON_FOREGROUND ),
		XmNbackground, hci_get_read_color( BUTTON_BACKGROUND ),
		XmNfontList, hci_get_fontlist( LIST ),
		XmNalignment, XmALIGNMENT_CENTER,
		NULL );

  XtAddCallback( button,
		XmNactivateCallback, hci_clear_console_msg_callback,
		NULL );

  frame = XtVaCreateManagedWidget( "msg_frame",
                xmFrameWidgetClass, form,
                XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
                XmNtopWidget, rowcol,
                XmNtopAttachment, XmATTACH_WIDGET,
                XmNtopWidget, rowcol,
                XmNleftAttachment, XmATTACH_FORM,
                XmNrightAttachment, XmATTACH_FORM,
                NULL );


  label = XtVaCreateManagedWidget( "Msg:",
		xmLabelWidgetClass, frame,
                XmNchildType, XmFRAME_TITLE_CHILD,
                XmNchildHorizontalAlignment, XmALIGNMENT_CENTER,
                XmNchildVerticalAlignment, XmALIGNMENT_CENTER,
		XmNforeground, hci_get_read_color( TEXT_FOREGROUND ),
		XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
		XmNfontList, hci_get_fontlist( LIST ),
		NULL );

  n = 0;
  XtSetArg( args[n], XmNrows, 5 ); n++;
  XtSetArg( args[n], XmNcolumns, 80 ); n++;
  XtSetArg( args[n], XmNforeground, hci_get_read_color( EDIT_FOREGROUND )); n++;
  XtSetArg( args[n], XmNbackground, hci_get_read_color( EDIT_BACKGROUND )); n++;
  XtSetArg( args[n], XmNeditMode, XmMULTI_LINE_EDIT ); n++;
  XtSetArg( args[n], XmNtopAttachment, XmATTACH_WIDGET ); n++;
  XtSetArg( args[n], XmNtopWidget, label ); n++;
  XtSetArg( args[n], XmNfontList, hci_get_fontlist( LIST ) ); n++;
  XtSetArg( args[n], XmNwordWrap, True ); n++;
  XtSetArg( args[n], XmNscrollHorizontal,False ); n++;
  XtSetArg( args[n], XmNleftAttachment, XmATTACH_FORM ); n++;
  XtSetArg( args[n], XmNrightAttachment, XmATTACH_FORM ); n++;
  XtSetArg( args[n], XmNbottomAttachment, XmATTACH_FORM ); n++;

  Console_msg_box = XmCreateScrolledText( frame, "msg_box", args, n );
  XtManageChild( Console_msg_box );
  XtManageChild( form );
  XtRealizeWidget( Top_widget );

  HCI_start( NULL, -1, NO_RESIZE_HCI );

  return 0;
}

/************************************************************************
 *	Description: This function is activated when the user selects	*
 *		     the "Close" button.				*
 ************************************************************************/

static void hci_close_console_msg_callback(
Widget		w,
XtPointer	client_data,
XtPointer	call_data
)
{
  HCI_LE_log( "Close button selected" );
  HCI_task_exit( HCI_EXIT_SUCCESS );
}

/************************************************************************
 *      Description: This function is activated when the user selects   *
 *                   the "Send" button.                                 *
 ************************************************************************/

static void hci_send_console_msg_callback(
Widget          w,
XtPointer       client_data,
XtPointer       call_data
)
{
  char buf[MAX_MSG_LENGTH+1];
  char *msg_string;
  RDA_RPG_console_message_t rda_msg;
  struct tm *timeval;
  time_t now = time( NULL );
  int jd, ms;

  HCI_LE_log( "Send button selected" );

  msg_string = XmTextGetString( Console_msg_box );

  if( strlen( msg_string ) > MAX_MSG_LENGTH )
  {
    strncpy( buf, msg_string, MAX_MSG_LENGTH );
    buf[MAX_MSG_LENGTH-1] = '\0';
  }
  else
  {
    strcpy( buf, msg_string );
  }

  timeval = gmtime( &now );
  jd = convert_date_to_julian( timeval );
  ms = ((timeval->tm_hour*3600) + (timeval->tm_min*60) + timeval->tm_sec) * 1000;

  rda_msg.msg_hdr.size = (unsigned short) (sizeof(RDA_RPG_console_message_t)/2);
  rda_msg.msg_hdr.rda_channel = (unsigned char) 9;
  rda_msg.msg_hdr.type = (unsigned char) 10;
  rda_msg.msg_hdr.sequence_num = (unsigned short) 1;
  rda_msg.msg_hdr.julian_date = (unsigned short) jd;
  rda_msg.msg_hdr.milliseconds = (unsigned int) ms;
  rda_msg.msg_hdr.num_segs = (unsigned short) 1;
  rda_msg.msg_hdr.seg_num = (unsigned short) 1;
  
  rda_msg.size = strlen( buf );
  strcpy( rda_msg.message, buf );

  ORPGDA_write( ORPGDAT_RDA_CONSOLE_MSG, (char *) &rda_msg, sizeof( rda_msg ), LB_ANY );

  HCI_LE_log( "MSG LENGTH: %d  MSG: %s", strlen( buf ), buf );
}

/************************************************************************
 *      Description: This function is activated when the user selects   *
 *                   the "Clear" button.                                *
 ************************************************************************/

static void hci_clear_console_msg_callback(
Widget          w,
XtPointer       client_data,
XtPointer       call_data
)
{
  HCI_LE_log( "Clear button selected" );
  XmTextSetString( Console_msg_box, "" );
}

/************************************************************************
 *      Description: This function converts to julian date.		*
 ************************************************************************/

static int convert_date_to_julian( struct tm *t )
{
  int yr, mo, day;

  yr = t->tm_year + 1900;
  mo = t->tm_mon + 1;
  day = t->tm_mday;

  return ((1461*(yr+4800+(mo-14)/12))/4     +
          (367*(mo-2-12*((mo-14)/12)))/12   -
          (3*((yr+4900+(mo-14)/12 )/100))/4 +
          day-32075) - 2440587;
}



