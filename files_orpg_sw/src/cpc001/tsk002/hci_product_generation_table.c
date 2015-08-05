/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/06/03 20:42:13 $
 * $Id: hci_product_generation_table.c,v 1.111 2014/06/03 20:42:13 steves Exp $
 * $Revision: 1.111 $
 * $State: Exp $
 */

/************************************************************************
     Module:  hci_product_generation_table.c
     Description: This module handles the display and editing of
                   the current and default product generation lists.
                   The current product generation list is not
                   password protected but the default tables are.
                   Editing the default tables requires at ROC
                   level access. ROC level access displays, in
                   addition to products with product codes >= 16,
                   all other RPG generated products, including
                   intermediate products.
 ************************************************************************/

/* Local includes */

#include <hci.h>

/* Define macros for various product sorting options. */

enum { SORT_BY_DESCRIPTION, SORT_BY_PRODUCT_CODE, SORT_BY_PRODUCT_MNE };

/* Define macros to identify specific generation list elements */

enum { GENERATION, ARCHIVE, RETENTION, CUT };

#define	MIN_PRODUCT_CODE	 16
#define	GENERATION_LIST_MAX	2048
#define	CURRENT_TABLE		ORPGPGT_CURRENT_TABLE
#define	DEFAULT_A_TABLE		ORPGPGT_DEFAULT_A_TABLE
#define	DEFAULT_B_TABLE		ORPGPGT_DEFAULT_B_TABLE

/* Static/Global variables */

static Widget Top_widget = (Widget) NULL;
static Widget Form = (Widget) NULL;
static Widget Data_scroll = (Widget) NULL;
static Widget Data_form = (Widget) NULL;
static Widget Baseline_rowcol = (Widget) NULL;
static Widget Replace_rowcol = (Widget) NULL;
static Widget Bottom_form = (Widget) NULL;
static Widget Current_button = (Widget) NULL;
static Widget Wx_mode_A_button = (Widget) NULL;
static Widget Wx_mode_B_button = (Widget) NULL;
static Widget Save_button = (Widget) NULL;
static Widget Undo_button = (Widget) NULL;
static Widget Sort_pcode_button = (Widget) NULL;
static Widget Sort_pid_button = (Widget) NULL;
static Widget Sort_description_button = (Widget) NULL;
static Widget Active_row = (Widget) NULL;
static Widget Lock_widget = (Widget) NULL;
static Widget Pid_text = (Widget) NULL;
static Widget Parameter_frame = (Widget) NULL;
static Widget Parameter_label = (Widget) NULL;
static Pixel Base_bg = (Pixel) NULL;
static Pixel Text_fg = (Pixel) NULL;
static Pixel Text_bg = (Pixel) NULL;
static Pixel Edit_fg = (Pixel) NULL;
static Pixel Edit_bg = (Pixel) NULL;
static Pixel Loca_fg = (Pixel) NULL;
static Pixel Button_fg = (Pixel) NULL;
static Pixel Button_bg = (Pixel) NULL;
static Pixel White_color = (Pixel) NULL;
static Pixel Black_color = (Pixel) NULL;
static Pixel Cut_color = (Pixel) NULL;
static XmFontList List_font = (XmFontList) NULL;

/* The following structure defines the widget elements of the
   parameter edit region. */

typedef struct
{
  Widget rowcol;      /* Parent rowcolumn widget for param */
  Widget description; /* Widget containing param description */
  Widget minimum;     /* Widget containing param min value */
  Widget maximum;     /* Widget containing param max value */
  Widget value;       /* Widget containing param value */
  Widget units;       /* Widget containing param unit descr */
  int    index;       /* Parameter number (0-5) */
} Gen_param_t;

typedef struct
{
  int buf_num;        /* Product buffer number. */
  int indx;           /* Index in generation table. */
  Widget rowcol;      /* Row column manager parent */
  Widget pid;         /* Product ID. */
  Widget mnemonic;    /* Product mnemonic */
  Widget pcode;       /* Product code */
  Widget generation;  /* Generation interval (volumes) */
  Widget retention;   /* Retention period (minutes) */
  Widget cut_toggle;  /* All elevation angles toggle */
  Widget cut;         /* Elevation angle (+)/slices (-) */
  Widget description; /* Product description */
} Gen_tbl_t;

static Gen_param_t Parameter_widget[ORPGPGT_MAX_PARAMETERS+1];
static Gen_tbl_t *Gen_tbl = (Gen_tbl_t *) NULL;;
static int Num_active_parameters = 0;
static int Num_table_items = 0;
static Boolean Cut_mode = True;
static int Show_all_products = HCI_NO_FLAG;
static int Num_gen_table_items = 0;
static int Unlocked_roc = HCI_NO_FLAG;
static int Close_flag = HCI_NO_FLAG;
static int Modify_flag = HCI_NO_FLAG;
static int Parameter_modify_flag = HCI_NO_FLAG;
static char *Data_tbl = (char *) NULL;
static int Current_flag = HCI_NOT_CHANGED_FLAG;
static int Wx_mode_A_flag = HCI_NOT_CHANGED_FLAG;
static int Wx_mode_B_flag = HCI_NOT_CHANGED_FLAG;
static int User_selected_table_to_save = -1;
static int User_selected_table_to_replace = -1;
static int Replace_flag = HCI_NO_FLAG;
static int Save_flag = HCI_NO_FLAG;
static int Undo_flag = HCI_NO_FLAG;
static char Buf[512];
static char Feedback[128];
static char Search_string[64];
static char Old_text[32];
static int Sort_method = SORT_BY_PRODUCT_CODE;
static int Which_table = CURRENT_TABLE;
static int Sort_index[GENERATION_LIST_MAX];
static int Att_map[GENERATION_LIST_MAX];
static int Gen_map[GENERATION_LIST_MAX];

/* Function prototypes */

static void Close_callback( Widget, XtPointer, XtPointer );
static void Undo_changes_callback( Widget, XtPointer, XtPointer );
static void Undo_changes();
static void Save_callback( Widget, XtPointer, XtPointer );
static void Verify_generation_table_save( Widget, int );
static void Accept_generation_table_save( Widget, XtPointer, XtPointer );
static void Cancel_generation_table_save( Widget, XtPointer, XtPointer );
static void Generation_table_save();
static void Select_generation_table_callback( Widget, XtPointer, XtPointer );
static void Replace_current_callback( Widget, XtPointer, XtPointer );
static void Accept_replace_current_callback( Widget, XtPointer, XtPointer );
static void Cancel_replace_current_callback( Widget, XtPointer, XtPointer );
static void Replace_current_table();
static void Sort_filter_callback( Widget, XtPointer, XtPointer );
static void Search_text_callback( Widget, XtPointer, XtPointer );
static void Gen_table_gain_focus( Widget, XtPointer, XtPointer );
static void Gen_table_modify( Widget, XtPointer, XtPointer );
static void Parameter_table_gain_focus( Widget, XtPointer, XtPointer );
static void Parameter_table_modify( Widget, XtPointer, XtPointer );
static void Hci_cut_toggle_callback( Widget, XtPointer, XtPointer );
static void Sort_products( int );
static void Show_generation_table( int );
static int Hci_product_generation_security();
static void Draw_parameters_widget();
static void Set_save_undo_flag();
static int Hci_create_new_table_entry( int, int, int, int, int, float, int );
static int Round( float ); 
static void Hci_timer_proc();

