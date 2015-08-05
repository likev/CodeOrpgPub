/************************************************************************

      The main source file for MSCF Comms Status HCI.

************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/05/14 15:20:38 $
 * $Id: mscf_comms_status.c,v 1.4 2014/05/14 15:20:38 steves Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */

/* RPG include files. */

#include <hci.h>

/* Macros. */

#define NUM_LE_LOG_MESSAGES	300
#define	MSCF_NAME_LEN		128
#define	MSCF_DEFAULT_CONF_NAME	"mscf1.conf"
#define	MSCF_CH2_CONF_NAME	"mscf2.conf"
#define	MAX_N_DEVICES		16
#define	MAX_N_COLS		32
#define	KEY_SIZE		32
#define	BUF_SIZE		256
#define	MAX_LINE_SIZE		200
#define	MAX_NUM_LINES		100
#define	MAX_COMMAND_SIZE	200
#define	MAX_OUT_TEXT_SIZE	10000
#define	MAX_COMBO_WIDTH		200
#define	MAX_SCROLL_WIDTH	800
#define	MAX_SCROLL_HEIGHT	400
#define	MAX_EVT_CNT		50
#define	COMMS_STATUS_TITLE	"Comms Status"
#define	TITLE_BUF_LEN		128

typedef struct /* Structure for Comms device. */
{
  char *name;			/* Name of the comms device */
  char *command;		/* Base comms command */
  char *col_cmds[MAX_N_COLS];	/* Column specific command */
  char *title[MAX_N_COLS];	/* Title of the column of information */
  int width[MAX_N_COLS];	/* String length of the title for a column */
  int n_cols;			/* number of columns */
} Cs_device_t;

enum { DEFAULT_CURSOR, BUSY_CURSOR };

/* Global variables. */

static Widget		Top_widget = ( Widget ) NULL;
static Widget		Update_button = ( Widget ) NULL;
static Widget		Comms_scroll = ( Widget ) NULL;
static Widget		H_scrollbar = ( Widget ) NULL;
static Widget		Frame_label = ( Widget ) NULL;
static Widget		Comms_form = ( Widget ) NULL;
static Widget		Columns_label = ( Widget ) NULL;
static Widget		Progress_dialog = ( Widget ) NULL;
static Pixel		Bg_color = (Pixel) NULL;
static Pixel		Text_color = (Pixel) NULL;
static Pixel		Normal_color = (Pixel) NULL;
static Pixel		Button_bg_color = (Pixel) NULL;
static Pixel		Button_fg_color = (Pixel) NULL;
static Pixel		Edit_color = (Pixel) NULL;
static XmFontList List_font = (XmFontList) NULL;

static char		Mscf_conf_name[MSCF_NAME_LEN];
static int		System_flag = 0;
static int		Channel_number = 0;
static int		Num_devices = 0;
static int		Device_sel_ind = 0; /* Index of selected device */
static char		*Lines[MAX_NUM_LINES];
static int		Current_line = 0;
static int		Num_lines = 0;
static int		Line_width = 0;
static int		Update_flag = 0;
static Cs_device_t	Devs[MAX_N_DEVICES];
static Cs_device_t	*Current_device;

/*  Function prototypes. */

static	void	Close_callback( Widget, XtPointer, XtPointer );
static	void	Device_callback( Widget, XtPointer, XtPointer );
static	void	Update_callback( Widget, XtPointer, XtPointer );
static	void	H_scrollbar_callback( Widget, XtPointer, XtPointer );
static	int	Read_mscf_conf();
static	void	Read_device_info();
static	int	Read_column_config( Cs_device_t * );
static	char	*Create_string( char *, int );
static	char	*Parse_output( char *, int, char * );
static	void	Update_comms_status();
static	char	*Get_next_line();
static	char	*Get_title();
static	void	Reset_scroll_bar();
static	void	Reset_initial_state();
static	void	Initialize_progress_dialog();
static	void	Popup_progress_dialog( char * );
static	void	Popdown_progress_dialog();
static	void	Timer_proc();


