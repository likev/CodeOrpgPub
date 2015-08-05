/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/01/24 16:52:42 $
 * $Id: hci_lock.c,v 1.41 2012/01/24 16:52:42 ccalvert Exp $
 * $Revision: 1.41 $
 * $State: Exp $
 */

/************************************************************************
 *									*
 *	Module:  hci_lock_widget.c					*
 *									*
 *	Description:  This module is used to create and control a	*
 *		      lock widget for the HCI GUI.			*
 *									*
 ************************************************************************/

/*	Local include file definitions.					*/

#include <hci.h>
#include <hci_lock_icon.h>
#include <hci_unlock_icon.h>
#include <hci_unlock_URC_icon.h>
#include <hci_unlock_AGE_icon.h>
#include <hci_unlock_OSF_icon.h>

#define	HCI_LOCK_CLOSE		0x00000001
#define	HCI_LOCK_OPEN		0x00000002
#define	HCI_LOCK_LOCA_SELECTED	0x00000004
#define	HCI_LOCK_LOCA_UNLOCKED	0x00000008

static	int	Initialize_bitmaps = 0; /* Flag for bitmap initialization */
static	int	Fg_color; /* Foreground color of lock button */
static	int	Bg_color; /* Background color of lock button */
static	int	Fg_unlock_color; /* Foreground color when unlocked */

/*	Global widget definitions.	*/

static	Widget	Parent_widget = (Widget) NULL;
static	Widget	Lock_button = (Widget) NULL;
static	Widget	Password_dialog = (Widget) NULL;
static	Widget	Password_text_box = (Widget) NULL;
static	Widget	Agency_button = (Widget) NULL;
static	Widget	Roc_button = (Widget) NULL;
static	Widget	Urc_button = (Widget) NULL;
static	Pixmap	Lock_button_pixmap;
static	int		Faa_redundant_override = HCI_NO_FLAG;
static	unsigned int	User_lock_LOCA_mask = HCI_LOCA_NONE;
static	unsigned int	User_selected_LOCA = HCI_LOCA_NONE;
static	unsigned int	Previous_user_selected_LOCA = HCI_LOCA_NONE;
static	unsigned int	Current_unlocked_LOCA = HCI_LOCA_NONE;
static	unsigned int	Previous_current_unlocked_LOCA = HCI_LOCA_NONE;
static	unsigned int	Password_dialog_state = HCI_LOCK_CLOSE;
static	unsigned int	Previous_password_dialog_state = HCI_LOCK_CLOSE;
static	char		*Passwd = (char *) NULL;
static	int		Operational_flag = HCI_NO_FLAG;
static	int		Lock_busy_flag = HCI_NO_FLAG;
static	int		Password_busy_flag = HCI_NO_FLAG;

static void	Lock_callback( Widget, XtPointer, XtPointer );
static void	Initialize_lock_bitmaps();
static void	Validate_password( char * );
static void	Close_lock_callback( Widget, XtPointer, XtPointer );
static void	LOCA_button_callback( Widget, XtPointer, XtPointer );
static void	Check_password( Widget, XtPointer, XtPointer );
static void	Create_password_dialog_widget();
static int	(*User_lock_callback)() = NULL;
static void	Change_lock_state( int );
static void	Set_lock_button_pixmap();
static void	Reset_password_text();
static void	Reset_password_LOCA_buttons();

/************************************************************************
 *	Description: This function creates a "Lock/Unlock" icon.  The	*
 *		     function handles all lock/unlock bitmap changes.	*
 *		     The user specified callback is called in addition	*
 *		     to the internal callbacks.  The parent widget id	*
 *		     of the lock/unlock	widget is returned.		*
 *									*
 *	Input:  parent		- parent widget for lock button		*
 *		(*user_callback)() - user callback			*
 *		lock_mask	- security mask				*
 *	Output: NONE							*
 *	Return: Widget ID of lock button				*
 ************************************************************************/