int main( int argc, char *argv[] )
{
  Widget top_rowcol = (Widget) NULL;
  Widget button = (Widget) NULL;
  Widget label = (Widget) NULL;
  Widget baseline_frame = (Widget) NULL;
  Widget replace_frame = (Widget) NULL;
  Widget replace_filter = (Widget) NULL;
  Widget table_filter = (Widget) NULL;
  Widget sort_filter = (Widget) NULL;
  Widget search_frame = (Widget) NULL;
  Widget search_rowcol = (Widget) NULL;
  Widget search_text = (Widget) NULL;
  Widget label_rowcol = (Widget) NULL;
  Widget text = (Widget) NULL;
  Widget lock_form = (Widget) NULL;
  Widget clip = (Widget) NULL;
  int n = 0;
  Arg args[16];

  /* HCI initialize. */

  HCI_init( argc, argv, HCI_PROD_TASK );

  /* Get reference to top-level widget. */

  Top_widget = HCI_get_top_widget();

  /* Set write permission so can register callback */

  ORPGDA_write_permission( ORPGDAT_PROD_INFO );

  /* Define colors and font */

  Base_bg = hci_get_read_color( BACKGROUND_COLOR1 );
  Text_fg = hci_get_read_color( TEXT_FOREGROUND );
  Text_bg = hci_get_read_color( TEXT_BACKGROUND );
  Edit_fg = hci_get_read_color( EDIT_FOREGROUND );
  Edit_bg = hci_get_read_color( EDIT_BACKGROUND );
  Loca_fg = hci_get_read_color( LOCA_FOREGROUND );
  Button_bg = hci_get_read_color( BUTTON_BACKGROUND );
  Button_fg = hci_get_read_color( BUTTON_FOREGROUND );
  White_color = hci_get_read_color( WHITE );
  Black_color = hci_get_read_color( BLACK );
  Cut_color = Edit_bg;
  List_font = hci_get_fontlist( LIST );

  /* Define a form widget to be used as the manager for widgets in
     the product generation window. */

  Form = XtVaCreateWidget( "parent",
                xmFormWidgetClass, Top_widget,
                XmNbackground, Base_bg,
                NULL );        

  /* If low bandwidth, create a popup progress meter to show I/O */

  HCI_PM( "Initialize Task Information" );

  /* Allocate memory for the table widgets/data. Right now we
     assume that a value twice the size of the generation table
     entries is sufficient. Find the largest table and use that
     for the max size. */

  HCI_PM( "Reading the product attributes table\n" );
  Num_table_items = ORPGPAT_num_tbl_items() * 2;

  if( Num_table_items <= 0 )
  {
    HCI_LE_error( "Invalid data in Attributes Table" );
    HCI_task_exit( HCI_EXIT_FAIL ) ;
  }

  Data_tbl = malloc( sizeof( Gen_tbl_t ) * Num_table_items );

  if( Data_tbl == (char *) NULL )
  {
    HCI_LE_error( "Unable to allocate table memory" );
    HCI_task_exit( HCI_EXIT_FAIL ) ;
  }

  Gen_tbl = (Gen_tbl_t *) Data_tbl;

  /* The following widget definitions define the pushbuttons for
     the various control panel items.  The exact placement of these
     widgets is done in the resize callback procedure. */

  top_rowcol = XtVaCreateWidget( "top_rowcol",
                xmRowColumnWidgetClass, Form,
                XmNforeground, Base_bg,
                XmNbackground, Base_bg,
                XmNpacking, XmPACK_TIGHT,
                XmNorientation, XmHORIZONTAL,
                XmNleftAttachment, XmATTACH_FORM,
                XmNtopAttachment, XmATTACH_FORM,
                XmNentryAlignment, XmALIGNMENT_CENTER,
                NULL );

  button = XtVaCreateManagedWidget( " Close ",
                xmPushButtonWidgetClass, top_rowcol,
                XmNforeground, Button_fg,
                XmNbackground, Button_bg,
                XmNfontList, List_font,
                NULL );

  XtAddCallback( button, XmNactivateCallback, Close_callback, NULL );

  Save_button = XtVaCreateManagedWidget( "  Save ",
                xmPushButtonWidgetClass, top_rowcol,
                XmNforeground, Button_fg,
                XmNbackground, Button_bg,
                XmNfontList, List_font,
                XmNsensitive, False,
                NULL );

  XtAddCallback( Save_button, XmNactivateCallback, Save_callback, NULL );

  /* The undo button is used to reinitialize the generation
     table display by throwing away all current edits and re-reading
     the table from the LB. */

  Undo_button = XtVaCreateManagedWidget( " Undo ",
                xmPushButtonWidgetClass, top_rowcol,
                XmNforeground, Button_fg,
                XmNbackground, Button_bg,
                XmNfontList, List_font,
                XmNsensitive, False,
                NULL );

  XtAddCallback( Undo_button, XmNactivateCallback, Undo_changes_callback, NULL );

  /* Define a set of widgets which control which product generation
     list is active.  By default, the current list is active when
     the task is started. */

  label = XtVaCreateManagedWidget( "   Table: ",
                xmLabelWidgetClass, top_rowcol,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                NULL );

  n = 0;
  XtSetArg( args[n], XmNforeground, Text_fg ); n++;
  XtSetArg( args[n], XmNbackground, Base_bg ); n++;
  XtSetArg( args[n], XmNfontList, List_font ); n++;
  XtSetArg( args[n], XmNpacking, XmPACK_TIGHT ); n++;
  XtSetArg( args[n], XmNorientation, XmHORIZONTAL ); n++;

  table_filter = XmCreateRadioBox( top_rowcol, "product_filter", args, n );

  Current_button = XtVaCreateManagedWidget( "Current  ",
                xmToggleButtonWidgetClass, table_filter,
                XmNselectColor, White_color,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                XmNset, True,
                NULL );

  XtAddCallback( Current_button,
                XmNvalueChangedCallback, Select_generation_table_callback,
                (XtPointer) CURRENT_TABLE );

  Wx_mode_A_button = XtVaCreateManagedWidget( "Precip (A)  ",
                xmToggleButtonWidgetClass, table_filter,
                XmNselectColor, White_color,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                XmNset, False,
                NULL );

  XtAddCallback( Wx_mode_A_button,
                XmNvalueChangedCallback, Select_generation_table_callback,
                (XtPointer) DEFAULT_A_TABLE );

  Wx_mode_B_button = XtVaCreateManagedWidget( "Clear Air (B)  ",
                xmToggleButtonWidgetClass, table_filter,
                XmNselectColor, White_color,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                XmNset, False,
                NULL );

  XtAddCallback( Wx_mode_B_button,
                XmNvalueChangedCallback, Select_generation_table_callback,
                (XtPointer) DEFAULT_B_TABLE );

  XtManageChild( table_filter );
  XtManageChild( top_rowcol );

  /* Create a pixmap and drawn button for the window lock. */

  lock_form = XtVaCreateWidget( "locl_form",
                xmFormWidgetClass, Form,
                XmNtopAttachment, XmATTACH_FORM,
                XmNrightAttachment, XmATTACH_FORM,
                XmNbackground, Base_bg,
                NULL );

  /* Create lock with appropriate LOCA mask. */

  Lock_widget = hci_lock_widget( lock_form, Hci_product_generation_security, HCI_LOCA_ROC );

  XtManageChild( lock_form );

  /* The lock button is not displayed when the current product
     generation list is active since it is not password protected.
     Since the current list is active by default, unmanage the form
     containing the lock button so it isn't visible. */

  XtUnmanageChild( lock_form );

  /* Define a blank area when table isn't "Current". */

  baseline_frame = XtVaCreateManagedWidget( "baseline_frame",
                xmFrameWidgetClass, Form,
                XmNtopAttachment, XmATTACH_WIDGET,
                XmNtopWidget, top_rowcol,
                XmNleftAttachment, XmATTACH_FORM,
                XmNrightAttachment, XmATTACH_FORM,
                NULL );

  Baseline_rowcol = XtVaCreateWidget( "baseline_rowcol",
                xmRowColumnWidgetClass, baseline_frame,
                XmNorientation, XmHORIZONTAL,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                NULL );

  label = XtVaCreateManagedWidget( "Can't see this text",
                xmLabelWidgetClass, Baseline_rowcol,
                XmNforeground, Base_bg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                NULL );

  /* Define a set of radiobuttons so user can copy one of the
     default lists to the current list.  These buttons should be
     visible only when the current list is active. */

  replace_frame = XtVaCreateManagedWidget( "replace_frame",
                xmFrameWidgetClass, Form,
                XmNtopAttachment, XmATTACH_WIDGET,
                XmNtopWidget, baseline_frame,
                XmNleftAttachment, XmATTACH_FORM,
                XmNrightAttachment, XmATTACH_FORM,
                NULL );

  Replace_rowcol = XtVaCreateWidget( "replace_rowcol",
                xmRowColumnWidgetClass, replace_frame,
                XmNorientation, XmHORIZONTAL,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                NULL );

  label = XtVaCreateManagedWidget( "Replace Current Table with: ",
                xmLabelWidgetClass, Replace_rowcol,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                NULL );

  n = 0;
  XtSetArg( args[n], XmNforeground, Text_fg ); n++;
  XtSetArg( args[n], XmNbackground, Base_bg ); n++;
  XtSetArg( args[n], XmNfontList, List_font ); n++;
  XtSetArg( args[n], XmNorientation, XmHORIZONTAL ); n++;

  replace_filter = XmCreateRadioBox( Replace_rowcol, "replace_filter", args, n );

  button = XtVaCreateManagedWidget( "Precip Mode  (A)",
                xmToggleButtonWidgetClass, replace_filter,
                XmNselectColor, White_color,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                XmNset, False,
                NULL );

  XtAddCallback( button,
                XmNvalueChangedCallback, Replace_current_callback,
                (XtPointer) DEFAULT_A_TABLE );

  button = XtVaCreateManagedWidget( "Clear Air Mode (B) ",
                xmToggleButtonWidgetClass, replace_filter,
                XmNselectColor, White_color,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
               XmNfontList, List_font,
                XmNset, False,
                NULL );

  XtAddCallback( button,
                XmNvalueChangedCallback, Replace_current_callback,
                (XtPointer) DEFAULT_B_TABLE );

  XtManageChild( replace_filter );
  XtManageChild( Replace_rowcol );

  /* Provide the user with the capability to filter the displayed
     portion of the product generation list to those products which
     contain the search string in its description. */

  search_frame = XtVaCreateManagedWidget( "search_frame",
                xmFrameWidgetClass, Form,
                XmNtopAttachment, XmATTACH_WIDGET,
                XmNtopWidget, replace_frame,
                XmNleftAttachment, XmATTACH_FORM,
                XmNrightAttachment, XmATTACH_FORM,
                XmNpacking, XmPACK_TIGHT,
                NULL );

  search_rowcol = XtVaCreateWidget( "search_rowcol",
                xmRowColumnWidgetClass, search_frame,
                XmNorientation, XmHORIZONTAL,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                NULL );

  label = XtVaCreateManagedWidget( "Search:",
                xmLabelWidgetClass, search_rowcol,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                NULL );

  search_text = XtVaCreateManagedWidget( "search_text",
                xmTextFieldWidgetClass, search_rowcol,
                XmNforeground, Edit_fg,
                XmNbackground, Edit_bg,
                XmNfontList, List_font,
                XmNcolumns, 16,
                XmNverifyBell, False,
                NULL );

  XtAddCallback( search_text, XmNactivateCallback, Search_text_callback, NULL );
  XtAddCallback( search_text, XmNlosingFocusCallback, Search_text_callback, NULL );

  /* Provide a set of options for chosing the order in which
     products are displayed in the generation list. */

  label = XtVaCreateManagedWidget( "       Sort by: ",
                xmLabelWidgetClass, search_rowcol,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                NULL );

  n = 0;
  XtSetArg( args[n], XmNforeground, Text_fg ); n++;
  XtSetArg( args[n], XmNbackground, Base_bg ); n++;
  XtSetArg( args[n], XmNfontList, List_font ); n++;
  XtSetArg( args[n], XmNorientation, XmHORIZONTAL ); n++;

  sort_filter = XmCreateRadioBox( search_rowcol, "product_filter", args, n );

  Sort_pcode_button = XtVaCreateManagedWidget( "Product Code ",
                xmToggleButtonWidgetClass, sort_filter,
                XmNselectColor, White_color,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                XmNset, True,
                NULL );

  XtAddCallback( Sort_pcode_button,
                XmNvalueChangedCallback, Sort_filter_callback,
                (XtPointer) SORT_BY_PRODUCT_CODE );

  /* The function of this button varies by LOCA.  This field is
     product mnemonic for all edit levels except ROC when it is
     product ID. */

  Sort_pid_button = XtVaCreateManagedWidget( "Product MNE",
                xmToggleButtonWidgetClass, sort_filter,
                XmNselectColor, White_color,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                NULL );

  XtAddCallback( Sort_pid_button,
                XmNvalueChangedCallback, Sort_filter_callback,
                (XtPointer) SORT_BY_PRODUCT_MNE );

  Sort_description_button = XtVaCreateManagedWidget( "Description",
                xmToggleButtonWidgetClass, sort_filter,
                XmNselectColor, White_color,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                NULL );

  XtAddCallback( Sort_description_button,
                XmNvalueChangedCallback, Sort_filter_callback,
                (XtPointer) SORT_BY_DESCRIPTION );

  XtManageChild( sort_filter );
  XtManageChild( search_rowcol );

  /* Display a label to go above the product geration list table to
     identify the contents of each column in the table. */

  label_rowcol = XtVaCreateWidget( "label_rowcol",
                xmRowColumnWidgetClass, Form,
                XmNorientation, XmHORIZONTAL,
                XmNbackground, Base_bg,
                XmNtopAttachment, XmATTACH_WIDGET,
                XmNtopWidget, search_frame,
                XmNleftAttachment, XmATTACH_FORM,
                XmNrightAttachment, XmATTACH_FORM,
                XmNmarginHeight, 1,
                XmNmarginWidth, 1,
                NULL );

  Pid_text = XtVaCreateManagedWidget( "PrID",
                xmTextFieldWidgetClass, label_rowcol,
                XmNfontList, List_font,
                XmNbackground, Base_bg,
                XmNforeground, Text_fg,
                XmNcolumns, 4,
                XmNeditable, False,
                XmNtraversalOn, False,
                XmNmarginWidth, 0,
                XmNmarginHeight, 0,
                XmNborderWidth, 0,
                NULL );

  sprintf( Buf, "PrID" );
  XmTextSetString( Pid_text, Buf );

  text = XtVaCreateManagedWidget( "MNE",
                xmTextFieldWidgetClass, label_rowcol,
                XmNfontList, List_font,
                XmNbackground, Base_bg,
                XmNforeground, Text_fg,
                XmNcolumns, 4,
                XmNeditable, False,
                XmNtraversalOn, False,
                XmNmarginWidth, 0,
                XmNmarginHeight, 0,
                XmNborderWidth, 0,
                NULL );

  sprintf( Buf, "MNE" );
  XmTextSetString( text, Buf );

  text = XtVaCreateManagedWidget( "Code",
                xmTextFieldWidgetClass, label_rowcol,
                XmNfontList, List_font,
                XmNbackground, Base_bg,
                XmNforeground, Text_fg,
                XmNcolumns, 4,
                XmNeditable, False,
                XmNtraversalOn, False,
                XmNmarginWidth, 0,
                XmNmarginHeight, 0,
                XmNborderWidth, 0,
                NULL );

  sprintf( Buf, "Code" );
  XmTextSetString( text, Buf );

  text = XtVaCreateManagedWidget( "Gen",
                xmTextFieldWidgetClass, label_rowcol,
                XmNfontList, List_font,
                XmNbackground, Base_bg,
                XmNforeground, Text_fg,
                XmNcolumns, 4,
                XmNeditable, False,
                XmNtraversalOn, False,
                XmNmarginWidth, 0,
                XmNmarginHeight, 0,
                XmNborderWidth, 0,
                NULL );

  sprintf( Buf, "Gen" );
  XmTextSetString( text, Buf );

  text = XtVaCreateManagedWidget( "mins",
                xmTextFieldWidgetClass, label_rowcol,
                XmNfontList, List_font,
                XmNbackground, Base_bg,
                XmNforeground, Text_fg,
                XmNcolumns, 6,
                XmNeditable, False,
                XmNtraversalOn, False,
                XmNmarginWidth, 0,
                XmNmarginHeight, 0,
                XmNborderWidth, 0,
                NULL );

  sprintf( Buf, "(mins)" );
  XmTextSetString( text, Buf );

  text = XtVaCreateManagedWidget( "Cut(s)",
                xmTextFieldWidgetClass, label_rowcol,
                XmNfontList, List_font,
                XmNbackground, Base_bg,
                XmNforeground, Text_fg,
                XmNcolumns, 12,
                XmNeditable, False,
                XmNtraversalOn, False,
                XmNmarginWidth, 0,
                XmNmarginHeight, 0,
                XmNborderWidth, 0,
                NULL );

  sprintf( Buf, "Elev/Cut(s)" );
  XmTextSetString( text, Buf );

  text = XtVaCreateManagedWidget( "Product_Description",
                xmTextFieldWidgetClass, label_rowcol,
                XmNfontList, List_font,
                XmNbackground, Base_bg,
                XmNforeground, Text_fg,
                XmNcolumns, 66,
                XmNeditable, False,
                XmNtraversalOn, False,
                XmNmarginWidth, 0,
                XmNmarginHeight, 0,
                XmNborderWidth, 0,
                NULL );

  sprintf( Buf,"Product Description" );
  XmTextSetString(text, Buf );

  XtManageChild( label_rowcol );

  Draw_parameters_widget();

  /* Define the scroll widget to contain the currently selected
     product generation list. */

  Data_scroll = XtVaCreateManagedWidget( "data_scroll",
                xmScrolledWindowWidgetClass, Form,
                XmNheight, 400,
                XmNscrollingPolicy, XmAUTOMATIC,
                XmNforeground, Base_bg,
                XmNbackground, Base_bg,
                XmNtopAttachment, XmATTACH_WIDGET,
                XmNtopWidget, label_rowcol,
                XmNbottomAttachment, XmATTACH_WIDGET,
                XmNbottomWidget, Bottom_form,
                XmNleftAttachment, XmATTACH_FORM,
                XmNrightAttachment, XmATTACH_FORM,
                NULL );

  XtManageChild( Bottom_form );

  /* Get the ID of the clip widget associated with the scrolled
     window containing the generation list and set the background
     color so it is consistent with other widgets in the window. */

  XtVaGetValues( Data_scroll, XmNclipWindow, &clip, NULL );
  XtVaSetValues( clip, XmNbackground, Base_bg, NULL );

  /* Set up the product display order using the default sort method. */

  Sort_products( Sort_method );

  /* Manage and make the window visible. */

  XtManageChild( Form );
  XtRealizeWidget( Top_widget );
  XtUnmanageChild( Pid_text );

  /* Populate the product generation display list using the current table. */

  Show_generation_table( Which_table );

  /* When the task starts up the current product generation list is
     the active one so we need to check if it is currently being
     edited by another user.  If so, we need to let the user decide
     if they want to proceed or exit. */

  if( ORPGEDLOCK_get_edit_status( ORPGDAT_PROD_INFO, CURRENT_TABLE ) == ORPGEDLOCK_EDIT_LOCKED )
  {
    sprintf( Buf, "Another user is currently editing the selected\ndefault product generation table.  Any changes\nyou make may be overwritten by the other user." );
    hci_warning_popup( Top_widget, Buf, NULL );
  }

  ORPGEDLOCK_set_edit_lock( ORPGDAT_PROD_INFO, CURRENT_TABLE );

  /* Start HCI loop. */

  HCI_start( Hci_timer_proc, HCI_QUARTER_SECOND, RESIZE_HCI );

  return 0;
}

/************************************************************************
     Description: This function is the timer procedure who's main
                  purpose is to allow X events from the keyboard to
                  be processed and updates to occur.
 ************************************************************************/

static void Hci_timer_proc()
{
  if( Replace_flag == HCI_YES_FLAG )
  {
    Replace_flag = HCI_NO_FLAG;
    Replace_current_table();
  }

  if( Save_flag == HCI_YES_FLAG )
  {
    Save_flag = HCI_NO_FLAG;
    Generation_table_save();
  }

  if( Undo_flag == HCI_YES_FLAG )
  {
    Undo_flag = HCI_NO_FLAG;
    Undo_changes();
  }

  /* Update gui if new info is available. */

  if( ORPGPGT_get_update_flag() )
  {
    Show_generation_table( Which_table );
  }
}

/************************************************************************
     Description: This function is called when the "Close" button is
                  selected.  If any unsaved edits are detected, the
                  user is first propmted about saving them before
                  exiting the task.
 ************************************************************************/

static void Close_callback( Widget w, XtPointer y, XtPointer z )
{
  Close_flag = HCI_YES_FLAG;

  HCI_LE_log( "Product Generation Table Edit Close selected" );

  /* If any of the 4 lists have been modified, invoke the save
     callback before closing.  The save callback will prompt for
     verification. */

  if( ( Current_flag == HCI_CHANGED_FLAG ) ||
      ( Wx_mode_A_flag == HCI_CHANGED_FLAG ) ||
      ( Wx_mode_B_flag == HCI_CHANGED_FLAG ) )
  {
    Save_callback( w, (XtPointer) NULL, (XtPointer) NULL );
  }
  else
  {
    HCI_task_exit( HCI_EXIT_SUCCESS );
  }
}

/************************************************************************
     Description: This function is called when the "Save" button is
                  selected or directly by another function.  If the
                  "Save" button was selected, then client data is
                  NULL and all 4 tables are checked to see if any
                  edits have been made.  If so, the verification
                  function is called.  If this function is called
                  directly, one can use client data to pass the
                  ID of the table to be saved.
 ************************************************************************/

static void Save_callback( Widget w, XtPointer y, XtPointer z )
{
  /* Log a message to the hci log file. */

  HCI_LE_log( "Product Generation Table Edit Save selected" );

  /* If client data is NULL we need to check the change flags for
     all 4 tables to see if they have been edited.  If they have,
     call the verification function for all of the tables that have
     been edited. */

  if( (int) y == (int) NULL )
  {
    if( Current_flag == HCI_CHANGED_FLAG )
    {
      Verify_generation_table_save( w, CURRENT_TABLE );
    };

    if( Wx_mode_A_flag == HCI_CHANGED_FLAG )
    {
      Verify_generation_table_save( w, DEFAULT_A_TABLE );
    }

    if( Wx_mode_B_flag == HCI_CHANGED_FLAG )
    {
      Verify_generation_table_save( w, DEFAULT_B_TABLE );
    }
  }
  else
  {
    /* Since client data is not NULL, call the verification function
       explicitly for that table identified */
    Verify_generation_table_save( w, (int) y );
  }
}

/************************************************************************
     Description: This function builds a message and displays a save
                  verification popup window for the specified table.
 ************************************************************************/

static void Verify_generation_table_save( Widget w, int table_id )
{
  char buf[HCI_BUF_128];

  switch( table_id )
  {
    case CURRENT_TABLE :

      User_selected_table_to_save = CURRENT_TABLE;
      sprintf( buf, "Do you want to save changes to the\ncurrent generation table?" );
      break;

    case DEFAULT_A_TABLE :

      User_selected_table_to_save = DEFAULT_A_TABLE;
      sprintf( buf, "Do you want to save changes to the\nPrecip Mode (A) generation table?" );
      break;

    case DEFAULT_B_TABLE :

      User_selected_table_to_save = DEFAULT_B_TABLE;
      sprintf( buf, "Do you want to save changes to the\nClear Air Mode (B) generation table?" );
      break;
  }

  hci_confirm_popup( Top_widget, buf, Accept_generation_table_save, Cancel_generation_table_save );
}

/************************************************************************
     Description: This function is activated when the "Yes" button
                  is selected in the save verification popup window.
 ************************************************************************/

static void Accept_generation_table_save( Widget w, XtPointer y, XtPointer z )
{
  Save_flag = HCI_YES_FLAG;
}

static void Generation_table_save()
{
  int status = 0;

  /* Write a message to the hci log file. */

  HCI_LE_log( "Product Generation Table Edit Save accepted" );

  /* Since we are going to perform I/O, display the progress meter
     if low bandwidth. */

  HCI_PM( "Saving product generation table" );

  /* Write the table indicated by client data. */

  status = ORPGPGT_write_tbl( User_selected_table_to_save );

  /* Build a feedback message using client data to identify the
     table which was saved. */

  switch( User_selected_table_to_save )
  {
    case CURRENT_TABLE :

      Current_flag = HCI_NOT_CHANGED_FLAG;
      if( status < 0 )
      {
        sprintf( Feedback, "Unable to update Current Product Generation List" );
      }
      else
      {
        sprintf( Feedback, "Current Product Generation List Updated" );
      }
      break;

    case DEFAULT_A_TABLE :

      Wx_mode_A_flag = HCI_NOT_CHANGED_FLAG;
      if( status < 0 )
      {
        sprintf( Feedback, "Unable to update Default A Product Generation List" );
      }
      else
      {
        sprintf( Feedback, "Default A Product Generation List Updated" );
      }
      break;

    case DEFAULT_B_TABLE :

      Wx_mode_B_flag = HCI_NOT_CHANGED_FLAG;
      if( status < 0 )
      {
        sprintf( Feedback, "Unable to update Default B Product Generation List" );
      }
      else
      {
        sprintf( Feedback, "Default B Product Generation List Updated" );
      }
      break;
  }

  /* Display the feedback message in the feedback line of the RPG
     Control/Status window (if it is running). */

  HCI_display_feedback( Feedback );

  /* If the close flag is set, then exit this task. */

  if( Close_flag == HCI_YES_FLAG )
  {
    HCI_task_exit( HCI_EXIT_SUCCESS );
  }

  /* Desensitize the Save and undo buttons since there should be no
     unsaved edits anymore. */

  XtSetSensitive( Save_button, False );
  XtSetSensitive( Undo_button, False );
}

/************************************************************************
     Description: This function is activated when the "No" button
                  is selected in the save verification popup window.
 ************************************************************************/