int main( int argc, char **argv )
{
  Widget        form = ( Widget ) NULL;
  Widget        form_rowcol = ( Widget ) NULL;
  Widget        status_frame = ( Widget ) NULL;
  Widget        frame_form = ( Widget ) NULL;
  Widget        device_combo = ( Widget ) NULL;
  Widget        combo_list = ( Widget ) NULL;
  Widget        rowcol = ( Widget ) NULL;
  Widget        label = ( Widget ) NULL;
  Widget        scroll_clip = ( Widget ) NULL;
  Widget        button = ( Widget ) NULL;
  char		title_buf[TITLE_BUF_LEN];
  int           i;
  XmString	str;

  /* Initialize HCI. */

  HCI_partial_init( argc, argv, -1 );

  Top_widget = HCI_get_top_widget();

  System_flag = HCI_get_system();

  Channel_number = HCI_get_channel_number();

  /* Need to reset the GUI title. */

  if( System_flag == HCI_FAA_SYSTEM )
  {
    sprintf( title_buf, "%s (FAA:%1d)", COMMS_STATUS_TITLE, Channel_number );
  }
  else if( System_flag == HCI_NWSR_SYSTEM )
  {
    sprintf( title_buf, "%s (NWS:%1d)", COMMS_STATUS_TITLE, Channel_number );
  }
  else
  {
    strcpy( title_buf, COMMS_STATUS_TITLE );
  }
  XtVaSetValues( Top_widget, XmNtitle, title_buf, NULL );

  /* Initialize miscellaneous variables. */

  Bg_color = hci_get_read_color( BACKGROUND_COLOR1 );
  Text_color = hci_get_read_color( TEXT_FOREGROUND );
  Normal_color = hci_get_read_color( NORMAL_COLOR );
  Button_bg_color = hci_get_read_color( BUTTON_BACKGROUND ); 
  Button_fg_color = hci_get_read_color( BUTTON_FOREGROUND );
  Edit_color = hci_get_read_color( EDIT_BACKGROUND );
  List_font = hci_get_fontlist( LIST );

  /* Read the MSCF configuration file. */

  Read_mscf_conf();
 
  /* Define top-level form and rowcol to hold all widgets. */

  form = XtVaCreateWidget( "mode_list_form",
                 xmFormWidgetClass, Top_widget,
                 NULL );

  form_rowcol = XtVaCreateManagedWidget( "form_rowcol",
                        xmRowColumnWidgetClass, form,
                        XmNbackground, Bg_color,
                        XmNorientation, XmVERTICAL,
                        XmNpacking, XmPACK_TIGHT,
                        XmNnumColumns, 1,
                        NULL );

  /* Add buttons and drop-down. */

  rowcol = XtVaCreateManagedWidget( "close_rowcol",
                         xmRowColumnWidgetClass, form_rowcol,
                         XmNbackground, Bg_color,
                         XmNorientation, XmHORIZONTAL,
                         XmNpacking, XmPACK_TIGHT,
                         XmNnumColumns, 1,
                         XmNtopAttachment, XmATTACH_FORM,
                         XmNleftAttachment, XmATTACH_FORM,
                         XmNrightAttachment, XmATTACH_FORM,
                         NULL );

  button = XtVaCreateManagedWidget( "Close",
                   xmPushButtonWidgetClass, rowcol,
                   XmNforeground, Button_fg_color,
                   XmNbackground, Button_bg_color,
                   XmNfontList, List_font,
                   NULL );

  XtAddCallback( button, XmNactivateCallback, Close_callback, NULL );

  label = XtVaCreateManagedWidget( "  ",
                   xmLabelWidgetClass, rowcol,
                   XmNbackground, Bg_color,
                   XmNfontList, List_font,
                   NULL );

  device_combo = XtVaCreateWidget( "Combo",
                xmComboBoxWidgetClass,  rowcol,
                XmNforeground,          Text_color,
                XmNbackground,          Bg_color,
                XmNfontList,            List_font,
                XmNcomboBoxType,        XmDROP_DOWN_LIST,
                XmNwidth,               MAX_COMBO_WIDTH,
                XmNvisibleItemCount,    Num_devices,
                XmNmarginHeight,        1,
                NULL );

  XtVaGetValues( device_combo, XmNlist, &combo_list, NULL );

  for( i = 0; i < Num_devices; i++ )
  {
    str = XmStringCreateLocalized( Devs[i].name );
    XmListAddItemUnselected(  combo_list, str, 0 );
    XmStringFree( str );
  }

  XtManageChild( device_combo );
  XtVaSetValues( device_combo, XmNselectedPosition, 0, NULL );
  XtAddCallback( device_combo, XmNselectionCallback, Device_callback, NULL );

  label = XtVaCreateManagedWidget( "  ",
                   xmLabelWidgetClass, rowcol,
                   XmNbackground, Bg_color,
                   XmNfontList,   List_font,
                   NULL );

  Update_button = XtVaCreateManagedWidget( "Update",
                   xmPushButtonWidgetClass, rowcol,
                   XmNforeground, Button_fg_color,
                   XmNbackground, Button_bg_color,
                   XmNfontList,   List_font,
                   NULL );

  XtAddCallback( Update_button, XmNactivateCallback, Update_callback, NULL );

  status_frame = XtVaCreateManagedWidget( "comms_frame",
              xmFrameWidgetClass,     form_rowcol,
              XmNbackground,          Bg_color,
              XmNtopAttachment,       XmATTACH_WIDGET,
              XmNtopWidget,           rowcol,
              XmNleftAttachment,      XmATTACH_FORM,
              XmNrightAttachment,     XmATTACH_FORM,
              XmNbottomAttachment,    XmATTACH_FORM,
              NULL );

  Frame_label = XtVaCreateManagedWidget( Current_device->name,
              xmLabelWidgetClass, status_frame,
              XmNchildType, XmFRAME_TITLE_CHILD,
              XmNchildHorizontalAlignment, XmALIGNMENT_CENTER,
              XmNchildVerticalAlignment, XmALIGNMENT_CENTER,
              XmNforeground, Text_color,
              XmNbackground, Bg_color,
              XmNfontList, List_font,
              NULL );

  frame_form = XtVaCreateWidget( "form",
              xmFormWidgetClass,      status_frame,
              XmNbackground,          Bg_color,
              NULL );

  Columns_label = XtVaCreateManagedWidget( Get_title(),
                   xmLabelWidgetClass, frame_form,
                   XmNforeground, Text_color,
                   XmNbackground, Bg_color,
                   XmNfontList, List_font,
                   XmNalignment, XmALIGNMENT_BEGINNING,
                   NULL );

  Comms_scroll = XtVaCreateManagedWidget( "Comms_scroll",
              xmScrolledWindowWidgetClass,    frame_form,
              XmNheight,              MAX_SCROLL_HEIGHT,
              XmNwidth,               MAX_SCROLL_WIDTH,
              XmNscrollingPolicy,     XmAUTOMATIC,
              XmNbackground,          Bg_color,
              XmNtopAttachment,       XmATTACH_WIDGET,
              XmNtopWidget,           Columns_label,
              XmNbottomAttachment,    XmATTACH_FORM,
              XmNleftAttachment,      XmATTACH_FORM,
              XmNrightAttachment,     XmATTACH_FORM,
              NULL );

  XtVaGetValues( Comms_scroll,
                 XmNhorizontalScrollBar, &H_scrollbar,
                 XmNclipWindow, &scroll_clip, NULL );

  XtAddCallback( H_scrollbar,
              XmNvalueChangedCallback, H_scrollbar_callback, NULL );
  XtAddCallback( H_scrollbar,
              XmNdecrementCallback, H_scrollbar_callback, NULL );
  XtAddCallback( H_scrollbar,
              XmNincrementCallback, H_scrollbar_callback, NULL );
  XtAddCallback( H_scrollbar,
              XmNpageDecrementCallback, H_scrollbar_callback, NULL );
  XtAddCallback( H_scrollbar,
              XmNpageIncrementCallback, H_scrollbar_callback, NULL );

  XtVaSetValues( scroll_clip, XmNbackground, Edit_color, NULL );

  XtManageChild( frame_form );
  XtManageChild( form );

  LE_send_msg( GL_INFO, "End creating widgets." );
  LE_send_msg( GL_INFO, "Calling XtRealizeWidget( Top_widget )" );

  /* Display GUI to screen. */

  XtRealizeWidget( Top_widget );

  /* Initialize progress dialog. */

  Initialize_progress_dialog();

  /* Make GUI read status as it appears on screen. */

  Update_callback( NULL, NULL, NULL );

  /* Start HCI loop. */

  HCI_start( Timer_proc, HCI_ONE_SECOND, NO_RESIZE_HCI );

  return 0;
}