Widget hci_lock_widget( Widget parent, int (*user_callback)(), int LOCA_mask )
{
  /* If the parent widget does not exist, do nothing. */

  if( parent == (Widget) NULL ){ return NULL; }

  Parent_widget = parent;
  User_lock_callback = user_callback;
  User_lock_LOCA_mask = (unsigned int) LOCA_mask;
  Create_password_dialog_widget();

  /* Set colors for various lock states. */

  Fg_color        = hci_get_read_color( TEXT_FOREGROUND );
  Fg_unlock_color = hci_get_read_color( ALARM_COLOR1 );
  Bg_color        = hci_get_read_color( BUTTON_BACKGROUND );

  /* If this is the first time, then initialize the bitmaps used
     for the various lock button states. */

  if( !Initialize_bitmaps ){ Initialize_lock_bitmaps(); }

  /* Check to see if the FAA redundant override flag is set.  If it
     is, we want to inhibit the active/inactive channel check that
     is normally done when editing adaptation data.  This override
     is for RDA control since we want to allow the lock to be opened
     regardless of the channel. */

  if( User_lock_LOCA_mask & HCI_LOCA_FAA_OVERRIDE )
  {
    Faa_redundant_override = HCI_YES_FLAG;
  }
  else
  {
    Faa_redundant_override = HCI_NO_FLAG;
  }

  /* Set operational system flag. */

  if( ORPGMISC_is_operational() ){ Operational_flag = HCI_YES_FLAG; }
  else{ Operational_flag = HCI_NO_FLAG; }

  /* Create the lock widget and register its callback. */

  Set_lock_button_pixmap();

  Lock_button = XtVaCreateManagedWidget ("Lock_button",
		xmDrawnButtonWidgetClass,	Parent_widget,
		XmNlabelType,			XmPIXMAP,
		XmNlabelPixmap,			Lock_button_pixmap,
		XmNpushButtonEnabled,		True,
		XmNforeground,			Fg_color,
		XmNbackground,			Bg_color,
		XmNsensitive,			True,
		NULL);

  XtAddCallback( Lock_button, XmNactivateCallback, Lock_callback, NULL );

  return Lock_button;
}

/************************************************************************
 *	Description: This is the callback when the lock button is	*
 *		     selected.						*
 ************************************************************************/

static void Lock_callback( Widget w, XtPointer cl, XtPointer ca )
{
  if( Lock_busy_flag == HCI_YES_FLAG )
  {
    /* Not finished with previous operation, so do nothing. This
       can happen when a user double-clicks on a slow connection. */
    return;
  }

  Lock_busy_flag = HCI_YES_FLAG;

  if( Password_dialog_state == HCI_LOCK_CLOSE )
  {
    /* If this is an FAA redundant configuration, then we will not
       allow editing (unlocking) if the current RPG channel is 
       inactive and the other channel active. */

    if( HCI_get_system() == HCI_FAA_SYSTEM && 
        Faa_redundant_override == HCI_NO_FLAG )
    {
      if( ORPGRED_channel_state(ORPGRED_MY_CHANNEL) == ORPGRED_CHANNEL_INACTIVE
          &&
          ORPGRED_channel_state(ORPGRED_OTHER_CHANNEL) == ORPGRED_CHANNEL_ACTIVE
          &&
          ORPGRED_rpg_rpg_link_state() == ORPGRED_CHANNEL_LINK_UP )
      {
        hci_warning_popup( Parent_widget, "You cannot edit this data because the\nother RPG channel is Active.\n", NULL );
        return;
      }
    }

    /* State is locked. Try to unlock it. */
    if( Operational_flag == HCI_NO_FLAG )
    {
      /* If nonoperational, skip the password. */
      Previous_current_unlocked_LOCA = Current_unlocked_LOCA;
      Current_unlocked_LOCA = HCI_LOCA_NONOP;
      Change_lock_state( HCI_LOCK_LOCA_UNLOCKED );
    }
    else
    {
      /* Pop up password dialog. */
      Change_lock_state( HCI_LOCK_OPEN );
    }
  }
  else
  {
    /* State is unlocked, so try to lock it. */
    Change_lock_state( HCI_LOCK_CLOSE );
  }

  Lock_busy_flag = HCI_NO_FLAG;
}

/************************************************************************
 *	Description: This function creates the Password_dialog widget.	*
 ************************************************************************/