static void Cancel_generation_table_save( Widget w, XtPointer y, XtPointer z )
{
  /* Write a message to the hci log file. */

  HCI_LE_log( "Product Generation Table Edit Save cancelled" );

  /* If the close flag is set, then exit this task. */

  if( Close_flag == HCI_YES_FLAG ){ HCI_task_exit( HCI_EXIT_SUCCESS ); }

  /* If the active table is one of the default lists and we are no
     longer in edit mode (window locked), call the undo function to
     discard edits. */

  if( ( Which_table != CURRENT_TABLE ) && !Cut_mode )
  {
    Undo_changes_callback( w, y, z );
  }
}

/************************************************************************
     Description: This function is activated when the "Undo" button
      is selected.  It can also be called directly from
      another function.  The currently selected table is
      refreshed from file.
 ************************************************************************/

static void Undo_changes_callback( Widget w, XtPointer y, XtPointer z )
{
  Undo_flag = HCI_YES_FLAG;
}

void Undo_changes()
{
  int status = 0;

  /* Write a message to the hci log file. */

  HCI_LE_log( "Product Generation Table Edit Reload selected" );

  /* Since we are going to perform I/O, display the progress meter
     if low bandwidth. */

  HCI_PM( "Reading the product generation table" );

  /* Force a read of the specified table and redisplay table. */

  status = ORPGPGT_read_tbl( Which_table );

  Show_generation_table( Which_table );

  /* Clear the change flag associated with the active table. */

  switch( Which_table )
  {
    case CURRENT_TABLE :

      Current_flag = HCI_NOT_CHANGED_FLAG;
      break;

    case DEFAULT_A_TABLE :

      Wx_mode_A_flag = HCI_NOT_CHANGED_FLAG;
      break;

    case DEFAULT_B_TABLE :

      Wx_mode_B_flag = HCI_NOT_CHANGED_FLAG;
      break;

  }

  /* Desensitize the Save and undo buttons since there should be no
     unsaved edits anymore. */

  XtSetSensitive( Save_button, False );
  XtSetSensitive( Undo_button, False );
}

/************************************************************************
     Description: This function is activated when one of the table
                  select radio buttons is selected.  The new table
                  is displayed.
 ************************************************************************/

static void Select_generation_table_callback( Widget w, XtPointer y, XtPointer z )
{
  static int old_table = CURRENT_TABLE;
  int selected_table = (int) y;
  XmString str;
  XmToggleButtonCallbackStruct *state;

  state = (XmToggleButtonCallbackStruct *) z;

  /* If the button is set, then proceed. */

  if( state->set )
  {
    /* If the new table is currently being edited by another user
       inform the user.  Otherwise, set the edit lock so other
       users can tell that we are editing it. */

    if( Cut_mode && ( old_table != CURRENT_TABLE ) )
    {
      /* Clear the edit lock for the previous edit table. */
      ORPGEDLOCK_clear_edit_lock( ORPGDAT_PROD_INFO, old_table );

      if( ORPGEDLOCK_get_edit_status( ORPGDAT_PROD_INFO, selected_table ) == ORPGEDLOCK_EDIT_LOCKED )
      {
        sprintf( Buf, "Another user is currently editing the selected\ndefault product generation table.  Any changes\nyou make may be overwritten by the other user." );
        hci_warning_popup( Top_widget, Buf, NULL );
      }

      if( selected_table != CURRENT_TABLE )
      {
        ORPGEDLOCK_set_edit_lock( ORPGDAT_PROD_INFO, selected_table );
      }
    }
    else if( old_table == CURRENT_TABLE )
    {
      /* Clear the edit lock for the previous edit table. */
      ORPGEDLOCK_clear_edit_lock( ORPGDAT_PROD_INFO, old_table );
    }

    /* The newly selected list is one of the default lists. */
    if( selected_table != CURRENT_TABLE )
    {
      /* If the previously selected list was the current list,
         then we want to make the lock visible, the replace
         buttons invisible, and the baseline buttons visible. */

      if( old_table == CURRENT_TABLE )
      {
        XtManageChild( XtParent( Lock_widget ) );
        XtUnmanageChild( Replace_rowcol );
        XtManageChild( Baseline_rowcol );

        Cut_color = Base_bg;
        Cut_mode = Unlocked_roc;

        /* If we made any changes to the current list then
           we need to prompt the user so they have a chance to
           save them. */

        if( Current_flag == HCI_CHANGED_FLAG )
        {
          Save_callback( (Widget) w, (XtPointer) NULL, (XtPointer) CURRENT_TABLE );
          Current_flag = HCI_NOT_CHANGED_FLAG;
        }
      }
      else if( old_table == DEFAULT_A_TABLE )
      {
        /* If the previously selected list was the precip (A)
           list and edits were made, we need to prompt the user
           so they have a chance to save them. */

        if( Wx_mode_A_flag == HCI_CHANGED_FLAG )
        {
          Save_callback( (Widget) w, (XtPointer) NULL, (XtPointer) DEFAULT_A_TABLE );
          Wx_mode_A_flag = HCI_NOT_CHANGED_FLAG;
        }
      }
      else if( old_table == DEFAULT_B_TABLE )
      {
        /* If the previously selected list was the clear air (B)
           list and edits were made, we need to prompt the user
           so they have a chance to save them. */

        if( Wx_mode_B_flag == HCI_CHANGED_FLAG )
        {
          Save_callback( (Widget) w, (XtPointer) NULL, (XtPointer) DEFAULT_B_TABLE );
          Wx_mode_B_flag = HCI_NOT_CHANGED_FLAG;
        }
      }
    }
    else
    {
      /* The current list has been selected so we need to make the
         lock button and baseline buttons invisible and the replace
         buttons visible. */

      XtUnmanageChild( XtParent( Lock_widget ) );
      XtManageChild( Replace_rowcol );
      XtUnmanageChild( Baseline_rowcol );

      Show_all_products = HCI_NO_FLAG;

      str = XmStringCreateLocalized( "Product MNE" );
      XtVaSetValues( Sort_pid_button, XmNlabelString, str, NULL );
      XmStringFree( str );

      XtUnmanageChild( Pid_text );

      Cut_color = Edit_bg;
      Cut_mode = True;

      /* If the previously selected list was the precip (A)
         list and edits were made, we need to prompt the user
         so they have a chance to save them. */

      if( old_table == DEFAULT_A_TABLE )
      {
        if( Wx_mode_A_flag == HCI_CHANGED_FLAG )
        {
          Save_callback( (Widget) w, (XtPointer) NULL, (XtPointer) DEFAULT_A_TABLE );
          Wx_mode_A_flag = HCI_NOT_CHANGED_FLAG;
        }
      }
      else if( old_table == DEFAULT_B_TABLE )
      {
        /* If the previously selected list was the clear air (B)
        list and edits were made, we need to prompt the user
        so they have a chance to save them. */

        if( Wx_mode_B_flag == HCI_CHANGED_FLAG )
        {
          Save_callback( (Widget) w, (XtPointer) NULL, (XtPointer) DEFAULT_B_TABLE );
          Wx_mode_B_flag = HCI_NOT_CHANGED_FLAG;
        }
      }

      /* If the new table is currently being edited by another
         which inform the user. */

      if( ORPGEDLOCK_get_edit_status( ORPGDAT_PROD_INFO, selected_table) == ORPGEDLOCK_EDIT_LOCKED )
      {
        sprintf( Buf, "Another user is currently editing the selected\ndefault product generation table.  Any changes\nyou make may be overwritten by the other user." );
        hci_warning_popup( Top_widget, Buf, NULL );
      }

      ORPGEDLOCK_set_edit_lock( ORPGDAT_PROD_INFO, selected_table );
    }

    /* Get the table corresponding to the selected button from
       the buttons XtPointer. */

    Which_table = selected_table;
    old_table = Which_table;

    /* Call the undo callback so the newly selected table is refreshed */

    Undo_changes_callback( w, (XtPointer) NULL, (XtPointer) NULL );

    /* Sort the products and redisplay the list. */

    Sort_products( Sort_method );
    Show_generation_table( Which_table );
  }
}

/************************************************************************
     Description: This function is activated when a new search
                  filter pattern is entered in the search edit box.
                  The table display is refreshed.
 ************************************************************************/

static void Search_text_callback( Widget w, XtPointer y, XtPointer z )
{
  char *text = NULL;
  static char old_search_string[256] = { "" };

  /* Get the new search filter string from the text widgets string. */

  text = XmTextGetString( w );

  /* If the new search string has not changed, do nothing. */

  if( ( ( strlen( text ) == 0 ) && ( strlen( old_search_string ) == 0 ) ) ||
      ( !strcmp( text, old_search_string ) ) )
  {
    return;
  }

  /* Copy the new search string to the common buffer. */

  strcpy( Search_string, text );

  /* Update the local string so we can compare changes in the future */

  strcpy( old_search_string, text );
  XtFree( text );

  /* Set the active row variable (Active_row) to point to the first
     row item so after the products are filtered, it cannot point
     to a row that no longer exists. */

  Active_row = Gen_tbl[0].rowcol;

  /* The following code forces the callback for the active sort
     method to be executed. This forces a display refresh. */

  switch( Which_table )
  {
    case CURRENT_TABLE :

      XmToggleButtonSetState( Current_button, False, False );
      XmToggleButtonSetState( Current_button, True, True );
      break;

    case DEFAULT_A_TABLE :

      XmToggleButtonSetState( Wx_mode_A_button, False, False );
      XmToggleButtonSetState( Wx_mode_A_button, True, True );
      break;

    case DEFAULT_B_TABLE :

      XmToggleButtonSetState( Wx_mode_B_button, False, False );
      XmToggleButtonSetState( Wx_mode_B_button, True, True );
  }
}

/************************************************************************
     Description: This function is activated when one of the sort
                  radio buttons is selected. The table display is
                  refreshed.
 ************************************************************************/

static void Sort_filter_callback( Widget w, XtPointer y, XtPointer z )
{
  XmToggleButtonCallbackStruct *state =
          (XmToggleButtonCallbackStruct *) z;

  /* Do only if the radio button is set, not cleared. */

  if( state->set )
  {
    /* Resort the products by selected sort method. */
    Sort_method = (int) y;
    Sort_products( Sort_method );
    /* Redisplay the product list. */
    Show_generation_table( Which_table );
  }
}

/************************************************************************
     Description: This function sorts the products by the specified
                  method.  The array Sort_index[] is used to map
                  a product attributes entry to a relative position
                  in the list.
 ************************************************************************/

static void Sort_products( int method )
{
  int i = 0;
  int j = 0;
  int temp = 0;
  int indx = 0;
  int att_tbl_size = 0;
  int pcode1 = 0;
  int pcode2 = 0;
  int buf_num1 = 0;
  int buf_num2 = 0;
  char *buf1 = NULL;
  char *buf2 = NULL;
  char mne1[8];
  char mne2[8];

  /* Get the number of items in the product attributes table. */

  att_tbl_size = ORPGPAT_num_tbl_items();

  /* Initialize the sort table. */

  indx = 0;

  for( i = 0; i < att_tbl_size; i++ )
  {
    if( ORPGPAT_get_prod_id( i ) > 0 ){ Sort_index[indx++] = i; }
  }

  /* Begin sorting using the chosen method */

  switch( method )
  {
    case SORT_BY_DESCRIPTION :

      for( i = 0; i < indx; i++ )
      {
        buf_num1 = ORPGPAT_get_prod_id( Sort_index[i] );
        buf1 = ORPGPAT_get_description( buf_num1, STRIP_MNEMONIC );
        for( j = i+1; j < indx; j++ )
        {
          buf_num2 = ORPGPAT_get_prod_id( Sort_index[j] );
          buf2 = ORPGPAT_get_description( buf_num2, STRIP_MNEMONIC );
          if( strcasecmp( buf2, buf1 ) < 0 )
          {
            temp = Sort_index[i];
            Sort_index[i] = Sort_index[j];
            Sort_index[j] = temp;
            buf1 = buf2;
          }
        }
      }
      break;

    case SORT_BY_PRODUCT_CODE :

      for( i = 0; i < indx; i++ )
      {
        buf_num1 = ORPGPAT_get_prod_id( Sort_index[i] );
        pcode1 = ORPGPAT_get_code( buf_num1 );
        for( j = i+1; j < indx; j++ )
        {
          buf_num2 = ORPGPAT_get_prod_id( Sort_index[j] );
          pcode2 = ORPGPAT_get_code( buf_num2 );
          if( ( ( pcode2 >  0 ) && ( pcode1 <= 0 ) ) ||
              ( ( pcode2 >  0 ) && ( pcode1 >  0 ) && ( pcode2 < pcode1 ) ) )
          {
            temp = Sort_index[i];
            Sort_index[i] = Sort_index[j];
            Sort_index[j] = temp;
            pcode1 = pcode2;
          }
        }
      }
      break;

    case SORT_BY_PRODUCT_MNE :

      if( Show_all_products == HCI_NO_FLAG )
      {
        /* Sort by product MNE */
        for( i = 0; i < indx; i++ )
        {
          buf_num1 = ORPGPAT_get_prod_id( Sort_index[i] );
          strcpy( mne1, ORPGPAT_get_mnemonic( buf_num1 ) );
          for( j = i+1; j < indx; j++ )
          {
            buf_num2 = ORPGPAT_get_prod_id( Sort_index[j] );
            strcpy( mne2, ORPGPAT_get_mnemonic( buf_num2 ) );
            if( strncasecmp( &mne2[0], &mne1[0], 1 ) < 0 )
            {
              temp = Sort_index[i];
              Sort_index[i] = Sort_index[j];
              Sort_index[j] = temp;
              buf1 = buf2;
              strcpy( mne1, ORPGPAT_get_mnemonic( buf_num2 ) );
            }
            else if( strncasecmp( &mne2[0], &mne1[0], 1 ) == 0 )
            {
              if( strncasecmp( &mne2[1], &mne1[1], 1 ) < 0 )
              {
                temp = Sort_index[i];
                Sort_index[i] = Sort_index[j];
                Sort_index[j] = temp;
                buf1 = buf2;
                strcpy( mne1, ORPGPAT_get_mnemonic( buf_num2 ) );
              }
              else if( strncasecmp( &mne2[1], &mne1[1], 1 ) == 0 )
              {
                if( strncasecmp( &mne2[2], &mne1[2], 1 ) < 0 )
                {
                  temp = Sort_index[i];
                  Sort_index[i] = Sort_index[j];
                  Sort_index[j] = temp;
                  buf1 = buf2;
                  strcpy( mne1, ORPGPAT_get_mnemonic( buf_num2 ) );
                }
              }
            }
          }
        }
      }
      else
      {
        /* Sort by product ID */
        for( i = 0; i < indx; i++ )
        {
          buf_num1 = ORPGPAT_get_prod_id( Sort_index[i] );
          for( j = i+1; j < indx; j++ )
          {
            buf_num2 = ORPGPAT_get_prod_id( Sort_index[j] );
            if( buf2 < buf1 )
            {
              temp = Sort_index[i];
              Sort_index[i] = Sort_index[j];
              Sort_index[j] = temp;
              buf_num1 = buf_num2;
            }
          }
        }
      }
      break;
  }

  /* Set the unused elements in the array to -1 */

  for( i = indx; i < att_tbl_size; i++ ){ Sort_index[indx++] = -1; }
}

/************************************************************************
     Description: This function displays the specified product
                  generation table.
 ************************************************************************/