/***************************************************************************
 CLOSE_CALLBACK(): Callback for "Close" button. Destroys main widget.
 ***************************************************************************/

static void Close_callback( Widget w, XtPointer x, XtPointer y )
{
  LE_send_msg( GL_INFO, "In close callback" );
  XtDestroyWidget( Top_widget );
}

/***************************************************************************
 TIMER_PROC(): Callback for event timer.
 ***************************************************************************/

static void Timer_proc()
{
  /* Popup can't be managed and updated within the same timer event.
     To overcome this limitation, create popup in one timer event,
     read statuses and update the popup in the next timer event. */

  if( Update_flag && !XtIsManaged( Progress_dialog ) )
  {
    Popup_progress_dialog( "Preparing to read statuses" );
  }
  else if( Update_flag )
  {
    Update_flag = 0;
    Update_comms_status();
    Popdown_progress_dialog();
    Reset_initial_state();
  }
}

/***************************************************************************
 READ_MSCF_CONF: Read mscf.conf file for user-defined information.
 ***************************************************************************/

static int Read_mscf_conf()
{
  int i, ret, len;
  char key[KEY_SIZE], tmp[BUF_SIZE];
  Cs_device_t *dev;

  /* Set name of MSCF configuration file. */

  if( System_flag == HCI_FAA_SYSTEM && Channel_number == 2 )
  {
    strcpy( Mscf_conf_name, MSCF_CH2_CONF_NAME );
  }
  else
  {
    strcpy( Mscf_conf_name, MSCF_DEFAULT_CONF_NAME );
  }

  LE_send_msg( GL_INFO, "MSCF conf file set to %s", Mscf_conf_name);

  CS_cfg_name( Mscf_conf_name );
  CS_control( CS_COMMENT | '#' );

  Num_devices = 0;

  /* Parse MSCF configuration file for device information. */

  while( 1 )
  {
    sprintf( key, "Cs_device_%d:", Num_devices );
    CS_control( CS_KEY_OPTIONAL );
    if( ( ret = CS_entry( key, 0, BUF_SIZE, tmp ) ) <= 0 )
    {
      LE_send_msg( GL_ERROR, "CS_entry for %s failed (%d)", key, ret );
      break;
    }
    CS_control( CS_KEY_REQUIRED );
    if( Num_devices >= MAX_N_DEVICES ){ break; }
    if( ( ret = CS_level( CS_DOWN_LEVEL ) ) < 0 )
    {
      LE_send_msg( GL_ERROR, "CS_level (DOWN) failed (%d) for device %d",
                   ret, Num_devices );
      exit(1);
    }

    dev = Devs + Num_devices;
    sprintf( key, "Name:" );
    if( ( len = CS_entry( key, 1, BUF_SIZE, tmp ) ) <= 0 )
    {
      LE_send_msg( GL_ERROR, "CS_entry for %s failed (%d)", key, len );
      exit(1);
    }
    dev->name = Create_string( tmp, len );
    sprintf( key, "Command:" );
    if( ( len = CS_entry( key, 1, BUF_SIZE, tmp ) ) <= 0 )
    {
      LE_send_msg( GL_ERROR, "CS_entry for %s failed (%d)", key, len );
      exit(1);
    }
    dev->command = Create_string( tmp, len );
    dev->n_cols = Read_column_config( dev );
    if( dev->n_cols <= 0 )
    {
      LE_send_msg( GL_ERROR, "No column found for device %d", Num_devices );
      exit(1);
    }

    if( ( ret = CS_level( CS_UP_LEVEL ) ) < 0 )
    {
      LE_send_msg( GL_ERROR, "CS_level (UP) failed (%d) for device %d",
                   ret, Num_devices );
      exit(1);
    }
    Num_devices++;
  }

  if( Num_devices <= 0 )
  {
    LE_send_msg( GL_STATUS, "No comms device configured" );
  }

  Current_device = Devs;

  for( i = 0; i < MAX_NUM_LINES; i++ ){ Lines[i] = NULL; }

  return 0;
}

