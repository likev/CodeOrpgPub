/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/02/27 22:04:56 $
 * $Id: hci_popup.c,v 1.1 2009/02/27 22:04:56 ccalvert Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

/************************************************************************
 *									*
 *	Module:  hci_popup.c						*
 *									*
 *	Description:  This module provides an easy API for creating	*
 *	popus for use in Motif applications.				*
 *									*
 ************************************************************************/

/* Local include file definitions. */

#include <hci.h>
#include <hci_popup.h>

/* Macros */

#define INFO_POPUP_TAG		"Info Popup"
#define WARNING_POPUP_TAG	"Warning Popup"
#define ERROR_POPUP_TAG		"Error Popup"
#define DEFAULT_LABEL		"Continue"
#define DEFAULT_YES_LABEL	"Yes"
#define DEFAULT_NO_LABEL	"No"

static void hci_popup_exit();

/************************************************************************
  Description: This function creates a popup for HCI applications when the
               RDA configuration changes. Acknowledgement close the task.
 ************************************************************************/

void hci_rda_config_change_popup()
{
  char buf[HCI_BUF_128];

  sprintf( buf, "The RDA configuration has changed. You must\nacknowledge this warning and reopen the %s window.", HCI_get_task_name() );
  hci_custom_popup( HCI_get_top_widget(), WARNING_POPUP_TAG, buf, DEFAULT_LABEL, hci_popup_exit, NULL, NULL, NULL, NULL, WARNING_COLOR, TEXT_FOREGROUND );
}

/************************************************************************
  Description: This function creates a customized 2 choice popup for HCI
               applications.
 ************************************************************************/

void hci_custom_confirm_popup( Widget w, char *msg, char *yes_label, void (*yes_cb)(), char *no_label, void (*no_cb)() )
{
  hci_custom_popup( w, WARNING_POPUP_TAG, msg, yes_label, yes_cb, no_label, no_cb, NULL, NULL, WARNING_COLOR, TEXT_FOREGROUND );
}

/************************************************************************
  Description: This function creates a Yes/No popup for HCI applications.
 ************************************************************************/

void hci_confirm_popup( Widget w, char *msg, void (*yes_cb)(), void (*no_cb)() )
{
  hci_custom_popup( w, WARNING_POPUP_TAG, msg, DEFAULT_YES_LABEL, yes_cb, DEFAULT_NO_LABEL, no_cb, NULL, NULL, WARNING_COLOR, TEXT_FOREGROUND );
}

/************************************************************************
  Description: This function creates a Info popup for HCI applications.
 ************************************************************************/

void hci_info_popup( Widget w, char *msg, void (*cb)() )
{
  hci_custom_popup( w, INFO_POPUP_TAG, msg, DEFAULT_LABEL, cb, NULL, NULL, NULL, NULL, BACKGROUND_COLOR1, TEXT_FOREGROUND );
}

/************************************************************************
  Description: This function creates a warning popup for HCI applications.
 ************************************************************************/

void hci_warning_popup( Widget w, char *msg, void (*cb)() )
{
  hci_custom_popup( w, WARNING_POPUP_TAG, msg, DEFAULT_LABEL, cb, NULL, NULL, NULL, NULL, WARNING_COLOR, TEXT_FOREGROUND );
}

/************************************************************************
  Description: This function creates a error popup for HCI applications.
 ************************************************************************/

void hci_error_popup( Widget w, char *msg, void (*cb)() )
{
  hci_custom_popup( w, ERROR_POPUP_TAG, msg, DEFAULT_LABEL, cb,NULL, NULL, NULL, NULL, WARNING_COLOR, TEXT_FOREGROUND );
}

/************************************************************************
  Description: This function creates a customized popup for HCI applications.
 ************************************************************************/