static void Show_generation_table( int id )
{
  int i = 0;
  int j = 0;
  int n = 0;
  int indx = 0;
  int pindx = 0;
  int ndx = 0;
  int pd = 0;
  int gen = 0;
  int elevation_based = HCI_NO_FLAG;
  char *string = NULL;
  int att_tbl_num = 0;
  int buf_num = 0;
  int pcode = 0;
  int found = 0;
  float slice = 0.0;
  int min = 0;
  int max = 0;
  int scale = 0;
  int param = 0;
  int value = 0;
  int flags = 0;
  char *gen_task = NULL;
  char *mne = NULL;
  XtPointer row = (XtPointer) NULL;
  XmString str;

  /* If this is the first time this module has been called we need
     first create all of the widgets making up the product list. */

  if( Data_form == (Widget) NULL )
  {
    Data_form = XtVaCreateWidget( "Data_form",
                xmFormWidgetClass, Data_scroll,
                XmNbackground, Black_color,
                XmNverticalSpacing, 1,
                NULL );

    sprintf( Buf, "0" );

    /* Generate a row for each product in the product attributes table */
    for( i = 0; i < Num_table_items; i++ )
    {
      /* The first rowcolunm widget is attached to the top of the form */
      if( i == 0 )
      {
        Gen_tbl[i].rowcol = XtVaCreateWidget( "data_rowcol",
                xmRowColumnWidgetClass, Data_form,
                XmNorientation, XmHORIZONTAL,
                XmNbackground, Base_bg,
                XmNtopAttachment, XmATTACH_FORM,
                XmNleftAttachment, XmATTACH_FORM,
                XmNrightAttachment, XmATTACH_FORM,
                XmNuserData, (XtPointer) i,
                XmNmarginHeight, 1,
                XmNmarginWidth, 0,
                NULL );

        Active_row = Gen_tbl[i].rowcol;
      }
      else
      {
        /* All other rowcolumn widgets are attached to the row above it */
        Gen_tbl[i].rowcol = XtVaCreateWidget( "data_rowcol",
                xmRowColumnWidgetClass, Data_form,
                XmNorientation, XmHORIZONTAL,
                XmNbackground, Base_bg,
                XmNtopAttachment, XmATTACH_WIDGET,
                XmNtopWidget, Gen_tbl[i-1].rowcol,
                XmNleftAttachment, XmATTACH_FORM,
                XmNrightAttachment, XmATTACH_FORM,
                XmNuserData, (XtPointer) i,
                XmNmarginHeight, 1,
                XmNmarginWidth, 0,
                NULL );
      }

      /* The product ID goes in the first column (only visible
         to the ROC level user). */
      Gen_tbl[i].pid = XtVaCreateManagedWidget( "tbl_pid",
                xmTextFieldWidgetClass, Gen_tbl[i].rowcol,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                XmNcolumns, 4,
                XmNtraversalOn, False,
                XmNeditable, False,
                XmNhighlightColor, White_color,
                XmNtopShadowColor, Edit_bg,
                XmNbottomShadowColor, Text_fg,
                XmNborderColor, Base_bg,
                XmNmarginWidth, 0,
                XmNmarginHeight, 0,
                XmNborderWidth, 0,
                NULL );

      /* The product mnemonic goes in the next column (first
         visible column for non ROC level user). */
      Gen_tbl[i].mnemonic = XtVaCreateManagedWidget( "tbl_mne",
                xmTextFieldWidgetClass, Gen_tbl[i].rowcol,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                XmNcolumns, 4,
                XmNtraversalOn, False,
                XmNeditable, False,
                XmNhighlightColor, White_color,
                XmNtopShadowColor, Edit_bg,
                XmNbottomShadowColor, Text_fg,
                XmNborderColor, Base_bg,
                XmNmarginWidth, 0,
                XmNmarginHeight, 0,
                XmNborderWidth, 0,
                NULL );

      /* Display the product code in the next column. */
      Gen_tbl[i].pcode = XtVaCreateManagedWidget( "tbl_pcode",
                xmTextFieldWidgetClass, Gen_tbl[i].rowcol,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                XmNcolumns, 4,
                XmNtraversalOn, False,
                XmNeditable, False,
                XmNhighlightColor, White_color,
                XmNtopShadowColor, Edit_bg,
                XmNbottomShadowColor, Text_fg,
                XmNborderColor, Base_bg,
                XmNmarginWidth, 0,
                XmNmarginHeight, 0,
                XmNborderWidth, 0,
                NULL );

      /* Display the generation interval in the next column. */
      Gen_tbl[i].generation = XtVaCreateManagedWidget( "tbl_gen",
                xmTextFieldWidgetClass, Gen_tbl[i].rowcol,
                XmNforeground, Text_fg,
                XmNbackground, Cut_color,
                XmNfontList, List_font,
                XmNcolumns, 4,
                XmNtraversalOn, Cut_mode,
                XmNeditable, Cut_mode,
                XmNhighlightColor, White_color,
                XmNtopShadowColor, Edit_bg,
                XmNbottomShadowColor, Text_fg,
                XmNborderColor, Base_bg,
                XmNmarginWidth, 0,
                XmNmarginHeight, 0,
                XmNborderWidth, 0,
                XmNverifyBell, False,
                NULL );

      XmTextSetString( Gen_tbl[i].generation, Buf );
      XtAddCallback( Gen_tbl[i].generation, XmNfocusCallback, Gen_table_gain_focus, NULL );
      XtAddCallback( Gen_tbl[i].generation, XmNmodifyVerifyCallback, hci_verify_unsigned_integer_callback, (XtPointer) 4 );
      XtAddCallback( Gen_tbl[i].generation, XmNlosingFocusCallback, Gen_table_modify, GENERATION );
      XtAddCallback( Gen_tbl[i].generation, XmNactivateCallback, Gen_table_modify, GENERATION );

      /* Display the retention period in the next column. */
      Gen_tbl[i].retention = XtVaCreateManagedWidget( "tbl_ret",
                xmTextFieldWidgetClass, Gen_tbl[i].rowcol,
                XmNforeground, Text_fg,
                XmNbackground, Cut_color,
                XmNfontList, List_font,
                XmNcolumns, 6,
                XmNtraversalOn, Cut_mode,
                XmNeditable, Cut_mode,
                XmNhighlightColor, White_color,
                XmNtopShadowColor, Edit_bg,
                XmNbottomShadowColor, Text_fg,
                XmNborderColor, Base_bg,
                XmNmarginWidth, 0,
                XmNmarginHeight, 0,
                XmNborderWidth, 0,
                XmNverifyBell, False,
                NULL );

      XmTextSetString( Gen_tbl[i].retention, Buf );

      XtAddCallback( Gen_tbl[i].retention, XmNfocusCallback, Gen_table_gain_focus, NULL );
      XtAddCallback( Gen_tbl[i].retention, XmNmodifyVerifyCallback, hci_verify_unsigned_integer_callback, (XtPointer) 6 );
      XtAddCallback( Gen_tbl[i].retention, XmNlosingFocusCallback, Gen_table_modify, (XtPointer) RETENTION );
      XtAddCallback( Gen_tbl[i].retention, XmNactivateCallback, Gen_table_modify, (XtPointer) RETENTION );

      /* Display the elevation/cut toggle in the next column. */
      Gen_tbl[i].cut_toggle = XtVaCreateManagedWidget( "<=",
                xmToggleButtonWidgetClass, Gen_tbl[i].rowcol,
                XmNforeground, Text_fg,
                XmNbackground, Button_bg,
                XmNselectColor, White_color,
                XmNfontList, List_font,
                XmNtraversalOn, Cut_mode,
                XmNsensitive, Cut_mode,
                XmNhighlightColor, White_color,
                XmNtopShadowColor, Edit_bg,
                XmNbottomShadowColor, Text_fg,
                XmNborderColor, Base_bg,
                XmNmarginWidth, 0,
                XmNmarginHeight, 0,
                XmNborderWidth, 0,
                XmNverifyBell, False,
                XmNset, False,
                NULL );

      XtAddCallback( Gen_tbl[i].cut_toggle, XmNvalueChangedCallback, Hci_cut_toggle_callback, (XtPointer) NULL );

      /* Display the elevation/cut in the next column. */
      Gen_tbl[i].cut = XtVaCreateManagedWidget( "tbl_cut",
                xmTextFieldWidgetClass, Gen_tbl[i].rowcol,
                XmNforeground, Text_fg,
                XmNbackground, Cut_color,
                XmNfontList, List_font,
                XmNcolumns, 6,
                XmNtraversalOn, Cut_mode,
                XmNeditable, Cut_mode,
                XmNhighlightColor, White_color,
                XmNtopShadowColor, Edit_bg,
                XmNbottomShadowColor, Text_fg,
                XmNborderColor, Base_bg,
                XmNmarginWidth, 0,
                XmNmarginHeight, 0,
                XmNborderWidth, 0,
                XmNverifyBell, False,
                NULL );

      XtAddCallback( Gen_tbl[i].cut, XmNfocusCallback, Gen_table_gain_focus, NULL );
      XtAddCallback( Gen_tbl[i].cut, XmNmodifyVerifyCallback, hci_verify_float_callback, (XtPointer) 6 );
      XtAddCallback( Gen_tbl[i].cut, XmNlosingFocusCallback, Gen_table_modify, (XtPointer) CUT );
      XtAddCallback( Gen_tbl[i].cut, XmNactivateCallback, Gen_table_modify, (XtPointer) CUT );

      /* Display the product description in the next column. */
      Gen_tbl[i].description = XtVaCreateManagedWidget( "tbl_desc",
                xmTextFieldWidgetClass, Gen_tbl[i].rowcol,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                XmNcolumns, 64,
                XmNtraversalOn, False,
                XmNeditable, False,
                XmNhighlightColor, White_color,
                XmNtopShadowColor, Edit_bg,
                XmNbottomShadowColor, Text_fg,
                XmNborderColor, Base_bg,
                XmNmarginWidth, 0,
                XmNmarginHeight, 0,
                XmNborderWidth, 0,
                XmNverifyBell, False,
                NULL );

    }
  }

  /* Get the row index for the row having focus (highlighted) */
  XtVaGetValues( Active_row, XmNuserData, &row, NULL );

  /* Initialize the lookup tables mapping the elements in the
     dispayed list to the product generation and product attributes
     tables. */
  for( i = 0; i < Num_table_items; i++ )
  {
    Gen_map[i] = -1;
    Att_map[i] = -1;
  }

  n = 0;

  att_tbl_num = ORPGPAT_num_tbl_items();

  /* For each item in the attributes table, check to see if is a
     valid external product type.  A valid external product has a
     product code greater than 15 and less than 300.  All other
     products are considered internal and not distributable.
     NOTE: When unlocked by the ROC user, all RPG generated products
     are listed, including intermediate. */
  for( i = 0; i < att_tbl_num; i++ )
  {
    if( Sort_index[i] < 0 ){ break; }

    buf_num = ORPGPAT_get_prod_id( Sort_index[i] );
    pcode = ORPGPAT_get_code( buf_num );
    gen_task = ORPGPAT_get_gen_task( buf_num );

    /* Check to see if product is valid for distribution. */
    if( ( ( pcode >= MIN_PRODUCT_CODE ) &&
          ( pcode <= ORPGPAT_MAX_PRODUCT_CODE ) &&
          ( gen_task != NULL ) ) ||
        ( Show_all_products && ( pcode >= 0 ) && ( buf_num < 1000 ) ) )
    {
      sprintf( Buf, "  " );
      XmTextSetString( Gen_tbl[n].cut, Buf );

      /* Get a pointer to the product description string. */
      string = (char *) ORPGPAT_get_description( buf_num, STRIP_NOTHING );

      /* If a filter string was specified, only show those
         products which contain the specified string. */
      if( ( !strlen( Search_string ) ||
            ( ( strlen( Search_string ) > 0 ) &&
              ( hci_string_in_string( string, Search_string ) != 0 ) ) ) )
      {
        /* Get the parameter index for the elevation/cut if it is
        elevation based. */
        indx = ORPGPAT_elevation_based( buf_num );
        pindx = -1;

        /* Get the attribute table index for the elevation parameter
           if the product elevation based. */
        if( indx >= 0 )
        {
          for( j = 0; j < ORPGPAT_get_num_parameters( buf_num ); j++ )
          {
            if( indx == ORPGPAT_get_parameter_index( buf_num, j ) )
            {
              pindx = j;
              break;
            }
          }
        }

        found = 0;

        /* If we are the ROC user, then we want to see all
           products (including intermediate).  Since iall
           intermediate products have product codes of 0 and
           no mnemonic, we will display product id also. */

        sprintf( Buf, "%3d", buf_num );
        XmTextSetString( Gen_tbl[n].pid, Buf );

        mne = ORPGPAT_get_mnemonic( buf_num );
        sprintf( Buf, "%3s", mne );
        XmTextSetString( Gen_tbl[n].mnemonic, Buf );

        if( Show_all_products ){ XtManageChild( Gen_tbl[n].pid ); }
        else{ XtUnmanageChild( Gen_tbl[n].pid ); }

        sprintf( Buf, "%2d", pcode );
        XmTextSetString( Gen_tbl[n].pcode, Buf );

        /* Now we need to determine if the product is in
           the generation table.  If so, we need to get the
           various generation fields and populate the widgets. */

        if( ( ndx = ORPGPGT_buf_in_tbl( id, buf_num ) ) >= 0 )
        {
          /* -----Generation Period----- */
          gen = ORPGPGT_get_generation_interval( id, ndx );
          sprintf( Buf, "%2d", gen );
          XmTextSetString( Gen_tbl[n].generation, Buf );

          /* -----Storage Retention----- */
          pd = ORPGPGT_get_retention_period( id, ndx );
          sprintf( Buf, "%3d", pd );
          XmTextSetString( Gen_tbl[n].retention, Buf );

          /* -----Elevation Angle/Slice----- */
          if( indx >= 0 )
          {
            slice = ORPGPGT_get_parameter( id, ndx, indx );
            scale = ORPGPAT_get_parameter_scale( buf_num, pindx );

            /* Separate the cut/elevation data from the state flag */
            flags = ( (int) slice ) & 0x6000;
            slice = ( (int) slice ) & 0x1fff;

            if( flags != ORPGPRQ_LOWER_CUTS )
            {
              slice = slice/scale;

              if( slice > 90.0 ){ slice -= 360.0; }

              sprintf( Buf, "%5.1f", slice );

              if( flags == ORPGPRQ_LOWER_ELEVATIONS )
              {
                XmToggleButtonSetState( Gen_tbl[n].cut_toggle, True, False );
              }
              else if( flags == ORPGPRQ_ALL_ELEVATIONS )
              {
                sprintf( Buf, "%5.1f", 45.0 );
                XmToggleButtonSetState( Gen_tbl[n].cut_toggle, True, False );
              }
              else
              {
                XmToggleButtonSetState( Gen_tbl[n].cut_toggle, False, False );
              }

              XtVaSetValues( Gen_tbl[n].cut_toggle,
                XmNbackground, Button_bg,
                XmNtraversalOn, Cut_mode,
                XmNsensitive, Cut_mode,
                NULL );
            }
            else
            {
              sprintf( Buf, "%5d", (int) -slice );

              XmToggleButtonSetState( Gen_tbl[n].cut_toggle, False, False );

              XtVaSetValues( Gen_tbl[n].cut_toggle,
                XmNbackground, Base_bg,
                XmNtraversalOn, False,
                XmNsensitive, False,
                NULL );
            }

            XtVaSetValues( Gen_tbl[n].cut,
                XmNbackground, Cut_color,
                XmNtraversalOn, Cut_mode,
                XmNeditable, Cut_mode,
                NULL );

            XmTextSetString( Gen_tbl[n].cut, Buf );
          }
          else
          {
            /* Volume based product.  Leave cut field blank. */
            sprintf( Buf, " " );
            XmTextSetString( Gen_tbl[n].cut, Buf );

            XtVaSetValues( Gen_tbl[n].cut,
                XmNbackground, Base_bg,
                XmNtraversalOn, False,
                XmNeditable, False,
                NULL );

            XtVaSetValues( Gen_tbl[n].cut_toggle, 
                                XmNbackground, Base_bg,
                XmNtraversalOn, False,
                XmNsensitive, False,
                NULL );

            XmToggleButtonSetState( Gen_tbl[n].cut_toggle, False, False );
          }

          /* -----Product Description----- */
          string = (char *) ORPGPAT_get_description( buf_num, STRIP_MNEMONIC );
          strcpy( Buf, string );
          XmTextSetString( Gen_tbl[n].description, Buf );

          /* Display entry */
          XtManageChild( Gen_tbl[n].rowcol );

          /* Map product to attribute table. */
          Att_map[n] = Sort_index[i];

          /* Map product to generation table. */
          Gen_map[n] = ndx;
        }
        else
        {
          /* Product not in generation table. Create a set of blank
             widgets for the product so the user can add it to the
             generation table by filling in the widget text fields. */
          sprintf( Buf, "0" );
          XmTextSetString( Gen_tbl[n].generation, Buf );
          XmTextSetString( Gen_tbl[n].retention, Buf );
          XmToggleButtonSetState( Gen_tbl[n].cut_toggle, False, False );
          XtVaSetValues( Gen_tbl[n].cut_toggle, XmNbackground, Base_bg, NULL );
          XtSetSensitive( Gen_tbl[n].cut_toggle, False );

          /* If the product is elevation based, then allow
             the cut field to be editable. */
          if( indx >= 0 )
          {
            XmTextSetString( Gen_tbl[n].cut, Buf );
            XtVaSetValues( Gen_tbl[n].cut,
                XmNbackground, Cut_color,
                XmNtraversalOn, Cut_mode,
                XmNeditable, Cut_mode,
                NULL );
          }
          else
          {
            /* Otherwise, the product is volume based so don't
               allow the cut field to be edited. */
            sprintf( Buf, " " );
            XmTextSetString( Gen_tbl[n].cut, Buf );
            XtVaSetValues( Gen_tbl[n].cut,
                XmNbackground, Base_bg,
                XmNtraversalOn, False,
                XmNeditable, False,
                NULL );
          }

          /* -----Product Description----- */
          string = (char *) ORPGPAT_get_description( buf_num, STRIP_MNEMONIC );
          strcpy( Buf, string );
          XmTextSetString( Gen_tbl[n].description, Buf );

          /* Display entry */
          XtManageChild( Gen_tbl[n].rowcol );
          Att_map[n] = Sort_index[i];
          Gen_map[n] = -1;
        }

        XtManageChild( Gen_tbl[n++].rowcol );
      }
    }
  }

  /* For each product in the generation table, set the color and
     editability based on the state of the lock.  If one of the
     default tables are active, only the ROC level user can edit
     them.  NOTE: The sensitivity of the table items remains the
     same for visibility reasons only. */

  Num_gen_table_items = n;

  for( i = 0; i < n; i++ )
  {
    XtVaSetValues( Gen_tbl[i].generation,
                XmNbackground, Cut_color,
                XmNeditable, Cut_mode,
                NULL );

    XtVaSetValues( Gen_tbl[i].retention,
                XmNbackground, Cut_color,
                XmNeditable, Cut_mode,
                NULL );

    if( !Cut_mode )
    {
      XtVaSetValues( Gen_tbl[i].cut_toggle,
                XmNbackground, Base_bg,
                XmNeditable, False,
                NULL );
    }
  }

  /* Unmanage table entries with no associated product table entry. */
  for( i = n; i < Num_table_items; i++ )
  {
    XtUnmanageChild( Gen_tbl[i].rowcol );
    Att_map[i] = -1;
    Gen_map[i] = -1;
  }

  /* Check the product attributes of the active product. If any
     parameters are defined, manage the form entry for those params. */
  buf_num = ORPGPAT_get_prod_id( Att_map[(int) row] );

  /* If buf_num < 0, there's no reason to continue. */
  if( buf_num < 0 ){ return; }

  ndx = ORPGPGT_buf_in_tbl( id, buf_num );
  mne = ORPGPAT_get_mnemonic( buf_num );
  Num_active_parameters = ORPGPAT_get_num_parameters( buf_num );


  /* Treat the CFC Product differently from the others since it
     actually consists of 4 pieces (two segments and 2 channels).
     If the user activates this product, then 4 entries are put
     in the product generation table; one for each piece.  We do
     not want to show the parameter to the user in this case. */
  if( !strncmp( mne, "CFC", 3 ) ){ Num_active_parameters = 0; }

  if( ORPGPAT_elevation_based( buf_num ) >= 0 )
  {
    elevation_based = HCI_YES_FLAG;
    if( Num_active_parameters == 1 ){ Num_active_parameters = 0; }
  }
  else
  {
    elevation_based = HCI_NO_FLAG;
  }

  for( i = Num_active_parameters+1; i <= ORPGPGT_MAX_PARAMETERS; i++ )
  {
    XtUnmanageChild( Parameter_widget[i].rowcol );
  }

  /* If the selected product has additional parameters, we want to
     display them below the table; excluding the elevation/cut
     parameter which is already defined in the table. */
  if( Num_active_parameters > 0 )
  {
    XtManageChild( Parameter_widget[0].rowcol );
    sprintf( Buf, "Use the below form to edit product parameters (if applicable)." );
    str = XmStringCreateLocalized( Buf );
    XtVaSetValues( Parameter_label, XmNlabelString, str, NULL );
    XmStringFree( str );
    XtManageChild( Parameter_frame );

    /* Go through each parameter and find the total number and
       exclude the elevation/cut parameter. */
    n = 0;
    for( i = 0; i < Num_active_parameters; i++ )
    {
      strcpy( Buf, ORPGPAT_get_parameter_name( buf_num, i ) );
      if( elevation_based == HCI_YES_FLAG && !strncmp( Buf, "Elevation", 9 ) )
      {
        continue;
      }
      n++;

      /* The parameter is not elevation/cut so manage the entry */
      XtManageChild( Parameter_widget[n].rowcol );

      /* Fill in the data for the parameter. */
      if( Buf != (char *) NULL )
      {
        XmTextSetString( Parameter_widget[n].description, Buf );
      }

      strcpy( Buf, ORPGPAT_get_parameter_units( buf_num, i ) );

      if( Buf != (char *) NULL )
      {
        XmTextSetString( Parameter_widget[n].units, Buf );
      }

      scale = ORPGPAT_get_parameter_scale( buf_num, i );
      min = ORPGPAT_get_parameter_min( buf_num, i );
      max = ORPGPAT_get_parameter_max( buf_num, i );
      param = ORPGPAT_get_parameter_index( buf_num, i );

      if( ndx >= 0 )
      {
        value = ORPGPGT_get_parameter( Which_table, ndx, param );

        if( ( ( !strncmp( mne, "SRR", 3 ) && ( value < 0 ) ) ||
              ( !strncmp( mne, "SRM", 3 ) && ( value < 0 ) ) ) &&
            ( ( param == 3 ) || ( param == 4 ) ) )
        {
          value = -10;
        }
      }
      else
      {
        value = PARAM_UNUSED;
      }

      Parameter_widget[n].index = i;

      /* Desensitize the value widget while we update it so the
         validation callbacks aren't called. */
      XtSetSensitive( Parameter_widget[n].value, False );

      /* Figure out what kind of scaling needs to be done to
         the parameter.  Make sure to check for the special
         types so that certain values are treated differently. */
      switch( scale )
      {
        case 1 :
          /* Integer */
          sprintf( Buf, "%d", (int) min );
          XmTextSetString( Parameter_widget[n].minimum, Buf );
          sprintf( Buf, "%d", (int) max );
          XmTextSetString( Parameter_widget[n].maximum, Buf );

          if( ( ndx >= 0 ) && ( value >= min ) && ( value <= max ) )
          {
            sprintf( Buf, "%6d", (int) value );
          }
          else
          {
            sprintf( Buf, "      " );
          }

          XmTextSetString( Parameter_widget[n].value, Buf );
          break;

        case 10 :
          /* Float .x */
          sprintf( Buf, "%6.1f", (float) ( min/(float) scale ) );
          XmTextSetString( Parameter_widget[n].minimum, Buf );
          sprintf( Buf, "%6.1f", (float) ( max/(float) scale ) );
          XmTextSetString( Parameter_widget[n].maximum, Buf );

          if( ( ndx >= 0 ) && ( value >= min ) && ( value <= max ) )
          {
            sprintf( Buf, "%6.1f", (float) ( value/(float) scale ) );
          }
          else
          {
            sprintf( Buf, "      " );
          }

          XmTextSetString( Parameter_widget[n].value, Buf );
          break;

        case 100 :
          /* Float .xx */
          sprintf( Buf, "%6.2f", (float) ( min/(float) scale ) );
          XmTextSetString( Parameter_widget[n].minimum, Buf );
          sprintf( Buf, "%6.2f", (float) ( max/(float) scale ) );
          XmTextSetString( Parameter_widget[n].maximum, Buf );

          if( ( ndx >= 0 ) && ( value >= min ) && ( value <= max ) )
          {
            sprintf( Buf, "%6.2f", (float) ( value/(float) scale ) );
          }
          else
          {
            sprintf( Buf, "      " );
          }

          XmTextSetString( Parameter_widget[n].value, Buf );
          break;

        case 1000 :
          /* Float .xxx */

        default :

          sprintf( Buf, "%6.3f", (float) ( min/(float) scale ) );
          XmTextSetString( Parameter_widget[n].minimum, Buf );
          sprintf( Buf, "%6.3f", (float) ( max/(float) scale ) );
          XmTextSetString( Parameter_widget[n].maximum, Buf );

          if( ( ndx >= 0 ) && ( value >= min ) && ( value <= max ) )
          {
            sprintf( Buf, "%6.3f", (float) ( value/(float) scale ) );
          }
          else
          {
            sprintf( Buf, "      " );
          }

          XmTextSetString( Parameter_widget[n].value, Buf );
          break;
      }

      XtVaSetValues( Parameter_widget[n].value,
                XmNbackground, Cut_color,
                XmNeditable, Cut_mode,
                XmNsensitive, True,
                NULL );

    }

    Num_active_parameters = Num_active_parameters - elevation_based;

  }
  else
  {
    sprintf( Buf, "This product has no extra parameters" );
    str = XmStringCreateLocalized( Buf );
    XtVaSetValues( Parameter_label, XmNlabelString, str, NULL );
    XmStringFree( str );
    XtUnmanageChild( Parameter_frame );
  }

  /*  Highligh the active table entry. */
  XtVaSetValues( Active_row, XmNbackground, White_color, NULL );

  /* Refresh the table. */
  XtManageChild( Data_form );
}