static void Create_password_dialog_widget()
{
  Widget control_form;
  Widget button;
  Widget password_rowcol;
  Widget password_form;
  Widget password_frame;
  Widget loca_frame;
  Widget loca_rowcol;
  Widget loca_list;
  int n;
  Arg args [10];

  /* Initialize password dialog. */

  HCI_Shell_init( &Password_dialog, "Password" );

  /* Use a form widget to manage the password menu. */

  password_form = XtVaCreateWidget( "password_form",
                       xmFormWidgetClass, Password_dialog,
                       XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
                       NULL );

  /* Create a control rowcolumn widget with a close button. */

  password_frame = XtVaCreateManagedWidget( "password_frame",
		xmFrameWidgetClass,	password_form,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		NULL );

  control_form = XtVaCreateWidget( "control_form",
		xmFormWidgetClass,	password_frame,
		XmNbackground,		hci_get_read_color(BACKGROUND_COLOR1),
		NULL );

  button = XtVaCreateManagedWidget( "Close",
		xmPushButtonWidgetClass, control_form,
		XmNforeground,		hci_get_read_color(BUTTON_FOREGROUND),
		XmNbackground,		hci_get_read_color(BUTTON_BACKGROUND),
		XmNfontList,		hci_get_fontlist(LIST),
		XmNtopAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_FORM,
		NULL );

  XtAddCallback( button, XmNactivateCallback, Close_lock_callback, NULL );

  /* Use the LOCA mask flag passed in by the user to determine what
     LOCA buttons need to be created. */

  loca_frame = XtVaCreateManagedWidget( "loca_frame",
		xmFrameWidgetClass,	control_form,
		XmNbackground,		hci_get_read_color(BACKGROUND_COLOR1),
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		button,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNbottomAttachment,	XmATTACH_FORM,
		NULL );

  XtVaCreateManagedWidget( "LOCA",
		xmLabelWidgetClass,	loca_frame,
		XmNchildType,		XmFRAME_TITLE_CHILD,
		XmNchildHorizontalAlignment,	XmALIGNMENT_CENTER,
		XmNchildVerticalAlignment,	XmALIGNMENT_CENTER,
		XmNforeground,		hci_get_read_color(BLACK),
		XmNbackground,		hci_get_read_color(BACKGROUND_COLOR1),
		XmNfontList,		hci_get_fontlist(LIST),
		NULL );

  loca_rowcol = XtVaCreateWidget( "loca_rowcol",
		xmRowColumnWidgetClass,	loca_frame,
		XmNforeground,		hci_get_read_color(BLACK),
		XmNbackground,		hci_get_read_color(BACKGROUND_COLOR1),
		XmNpacking,		XmPACK_TIGHT,
		XmNorientation,		XmHORIZONTAL,
		XmNmarginHeight,	0,
		NULL );

  n = 0;
  XtSetArg( args[n], XmNforeground, hci_get_read_color(TEXT_FOREGROUND) );
  n++;
  XtSetArg( args[n], XmNbackground, hci_get_read_color(BACKGROUND_COLOR1) );
  n++;
  XtSetArg( args[n], XmNfontList, hci_get_fontlist(LIST) );
  n++;
  XtSetArg( args[n], XmNorientation, XmHORIZONTAL);
  n++;

  loca_list = XmCreateRadioBox( loca_rowcol, "loca_list", args, n );

  /* AGENCY LOCA */

  if( User_lock_LOCA_mask & HCI_LOCA_AGENCY )
  {
    Agency_button = XtVaCreateManagedWidget( "Agency",
		xmToggleButtonWidgetClass,	loca_list,
		XmNselectColor,	hci_get_read_color(WHITE),
		XmNforeground,	hci_get_read_color(TEXT_FOREGROUND),
		XmNbackground,	hci_get_read_color(BACKGROUND_COLOR1),
		XmNfontList,	hci_get_fontlist(LIST),
		XmNset,		False,
		NULL );

    XtAddCallback( Agency_button,
	XmNvalueChangedCallback, LOCA_button_callback,
	NULL );
  }

  /* ROC LOCA */

  if( User_lock_LOCA_mask & HCI_LOCA_ROC )
  {
    Roc_button = XtVaCreateManagedWidget( "ROC",
		xmToggleButtonWidgetClass,	loca_list,
		XmNselectColor,	hci_get_read_color(WHITE),
		XmNforeground,	hci_get_read_color(TEXT_FOREGROUND),
		XmNbackground,	hci_get_read_color(BACKGROUND_COLOR1),
		XmNfontList,	hci_get_fontlist(LIST),
		XmNset,		False,
		NULL );

    XtAddCallback( Roc_button,
	XmNvalueChangedCallback, LOCA_button_callback,
	NULL );

  }

  /* URC LOCA */

  if( User_lock_LOCA_mask & HCI_LOCA_URC )
  {
    Urc_button = XtVaCreateManagedWidget( "URC",
		xmToggleButtonWidgetClass,	loca_list,
		XmNselectColor,	hci_get_read_color(WHITE),
		XmNforeground,	hci_get_read_color(TEXT_FOREGROUND),
		XmNbackground,	hci_get_read_color(BACKGROUND_COLOR1),
		XmNfontList,	hci_get_fontlist(LIST),
		XmNset,		False,
		NULL );

    XtAddCallback( Urc_button,
	XmNvalueChangedCallback, LOCA_button_callback,
	NULL );
  }

  XtManageChild(loca_list);
  XtManageChild(loca_rowcol);
  XtManageChild(control_form);

  /* Create a label and text widget to be used to enter the
     password. As the password is entered, the text is to
     be overwritten by "*". */

  password_rowcol = XtVaCreateWidget ("password_rowcol",
	xmRowColumnWidgetClass,	password_form,
	XmNorientation,		XmHORIZONTAL,
	XmNforeground,		hci_get_read_color(TEXT_FOREGROUND),
	XmNbackground,		hci_get_read_color(BACKGROUND_COLOR1),
	XmNtopAttachment,	XmATTACH_WIDGET,
	XmNtopWidget,		control_form,
	XmNbottomAttachment,	XmATTACH_FORM,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNrightAttachment,	XmATTACH_FORM,
	NULL );

  XtVaCreateManagedWidget( "Password: ",
	xmLabelWidgetClass,	password_rowcol,
	XmNforeground,		hci_get_read_color(TEXT_FOREGROUND),
	XmNbackground,		hci_get_read_color(BACKGROUND_COLOR1),
	XmNfontList,		hci_get_fontlist(LIST),
	NULL );

  Password_text_box = XtVaCreateManagedWidget( "Password_text",
	xmTextWidgetClass,	password_rowcol,
	XmNforeground,		hci_get_read_color(EDIT_FOREGROUND),
	XmNbackground,		hci_get_read_color(EDIT_BACKGROUND),
	XmNfontList,		hci_get_fontlist(LIST),
	NULL );

  XtAddCallback( Password_text_box,
                 XmNmodifyVerifyCallback,
                 (XtCallbackProc) Check_password, NULL );

  XtAddCallback( Password_text_box,
                 XmNactivateCallback,
                 (XtCallbackProc) Check_password, NULL );

  XtManageChild(password_rowcol);
  XtManageChild(password_form);
  XtRealizeWidget(Password_dialog);

  /* Call function to initially pop up dialog. */

  HCI_Shell_start( Password_dialog, NO_RESIZE_HCI );
  HCI_Shell_popdown( Password_dialog );
}