Widget hci_custom_popup(
  Widget w,
  char *popup_title,
  char *msg,
  char *label1, void (*callback1)(),
  char *label2, void (*callback2)(),
  char *label3, void (*callback3)(),
  int background_color, int foreground_color )
{
  Widget popup_dialog = NULL;
  Widget msg_widget;
  XmString msg_string;
  Widget b1, b2, b3;
  XmString b1_string, b2_string, b3_string;
  Widget separator_widget;
  Widget symbol_widget;

  /* Convert C-style buf to XmString. */
  msg_string = XmStringCreateLtoR( msg, XmFONTLIST_DEFAULT_TAG );

  /* Create dialog that will form the popup. */
  popup_dialog = XmCreateInformationDialog( w, "RPG", NULL, 0 );

  /* Button/label 1. If no button label, remove button. */
  b1 = XmMessageBoxGetChild( popup_dialog, XmDIALOG_OK_BUTTON );
  if( label1 != NULL )
  {
    b1_string = XmStringCreateLocalized( label1 );
    XtVaSetValues( popup_dialog, XmNokLabelString, b1_string, NULL );
    XtVaSetValues( b1,
                   XmNforeground, hci_get_read_color( BUTTON_FOREGROUND ),
                   XmNbackground, hci_get_read_color( BUTTON_BACKGROUND ),
                   XmNfontList, hci_get_fontlist( LIST ), NULL );
    XmStringFree( b1_string );
    if( callback1 != NULL )
    {
      XtAddCallback( b1, XmNactivateCallback, callback1, NULL );
    }
  }
  else
  {
    XtUnmanageChild( b1 );
  }

  /* Button/label 2. If no button label, remove button. */
  b2 = XmMessageBoxGetChild( popup_dialog, XmDIALOG_CANCEL_BUTTON );
  if( label2 != NULL )
  {
    b2_string = XmStringCreateLocalized( label2 );
    XtVaSetValues( popup_dialog, XmNcancelLabelString, b2_string, NULL );
    XtVaSetValues( b2,
                   XmNforeground, hci_get_read_color( BUTTON_FOREGROUND ),
                   XmNbackground, hci_get_read_color( BUTTON_BACKGROUND ),
                   XmNfontList, hci_get_fontlist( LIST ), NULL );
    XmStringFree( b2_string );
    if( callback2 != NULL )
    {
      XtAddCallback( b2, XmNactivateCallback, callback2, NULL );
    }
  }
  else
  {
    XtUnmanageChild( b2 );
  }

  /* Button/label 3. If no button label, remove button. */
  b3 = XmMessageBoxGetChild( popup_dialog, XmDIALOG_HELP_BUTTON );
  if( label3 != NULL )
  {
    b3_string = XmStringCreateLocalized( label3 );
    XtVaSetValues( popup_dialog, XmNhelpLabelString, b3_string, NULL );
    XtVaSetValues( b3,
                   XmNforeground, hci_get_read_color( BUTTON_FOREGROUND ),
                   XmNbackground, hci_get_read_color( BUTTON_BACKGROUND ),
                   XmNfontList, hci_get_fontlist( LIST ), NULL );
    XmStringFree( b3_string );
    if( callback3 != NULL )
    {
      XtAddCallback( b3, XmNactivateCallback, callback3, NULL );
    }
  }
  else
  {
    XtUnmanageChild( b3 );
  }

  /* If no buttons, then remove separator. */
  if( label1 == NULL && label2 == NULL && label3 == NULL )
  {
    separator_widget = XmMessageBoxGetChild( popup_dialog, XmDIALOG_SEPARATOR );
    XtUnmanageChild( separator_widget );
  }

  /* Message widget. */
  msg_widget = XmMessageBoxGetChild( popup_dialog, XmDIALOG_MESSAGE_LABEL );
  XtVaSetValues( msg_widget,
                 XmNforeground, hci_get_read_color( foreground_color ),
                 XmNbackground, hci_get_read_color( background_color ),
                 XmNfontList, hci_get_fontlist( LIST ), NULL );

  /* Remove symbol widget. */
  symbol_widget = XmMessageBoxGetChild( popup_dialog, XmDIALOG_SYMBOL_LABEL );
  XtUnmanageChild( symbol_widget );

  /* Finish setting dialog attributes. */
  XtVaSetValues( popup_dialog,
                 XmNmessageString, msg_string,
                 XmNforeground, hci_get_read_color( foreground_color ),
                 XmNbackground, hci_get_read_color( background_color ),
                 XmNfontList, hci_get_fontlist( LIST ),
                 XmNdialogStyle, XmDIALOG_PRIMARY_APPLICATION_MODAL,
                 XmNdeleteResponse, XmDESTROY, NULL );

  /* Remove close button to force use of buttons. */
  XtVaSetValues( XtParent( popup_dialog),
                 XmNmwmFunctions, MWM_FUNC_ALL | MWM_FUNC_CLOSE, NULL );

  /* Set title of popup. */
  if( popup_title != NULL && strlen( popup_title ) > 0 )
  {
    XtVaSetValues( XtParent( popup_dialog), XmNtitle, popup_title, NULL );
  }

  /* Cleanup. */
  XmStringFree( msg_string );

  /* Manage and show popup. */
  XtManageChild( popup_dialog );
  XtPopup( XtParent( popup_dialog ), XtGrabNone );
  return popup_dialog;
}

/************************************************************************
  Description: This function exits the calling HCI task.
 ************************************************************************/

static void hci_popup_exit()
{
  XtDestroyWidget( HCI_get_top_widget() );
}