/************************************************************************
     Description: This function is called when the input focus is
      changed to one of the editable widgets in the
      displayed product generation list.  The ID of the
      parent (rowcol) widget is saved and its border
      color changed to white (to indicate it is the
      active row).  Any parameters associated with the
      new product is displayed at the bottom of the window.
 ************************************************************************/

static void Gen_table_gain_focus( Widget w, XtPointer y, XtPointer z )
{
  XtPointer data = (XtPointer) NULL;
  XmString str;
  int buf_num = 0;
  char *mne = NULL;
  int parameters = 0;
  int elevation_based = HCI_NO_FLAG;
  int ndx = 0;
  int i = 0;
  int n = 0;
  int min = 0;
  int max = 0;
  int scale = 0;
  int param = 0;
  int value = 0;
  Dimension row_height;
  char *text = NULL;
  static Widget widget = (Widget) NULL;

  /* If this is a new widget then we want to keep its gain focus
     value for when we restore it. */
  if( w != widget )
  {
    text = XmTextGetString( w );
    strcpy( Old_text, text );
    XtFree( text );

    XtVaSetValues( w, XmNverifyBell, True, NULL );

    if( widget != (Widget) NULL )
    {
      XtVaSetValues( widget, XmNverifyBell, False, NULL );
    }
  }

  /* If we are still in the same row, then do nothing. */
  if( Active_row == XtParent( w ) )
  {
    return;
  }

  /* Clear out any parameter fields from previously selected product */
  for( i = 1; i < Num_active_parameters+1; i++ )
  {
    Parameter_modify_flag = HCI_YES_FLAG;
    XmTextSetString( Parameter_widget[i].value, "      " );
    Parameter_modify_flag = HCI_NO_FLAG;
  }

  /* Unhighlight the previously active row. */
  XtVaSetValues( Active_row,
                XmNbackground, Base_bg,
                XmNborderColor, Base_bg,
                XmNhighlightColor, Base_bg,
                XmNtopShadowColor, Base_bg,
                XmNbottomShadowColor, Base_bg,
                NULL );

  XtVaGetValues( Active_row, XmNuserData, &data, NULL );

  XtVaSetValues( Gen_tbl[(int) data].mnemonic,
                XmNhighlightColor, Base_bg,
                XmNborderColor, Base_bg,
                NULL );

  XtVaSetValues( Gen_tbl[(int) data].pcode,
                XmNhighlightColor, Base_bg,
                XmNborderColor, Base_bg,
                NULL );

  XtVaSetValues( Gen_tbl[(int) data].generation,
                XmNhighlightColor, Base_bg,
                XmNborderColor, Base_bg,
                NULL );

  XtVaSetValues( Gen_tbl[(int) data].retention,
                XmNhighlightColor, Base_bg,
                XmNborderColor, Base_bg,
                NULL );

  XtVaSetValues( Gen_tbl[(int) data].cut,
                XmNhighlightColor, Base_bg,
                XmNborderColor, Base_bg,
                NULL );

  XtVaSetValues( Gen_tbl[(int) data].description,
                XmNhighlightColor, Base_bg,
                XmNborderColor, Base_bg,
                NULL );

  /* Get the ID of the newly selected row. */
  Active_row = XtParent( w );

  /* Highlight the newly selected row. */
  XtVaGetValues( Active_row, XmNuserData, &data, NULL );

  XtVaSetValues( Active_row,
                XmNbackground, White_color,
                XmNborderColor, White_color,
                XmNhighlightColor, White_color,
                XmNbottomShadowColor, White_color,
                XmNtopShadowColor, White_color,
                NULL );

  /* Get the product ID of the product corresponding to the newly
     selected row. */

  buf_num = ORPGPAT_get_prod_id( Att_map[(int) data] );

  ndx = ORPGPGT_buf_in_tbl( Which_table, buf_num );

  /* The next section determines whether any parameters (other than
     elevation) exist for the product.  If they do, then the
     parameter form at the botom of the window needs to be built and
     made visible. */
 
  parameters = ORPGPAT_get_num_parameters( buf_num );

  if( ORPGPAT_elevation_based( buf_num ) >= 0 )
  {
    /* if the product is elevation based and there is only one
       parameter, then assume that one parameter is elevation and
       no parameter form is required. */

    if( parameters == 1 )
    {
      parameters = 0;
      elevation_based = HCI_NO_FLAG;
    }
    else if( parameters > 1 )
    {
      /* if the product is elevation based and more than one
         parameter is defined for that product, then a parameter
         form is required for all parameters other than elevation. */
      elevation_based = HCI_YES_FLAG;
    }
    else
    {
      /* this condition shouldn't happen since all elevation based
      products should have at last one parameter (elevation). */
      elevation_based = HCI_NO_FLAG;
    }
  }
  else
  {
    /* The product is not elevation based so we do not have to worry
       about excluding the elevation parameter from the parameter
       form (if any parameters exist). */
    elevation_based = HCI_NO_FLAG;
  }

  /* Check the product attributes of the new product. If doess not
     have any selectable parameters, then unmanage all entries for
     the old active product if applicable. */

  /* Treat the CFC Product differently from the others since it
     actually consists of 4 pieces (two segments and 2 channels).
     If the user activates this product, then 4 entries are put
     in the product generation table; one for each piece.  We do
     not want to show the parameter to the user in this case. */

  mne = ORPGPAT_get_mnemonic( buf_num );

  if( !strncmp( mne, "CFC", 3 ) )
  {
    parameters = 0;
  }

  if( parameters > 0 )
  {
    /* if the old product had more parameters, unmanage the excess */
    if( Num_active_parameters > 0 )
    {
      for( i = parameters-elevation_based+1; i < Num_active_parameters+1; i++ )
      {
        XtUnmanageChild( Parameter_widget[i].rowcol );
      }
    }
    else if( Num_active_parameters == 0 )
    {
      /* else if the old product did not have any, display the
         top form label for the new product. */
      XtManageChild( Parameter_widget[0].rowcol );
      sprintf( Buf, "Use the below form to edit product parameters (if applicable)." );
      str = XmStringCreateLocalized( Buf );
      XtVaSetValues( Parameter_label, XmNlabelString, str, NULL );
      XmStringFree( str );
      XtManageChild( Parameter_frame );
    }

    /* Go through each parameter and find the total number and
       exclude the elevation/cut parameter. */
    n = 0;
    for( i = 0; i < parameters; i++ )
    {
      strcpy( Buf, ORPGPAT_get_parameter_name( buf_num, i ) );

      if( elevation_based == HCI_YES_FLAG && !strncmp( Buf, "Elevation", 9 ) )
      {
        continue;
      }

      /* Fill in the data for the parameter. */
      n++;
      XtVaSetValues( Parameter_widget[n].rowcol, XmNuserData, (XtPointer) i, NULL );
      if( Buf != (char *) NULL )
      {
        XmTextSetString( Parameter_widget[n].description, Buf );
      }
      strcpy( Buf, ORPGPAT_get_parameter_units( buf_num, i ) );
      if( Buf != (char *) NULL )
      {
        XmTextSetString( Parameter_widget[n].units, Buf );
      }

      scale = ORPGPAT_get_parameter_scale( buf_num, i );
      min = ORPGPAT_get_parameter_min( buf_num, i );
      max = ORPGPAT_get_parameter_max( buf_num, i );
      param = ORPGPAT_get_parameter_index( buf_num, i );
      value = ORPGPGT_get_parameter( Which_table, Gen_map[(int) data], param );

      if( ( ( !strncmp( mne, "SRR", 3 ) && ( value < 0 ) ) ||
            ( !strncmp( mne, "SRM", 3 ) && ( value < 0 ) ) ) &&
          ( ( param == 3 ) || ( param == 4 ) ) )
      {
        value = -10;
      }

      Parameter_widget[n].index = i;

      /* Since we store the data as integers, a scale factor is
         also defined for each parameter for handling non-integer
         data.  Show the min and max allowed values with the
         proper precision using the scale factor to define the
         precision.  If a scale factor of 1 is defined, then the
         parameter is an integer, otherwise it is a scaled real number. */

      switch( scale )
      {
        case 1 :
          /* Integer */
          sprintf( Buf, "%d", (int) min );
          XmTextSetString( Parameter_widget[n].minimum, Buf );
          sprintf( Buf, "%d", (int) max );
          XmTextSetString( Parameter_widget[n].maximum, Buf );

          if( ( ndx >= 0 ) && ( value >= min ) && ( value <= max ) )
          {
            sprintf( Buf, "%d", (int) value );
          }
          else
          {
            sprintf( Buf, " " );
          }

          XmTextSetString( Parameter_widget[n].value, Buf );
          break;

        case 10 :
          /* Float with 1 digit to right of DP */
          sprintf( Buf, "%7.1f", (float) ( min/(float) scale ) );
          XmTextSetString( Parameter_widget[n].minimum, Buf );
          sprintf( Buf, "%7.1f", (float) ( max/(float) scale ) );
          XmTextSetString( Parameter_widget[n].maximum, Buf );

          if( ( ndx >= 0 ) && ( value >= min ) && ( value <= max ) )
          {
            sprintf( Buf, "%7.1f", (float) ( value/(float) scale ) );
          }
          else
          {
            sprintf( Buf, " " );
          }

          XmTextSetString( Parameter_widget[n].value, Buf );
          break;

        case 100 :
          /* Float with 2 digits to right of DP */
          sprintf( Buf, "%7.2f", (float) ( min/(float) scale ) );
          XmTextSetString( Parameter_widget[n].minimum, Buf );
          sprintf( Buf, "%7.2f", (float) ( max/(float) scale ) );
          XmTextSetString( Parameter_widget[n].maximum, Buf );

          if( ( ndx >= 0 ) && ( value >= min ) && ( value <= max ) )
          {
            sprintf( Buf, "%7.2f", (float) ( value/(float) scale ) );
          } 
          else
          {
            sprintf( Buf, " " );
          }

          XmTextSetString( Parameter_widget[n].value, Buf );
          break;

        case 1000 :
          /* Float with 3 digits to right of DP */

        default :

          sprintf( Buf, "%7.3f", (float) ( min/(float) scale ) );
          XmTextSetString( Parameter_widget[n].minimum, Buf );
          sprintf( Buf, "%7.3f", (float) ( max/(float) scale ) );
          XmTextSetString( Parameter_widget[n].maximum, Buf );

          if( ( ndx >= 0 ) && ( value >= min ) && ( value <= max ) )
          {
            sprintf( Buf, "%7.3f", (float) ( value/(float) scale ) );
          }
          else
          {
            sprintf( Buf, " " );
          }

          XmTextSetString( Parameter_widget[n].value, Buf );
          break;
      }

      XtVaSetValues( Parameter_widget[n].value,
                XmNbackground, Cut_color,
                XmNeditable, Cut_mode,
                XmNuserData, (XtPointer) n,
                NULL );

      XtManageChild( Parameter_widget[n].rowcol );
    }

    XtVaGetValues( Parameter_widget[0].rowcol, XmNheight, &row_height, NULL );
  }
  else if( Num_active_parameters > 0 )
  {
    XtVaGetValues( Parameter_widget[0].rowcol, XmNheight, &row_height, NULL );

    for( i = 0; i < Num_active_parameters+1; i++ )
    {
      XtUnmanageChild( Parameter_widget[i].rowcol );
    }

    sprintf( Buf, "This product has no extra parameters ");
    str = XmStringCreateLocalized( Buf );
    XtVaSetValues( Parameter_label, XmNlabelString, str, NULL );
    XmStringFree( str );

    XtUnmanageChild( Parameter_frame );
  }

  Num_active_parameters = parameters - elevation_based;

  XtManageChild( Bottom_form );
}