/***************************************************************************
 CREATE_STRING: Create string by allocating memory and copying.
 ***************************************************************************/

static char *Create_string( char *str, int len )
{
  char *b = malloc( len + 1 );
  if( b == NULL )
  {
    LE_send_msg( GL_ERROR, "malloc of %d bytes failed", len+1 );
    exit(1);
  }
  memcpy( b, str, len );
  b[len] = '\0';
  return b;
}

/***************************************************************************
 READ_COLUMN_CONFIG: Read in column configuration options.
 ***************************************************************************/

static int Read_column_config( Cs_device_t *dev )
{
  char key[KEY_SIZE], tmp[BUF_SIZE];
  int cnt = 0;
  int len = 0;

  cnt = 0;
  while( 1 )
  {
    sprintf( key, "Col_%d:", cnt );
    CS_control( CS_KEY_OPTIONAL );
    if( ( len = CS_entry( key, 1, BUF_SIZE, tmp ) ) <= 0 )
    {
      LE_send_msg( GL_ERROR, "CS_entry for %s failed (%d) token 1", key, len );
      break;
    }
    CS_control( CS_KEY_REQUIRED );
    dev->width[cnt] = strlen( tmp );
    dev->title[cnt] = Create_string( tmp, len );
    if( ( len = CS_entry( key, 2, BUF_SIZE, tmp ) ) <= 0 )
    {
      LE_send_msg( GL_ERROR, "CS_entry for %s failed (%d) token 2", key, len );
      exit(1);
    }
    dev->col_cmds[cnt] = Create_string( tmp, len );
    cnt++;
  }
  return cnt;
}