/************************************************************************
 *	Description: This is the callback when the user types something *
 *		     in the password text box.                          *
 ************************************************************************/

static void Check_password( Widget w, XtPointer cl, XtPointer ca )
{
  int len;
  int allocate_len;
  char *new;

  XmTextVerifyCallbackStruct *cbs =
          (XmTextVerifyCallbackStruct *) ca;

  if( cbs->reason == XmCR_ACTIVATE )
  {

    if( Password_busy_flag == HCI_YES_FLAG )
    {
      /* Not finished with previous password operation, so do nothing. This
         can happen when a user double-clicks on a slow connection. */
      return;
    }

    Password_busy_flag = HCI_YES_FLAG;

    /* User has submitted (i.e. pressed Enter key) a password. */

    HCI_LE_log( "password entered" );

    if( Passwd == (char *) NULL )
    {
      /* Password text box is empty. */
      hci_warning_popup( Password_dialog, "Empty password", NULL );
      Password_busy_flag = HCI_NO_FLAG;
      return;
    }
    else
    {
      /* Password text box not empty. Validate password. */
      Validate_password( Passwd );
    }

    if( Current_unlocked_LOCA == HCI_LOCA_NONE )
    {
      /* Password not valid. Let user try again. */
      hci_warning_popup( Password_dialog, "Invalid password entered", NULL );
      Reset_password_text();
      Password_busy_flag = HCI_NO_FLAG;
      return;
    }
    else
    {
      Change_lock_state( HCI_LOCK_LOCA_UNLOCKED );
    }

    Password_busy_flag = HCI_NO_FLAG;
  }
  else
  {
    /* User typed letter in password text widget. */

    if( User_selected_LOCA == HCI_LOCA_NONE &&
        Password_dialog_state != HCI_LOCK_CLOSE )
    {
      /* Force user to select LOCA before proceeding. If user
         is closing the lock widget, skip this block or it
         will crash. */
      hci_warning_popup( Password_dialog, "You must select a LOCA", NULL );
      cbs->text->ptr[0] = '\0';
      cbs->text->length = 0;
      return;
    }

    /* Check for backspace. */

    if( cbs->startPos < cbs->endPos )
    {
      if( Passwd != (char *) NULL ){ Passwd [cbs->startPos] = 0; }
      return;
    }

    /* Don't let user cut and paste passwords. Limit
       input to only one character at a time. */

    if( cbs->text->length > 1 )
    {
      cbs->doit = False; /* don't allow paste ops */
      return;
    }

    /* Use the longer of the new and old password as the length for
       allocation.   We do this just in case there is an input
       sequence we don't deal with correctly - this prevents a memory
       overwrite from happening. */

    allocate_len = cbs->endPos + 2;
    if( Passwd && (strlen( Passwd ) + 2 > allocate_len) )
    {
      allocate_len = strlen( Passwd ) + 2;
    }

    new = XtMalloc( allocate_len ); /* new char + NULL term */

    if( Passwd )
    {
      strcpy( new, Passwd );
      XtFree( Passwd );
    }
    else
    {
      memset( new, 0, 1 );
    }

    Passwd = new;
    strncat( Passwd, cbs->text->ptr, cbs->text->length );
    Passwd [cbs->endPos + cbs->text->length] = 0;

    /* Hide password with "*". */

    for( len = 0; len < cbs->text->length; len++ )
    {
      cbs->text->ptr [len] = '*';
    }
  }
}