/************************************************************************
     Description: This function is called when the value of one of
                  the text widgets in the product generation list
                  is modified or when it loses focus.
 ************************************************************************/

static void Gen_table_modify( Widget w, XtPointer y, XtPointer z )
{
  XtPointer data = (XtPointer) NULL;
  int row = 0;
  float value = 0.0;
  int ivalue = 0;
  float old_value = 0.0;
  int status = 0;
  int generation = 0;
  int buf_num = 0;
  int scale = 0;
  int pindx = 0;
  int eindex = 0;
  int i = 0;
  int min = 0;
  int max = 0;
  int ival = 0;
  int err_flag = HCI_NO_FLAG;
  char *text = NULL;
  float old_elevation = 0.0;
  int old_cut = 0;
  int flags = 0;
  char *mne = NULL;
  char string[32];
  char buf[32];

  /* Don't allow this routine to be re-entered if active. */

  if( Modify_flag == HCI_YES_FLAG ){ return; }

  Modify_flag = HCI_YES_FLAG;
  err_flag = HCI_NO_FLAG;

  /* We get the active row number from the parent (in this case
     the rowcolumn widget).  We want to highlight the row so we
     do this by changing the parent foreground color. */

  XtVaGetValues( XtParent( w ), XmNuserData, &data, NULL );

  row = (int) data;

  text = XmTextGetString( w );

  if( text == (char *) NULL )
  {
    Modify_flag = HCI_NO_FLAG;
    return;
  }
  else if( !strlen( text ) || !hci_number_found( text ) )
  {
    XtFree( text );
    XmTextSetString( w, Old_text );
    Modify_flag = HCI_NO_FLAG;
    return;
  }

  /* Extract the data value from the text widget string. */

  sscanf( text, "%f", &value );
  sscanf( text, "%d", &ivalue );
  strcpy( string, text );
  XtFree( text );

  /* Next, get the ID and mnemonic for the product associated with
     this table row. */

  buf_num = ORPGPAT_get_prod_id( Att_map[row] );
  mne = ORPGPAT_get_mnemonic( buf_num );

  /* Check to see which table item was changed. */

  switch( (int) y )
  {
    case GENERATION :

      /* Generation period udated */
      old_value = ORPGPGT_get_generation_interval( Which_table, Gen_map[row] );
      /* Don't do anything if the value hasn't changed. */
      if( ( old_value == value ) ||
          ( ( (int) old_value == ORPGPGT_ERROR ) && ( (int) value == 0 ) ) )
      {
        Modify_flag = HCI_NO_FLAG;
        return;
      }

      /* If the generation interval is 0, then remove the
         product from the generation table.  If the product
         is elevation based, all following cuts are removed. */

      if( (int) value == 0 )
      {
        if( Gen_map[row] >= 0 )
        {
          /* If the Clutter Filter Control product is being
             deleted, then remove all entries (only one
             entry is shown in the table but 4 exist
             internally). */

          if( !strncmp( mne, "CFC", 3 ) )
          {
            for( i = ORPGPGT_get_tbl_num( Which_table )-1; i >= Gen_map[row]; i-- )
            {
              if( buf_num == ORPGPGT_get_prod_id( Which_table, i ) )
              {
                ORPGPGT_delete_entry( Which_table, i );
              }
            }
          }
          else
          {
            ORPGPGT_delete_entry( Which_table, Gen_map[row] );
          }

          Gen_map[row] = -1;
          Show_generation_table( Which_table );
        }
      }
      else
      {
        /* The entry is not being deleted so we must either modify
           the existing entry or add a new one if it doesnt exist. */
        if( Gen_map[row] >= 0 )
        {
          status = ORPGPGT_set_generation_interval( Which_table, Gen_map[row], (int) value );

          /* If an error occurred, inform the user and
             reset the field to it's prior state. */
          if( status == ORPGPGT_INVALID_DATA )
          {
            sprintf( Buf, "An invalid generation interval of %d was\nentered.  The valid range is %d to %d.", (int) value, ORPGPGT_MIN_GENERATION_INTERVAL, ORPGPGT_MAX_GENERATION_INTERVAL );
            hci_warning_popup( Top_widget, Buf, NULL );
            err_flag = HCI_YES_FLAG;
            XmTextSetString( w, Old_text );
            break;
          }
        }
        else
        {
          /* Lets create a new entry to the product generation table */
          if( ( (int) value < ORPGPGT_MIN_GENERATION_INTERVAL ) ||
              ( (int) value > ORPGPGT_MAX_GENERATION_INTERVAL ) )
          {
            sprintf( Buf, "An invalid generation interval of %d was\nentered.  The valid range is %d to %d.", (int) value, ORPGPGT_MIN_GENERATION_INTERVAL, ORPGPGT_MAX_GENERATION_INTERVAL );
            hci_warning_popup( Top_widget, Buf, NULL );
            XmTextSetString( w, Old_text );
            err_flag = HCI_YES_FLAG;
            break;
          }
          else
          {
            Hci_create_new_table_entry( Which_table, row, buf_num,
                                        (int) value, 30, 0, -1 );

          }
          Show_generation_table( Which_table );
        }
      }
      break;

    case RETENTION :

      value = (int) ( value + 0.001 );
      if( ( Gen_map[row] < 0 ) && ( value == 0 ) )
      {
        Modify_flag = HCI_NO_FLAG;
        return;
      }

      old_value = ORPGPGT_get_retention_period( Which_table, Gen_map[row] );

      /* Don't do anything if the value hasn't changed. */
      if( old_value == value )
      {
        Modify_flag = HCI_NO_FLAG;
        return;
      }
      else if( (int) old_value == ORPGPGT_ERROR )
      {
        sprintf( Buf, "The storage interval must be non-zero\nbefore you can define a retention time." );
        hci_warning_popup( Top_widget, Buf, NULL );
        XmTextSetString( w, Old_text );
        Modify_flag = HCI_NO_FLAG;
        XtManageChild( XtParent( w ) );
        return;
      } 

      if( Gen_map[row] >= 0 )
      {
        /* If the CFC product is selected, then set the
           retention period for all instances of CFC. */
        if( !strncmp( mne, "CFC", 3 ) )
        {
          for( i = Gen_map[row]; i < ORPGPGT_get_tbl_num( Which_table ); i++ )
          {
            if( buf_num == ORPGPGT_get_prod_id( Which_table, i ) )
            {
              status = ORPGPGT_set_retention_period( Which_table, i, (int) value );

              if( status == ORPGPGT_INVALID_DATA )
              {
                sprintf( Buf, "An invalid retention period of %d was\nentered.  The valid range is 30 to 360\nwith the storage interval > 0.", (int) value );

                if( i == Gen_map[row] )
                {
                  hci_warning_popup( Top_widget, Buf, NULL );
                }
                XmTextSetString( w, Old_text );
                err_flag = HCI_YES_FLAG;
                break;
              }
            }
          }
        }
        else
        {
          status = ORPGPGT_set_retention_period( Which_table, Gen_map[row], (int) value );

          if( status == ORPGPGT_INVALID_DATA )
          {
            sprintf( Buf, "An invalid retention period of %d was\nentered.  The valid range is 30 to 360\nwith the storage interval > 0.", (int) value );
            hci_warning_popup( Top_widget, Buf, NULL );
            err_flag = HCI_YES_FLAG;
            XmTextSetString( w, Old_text );
            break;
          }

          sprintf( buf, "%3d", (int) value );
          XmTextSetString( w, buf );
          strcpy( Old_text, buf );
        }
      }
      break;

    case CUT :
      /* if the value is negative, then it is interpreted as a slice,
         otherwise it is interpreted as an elevation angle. The CUT
         item should only be valid for elevation based products. Get
         the parameter index for the elevation angle. If it is not
         defined, then log an error and return */
      if( ( eindex = ORPGPAT_elevation_based( buf_num ) ) < 0 )
      {
        HCI_LE_error( "expected elevation index not found for product %d", buf_num );
        Modify_flag = HCI_NO_FLAG;
        return;
      }

      pindx = -1;

      /* Get the index of the cut field in the attributes table. */
      for( i = 0; i < ORPGPAT_get_num_parameters( buf_num ); i++ )
      {
        if( eindex == ORPGPAT_get_parameter_index( buf_num, i ) )
        {
          pindx = i;
          break;
        }
      }

      if( pindx == -1 ){ break; }

      scale = ORPGPAT_get_parameter_scale( buf_num, pindx );
      old_cut = ORPGPGT_get_parameter( Which_table, Gen_map[row], eindex );

      /* We first must mask off bits 13-15 since they are
         used for special conditions (i.e., denote range of
         cuts or elevations). */
      flags = old_cut & 0x6000;
      old_cut = old_cut & 0x1fff;

      /* Let's see if the value is within the allowed range
         for slice/elevation using information from the PAT. */
      min = ORPGPAT_get_parameter_min( buf_num, pindx );
      max = ORPGPAT_get_parameter_max( buf_num, pindx );

      text = XmTextGetString( Gen_tbl[row].generation );
      sscanf( text, "%d", &generation );
      XtFree( text );

      /* If the text string contains a "-" character and no "."
         character then it is considered a slice.  If not, then
         it is considered an elevation angle.  The code allows
         for negative elevation angles (although none are defined
         in any current VCP) by internally storing them as
         (360-angle)*scale. */

      if( strstr( string, "." ) || ( value > 0.0 ) )
      {
        /* angle */
        if( ( value < -1.0 ) || ( value > 45.0 ) )
        {
          sprintf( Buf, "An invalid angle of %4.1f was entered.\nThe valid range is -1.0 to 45.0.", value );
          hci_warning_popup( Top_widget, Buf, NULL );
          XmTextSetString( w, Old_text );
          Modify_flag = HCI_NO_FLAG;
          return;
        }

        /* If the angle is negative make it positive by
           adding 360.0 so we can store the angle as a
           positive number. */
        if( value < 0 ){ value += 360.0; }
      }
      else
      {
        /* slice */
        if( ( ivalue == 0 ) && ( generation == 0 ) )
        {
          XmTextSetString( w, Old_text );
          XtManageChild( XtParent( w ) );
          Modify_flag = HCI_NO_FLAG;
          return;
        }
        else if( ivalue < min )
        {
          sprintf( Buf, "An invalid cut of %d was entered.\nThe valid range is %d to -1.", ivalue, min );
          hci_warning_popup( Top_widget, Buf, NULL );
          XmTextSetString( w, Old_text );
          XtManageChild( XtParent( w ) );
          Modify_flag = HCI_NO_FLAG;
          return;
        } 
      }

      /* if the cut is >= 0, then it is an elevation angle
         and we need to unscale it. */
      if( flags != ORPGPRQ_LOWER_CUTS )
      {
        if( flags == ORPGPRQ_ALL_ELEVATIONS ){ old_elevation = 45.0; }
        else{ old_elevation = old_cut/(float) scale; }

        /* Don't do anything if the value hasn't changed. This
           is needed so that focus events don't cause this code
           to be executed when not necessary. */
        if( old_elevation == value )
        {
          Modify_flag = HCI_NO_FLAG;
          return;
        }
      }

      /* If this product isn't currently in the generation list
         we need to add it. */
      if( Gen_map[row] < 0 )
      {
        /* Lets create a new entry to the product generation table */
        if( strstr( string, "." ) || ( value >= 0.0 ) )
        {
          /* angle */
          status = Hci_create_new_table_entry( Which_table, row, buf_num,
                                               1, 30, value, 0 );
        }
        else
        {
          status = Hci_create_new_table_entry( Which_table, row, buf_num,
                                               1, 30, 0.0, ivalue );
        }

        Show_generation_table( Which_table );
      }
      else
      {
        if( value >= 0 )
        {
          /* We need to check the current state of the
             special elevation/cut bits.  If all lower
             elevations are currently flagged or if all
             elevations are flagged, set the state to all
             lower elevations. */
          if( ( flags == ORPGPRQ_ALL_ELEVATIONS ) ||
              ( flags == ORPGPRQ_LOWER_ELEVATIONS ) )
          {
            flags = ORPGPRQ_LOWER_ELEVATIONS;
            XmToggleButtonSetState( Gen_tbl[row].cut_toggle, True, False );
          }
          else
          {
            /* Else, set the state to a single elevation. */
            XmToggleButtonSetState( Gen_tbl[row].cut_toggle, False, False );
            flags = 0;
          }

          ival = ( (int) ( value * scale ) ) | flags;
          ORPGPGT_set_parameter( Which_table, Gen_map[row], eindex, ival );
          if( value > 45.0 ){ value -= 360.0; }

          sprintf( buf, "%5.1f", value );
          XtVaSetValues( Gen_tbl[row].cut_toggle,
          XmNbackground, Button_bg,
          XmNtraversalOn, True,
          XmNsensitive, True,
          NULL );
        }
        else
        {
          /* We can no longer store cuts as a negative
             value so we need to negate it first. */
          sprintf( buf, "%5d", ivalue );
          ivalue = -ivalue;
          ivalue = ivalue | ORPGPRQ_LOWER_CUTS;
          ORPGPGT_set_parameter( Which_table, Gen_map[row], eindex, ivalue );
          XtVaSetValues( Gen_tbl[row].cut_toggle,
                XmNbackground, Base_bg,
                XmNtraversalOn, False,
                XmNsensitive, False,
                NULL );
          XmToggleButtonSetState( Gen_tbl[row].cut_toggle, False, False );
        }

        XmTextSetString( w, buf );
        strcpy( Old_text, buf );
        XtManageChild( XtParent( w ) );
      }
      break;

    default :

      break;
  }

  /* The value is different so no determine which table is being edited
     If the Save and Undo buttons are not sensitive, then make them
     so and set the appropriate change flag */
  if( err_flag == HCI_NO_FLAG ){ Set_save_undo_flag(); }

  XtManageChild( XtParent( w ) );
  Modify_flag = HCI_NO_FLAG;
}

/************************************************************************
     Description: This function is the callback for the Replace button.
                  The user is prompted as to whether they want to
                  replace the current list with the specified default
                  list.
 ************************************************************************/