/***************************************************************************
 DEVICE_CALLBACK: Callback for Device combo box.
 ***************************************************************************/

static void Device_callback( Widget w, XtPointer x, XtPointer y )
{
  XmComboBoxCallbackStruct *cbs = ( XmComboBoxCallbackStruct * ) y;

  /* If user selected different device, update GUI. */
  if( Device_sel_ind != cbs->item_position )
  {
    Device_sel_ind = cbs->item_position;
    Update_callback( NULL, NULL, NULL );
  }
}

/***************************************************************************
 UPDATE_COMMS_STATUS: Update Comms Status labels.
 ***************************************************************************/

static void Update_comms_status()
{
  Widget prev_label = ( Widget ) NULL;
  Widget label = ( Widget ) NULL;
  int cnt, i;
  char *text;
  XmString str;
  Pixel bc, fc;

  /* Get status info via SNMP from selected device. */

  Read_device_info();

  /* Update GUI according to selected device. */

  str = XmStringCreateLocalized( Get_title() );
  XtVaSetValues( Columns_label, XmNlabelString, str, NULL );
  XmStringFree( str );

  str = XmStringCreateLocalized( Current_device->name );
  XtVaSetValues( Frame_label, XmNlabelString, str, NULL );
  XmStringFree( str );

  /* Re-create form for new status labels. */

  if( Comms_form != ( Widget ) NULL ){ XtDestroyWidget( Comms_form ); }

  Comms_form = XtVaCreateWidget( "Comms_form",
                xmFormWidgetClass,      Comms_scroll,
                XmNbackground,          Edit_color,
                XmNverticalSpacing,     1,
                NULL );

  if( Num_lines < 1 )
  {
    cnt = sizeof( char ) * Line_width;
    text = malloc( cnt );
    if( text == NULL )
    {
      LE_send_msg( GL_ERROR, "malloc failed for %d bytes", cnt );
      exit(1);
    }
    for( i = 0; i < Line_width-1; i++ ){ text[i] = ' '; }
    text[i]='\0';
    label = XtVaCreateManagedWidget( text,
                  xmLabelWidgetClass,     Comms_form,
                  XmNfontList,            List_font,
                  XmNtopAttachment,       XmATTACH_FORM,
                  XmNleftAttachment,      XmATTACH_FORM,
                  XmNbackground,          Edit_color,
                  XmNforeground,          Text_color,
                  NULL );
    free( text );
  }
  else
  {
    cnt = 0;
    while( ( text = Get_next_line() ) != NULL )
    {
      str = XmStringCreateLocalized( text  );
      if( strstr( text, "up(1)" ) != NULL ){ bc = Normal_color; }
      else{ bc = Edit_color; }
      fc = Text_color;

      label = XtVaCreateManagedWidget( "label",
                  xmLabelWidgetClass,     Comms_form,
                  XmNfontList,            List_font,
                  XmNleftAttachment,      XmATTACH_FORM,
                  XmNlabelString,         str,
                  XmNbackground,          bc,
                  XmNforeground,          fc,
                  NULL );

      if( cnt == 0 )
      {
        XtVaSetValues( label, XmNtopAttachment, XmATTACH_FORM, NULL );
      }
      else
      {
        XtVaSetValues( label, XmNtopAttachment, XmATTACH_WIDGET,
                              XmNtopWidget, prev_label, NULL );
      }

      XmStringFree( str );
      prev_label = label;
      cnt++;
    }
  }
  XtManageChild( Comms_form );
}