/************************************************************************
 *	Description: This function is used to change the state of	*
 *		     the lock icon.					*
 ************************************************************************/

static void Change_lock_state( int new_dialog_state )
{
  Previous_password_dialog_state = Password_dialog_state;
  Password_dialog_state = new_dialog_state;

  /* Call user callback and make sure we can proceed. */

  if( User_lock_callback() == HCI_LOCK_CANCEL )
  {
    switch( Password_dialog_state )
    {
      case HCI_LOCK_OPEN:
      {
        /* Do nothing. */
        break;
      }

      case HCI_LOCK_CLOSE:
      {
        /* Do nothing. */
        break;
      }

      case HCI_LOCK_LOCA_SELECTED:
      {
        Reset_password_LOCA_buttons();
        break;
      }

      case HCI_LOCK_LOCA_UNLOCKED:
      {
        Reset_password_text();
        Reset_password_LOCA_buttons();
        break;
      }
    }
    Password_dialog_state = Previous_password_dialog_state;
    User_selected_LOCA = Previous_user_selected_LOCA;
    Current_unlocked_LOCA = Previous_current_unlocked_LOCA;
    return;
  }

  /* Proceed with changing lock state. */

  switch( Password_dialog_state )
  {
    case HCI_LOCK_OPEN:
    {
      HCI_Shell_popup( Password_dialog );
      XtSetSensitive( Lock_button, False );
      break;
    }

    case HCI_LOCK_CLOSE:
    {
      if( Operational_flag == HCI_YES_FLAG )
      {
        HCI_Shell_popdown( Password_dialog );
        if( Passwd != (char *) NULL )
        {
          XtFree( Passwd );
          Passwd = (char *) NULL;
        }
        Reset_password_text();
        Reset_password_LOCA_buttons();
      }
      XtSetSensitive( Lock_button, True );
      User_selected_LOCA =  HCI_LOCA_NONE;
      Previous_user_selected_LOCA =  HCI_LOCA_NONE;
      Current_unlocked_LOCA = HCI_LOCA_NONE;
      Previous_current_unlocked_LOCA = HCI_LOCA_NONE;
      Password_dialog_state = HCI_LOCK_CLOSE;
      Previous_password_dialog_state = HCI_LOCK_CLOSE;
    }

    case HCI_LOCK_LOCA_SELECTED:
    {
      /* Nothing else to do. */
      break;
    }

    case HCI_LOCK_LOCA_UNLOCKED:
    {
      if( Operational_flag == HCI_YES_FLAG )
      {
        HCI_Shell_popdown( Password_dialog );
      }
      XtSetSensitive( Lock_button, True );
      User_selected_LOCA =  HCI_LOCA_NONE;
      Previous_user_selected_LOCA =  HCI_LOCA_NONE;
    }
  }

  Set_lock_button_pixmap();
  XtVaSetValues( Lock_button, XmNlabelPixmap, Lock_button_pixmap, NULL );
}