static void Replace_current_callback( Widget w, XtPointer y, XtPointer z )
{
  char buf[HCI_BUF_128];
  XmToggleButtonCallbackStruct *state;

  state = (XmToggleButtonCallbackStruct *) z;

  /* Only do this if the button is set. */

  if( state->set )
  {
    HCI_LE_log( "Product Generation Table Edit Replace selected" );

    XtVaSetValues( w, XmNset, False, NULL );

    User_selected_table_to_replace = (int) y;

    /* Use client data to determine which default list to copy to
       the current list.  Only build a message that we can use for
       the verification popup. */

    switch( User_selected_table_to_replace )
    {
      case DEFAULT_A_TABLE :

        sprintf( buf, "You are about to replace the contents of the\nCurrent List with the Default Precip (A) List.\nDo you want to continue?" );
        break;

      case DEFAULT_B_TABLE :

        sprintf( buf, "You are about to replace the contents of the\nCurrent List with the Default Clear Air (B) List.\nDo you want to continue?" );
        break;

      default :

        return;
    }

    /* Display a verification popup first. */
    hci_confirm_popup( Top_widget, buf, Accept_replace_current_callback, Cancel_replace_current_callback );
  }
}

/************************************************************************
     Description: This function is the callback for the "No" button
                  in the replace current table verification popup.
 ************************************************************************/

static void Cancel_replace_current_callback( Widget w, XtPointer y, XtPointer z )
{
  /* Only log a message. */
  HCI_LE_log( "Product Generation Table Edit Replace cancelled" );
}

/************************************************************************
 *      Description: This function is the callback for the "Yes" button *
 *                   in the replace current table verification popup.   *
 ************************************************************************/

static void Accept_replace_current_callback( Widget w, XtPointer y, XtPointer z )
{
  Replace_flag = HCI_YES_FLAG;
}

static void Replace_current_table()
{
  int status = 0;

  /* If low bandwidth, display the progress meter since we are going
     to do I/O. */

  HCI_PM( "Replacing current product generation table" );

  /* Log a message in the hci log file. */

  HCI_LE_log( "Product Generation Table Edit Replace accepted" );

  /* Use client data to determine which default table to copy. */

  switch( User_selected_table_to_replace )
  {
    case DEFAULT_A_TABLE :

      /* Copy the Default A message to the Current message */

      status = ORPGPGT_copy_tbl( DEFAULT_A_TABLE, CURRENT_TABLE );

      /* Set the change flag for the Current list and sensitize
         the Save and Undo buttons. */

      if( status == 0 )
      {
        Current_flag = HCI_CHANGED_FLAG;
        XtSetSensitive( Save_button, True );
        XtSetSensitive( Undo_button, True );
      }
      break;

    case DEFAULT_B_TABLE :

      /* Copy the Default B message to the Current message */

      status = ORPGPGT_copy_tbl( DEFAULT_B_TABLE, CURRENT_TABLE );

      /* Set the change flag for the Current list and sensitize
         the Save and Undo buttons. */

      if( status == 0 )
      {
        Current_flag = HCI_CHANGED_FLAG;
        XtSetSensitive( Save_button, True );
        XtSetSensitive( Undo_button, True );
      }
      break;

    default :

      status = -1;
      break;
  }

  /* Refresh the display table. */
  if( status == 0 ){ Show_generation_table( Which_table ); }
}

/************************************************************************
     Description: This function is invoked when the user enters a
                  password or changes a LOCA radio button in the
                  password window.
 ************************************************************************/

static int Hci_product_generation_security()
{
  Pixel bg_color = (Pixel) NULL;
  Pixel fg_color = (Pixel) NULL;
  int i = 0;
  XmString str;

  if( hci_lock_open() )
  {
    /* Do nothing when user opens password dialog. */
  }
  else if( hci_lock_loca_selected() )
  {
    /* If the ROC LOCA button is selected, then we want
       to make the Product ID label visible. */

    if( hci_lock_ROC_selected() )
    {
      Show_all_products = HCI_YES_FLAG;
      XtManageChild( Pid_text );
    }
    else
    {
      Show_all_products = HCI_NO_FLAG;
      XtUnmanageChild( Pid_text );
    }
  }
  else if( hci_lock_loca_unlocked() )
  {
    if( hci_lock_ROC_unlocked() )
    {
      /* If the ROC level password entered, then we want to show
         all products and change the MNE button to Product ID. */

      Unlocked_roc = HCI_YES_FLAG;
      Show_all_products = HCI_YES_FLAG;
      str = XmStringCreateLocalized( "Product ID" );
      XtVaSetValues( Sort_pid_button, XmNlabelString, str, NULL );
      XmStringFree( str );
      XtManageChild( Pid_text );

      /* Set the Edit flags. */

      Cut_color = Edit_bg;
      Cut_mode = True;

      /* If the table is currently being edited by another user
         inform the user. */

      if( ORPGEDLOCK_get_edit_status( ORPGDAT_PROD_INFO, Which_table) == ORPGEDLOCK_EDIT_LOCKED )
      {
        sprintf( Buf, "Another user is currently editing the selected\ndefault product generation table.  Any changes\nyou make may be overwritten by the other user.");
        hci_warning_popup( Top_widget, Buf, NULL );
      }

      ORPGEDLOCK_set_edit_lock( ORPGDAT_PROD_INFO, Which_table );
    }
    else
    {
      /* The lock state has been changed to locked so we want to
         clear the edit flags. */

      Show_all_products = HCI_NO_FLAG;
      ORPGEDLOCK_clear_edit_lock( ORPGDAT_PROD_INFO, Which_table );
      str = XmStringCreateLocalized( "Product MNE" );
      XtVaSetValues( Sort_pid_button, XmNlabelString, str, NULL );
      XmStringFree( str );
      XtUnmanageChild( Pid_text );

      if( Which_table != CURRENT_TABLE )
      {
        Cut_color = Base_bg;
        Cut_mode = False;
      }
      else
      {
        /* If the table is currently being edited by another
           user inform the user. */

        if( ORPGEDLOCK_get_edit_status( ORPGDAT_PROD_INFO, Which_table ) == ORPGEDLOCK_EDIT_LOCKED )
        {
          sprintf( Buf,"Another user is currently editing the selected\ndefault product generation table.  Any changes\nyou make may be overwritten by the other user.");
          hci_warning_popup( Top_widget, Buf, NULL );
        }

        ORPGEDLOCK_set_edit_lock( ORPGDAT_PROD_INFO, Which_table );
      }
    }
  }
  else if( hci_lock_close() && Unlocked_roc == HCI_YES_FLAG )
  {
    Unlocked_roc = HCI_NO_FLAG;
    Show_all_products = HCI_NO_FLAG;
    ORPGEDLOCK_clear_edit_lock( ORPGDAT_PROD_INFO, Which_table );
    str = XmStringCreateLocalized( "Product MNE" );
    XtVaSetValues( Sort_pid_button, XmNlabelString, str, NULL );
    XmStringFree( str );
    XtUnmanageChild( Pid_text );

    if( Which_table != CURRENT_TABLE )
    {
      Cut_color = Base_bg;
      Cut_mode = False;
    }

    Save_callback( Top_widget, NULL, NULL );
    Sort_products( Sort_method );
  }

  /* Redisplay the product generation list based on the new edit mode. */

  Show_generation_table( Which_table );

  for( i = 0; i < Num_gen_table_items; i++ )
  {
    XtVaGetValues( Gen_tbl[i].generation, XmNbackground, &bg_color, NULL );

    if( bg_color == Base_bg && hci_lock_ROC_selected() )
    {
      fg_color = Loca_fg;
    }
    else
    {
      fg_color = Edit_fg;
    }

    XtVaSetValues( Gen_tbl[i].generation,
                XmNforeground, fg_color,
                XmNeditable, Cut_mode,
                NULL );

    XtVaSetValues( Gen_tbl[i].retention,
                XmNforeground, fg_color,
                XmNeditable, Cut_mode,
                NULL );

    XtVaGetValues( Gen_tbl[i].cut,
                XmNbackground, &bg_color,
                NULL );

    XtVaSetValues( Gen_tbl[i].cut,
                XmNforeground, fg_color,
                XmNeditable, Cut_mode,
                NULL );
  }

  return HCI_LOCK_PROCEED;
}

/************************************************************************
     Description: This function handles gain focus events from text
                  widgets in the product parameter list.
 ************************************************************************/

static void Parameter_table_gain_focus( Widget w, XtPointer y, XtPointer z )
{
}

/************************************************************************
     Description: This function is invoked when an item in the
                  parameter list is modified or loses focus.
 ************************************************************************/

static void Parameter_table_modify( Widget w, XtPointer y, XtPointer z )
{
  char *text = NULL;
  XtPointer row = (XtPointer) NULL;
  XtPointer indx = (XtPointer) NULL;
  float value = 0.0;
  int i = 0;
  int ivalue = 0;
  int scale = 0;
  float min, max = 0.0;
  float old_value = 0.0;
  int buf_num = 0;
  int status = 0;
  int digit = HCI_NO_FLAG;

  /* If this function is being called before a previous call has
     completed, do nothing and return. */

  if( Parameter_modify_flag == HCI_YES_FLAG ){ return; }

  Parameter_modify_flag = HCI_YES_FLAG;

  /* Get the row of the active product. */

  XtVaGetValues( Active_row, XmNuserData, &row, NULL );

  /* Get the parameter index. */

  XtVaGetValues( XtParent( w ), XmNuserData, &indx, NULL );

  /* Get the new value from the widget label. */

  text = XmTextGetString( w );

  /* If the input is NULL then return.  If the input is all blanks
     return also. */

  if( text == (char *) NULL )
  {
    Parameter_modify_flag = HCI_NO_FLAG;
    return;
  }
  else
  {
    digit = HCI_NO_FLAG;

    for( i = 0; i < strlen( text ); i++ )
    {
      if( isdigit( (int) *( text + i ) ) )
      {
        digit = HCI_YES_FLAG;
        break;
      }
    }

    if( digit == HCI_NO_FLAG )
    {
      Parameter_modify_flag = HCI_NO_FLAG;
      return;
    }
  }

  /* Decode the value from the string. */
  sscanf( text,"%f",&value );
  XtFree( text );

  /* If the generation interval is 0, then inform the user that they
     must first set a non-zero generation interval. */
  if( Gen_map[(int) row] < 0 )
  {
    XmTextSetString( w, "      " );
    hci_warning_popup( Top_widget, "You must first set the generation interval\nto a value > 0 before defining parameters.", NULL );
    Parameter_modify_flag = HCI_NO_FLAG;
    return;
  }

  /* Get the previous value for the field. */
  buf_num = ORPGPAT_get_prod_id( Att_map[(int) row]);

  scale = ORPGPAT_get_parameter_scale( buf_num, (int) indx );
  min = ORPGPAT_get_parameter_min( buf_num,(int) indx ) / (float) scale;
  max = ORPGPAT_get_parameter_max( buf_num,(int) indx ) / (float) scale;
  ivalue = ORPGPGT_get_parameter(Which_table, Gen_map[(int) row],
                   ORPGPAT_get_parameter_index( buf_num, (int) indx ) );

  /* If the current saved value is not one of the special values.
     then unscale it and check to see if it is different from the
     new value. If so, validate it. */
  if( ivalue > PARAM_MAX_SPECIAL )
  {
    old_value = (float) ivalue / (float) scale;

    if( value == old_value )
    {
      Parameter_modify_flag = HCI_NO_FLAG;
      return;
    }
  }
  else
  {
    old_value = ivalue;
  }

  /* The value has changed so set the appropriate change flags. */
  switch( Which_table )
  {
    case CURRENT_TABLE :

      if( Current_flag == HCI_NOT_CHANGED_FLAG )
      {
        /* Sensitize the Save and Undo buttons. */
        XtSetSensitive( Save_button, True );
        XtSetSensitive( Undo_button, True );
      }

      Current_flag = HCI_CHANGED_FLAG;
      break;

    case DEFAULT_A_TABLE :

      if( Wx_mode_A_flag == HCI_NOT_CHANGED_FLAG )
      {
        /* Sensitize the Save and Undo buttons. */
        XtSetSensitive( Save_button, True );
        XtSetSensitive( Undo_button, True );
      }

      Wx_mode_A_flag = HCI_CHANGED_FLAG;
      break;

    case DEFAULT_B_TABLE :

      if( Wx_mode_B_flag == HCI_NOT_CHANGED_FLAG )
      {
        /* Sensitize the Save and Undo buttons. */
        XtSetSensitive( Save_button, True );
        XtSetSensitive( Undo_button, True );
      }

      Wx_mode_B_flag = HCI_CHANGED_FLAG;
      break;

    default :

      Parameter_modify_flag = HCI_NO_FLAG;
      return;
  }

  /* Update the changed value.  If an error is detected, popup an
     information window describing the problem. */

  status = ORPGPGT_set_parameter( Which_table, Gen_map[(int) row],
                 ORPGPAT_get_parameter_index( buf_num, (int) indx ),
                 (int) Round( value*scale ) );

  if( status != 0 )
  {
    switch( scale )
    {
      case 1 :

        sprintf( Buf, "An invalid parameter value of %d was\nentered.  The valid range is %d to %d.", (int) Round( value ), (int) min, (int) max );

        hci_warning_popup( Top_widget, Buf, NULL );

        if( ivalue > PARAM_MAX_SPECIAL )
        {
          sprintf( Buf,"%6d", (int) ( old_value + 0.001 ) );
        }
        else
        {
          sprintf( Buf, "      " );
        }
        break;

      case 10 :

        if( 
            ( 
              (
                !strncmp( ORPGPAT_get_mnemonic( buf_num ), "SRR", 3 ) &&
                ( ivalue == PARAM_ALG_SET )
              ) ||
              (
                !strncmp( ORPGPAT_get_mnemonic( buf_num ), "SRM", 3 ) &&
                ( ivalue == PARAM_ALG_SET )
              )
            ) &&
            (
              ( ORPGPAT_get_parameter_index( buf_num, (int) indx ) == 3 ) ||
              ( ORPGPAT_get_parameter_index( buf_num, (int) indx ) == 4 )
            )
          )
        {
          sprintf( Buf,"An invalid parameter value of %.1f was\nentered.  The valid range is %.1f or\n 0.0 to %.1f.", value, min, max );
        }
        else
        {
          sprintf( Buf, "An invalid parameter value of %.1f was\nentered.  The valid range is %.1f to %.1f.", value, min, max );
        }

        hci_warning_popup( Top_widget, Buf, NULL );

        if( ivalue > PARAM_MAX_SPECIAL )
        {
          sprintf( Buf, "%6.1f", old_value );
        }
        else
        {
          if(
              (
                (
                  !strncmp( ORPGPAT_get_mnemonic( buf_num ), "SRR", 3 ) &&
                  ( ivalue == PARAM_ALG_SET )
                ) ||
                (
                  !strncmp( ORPGPAT_get_mnemonic( buf_num ), "SRM", 3 ) &&
                  ( ivalue == PARAM_ALG_SET )
                )
              ) &&
              (
                ( ORPGPAT_get_parameter_index( buf_num, (int) indx ) == 3 ) ||
                ( ORPGPAT_get_parameter_index( buf_num, (int) indx ) == 4 )
              )
            )
          {
            sprintf( Buf, "%6.1f", -1.0 );
          }
          else
          {
            sprintf( Buf, "      " );
          }
        }
        break;

      case 100 :

        sprintf( Buf, "An invalid parameter value of %.2f was\nentered.  The valid range is %.2f to %.2f.", value, min, max );

        hci_warning_popup( Top_widget, Buf, NULL );

        if( ivalue > PARAM_MAX_SPECIAL )
        {
          sprintf( Buf, "%6.2f", old_value );
        }
        else
        {
          sprintf( Buf, "      " );
        }
        break;

      default :

        sprintf( Buf, "An invalid parameter value of %.3f was\nentered.  The valid range is %.3f to %.3f.", value, min, max );

        hci_warning_popup( Top_widget, Buf, NULL );

        if( ivalue > PARAM_MAX_SPECIAL )
        {
          sprintf( Buf, "%6f.3", old_value );
        }
        else
        {
          sprintf( Buf, "      " );
        }
        break;
    }

    XtSetSensitive( w, False );
    XmTextSetString( w, Buf );
    XtSetSensitive( w, True );
  }
  else
  {
    switch( scale )
    {
      case 1 :

        sprintf( Buf, "%6d", (int) Round( value ) );
        break;

      case 10 :

        sprintf( Buf, "%6f.1", value );
        break;

      case 100 :

        sprintf( Buf, "%6f.2", value );
        break;

      default :

        sprintf( Buf, "%6f.3", value );
        break;
    }

    XtSetSensitive( w, False );
    XmTextSetString( w, Buf );
    XtSetSensitive( w, True );
  }

    Parameter_modify_flag = HCI_NO_FLAG;
}