/***************************************************************************
 READ_DEVICE_INFO: Queries device to obtain device's status info.
 ***************************************************************************/

static void Read_device_info()
{
  char output_buf[MAX_OUT_TEXT_SIZE];
  char cmd[MAX_COMMAND_SIZE];
  int column_offset, width;
  int len, i, j, temp_len;
  int ret, n_bytes;
  char *output;
  char popup_msg[BUF_SIZE];

  LE_send_msg( GL_INFO, "Reading comms device status..." );

  /* Check validity of device index. It should never be invalid, so
     exit with error if it is. */

  if( Device_sel_ind < 0 || Device_sel_ind >= Num_devices )
  {
    LE_send_msg( GL_ERROR, "Device_sel_ind = %d is < 0 or >= %d",
                 Device_sel_ind, Num_devices );
    exit(1);
  }

  /* Initialize variables for new status. */

  Current_device = Devs + Device_sel_ind;
  Current_line = 0;
  Num_lines = 0;
  Line_width = 0;
  for( i = 0; i < Current_device->n_cols; i++ )
  {
    Line_width += Current_device->width[i];
  }

  len = strlen( Current_device->command );
  strncpy( cmd, Current_device->command, MAX_COMMAND_SIZE );

  /* Loop through statuses to obtain and get via SNMP. */

  for( i = 0; i < Current_device->n_cols; i++ )
  {
    /* If command length exceeds max allowed, exit with error. */
    temp_len = len + strlen( Current_device->col_cmds[i]) + 1;
    if( temp_len >= MAX_COMMAND_SIZE )
    {
      LE_send_msg( GL_ERROR, "Command length (%d) exceeds %d",
                   temp_len, MAX_COMMAND_SIZE );
      exit(1);
    }
    strcpy( cmd + len, " " );
    strcpy( cmd + len + 1, Current_device->col_cmds[i] );

    /* Provide user with popup of progress. */
    sprintf( popup_msg, "Reading %d of %d statuses", i+1, Current_device->n_cols );
    Popup_progress_dialog( popup_msg );

    /* Make SNMP call. */
    ret = MISC_system_to_buffer( cmd, output_buf, MAX_OUT_TEXT_SIZE, &n_bytes );
    if( ret != 0 )
    {
      output_buf[0] = '\0';
      LE_send_msg( GL_ERROR, "Failed (%d) in executing \"%s\"", ret, cmd );
    }
    else
    {
      column_offset = 0;
      for( j = 0; j < i; j++ ){ column_offset += Current_device->width[j]; }
      /* If comms line length exceeds max allowed, exit with error. */
      width = Current_device->width[i];
      if( width + column_offset >= MAX_LINE_SIZE )
      {
        LE_send_msg( GL_ERROR, "Comms line length (%d) exceeds %d",
                     ( width + column_offset ), MAX_LINE_SIZE );
        exit(1);
      }

      output = &output_buf[0];
      j = 0;
      while( j < MAX_NUM_LINES )
      {
        if( Lines[j] == NULL )
        {
          Lines[j] = malloc( MAX_LINE_SIZE );
          if( Lines[j] == NULL )
          {
            LE_send_msg( GL_ERROR, "malloc failed for %d bytes", MAX_LINE_SIZE );
            exit(1);
          }
        }
        output = Parse_output( output, width, Lines[j]+column_offset );
        *(Lines[j] + (column_offset + width)) = '\0';
        j++;
        /* No more lines of output to process */
        if( output[0] == '\0' ){ j--; break; }
      }
      Num_lines = j;
    }
  }
}