/************************************************************************
 *	Description: This function is used to initialize the bitmaps	*
 *		     used for the lock icon.				*
 ************************************************************************/

static void Initialize_lock_bitmaps()
{
  static XImage lock_ximage;
  static XImage unlock_ximage;
  static XImage unlock_URC_ximage;
  static XImage unlock_AGE_ximage;
  static XImage unlock_ROC_ximage;

  /* All of the bitmaps are assumed to e 24x48.  In any event,
     they all should be a multiple of 8. */

  lock_ximage.width            = lock_icon_width;
  lock_ximage.height           = lock_icon_height;
  lock_ximage.data             = (char *) lock_icon_bits;
  lock_ximage.xoffset          = 0;
  lock_ximage.format           = XYBitmap;
  lock_ximage.byte_order       = MSBFirst;
  lock_ximage.bitmap_pad       = 8;
  lock_ximage.bitmap_bit_order = LSBFirst;
  lock_ximage.bitmap_unit      = 8;
  lock_ximage.depth            = 1;
  lock_ximage.bytes_per_line   = lock_icon_width/8;
  lock_ximage.obdata           = NULL;

  XmInstallImage( &lock_ximage, "lock_icon" );

  unlock_ximage.width            = unlock_icon_width;
  unlock_ximage.height           = unlock_icon_height;
  unlock_ximage.data             = (char *) unlock_icon_bits;
  unlock_ximage.xoffset          = 0;
  unlock_ximage.format           = XYBitmap;
  unlock_ximage.byte_order       = MSBFirst;
  unlock_ximage.bitmap_pad       = 8;
  unlock_ximage.bitmap_bit_order = LSBFirst;
  unlock_ximage.bitmap_unit      = 8;
  unlock_ximage.depth            = 1;
  unlock_ximage.bytes_per_line   = unlock_icon_width/8;
  unlock_ximage.obdata           = NULL;

  XmInstallImage( &unlock_ximage, "unlock_icon" );

  unlock_URC_ximage.width            = unlock_URC_icon_width;
  unlock_URC_ximage.height           = unlock_URC_icon_height;
  unlock_URC_ximage.data             = (char *) unlock_URC_icon_bits;
  unlock_URC_ximage.xoffset          = 0;
  unlock_URC_ximage.format           = XYBitmap;
  unlock_URC_ximage.byte_order       = MSBFirst;
  unlock_URC_ximage.bitmap_pad       = 8;
  unlock_URC_ximage.bitmap_bit_order = LSBFirst;
  unlock_URC_ximage.bitmap_unit      = 8;
  unlock_URC_ximage.depth            = 1;
  unlock_URC_ximage.bytes_per_line   = unlock_URC_icon_width/8;
  unlock_URC_ximage.obdata           = NULL;

  XmInstallImage( &unlock_URC_ximage, "unlock_URC_icon" );

  unlock_AGE_ximage.width            = unlock_AGE_icon_width;
  unlock_AGE_ximage.height           = unlock_AGE_icon_height;
  unlock_AGE_ximage.data             = (char *) unlock_AGE_icon_bits;
  unlock_AGE_ximage.xoffset          = 0;
  unlock_AGE_ximage.format           = XYBitmap;
  unlock_AGE_ximage.byte_order       = MSBFirst;
  unlock_AGE_ximage.bitmap_pad       = 8;
  unlock_AGE_ximage.bitmap_bit_order = LSBFirst;
  unlock_AGE_ximage.bitmap_unit      = 8;
  unlock_AGE_ximage.depth            = 1;
  unlock_AGE_ximage.bytes_per_line   = unlock_AGE_icon_width/8;
  unlock_AGE_ximage.obdata           = NULL;

  XmInstallImage( &unlock_AGE_ximage, "unlock_AGE_icon" );

  unlock_ROC_ximage.width            = unlock_OSF_icon_width;
  unlock_ROC_ximage.height           = unlock_OSF_icon_height;
  unlock_ROC_ximage.data             = (char *) unlock_OSF_icon_bits;
  unlock_ROC_ximage.xoffset          = 0;
  unlock_ROC_ximage.format           = XYBitmap;
  unlock_ROC_ximage.byte_order       = MSBFirst;
  unlock_ROC_ximage.bitmap_pad       = 8;
  unlock_ROC_ximage.bitmap_bit_order = LSBFirst;
  unlock_ROC_ximage.bitmap_unit      = 8;
  unlock_ROC_ximage.depth            = 1;
  unlock_ROC_ximage.bytes_per_line   = unlock_OSF_icon_width/8;
  unlock_ROC_ximage.obdata           = NULL;

  XmInstallImage( &unlock_ROC_ximage, "unlock_ROC_icon" );

  Initialize_bitmaps = 1;
}