/************************************************************************
     Description: This function creates a new entry in the active
                  product generation list.

     Input: table - ID of product generation list
     row - Row in display table for new entry
     prod_id - Product ID for new entry
     generation - Generation interval
     retention - retention period
     elevation - elevation angle
     cut - cut (if cut = 0, elevation used, otherwise
           cut is used instead of elevation

     Return: Index of new entry in list
 ************************************************************************/

static int Hci_create_new_table_entry( int table, int row, int prod_id,
                  int generation, int retention, float elevation, int cut )
{
  int indx = 0;
  int pindx = 0;
  int eindex = 0;
  int i = 0;
  int j = 0;
  int cnt = 0;
  int status = 0;
  int scale = 0;
  int ival = 0;
  short cfc_map = 0;
  char *mne = NULL;

  /* Get the products mnemonic. */

  mne = ORPGPAT_get_mnemonic( prod_id );

  /* If the product is the clutter filter control product then it
     has 4 parts and requires 4 entries in the internal product
     generation list.  The user will only see a single entry in the
     visible table. */

  if( !strncmp( mne, "CFC", 3 ) ){ cnt = 4; }
  else{ cnt = 1; }

  /* Add an entry in the internal product generation table for each
     part (i.e., CFC product needs 4 entries). */

  for( j = 0; j < cnt; j++ )
  {
    indx = ORPGPGT_add_entry( table );

    if( indx < 0 )
    {
      HCI_LE_error( "ORPGPGT_add_entry (%d) = %d", table, indx );
      return( -1 );
    }

    Gen_map[row] = indx;

    /* Set the product ID for the new entry */

    if( ( status = ORPGPGT_set_prod_id( table, indx, prod_id ) ) < 0 )
    {
      HCI_LE_error( "ORPGPGT_set_prod_id (%d,%d,%d) = %d", table, indx, prod_id, status );
      return( -1 );
    }

    /* Set the generation interval for the new entry. */

    status = ORPGPGT_set_generation_interval( table, indx, generation );

    if( status < 0 )
    {
      HCI_LE_error( "ORPGPGT_set_generation_interval (%d,%d,%d) = %d", table, indx, generation, status );
      return( -1 );
    }

    /* Set the retention period for the new entry. */

    if( ( status = ORPGPGT_set_retention_period( table, indx, retention ) ) < 0 )
    {
      HCI_LE_error( "ORPGPGT_set_retention_period (%d,%d,%d) = %d", table, indx, retention, status );
      return( -1 );
    }

    /* Initialize the parameters (if any are defined) */

    for( i = 0; i < ORPGPGT_MAX_PARAMETERS; i++ )
    {
      status = ORPGPGT_set_parameter( table, indx, ORPGPAT_get_parameter_index( prod_id, i ), PARAM_UNUSED );
    }

    /* For each parameter associated with this product set the
       value to default. */

    for( i = 0; i < ORPGPAT_get_num_parameters( prod_id ); i++ )
    {
      status = ORPGPGT_set_parameter( table, indx, ORPGPAT_get_parameter_index( prod_id, i ), ORPGPAT_get_parameter_default( prod_id, i ) );
    }

    eindex = ORPGPAT_elevation_based( prod_id );

    pindx = -1;

    /* If the product is elevation based then we need to find the
       parameter index for the elevation/cut field. */

    if( eindex >= 0 )
    {
      for( i = 0; i < ORPGPAT_get_num_parameters( prod_id ); i++ )
      {
        if( eindex == ORPGPAT_get_parameter_index( prod_id,  i ) )
        {
          pindx = i;
          break;
        }
      }
    }

    /* If the product is elevation based then we need to set the
       elevation/cut field. */

    if( eindex >= 0 )
    {
      scale = ORPGPAT_get_parameter_scale( prod_id, pindx );

      /* If the cut field is not 0, then we use it. Otherwise,
         we use the elevation angle */

      if( cut == 0 )
      {
        /* If the elevation angle is negative, we need to
           normalize it to 360.0. */

        if( elevation < 0 ){ elevation += 360.0; }

        /* The number is saved as a scaled integer */

        ival = (int) ( elevation * scale );

      }
      else
      {
        /* We need to set bits 13 and 14 for cut mode. */

        if( cut < 0 ){ cut = -cut; }
        ival = cut | ORPGPRQ_LOWER_CUTS;
      }

      for( i = 0; i < ORPGPAT_get_num_parameters( prod_id ); i++ )
      {
        if( i == pindx )
        {
          status = ORPGPGT_set_parameter( table, indx, ORPGPAT_get_parameter_index( prod_id, i ), (int) ival );
        }

        if( status == ORPGPGT_ERROR )
        {
          HCI_LE_error( "ORPGPGT_set_parameter (%d,%d,%d,%d) = %d", table, indx, ORPGPAT_get_parameter_index( prod_id, i ), ival, status );
        }
      }
    }
    else if( cnt > 1 )
    {
      switch( j )
      {
        case 0 :
          cfc_map = (short) 0x2000;
          break;

        case 1 :
          cfc_map = (short) 0x4000;
          break;

        case 2 :
          cfc_map = (short) 0xA000;
          break;

        case 3 :
          cfc_map = (short) 0xC000;
          break;
      }

      status = ORPGPGT_set_parameter( table, indx, ORPGPAT_get_parameter_index( prod_id, i ), (int) cfc_map );

      if( status == ORPGPGT_ERROR )
      {
        HCI_LE_error( "ORPGPGT_set_parameter (%d,%d,%d,%d) = %d", table,
                      indx, ORPGPAT_get_parameter_index( prod_id, i ), (int) cfc_map, status );
        return( -1 );
      }
    }
  }

  XmToggleButtonSetState( Gen_tbl[row].cut_toggle, False, False );

  return( 0 );
}

/************************************************************************
     Description: This function is invoked when a cuts toggle
                  button is selected.  If the cuts field is an
                  elevation angle and this toggle is set, all
                  elevations at/or below the specified elevation
                  angle are generated.
 ************************************************************************/

static void Hci_cut_toggle_callback( Widget w, XtPointer y, XtPointer z )
{
  int value = 0;
  int indx = 0;
  int buf_num = 0;
  int ndx = 0;
  XtPointer data = (XtPointer) NULL;
  XmToggleButtonCallbackStruct *cbs;

  cbs = (XmToggleButtonCallbackStruct *) z;

  /* Get the table row number from the parent rowcolumn user data */

  XtVaGetValues( XtParent( w ), XmNuserData, &data, NULL );

  buf_num = ORPGPAT_get_prod_id( Att_map[(int) data] );

  indx = ORPGPAT_elevation_based( buf_num );
  ndx = ORPGPGT_buf_in_tbl( Which_table, buf_num );

  value = ORPGPGT_get_parameter( Which_table, ndx, indx );

  if( cbs->set )
  {
    value = value & 0x1fff;
    value = value | ORPGPRQ_LOWER_ELEVATIONS;
  }
  else
  {
    value = value & 0x1fff;
  }

  value = ORPGPGT_set_parameter( Which_table, ndx, indx, value );
  Set_save_undo_flag();
}

static void Draw_parameters_widget()
{
  int i = 0;
  Widget left_spacer = (Widget) NULL;
  Widget parameter_form = (Widget) NULL;

  /* At the bottom of the list, add an area to display any product
     parameters for the product currently selected. */

  Bottom_form = XtVaCreateWidget( "bottom_form",
                xmFormWidgetClass, Form,
                XmNleftAttachment, XmATTACH_FORM,
                XmNrightAttachment, XmATTACH_FORM,
                XmNbottomAttachment, XmATTACH_FORM,
                XmNbackground, Base_bg,
                XmNverticalSpacing, 1,
                NULL );

  Parameter_label = XtVaCreateManagedWidget(
                "Use the below form to edit product parameters (if applicable).",
                xmLabelWidgetClass, Bottom_form,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                XmNleftAttachment, XmATTACH_FORM,
                XmNrightAttachment, XmATTACH_FORM,
                XmNtopAttachment, XmATTACH_FORM,
                NULL );

  left_spacer = XtVaCreateManagedWidget( "   parameter spacer ",
                xmLabelWidgetClass, Bottom_form,
                XmNforeground, Base_bg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                XmNtopAttachment, XmATTACH_WIDGET,
                XmNtopWidget, Parameter_label,
                XmNleftAttachment, XmATTACH_FORM,
                XmNbottomAttachment, XmATTACH_FORM,
                NULL );

  Parameter_frame = XtVaCreateManagedWidget( "parameter_frame",
                xmFrameWidgetClass, Bottom_form,
                XmNleftAttachment, XmATTACH_WIDGET,
                XmNleftWidget, left_spacer,
                XmNtopAttachment, XmATTACH_WIDGET,
                XmNtopWidget, Parameter_label,
                XmNbottomAttachment, XmATTACH_FORM,
                NULL );

  parameter_form = XtVaCreateWidget( "parameter_form",
                xmFormWidgetClass, Parameter_frame,
                XmNbackground, Black_color,
                XmNverticalSpacing, 1,
                NULL );

  Parameter_widget[0].rowcol = XtVaCreateWidget( "parameter_rowcol",
                xmRowColumnWidgetClass, parameter_form,
                XmNbackground, Base_bg,
                XmNtopAttachment, XmATTACH_FORM,
                XmNleftAttachment, XmATTACH_FORM,
                XmNrightAttachment, XmATTACH_FORM,
                XmNorientation, XmHORIZONTAL,
                XmNmarginHeight, 1,
                XmNmarginWidth, 0,
                NULL );

  Parameter_widget[0].description = XtVaCreateManagedWidget( "parameter_description",
                xmTextFieldWidgetClass, Parameter_widget[0].rowcol,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                XmNcolumns, 24,
                XmNtraversalOn, False,
                XmNeditable, False,
                XmNmarginWidth, 0,
                XmNmarginHeight, 0,
                XmNborderWidth, 0,
                NULL );

  sprintf( Buf, "Description" );
  XmTextSetString( Parameter_widget[0].description, Buf );

  Parameter_widget[0].minimum = XtVaCreateManagedWidget( "parameter_minimum",
                xmTextFieldWidgetClass, Parameter_widget[0].rowcol,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                XmNcolumns, 7,
                XmNtraversalOn, False,
                XmNeditable, False,
                XmNmarginWidth, 0,
                XmNmarginHeight, 0,
                XmNborderWidth, 0,
                NULL );

  sprintf( Buf, "Minimum" );
  XmTextSetString( Parameter_widget[0].minimum, Buf );

  Parameter_widget[0].maximum = XtVaCreateManagedWidget( "parameter_maximum",
                xmTextFieldWidgetClass, Parameter_widget[0].rowcol,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                XmNcolumns, 7,
                XmNtraversalOn, False,
                XmNeditable, False,
                XmNmarginWidth, 0,
                XmNmarginHeight, 0,
                XmNborderWidth, 0,
                NULL );

  sprintf( Buf, "Maximum" );
  XmTextSetString( Parameter_widget[0].maximum, Buf );

  Parameter_widget[0].value = XtVaCreateManagedWidget( "parameter_value",
                xmTextFieldWidgetClass, Parameter_widget[0].rowcol,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                XmNcolumns, 7,
                XmNtraversalOn, False,
                XmNeditable, False,
                XmNmarginWidth, 0,
                XmNmarginHeight, 0,
                XmNborderWidth, 0,
                XmNverifyBell, False,
                NULL );

  sprintf( Buf,"Value" );
  XmTextSetString( Parameter_widget[0].value, Buf );

  Parameter_widget[0].units = XtVaCreateManagedWidget( "parameter_units",
                xmTextFieldWidgetClass, Parameter_widget[0].rowcol,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                XmNcolumns, 12,
                XmNtraversalOn, False,
                XmNeditable, False,
                XmNmarginWidth, 0,
                XmNmarginHeight, 0,
                XmNborderWidth, 0,
                NULL );

  sprintf( Buf, "Units" );
  XmTextSetString( Parameter_widget[0].units, Buf );

  XtManageChild( Parameter_widget[0].rowcol );

  for( i = 1; i <= ORPGPGT_MAX_PARAMETERS; i++ )
  {
    Parameter_widget[i].rowcol = XtVaCreateWidget( "parameter_rowcol",
                xmRowColumnWidgetClass, parameter_form,
                XmNbackground, Base_bg,
                XmNtopAttachment, XmATTACH_WIDGET,
                XmNtopWidget, Parameter_widget[i-1].rowcol,
                XmNleftAttachment, XmATTACH_FORM,
                XmNrightAttachment, XmATTACH_FORM,
                XmNorientation, XmHORIZONTAL,
                XmNmarginHeight, 1,
                XmNmarginWidth, 0,
                NULL );

    Parameter_widget[i].description = XtVaCreateManagedWidget( "param_desc",
                xmTextFieldWidgetClass, Parameter_widget[i].rowcol,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                XmNcolumns, 24,
                XmNtraversalOn, False,
                XmNeditable, False,
                XmNmarginWidth, 0,
                XmNmarginHeight, 0,
                XmNborderWidth, 0,
                NULL );

    Parameter_widget[i].minimum = XtVaCreateManagedWidget( "parameter_minimum",
                xmTextFieldWidgetClass, Parameter_widget[i].rowcol,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                XmNcolumns, 7,
                XmNtraversalOn, False,
                XmNeditable, False,
                XmNmarginWidth, 0,
                XmNmarginHeight, 0,
                XmNborderWidth, 0,
                NULL );

    Parameter_widget[i].maximum = XtVaCreateManagedWidget( "parameter_maximum",
                xmTextFieldWidgetClass, Parameter_widget[i].rowcol,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                XmNcolumns, 7,
                XmNtraversalOn, False,
                XmNeditable, False,
                XmNmarginWidth, 0,
                XmNmarginHeight, 0,
                XmNborderWidth, 0,
                NULL );

    Parameter_widget[i].value = XtVaCreateManagedWidget( "parameter_value",
                xmTextFieldWidgetClass, Parameter_widget[i].rowcol,
                XmNforeground, Edit_fg,
                XmNbackground, Edit_bg,
                XmNfontList, List_font,
                XmNcolumns, 7,
                XmNtraversalOn, True,
                XmNeditable, True,
                XmNmarginWidth, 0,
                XmNmarginHeight, 0,
                XmNborderWidth, 0,
                XmNverifyBell, False,
                NULL );

    XtAddCallback( Parameter_widget[i].value,
                   XmNfocusCallback, Parameter_table_gain_focus, NULL );
    XtAddCallback( Parameter_widget[i].value,
                   XmNmodifyVerifyCallback, hci_verify_float_callback,
                   (XtPointer) 7 );
    XtAddCallback( Parameter_widget[i].value,
                   XmNlosingFocusCallback, Parameter_table_modify,
                   (XtPointer) i );
    XtAddCallback( Parameter_widget[i].value,
                   XmNactivateCallback, Parameter_table_modify,
                   (XtPointer) i );

    Parameter_widget[i].units = XtVaCreateManagedWidget( "parameter_units",
                xmTextFieldWidgetClass, Parameter_widget[i].rowcol,
                XmNforeground, Text_fg,
                XmNbackground, Base_bg,
                XmNfontList, List_font,
                XmNcolumns, 12,
                XmNtraversalOn, False,
                XmNeditable, False,
                XmNmarginWidth, 0,
                XmNmarginHeight, 0,
                XmNborderWidth, 0,
                NULL );

    Parameter_widget[i].index = -1;

    XtManageChild( Parameter_widget[i].rowcol );
  }

  XtVaSetValues( Parameter_widget[ORPGPGT_MAX_PARAMETERS].rowcol,
                XmNbottomAttachment, XmATTACH_FORM, NULL );

  XtManageChild( parameter_form );
}

static void Set_save_undo_flag()
{
  switch( Which_table )
  {
    case CURRENT_TABLE :

      if( Current_flag == HCI_NOT_CHANGED_FLAG )
      {
        XtSetSensitive( Save_button, True );
        XtSetSensitive( Undo_button, True );
      }

      Current_flag = HCI_CHANGED_FLAG;
      break;

    case DEFAULT_A_TABLE :

      if( Wx_mode_A_flag == HCI_NOT_CHANGED_FLAG )
      {
        XtSetSensitive( Save_button, True );
        XtSetSensitive( Undo_button, True );
      }

      Wx_mode_A_flag = HCI_CHANGED_FLAG;
      break;

    case DEFAULT_B_TABLE :

      if( Wx_mode_B_flag == HCI_NOT_CHANGED_FLAG )
      {
        XtSetSensitive( Save_button, True );
        XtSetSensitive( Undo_button, True );
      }

      Wx_mode_B_flag = HCI_CHANGED_FLAG;
      break;
  }
}

/*********************************************************************
     Description: Returns rounded integer of a floating point number.
 **********************************************************************/

static int Round( float r )
{
  if( (double) r >= 0. ){ return( (int) ( r + .5 ) ); }
  return( -(int) ( ( -r ) + .5 ) );
}