/***************************************************************************
 PARSE_OUTPUT: Parse output for value to display.
 ***************************************************************************/

static char *Parse_output( char *src, int width, char *buf )
{
  int i, len, offset;
  char *cpt, *st;

  /* Make sure buf is empty. */
  for( i = 0; i < width; i++ ){ buf[i] = ' '; }
  /* Set temporary pointer to "source" text line. */
  cpt = src;
  /* Format of a line of output is MIB-variable = data-type: value */
  /* Skip over stuff we don't care about (MIB-variable) until we reach "=". */
  while( *cpt != '\0' && *cpt != '=' ){ cpt++; }
  /* If cpt == "=", we've found next result. */
  if( *cpt == '=' )
  {
    /* Skip "=" */
    cpt++;
    /* Skip any white spaces(and tabs. */
    while( *cpt == ' ' || *cpt == '\t' ){ cpt++; }
    /* Skip over data-type: (i.e. INTEGER: ,STRING:, etc.) tokens. */
    while( *cpt != ' ' ){ cpt++; }
    /* Skip any white spaces, tabs, and '"' ('"' is for a string). */
    while( *cpt == ' ' || *cpt == '\t' || *cpt == '"' ){ cpt++; }
    /* Set st equal to the start of the value token we've found. */
    st = cpt;
    /* Move cpt pointer to the end of the value token we've found. */
    while( *cpt != '\0' && *cpt != '\n' && *cpt != '"' ){ cpt++; }
    /* Determine length of the value token. */
    len = cpt - st;
    /* Sanity check. If the length of the value token is longer than the
       allowed width, there is no offset. Otherwise, to center-align the
       value token within the allowed width, offset the value token by 1/2
       of the number of unused characters. */
    if( len > width ){ len = width; offset = 0; }
    else{ offset = (width - len)/2; }
    memcpy( buf + offset, st, len );
  }
  /* Skip any trailing newlines, if any */
  while( *cpt == '\n' ){ cpt++; }
  return( cpt );
}

/***************************************************************************
 GET_NEXT_LINE: Get next line of the comms status.
 ***************************************************************************/

static char *Get_next_line()
{
  char *ret;

  if( Current_line < Num_lines ){ ret = Lines[Current_line]; }
  else{ ret = NULL; }
  Current_line++;
  return ret;
}

/***************************************************************************
 H_SCROLLBAR_CALLBACK: Callback for horizontal scrollbar.
 ***************************************************************************/

static void H_scrollbar_callback( Widget w, XtPointer x, XtPointer y )
{
  int max, value;
  int len, off;
  char *title;
  XmString str;

  XtVaGetValues( H_scrollbar, XmNmaximum, &max,
                              XmNvalue, &value, NULL );
  off = (value * Line_width + (max/2)) / max;
  title = Get_title();
  len = strlen( title );
  if( off > len ){ off = len; }
  str = XmStringCreateLocalized( title + off );
  XtVaSetValues( Columns_label, XmNlabelString, str, NULL );
  XmStringFree( str );
}

/***************************************************************************
 GET_TITLE: Get comms status title of current device.
 ***************************************************************************/

static char *Get_title()
{
  static char title[MAX_LINE_SIZE];
  char *cpt;
  int i;

  cpt = title;
  for( i = 0; i < Current_device->n_cols; i++ )
  {
    if( cpt - title + Current_device->width[i] + 1 > MAX_LINE_SIZE ){ break; }
    strncpy( cpt, Current_device->title[i], Current_device->width[i] );
    cpt += Current_device->width[i];
  }
  *cpt = '\0';
  return title;
}