/************************************************************************
 *	Description: This function is used to validate a password	*
 *		     string against set LOCA radio button.		*
 ************************************************************************/

static void Validate_password( char *password )
{
  char *check_pwd;
  char *official_pwd;

  /* Encrypt the user-entered password so we can compare it to the
     encrypted password obtained above. */

  check_pwd = ORPGMISC_crypt( password );
  if( check_pwd == NULL )
  {
    HCI_LE_error( "ORPGMISC_crypt(%s) failed", password );
    User_selected_LOCA = HCI_LOCA_NONE;
  }

  /* Check appropriate password. */

  if( User_selected_LOCA & HCI_LOCA_ROC )
  {
    if( HCI_roc_password( &official_pwd ) < 0 || official_pwd == NULL )
    {
      hci_warning_popup( Password_dialog, "Password data (roc) unavailable", NULL );
      return;
    }
    if( !strcmp( check_pwd, official_pwd ) )
    {
      Previous_current_unlocked_LOCA = Current_unlocked_LOCA;
      Current_unlocked_LOCA = HCI_LOCA_ROC;
    }
  }
  else if( User_selected_LOCA & HCI_LOCA_URC )
  {
    if( HCI_urc_password( &official_pwd ) < 0 || official_pwd == NULL )
    {
      hci_warning_popup( Password_dialog, "Password data (urc) unavailable", NULL );
      return;
    }
    if( !strcmp( check_pwd, official_pwd ) )
    {
      Previous_current_unlocked_LOCA = Current_unlocked_LOCA;
      Current_unlocked_LOCA = HCI_LOCA_URC;
    }
  }
  else /* Assume Agency LOCA */
  {
    if( HCI_agency_password( &official_pwd ) < 0 || official_pwd == NULL )
    {
      hci_warning_popup( Password_dialog, "Password data (agency) unavailable", NULL );
      return;
    }
    if( !strcmp( check_pwd, official_pwd ) )
    {
      Previous_current_unlocked_LOCA = Current_unlocked_LOCA;
      Current_unlocked_LOCA = HCI_LOCA_AGENCY;
    }
  }
}

/************************************************************************
 *	Description: This is the callback when the "Close" button is	*
 *		     selected in the Password window.			*
 ************************************************************************/

static void Close_lock_callback( Widget w, XtPointer cl, XtPointer ca )
{
  Change_lock_state( HCI_LOCK_CLOSE );
}

/************************************************************************
 *      Description: This function is called when one of the LOCA       *
 *                   radio buttons is selected.                         *
 ************************************************************************/

static void LOCA_button_callback( Widget w, XtPointer cl, XtPointer ca )
{
  XmToggleButtonCallbackStruct *state =
            (XmToggleButtonCallbackStruct *) ca;

  /* If toggle button is set, then user selected a LOCA. */

  if( state->set )
  {
    Previous_user_selected_LOCA = User_selected_LOCA;
    if( w == Roc_button )
    {
      User_selected_LOCA = HCI_LOCA_ROC;
    }
    else if( w == Urc_button )
    {
      User_selected_LOCA = HCI_LOCA_URC;
    }
    else /* Assume Agency LOCA. */
    {
      User_selected_LOCA = HCI_LOCA_AGENCY;
    }
    Change_lock_state( HCI_LOCK_LOCA_SELECTED );
  }
}

int hci_lock_ROC_selected()
{
  return (User_selected_LOCA & HCI_LOCA_ROC);
}
 
int hci_lock_URC_selected()
{
  return (User_selected_LOCA & HCI_LOCA_URC);
}
 
int hci_lock_AGENCY_selected()
{
  return (User_selected_LOCA & HCI_LOCA_AGENCY);
}
 
int hci_lock_ROC_unlocked()
{
  return (Current_unlocked_LOCA & HCI_LOCA_ROC);
}
 
int hci_lock_URC_unlocked()
{
  return (Current_unlocked_LOCA & HCI_LOCA_URC);
}
 
int hci_lock_AGENCY_unlocked()
{
  return (Current_unlocked_LOCA & HCI_LOCA_AGENCY);
}
 
int hci_lock_open()
{
  return (Password_dialog_state & HCI_LOCK_OPEN);
}
 
int hci_lock_close()
{
  return (Password_dialog_state & HCI_LOCK_CLOSE);
}
 
int hci_lock_loca_selected()
{
  return (Password_dialog_state & HCI_LOCK_LOCA_SELECTED);
}
 
int hci_lock_loca_unlocked()
{
  return (Password_dialog_state & HCI_LOCK_LOCA_UNLOCKED);
}

static void
Set_lock_button_pixmap()
{
  if( Current_unlocked_LOCA != HCI_LOCA_NONE )
  {
    if( Operational_flag == HCI_NO_FLAG )
    {
      Lock_button_pixmap = XmGetPixmap( XtScreen(Parent_widget), "unlock_icon", Fg_unlock_color, Bg_color );
    }
    else if( Current_unlocked_LOCA == HCI_LOCA_URC )
    {
      Lock_button_pixmap = XmGetPixmap( XtScreen(Parent_widget), "unlock_URC_icon", Fg_unlock_color, Bg_color );
    }
    else if( Current_unlocked_LOCA == HCI_LOCA_ROC )
    {
      Lock_button_pixmap = XmGetPixmap( XtScreen(Parent_widget), "unlock_ROC_icon", Fg_unlock_color, Bg_color );
    }
    else /* Assume Agency LOCA. */
    {
      Lock_button_pixmap = XmGetPixmap( XtScreen(Parent_widget), "unlock_AGE_icon", Fg_unlock_color, Bg_color );
    }
  }
  else
  {
    Lock_button_pixmap = XmGetPixmap( XtScreen(Parent_widget), "lock_icon", Fg_color, Bg_color );
  }
}

void
Reset_password_LOCA_buttons()
{
  if( User_lock_LOCA_mask & HCI_LOCA_ROC )
  {
    XmToggleButtonSetState( Roc_button, False, False );
  }
  if( User_lock_LOCA_mask & HCI_LOCA_URC )
  {
    XmToggleButtonSetState( Urc_button, False, False );
  }
  if( User_lock_LOCA_mask & HCI_LOCA_AGENCY )
  {
    XmToggleButtonSetState( Agency_button, False, False );
  }
}

static void Reset_password_text()
{
  /* Clear the password text widget. */
  XmTextSetString( Password_text_box, "" );
}