/***************************************************************************
 UPDATE_CALLBACK: Callback for "Update" button.
 ***************************************************************************/

static void Update_callback( Widget w, XtPointer x, XtPointer y )
{
  Update_flag = 1;
  XtSetSensitive( Update_button, False );
  HCI_busy_cursor();
}

/***************************************************************************
 RESET_SCROLL_BAR: Reset scroll bar to starting position.
 ***************************************************************************/

static void Reset_scroll_bar()
{
  int value, size, incr, pg_incr;
 
  XmScrollBarGetValues( H_scrollbar, &value, &size, &incr, &pg_incr );
  value = 0;
  XmScrollBarSetValues( H_scrollbar, value, size, incr, pg_incr, 1 );
}

/***************************************************************************
 RESET_INITIAL_STATE: Reset widgets, variables, etc. to initial values.
 ***************************************************************************/

static void Reset_initial_state()
{
  XtSetSensitive( Update_button, True );
  HCI_default_cursor();
  Reset_scroll_bar();
}

/***************************************************************************
 POPUP_PROGRESS_DIALOG: Popup dialog with progress info.
 ***************************************************************************/

static void Popup_progress_dialog( char *msg )
{
  /* If popup not managed, do so to make it appear. */
  if( !XtIsManaged( Progress_dialog ) )
  {
    XtManageChild( Progress_dialog );
  }

  /* Convert C-style buf to X-style buf. */
  XmString msg_string = XmStringCreateLtoR( msg, XmFONTLIST_DEFAULT_TAG );
  /* Set message in dialog. */
  XtVaSetValues( Progress_dialog, XmNmessageString, msg_string, NULL );
  /* Force update of widget. */
  XmUpdateDisplay( Progress_dialog );
  /* Cleanup allocated memory. */
  XmStringFree( msg_string );
}

/***************************************************************************
 POPDOWN_PROGRESS_DIALOG: Popdown dialog with progress info.
 ***************************************************************************/

static void Popdown_progress_dialog()
{
  /* Unmanage popup to make it disappear. */
  XtUnmanageChild( Progress_dialog );
}

/***************************************************************************
 INITIALIZE_PROGRESS_DIALOG: Initialize dialog to show progress info.
 ***************************************************************************/

static void Initialize_progress_dialog()
{
  /* Initialize progress dialog. */
  Progress_dialog = XmCreateInformationDialog( Top_widget, "Progress", NULL, 0 );

  /* Remove unwanted widgets. */
  XtUnmanageChild( XmMessageBoxGetChild( Progress_dialog, XmDIALOG_OK_BUTTON ) );
  XtUnmanageChild( XmMessageBoxGetChild( Progress_dialog, XmDIALOG_CANCEL_BUTTON ) );
  XtUnmanageChild( XmMessageBoxGetChild( Progress_dialog, XmDIALOG_HELP_BUTTON ) );
  XtUnmanageChild( XmMessageBoxGetChild( Progress_dialog, XmDIALOG_SEPARATOR ) );
  XtUnmanageChild( XmMessageBoxGetChild( Progress_dialog, XmDIALOG_SYMBOL_LABEL ) );

  /* Set various attributes of popup. */
  XtVaSetValues( XmMessageBoxGetChild( Progress_dialog, XmDIALOG_MESSAGE_LABEL ),
                 XmNforeground, Text_color,
                 XmNbackground, Bg_color,
                XmNfontList,    List_font, NULL );

  XtVaSetValues( Progress_dialog,
                 XmNforeground, Text_color,
                 XmNbackground, Bg_color,
                 XmNdeleteResponse, XmDO_NOTHING, NULL );

  /* Set various attributes of popup parent. */
  XtVaSetValues( XtParent( Progress_dialog ),
                 XmNtitle, "Read Status Progress",
                 XmNmwmFunctions, MWM_FUNC_ALL | MWM_FUNC_CLOSE, NULL );
}

