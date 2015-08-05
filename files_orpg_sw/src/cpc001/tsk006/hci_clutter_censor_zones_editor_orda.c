/************************************************************************
 *                                                                      *
 *      Module:  hci_clutter_region_editor.c                            *
 *                                                                      *
 *      Description:  This modile contains a group of routines used     *
 *      by the HCI to display and edit clutter suppression regions.     *
 *      The basic premise is to define clutter regions over a base      *
 *      data PPI display.                                               *
 *                                                                      *
 ************************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/08/02 17:30:50 $
 * $Id: hci_clutter_censor_zones_editor_orda.c,v 1.62 2013/08/02 17:30:50 steves Exp $
 * $Revision: 1.62 $
 * $State: Exp $
 */

/*      Local include file definitions.                                 */

#include <hci.h>
#include <hci_clutter_censor_zones_orda.h>

/* Macros */

#define HCI_CCZ_DATA_TIMED_OUT		-2
#define VISIBLE_CLUTTER_REGION_FILES	10 
#define HCI_CCZ_MAX_ZOOM		32
#define HCI_KM_TO_KM			1.0
#define LEGEND_BOX_HEIGHT		20
#define LEGEND_BOX_WIDTH		60
#define LEGEND_SPACE			15
#define COLOR_BAR_WIDTH			100
#define LABEL_OFFSET			5
#define DEFAULT_FILE_INDEX		0
#define MAX_BACKGROUND_PRODUCTS		128
#define	NO_INIT_ERROR			0x00
#define	READ_CLUTTER_FAILED		0x01
#define	READ_DOWNLOAD_INFO_FAILED	0x02
#define	DOWNLOAD_FILE_DELETED		0x04

enum { UNITS_KM, UNITS_NM };
enum { HCI_CCZ_ZOOM_MODE, HCI_CCZ_SECTOR_MODE };
enum { HCI_CCZ_BACKGROUND_BREF, HCI_CCZ_BACKGROUND_CFC };

typedef struct {
        int     cut;
        Widget  region;
        Widget  start_azimuth;
        Widget  stop_azimuth;
        Widget  start_range;
        Widget  stop_range;
        Widget  filter;
} Hci_clutter_widgets_t;

/*      Global definitions for various widgets.                         */

static Display *Clutter_display               = (Display *) NULL;
static Window  Clutter_window                 = (Window) NULL;
static Pixmap  Clutter_pixmap                 = (Pixmap) NULL;
static GC      Clutter_gc                     = (GC) NULL;
static Widget  Top_widget                     = (Widget) NULL;
static Widget  File_dialog                    = (Widget) NULL;
static Widget  Save_as_dialog                 = (Widget) NULL;
static Widget  Close_button                   = (Widget) NULL;
static Widget  File_button                    = (Widget) NULL;
static Widget  Undo_button                    = (Widget) NULL;
static Widget  Download_button                = (Widget) NULL;
static Widget  Restore_button                 = (Widget) NULL;
static Widget  Update_button                  = (Widget) NULL;
static Widget  Last_downloaded_file_label     = (Widget) NULL;
static Widget  Time_last_downloaded_label     = (Widget) NULL;
static Widget  Currently_displayed_file_label = (Widget) NULL;
static Widget  Time_last_modified_label       = (Widget) NULL;
static Widget  Segment1_button                = (Widget) NULL;
static Widget  Segment2_button                = (Widget) NULL;
static Widget  Segment3_button                = (Widget) NULL;
static Widget  Segment4_button                = (Widget) NULL;
static Widget  Segment5_button                = (Widget) NULL;
static Widget  Background_product_button      = (Widget) NULL;
static Widget  CFC_product_button             = (Widget) NULL;
static Widget  Refresh_button                 = (Widget) NULL;
static Widget  Delete_button                  = (Widget) NULL;
static Widget  Select_code_button             = (Widget) NULL;
static Widget  Cfc_rowcol                     = (Widget) NULL;
static Widget  Segment_label                  = (Widget) NULL;
static Widget  Segment_scroll                 = (Widget) NULL;
static Widget  Filename_text_box              = (Widget) NULL;
static Widget  Start_range_text_box           = (Widget) NULL;
static Widget  End_range_text_box             = (Widget) NULL;
static Widget  Background_product_rowcol      = (Widget) NULL;
static Widget  Background_product             = (Widget) NULL;
static Widget  Background_product_text        = (Widget) NULL;
static Widget  Background_product_list        = (Widget) NULL;
static Widget  File_dialog_close_button       = (Widget) NULL;
static Widget  File_dialog_new_button         = (Widget) NULL;
static Widget  File_dialog_save_button        = (Widget) NULL;
static Widget  File_dialog_save_as_button     = (Widget) NULL;
static Widget  File_dialog_delete_button      = (Widget) NULL;
static Widget  File_dialog_open_button        = (Widget) NULL;
static Widget  File_dialog_list_widget        = (Widget) NULL;
static XFontStruct *Large_fontinfo            = (XFontStruct *) NULL;
static XFontStruct *Small_fontinfo            = (XFontStruct *) NULL;

/* Drawing area variable. */

static Dimension Drawing_area_width = 900;
static Dimension Drawing_area_height = 460;
static unsigned int Drawing_area_depth = 0;

/* Clutter display variables. */

static Dimension Clutter_display_width;
static Dimension Clutter_display_height;
static int Clutter_display_input_mode = HCI_CCZ_SECTOR_MODE;
static int Clutter_display_background_product_type = HCI_CCZ_BACKGROUND_BREF;
static int Clutter_display_background_product_id = 2;
static int Clutter_display_background_product_LUT[MAX_BACKGROUND_PRODUCTS];
static int Clutter_display_zoom_factor = 1;
static float Clutter_display_scale_x = 460.0/(2*HCI_CCZ_MAX_RANGE);
static float Clutter_display_scale_y = -(460.0/(2*HCI_CCZ_MAX_RANGE));
static int Clutter_display_center_pixel = 230;
static int Clutter_display_center_scanl = 230;
static float Clutter_display_center_x_offset = 0;
static float Clutter_display_center_y_offset = 0;

/* Composite display variables */

static Dimension Composite_display_width;
static Dimension Composite_display_height;
static unsigned char Composite_display_data[HCI_CCZ_RADIALS+1][HCI_CCZ_GATES+1];
static int Filter_bypass_color = ORANGE;
static int Filter_all_bins_color = DARKSEAGREEN;
static int Filter_none_color = PRODUCT_BACKGROUND_COLOR;

/* Variables for current edit. */

static short Edit_segment = HCI_CCZ_SEGMENT_ONE; /* Current active segment */
static short Edit_region = -1; /* Current active region */
static short Edit_filter = HCI_CCZ_FILTER_ALL; /* Current active filter */
static char Edit_file_label[MAX_LABEL_SIZE]; /* Current active filename */
static int Edit_num_regions = 0; /* Number of regions for current active file */
static int Edit_row = 0; /* Selected table row of current active file */
static Hci_clutter_data_t Edit_data[MAX_NUMBER_CLUTTER_ZONES+1];
static Hci_clutter_widgets_t Edit_table_widgets[MAX_NUMBER_CLUTTER_ZONES+1];
static Widget Edit_table_rowcol[MAX_NUMBER_CLUTTER_ZONES+1];

/* Background product query variables. */

static ORPGDBM_query_data_t Query_data[16];    /* product query parameters */
static RPG_prod_rec_t Db_info[50];             /* product query results */
static int Query_time[50];                     /* product query times */
static int Background_product_status_flag = 0;

/* Miscellaneous variables. */

static int System_type = HCI_NWS_SYSTEM;
static int Channel_number = 1;
static int Unlocked_loca = HCI_NO_FLAG;
static int Config_change_popup = 0;
static char Buff[HCI_BUF_512];
static float Units_factor = HCI_KM_TO_NM;
static int Num_clutter_files = -1;
static int Clutter_file_index = DEFAULT_FILE_INDEX;
static int Selected_file_index = DEFAULT_FILE_INDEX;
static float Segment_one_max_elevation = 45.0;
static float Segment_two_max_elevation = 45.0;
static float Segment_three_max_elevation = 45.0;
static float Segment_four_max_elevation = 45.0;
static int Num_valid_segments = -1;
static int Load_flag = HCI_NO_FLAG;
static int Refresh_background_flag = 0;
static int Restore_flag = HCI_NO_FLAG;
static int New_file_flag = HCI_NO_FLAG;
static int Download_flag = HCI_NO_FLAG;
static int Rda_adapt_updated_flag = HCI_NO_FLAG;
static int Update_flag = HCI_NOT_MODIFIED_FLAG;
static int Update_baseline_flag = HCI_NO_FLAG;
static int Save_flag = HCI_NO_FLAG;
static int Save_as_flag = HCI_NO_FLAG;
static int CCZ_updated_flag = HCI_NO_FLAG;
static int Initialization_error_flag = NO_INIT_ERROR;
        
/*      Definitions for translation table to be used for mouse events   *
 *      in region display area.                                         */

static String  Clutter_translations =
        "<PtrMoved>:    Mouse_input(move) \n\
        <Btn1Down>:     Mouse_input(down1)  ManagerGadgetArm() \n\
        <Btn1Up>:       Mouse_input(up1)    ManagerGadgetActivate() \n\
        <Btn1Motion>:   Mouse_input(motion1) ManagerGadgetButtonMotion() \n\
        <Btn2Down>:     Mouse_input(down2)  ManagerGadgetArm() \n\
        <Btn2Up>:       Mouse_input(up2)    ManagerGadgetActivate() \n\
        <Btn2Motion>:   Mouse_input(motion2) ManagerGadgetButtonMotion() \n\
        <Btn3Down>:     Mouse_input(down3)  ManagerGadgetArm() \n\
        <Btn3Up>:       Mouse_input(up3)    ManagerGadgetActivate() \n\
        <Btn3Motion>:   Mouse_input(motion3) ManagerGadgetButtonMotion()";

/* Prototypes associated with main HCI */

static void Close_button_cb( Widget, XtPointer, XtPointer );
static void Close_button_cb_no_save( Widget, XtPointer, XtPointer );
static void File_button_cb( Widget, XtPointer, XtPointer );
static void Undo_button_cb( Widget, XtPointer, XtPointer );
static void Download_button_cb( Widget, XtPointer, XtPointer );
static void Download_button_cb_accept( Widget, XtPointer, XtPointer );
static void Download_button_cb_cancel( Widget, XtPointer, XtPointer );
static void Restore_button_cb( Widget, XtPointer, XtPointer );
static void Restore_button_cb_accept( Widget, XtPointer, XtPointer );
static void Update_button_cb( Widget, XtPointer, XtPointer );
static void Update_button_cb_accept( Widget, XtPointer, XtPointer );
static void Delete_button_cb( Widget, XtPointer, XtPointer );
static void Refresh_button_cb( Widget, XtPointer, XtPointer );
static void Draw_area_expose_cb( Widget, XtPointer, XtPointer );
static void Mouse_input( Widget, XEvent *, String *, int * );
static void Change_azimuth_cb( Widget, XtPointer, XtPointer );
static void Change_range_cb( Widget, XtPointer, XtPointer );
static void Reset_zoom_cb( Widget, XtPointer, XtPointer );
static void Select_mode_cb( Widget, XtPointer, XtPointer );
static void Select_region_cb( Widget, XtPointer, XtPointer );
static void Select_segment_cb( Widget, XtPointer, XtPointer );
static void Select_units_cb( Widget, XtPointer, XtPointer );
static void Select_filter_cb( Widget,XtPointer, XtPointer );
static void Background_product_id_cb( Widget, XtPointer, XtPointer );
static void Background_product_type_cb( Widget, XtPointer, XtPointer );

static void Restore_baseline();
static void Update_baseline();
static void Clutter_download();
static void Clutter_load();
static void Clutter_draw();
static void Display_composite_clutter_map();
static void Display_background_product_and_censor_zones();
static void Draw_censor_zone( Drawable, int, float, float, float, float, int );
static void Update_clutter_files_list();
static void Update_clutter_regions_table();
static void Load_background_product( int );
static void Draw_labels_on_display();
static void Draw_no_background_label();
static void Hci_set_segment_button_sensitivities();
static void Hci_set_control_button_sensitivities();
static void Copy_pixmap_to_window();
static void Copy_file_to_edit_buffer();
static void Rda_adaptation_updated( int, LB_id_t, int, void * );
static void Read_rda_adaptation();
static void Rda_config_change();
static void Timer_proc();
static void Ccz_deau_cb();
static void Set_file_info_label();
static void Set_download_info_label();
static void Time_bypass_map_created( int *, int *, int *, int *, int * );
static void Time_clutter_map_created( int *, int *, int *, int *, int * );
static void Time_product_created( int *, int *, int *, int *, int * );
static void Hci_set_clip_rectangle( int, int );
static void Hci_set_foreground( int );
static void Hci_set_background( int );
static void Hci_set_display_font( int );
static void Hci_set_line_width( int );
static void Hci_fill_polygon( XPoint *, int );
static void Hci_fill_rectangle( int, int, int, int );
static void Hci_draw_string( int, int, char * );
static void Hci_draw_rectangle( int, int, int, int );
static void Hci_draw_line( Drawable, XPoint *, int );
static void Hci_draw_imagestring( int, int, char * );
static int  Lock_cb();

/* Prototypes associated with the File Dialog */

static void File_dialog_close_button_cb( Widget, XtPointer, XtPointer );
static void File_dialog_delete_button_cb( Widget, XtPointer, XtPointer );
static void File_dialog_delete_button_cb_accept( Widget, XtPointer, XtPointer );
static void File_dialog_new_button_cb( Widget, XtPointer, XtPointer );
static void File_dialog_save_button_cb( Widget, XtPointer, XtPointer );
static void File_dialog_save_button_cb_accept( Widget, XtPointer, XtPointer );
static void File_dialog_save_button_cb_cancel( Widget, XtPointer, XtPointer );
static void File_dialog_save_as_button_cb( Widget, XtPointer, XtPointer );
static void File_dialog_open_button_cb( Widget, XtPointer, XtPointer );
static void File_dialog_list_widget_cb( Widget, XtPointer, XtPointer );
static void Save_as_dialog_accept_button_cb( Widget, XtPointer, XtPointer );
static void Save_as_dialog_cancel_button_cb( Widget, XtPointer, XtPointer );
static void Clutter_file_save();
static void Clutter_file_save_as();
static void Create_save_as_dialog();

/************************************************************************
 *      Description: This is the main routine for the HCI Clutter       *
 *                   Suppression Regions Editor task.                   *
 *                                                                      *
 *      Input:  argc - number of command line arguments                 *
 *              argv - string containing command line arguments         *
 *      Output: NONE                                                    *
 *      Return: exit code                                               *
 ************************************************************************/

int main( int argc, char *argv[] )
{
        Arg             arg [10];
        XGCValues       gcv;
        XtActionsRec    actions;
        int             i;
        int             status;
        XmString        str;
        int             n;
        Dimension       select_button_width;

/*      Define the set of widgets used to define the elements of the    *
 *      clutter censor zone edit window.                                */

        Widget          form;
        Widget          control_frame;
        Widget          control_rowcol;
        Widget          download_info_rowcol;
        Widget          file_info_rowcol;
        Widget          clutter_rowcol;
        Widget          label;
        Widget          zoom_button;
        Widget          sector_button;
        Widget          reset_zoom_button;
        Widget          km_button;
        Widget          nm_button;
        Widget          draw;
        Widget          list_frame;
        Widget          list_form;
        Widget          list1_rowcol;
        Widget          list2_rowcol;
        Widget          list2_frame;
        Widget          low_segment_frame;
        Widget          segment_form;
        Widget          label_rowcol;
        Widget          low_form;
        Widget          segment;
        Widget          units;
        Widget          mode;
        Widget          background;
        Widget          text;
	Widget          lock_widget;

        int     retval;
        int     prod_id;
        char    *buf;
        char    *filename;
        Hci_ccz_data_t  *config_data;

        /* Initialize HCI. */

        HCI_init( argc, argv, HCI_CCZ_TASK );

        Top_widget = HCI_get_top_widget();
        Clutter_display = HCI_get_display();
        System_type = HCI_get_system();
        Channel_number = HCI_get_channel_number();

        Drawing_area_depth   = XDefaultDepth   (Clutter_display,
                        DefaultScreen (Clutter_display));

        hci_initialize_product_colors (Clutter_display, HCI_get_colormap());

/*      Use a form widget to manage placement of various widgets        *
 *      in the window.  The objects are:  a rowcolumn widget at the     *
 *      top which contains all of the file control functions, a         *
 *      drawingarea widget in which basedata and cluttermap data are    *
 *      displayed and clutter regions defined, a rowcolumn widget       *
 *      which contains buttons for region operations, and two scrolled  *
 *      windows which contain the definitions of each clutter censor    *
 *      zone.                                                           */

        form = XtVaCreateWidget ("form",
                xmFormWidgetClass,      Top_widget,
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                NULL);

        XtAddCallback (form,
                XmNfocusCallback, hci_force_resize_callback,
                (XtPointer) Top_widget);

        control_frame = XtVaCreateManagedWidget ("clutter_control_frame",
                xmFrameWidgetClass,     form,
                XmNleftAttachment,      XmATTACH_FORM,
                XmNrightAttachment,      XmATTACH_FORM,
                XmNtopAttachment,       XmATTACH_FORM,
                NULL);
            
        HCI_PM("Initialize Task Information");              

        Clutter_display_width    = Drawing_area_height;
        Clutter_display_height   = Drawing_area_height;
        Composite_display_width  = Drawing_area_width - Clutter_display_width;
        Composite_display_height = Drawing_area_height;
        
/*      Register for updates to the rda adaptation data here */

        ORPGDA_write_permission( ORPGDAT_RDA_ADAPT_DATA );

        status = ORPGDA_UN_register( ORPGDAT_RDA_ADAPT_DATA,
                   ORPGDAT_RDA_ADAPT_MSG_ID, Rda_adaptation_updated );

        if( status != LB_SUCCESS )
        {
          HCI_LE_error("LB Register RDA_ADAPT_MSG_ID failed (%d)", status);
          HCI_task_exit( HCI_EXIT_FAIL );
        }

/*	Make initial read of rda adaptation data */

        Read_rda_adaptation();

/*	Progress meter */

        HCI_PM("Read Clutter Suppresion Regions Data");             

/*      Read clutter info and initialize accordingly. */

        if( ( status = hci_read_clutter_regions_file() ) <= 0 )
	{
          /* If reading the clutter censor zones message failed,
             create popup and then exit. */
            HCI_LE_error("hci_read_clutter_regions_file failed (%d)", status);
            Initialization_error_flag |= READ_CLUTTER_FAILED;
        }

        if( ( status = hci_read_clutter_download_info() ) > 0 )
	{
          filename = hci_get_clutter_download_file_name( Channel_number );
          if( strlen( filename ) == 0 )
          {
            /* If the previously downloaded clutter file no longer
               exists, create popup and use the default file. */
            HCI_LE_error( "Last downloaded clutter file no longer exists" );
            Initialization_error_flag |= DOWNLOAD_FILE_DELETED;
            Clutter_file_index = DEFAULT_FILE_INDEX;
            Selected_file_index = DEFAULT_FILE_INDEX;
          }
          else
          {
            Clutter_file_index = hci_get_clutter_file_index( filename );
            Selected_file_index = Clutter_file_index;
          }
        }
	else
	{
          /* If reading the clutter download info failed, create
             popup and use the default file. */
          HCI_LE_error("read clutter download info failed (%d)", status);
          Initialization_error_flag |= READ_DOWNLOAD_INFO_FAILED;
          Clutter_file_index = DEFAULT_FILE_INDEX;
          Selected_file_index = DEFAULT_FILE_INDEX;
        }

/*      Copy file to edit buffer. */
        Copy_file_to_edit_buffer();

/*      Define the fonts to use in the various displays.                */

        Large_fontinfo = hci_get_fontinfo( LARGE );
        Small_fontinfo = hci_get_fontinfo( SMALL );

/*      Initialize the clutter suppression region data structure        */

        Edit_region = -1;

/*      Build a clutter map using the priority definitions in the       *
 *      RDA/RPG ICD.                                                    */

        hci_build_clutter_map (&Composite_display_data[0][0],
                        Edit_data,
                        Edit_segment,
                        Edit_num_regions);


/*      Define a rowcolumn widget to contain file main file control     *
 *      functions.                                                      */

        control_rowcol = XtVaCreateManagedWidget ("clutter_control_rowcol",
                xmRowColumnWidgetClass, control_frame,
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNorientation,         XmHORIZONTAL,
                XmNmarginHeight,        1,
                NULL);

        Close_button = XtVaCreateManagedWidget ("Close",
                xmPushButtonWidgetClass,        control_rowcol,
                XmNforeground,          hci_get_read_color (BUTTON_FOREGROUND),
                XmNbackground,          hci_get_read_color (BUTTON_BACKGROUND),
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

        XtAddCallback (Close_button,
                XmNactivateCallback, Close_button_cb, NULL);

        File_button = XtVaCreateManagedWidget ("File",
                xmPushButtonWidgetClass,        control_rowcol,
                XmNforeground,          hci_get_read_color (BUTTON_FOREGROUND),
                XmNbackground,          hci_get_read_color (BUTTON_BACKGROUND),
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

        XtAddCallback (File_button,
                XmNactivateCallback, File_button_cb, NULL);

        Undo_button = XtVaCreateManagedWidget ("Undo",
                xmPushButtonWidgetClass,        control_rowcol,
                XmNforeground,          hci_get_read_color (BUTTON_FOREGROUND),
                XmNbackground,          hci_get_read_color (BUTTON_BACKGROUND),
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

        XtAddCallback (Undo_button,
                XmNactivateCallback, Undo_button_cb, NULL);

        Download_button = XtVaCreateManagedWidget ("Download",
                xmPushButtonWidgetClass,        control_rowcol,
                XmNforeground,          hci_get_read_color (BUTTON_FOREGROUND),
                XmNbackground,          hci_get_read_color (BUTTON_BACKGROUND),
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

        XtAddCallback (Download_button,
                XmNactivateCallback, Download_button_cb, NULL);

        XtVaCreateManagedWidget ("  Baseline:",
                xmLabelWidgetClass,     control_rowcol,
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

        Restore_button = XtVaCreateManagedWidget ("Restore",
                xmPushButtonWidgetClass,        control_rowcol,
                XmNforeground,          hci_get_read_color (BUTTON_FOREGROUND),
                XmNbackground,          hci_get_read_color (BUTTON_BACKGROUND),
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

        XtAddCallback (Restore_button,
                XmNactivateCallback, Restore_button_cb, NULL);

        Update_button = XtVaCreateManagedWidget ("Update",
                xmPushButtonWidgetClass,        control_rowcol,
                XmNforeground,          hci_get_read_color (BUTTON_FOREGROUND),
                XmNbackground,          hci_get_read_color (BUTTON_BACKGROUND),
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

        XtAddCallback (Update_button,
                XmNactivateCallback, Update_button_cb, NULL);

	label = XtVaCreateManagedWidget( "                                          ",
                xmLabelWidgetClass,     control_rowcol,
                XmNforeground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

        lock_widget = hci_lock_widget( control_rowcol, Lock_cb, HCI_LOCA_URC | HCI_LOCA_ROC );

/*	Define area for downloaded file name and downloaded time. */

	download_info_rowcol = XtVaCreateManagedWidget( "download_info_rowcol",
                xmRowColumnWidgetClass, form,
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNorientation,         XmHORIZONTAL,
                XmNcolumns,             1,
                XmNpacking,             XmPACK_TIGHT,
                XmNtopAttachment,       XmATTACH_WIDGET,
                XmNtopWidget,           control_rowcol,
                XmNleftAttachment,      XmATTACH_FORM,
                XmNrightAttachment,     XmATTACH_FORM,
                XmNmarginWidth,         5,
                NULL);

	label = XtVaCreateManagedWidget( "    Last Downloaded File: ",
                xmLabelWidgetClass,     download_info_rowcol,
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

	Last_downloaded_file_label = XtVaCreateManagedWidget( "file_download_name_label        ",
                xmLabelWidgetClass,     download_info_rowcol,
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR2),
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

	label = XtVaCreateManagedWidget( " ",
                xmLabelWidgetClass,     download_info_rowcol,
                XmNforeground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

        label = XtVaCreateManagedWidget( "Time Last Downloaded: ",
                xmLabelWidgetClass,     download_info_rowcol,
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

        Time_last_downloaded_label = XtVaCreateManagedWidget( "file_download_label",
                xmLabelWidgetClass,     download_info_rowcol,
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (WHITE),
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

        Set_download_info_label();

/*	Define area for file name, modified time, and download time. */

	file_info_rowcol = XtVaCreateManagedWidget( "file_info_rowcol",
                xmRowColumnWidgetClass, form,
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNorientation,         XmHORIZONTAL,
                XmNcolumns,             1,
                XmNpacking,             XmPACK_TIGHT,
                XmNtopAttachment,       XmATTACH_WIDGET,
                XmNtopWidget,           download_info_rowcol,
                XmNleftAttachment,      XmATTACH_FORM,
                XmNrightAttachment,     XmATTACH_FORM,
                XmNmarginWidth,         5,
                NULL);

	label = XtVaCreateManagedWidget( "Currently Displayed File: ",
                xmLabelWidgetClass,     file_info_rowcol,
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

	Currently_displayed_file_label = XtVaCreateManagedWidget( "file_name_label                 ",
                xmLabelWidgetClass,     file_info_rowcol,
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR2),
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

	label = XtVaCreateManagedWidget( " ",
                xmLabelWidgetClass,     file_info_rowcol,
                XmNforeground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

        label = XtVaCreateManagedWidget( "Time Last Modified:   ",
                xmLabelWidgetClass,     file_info_rowcol,
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

        Time_last_modified_label = XtVaCreateManagedWidget( "file_modified_label",
                xmLabelWidgetClass,     file_info_rowcol,
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (WHITE),
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

        Set_file_info_label();

/*      Define the main drawing area widget for displaying background  *
 *      and region data in the left 2/3 and the resultant clutter       *
 *      filter map in the right 1/3.                  */

        clutter_rowcol = XtVaCreateManagedWidget ("clutter_rowcol",
                xmRowColumnWidgetClass, form,
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNorientation,         XmHORIZONTAL,
                XmNcolumns,             1,
                XmNpacking,             XmPACK_TIGHT,
                XmNtopAttachment,       XmATTACH_WIDGET,
                XmNtopWidget,           file_info_rowcol,
                XmNleftAttachment,      XmATTACH_FORM,
                XmNrightAttachment,     XmATTACH_FORM,
                NULL);

        actions.string = "Mouse_input";
        actions.proc   = (XtActionProc) Mouse_input;
        XtAppAddActions (HCI_get_appcontext(), &actions, 1);

        draw = XtVaCreateWidget ("clutter_drawing_area",
                xmDrawingAreaWidgetClass,       clutter_rowcol,
                XmNwidth,               Drawing_area_width,
                XmNheight,              Drawing_area_height,
                XmNbackground,          hci_get_read_color (PRODUCT_BACKGROUND_COLOR),
                XmNleftAttachment,      XmATTACH_FORM,
                XmNrightAttachment,     XmATTACH_FORM,
                XmNtopAttachment,       XmATTACH_WIDGET,
                XmNtopWidget,           control_frame,
                XmNtranslations,        XtParseTranslationTable (Clutter_translations),
                NULL);

        XtAddCallback (draw,
                XmNexposeCallback, Draw_area_expose_cb, NULL);

        XtManageChild (draw);

/*      Frame the elements of each segment.                             */

        list_frame = XtVaCreateManagedWidget ("clutter_list_frame",
                xmFrameWidgetClass,     form,
                XmNtopAttachment,       XmATTACH_WIDGET,
                XmNtopWidget,           clutter_rowcol,
                XmNleftAttachment,      XmATTACH_FORM,
                XmNrightAttachment,     XmATTACH_FORM,
                NULL);

/*      Use a form to manage the wodgets for this segment.              */

        list_form = XtVaCreateWidget ("clutter_form",
                xmFormWidgetClass,      list_frame,
                XmNforeground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                NULL);

/*      Use a rowcolumn widget to manage any control buttons.           */

        list1_rowcol = XtVaCreateWidget ("clutter_list1_rowcol",
                xmRowColumnWidgetClass, list_form,
                XmNorientation,         XmHORIZONTAL,
                XmNnumColumns,          1,
                XmNmarginHeight,        1,
                XmNpacking,             XmPACK_TIGHT,
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNtopAttachment,       XmATTACH_FORM,
                XmNleftAttachment,      XmATTACH_FORM,
                XmNrightAttachment,     XmATTACH_FORM,
                NULL);

        XtVaCreateManagedWidget ("Segment:",
                xmLabelWidgetClass,     list1_rowcol,
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNfontList,            hci_get_fontlist (LIST),
                XmNalignment,           XmALIGNMENT_CENTER,
                NULL);

        n = 0;

        XtSetArg (arg [n], XmNforeground,       hci_get_read_color (TEXT_FOREGROUND)); n++;
        XtSetArg (arg [n], XmNbackground,       hci_get_read_color (BACKGROUND_COLOR1)); n++;
        XtSetArg (arg [n], XmNfontList, hci_get_fontlist (LIST)); n++;
        XtSetArg (arg [n], XmNorientation,      XmHORIZONTAL); n++;

        segment = XmCreateRadioBox (list1_rowcol,
                "segment_filter", arg, n);

        Segment1_button = XtVaCreateManagedWidget ("1",
                xmToggleButtonWidgetClass,      segment,
                XmNselectColor,         hci_get_read_color (WHITE),
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNset,                 True,
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

        XtAddCallback (Segment1_button,
                XmNvalueChangedCallback, Select_segment_cb,
                (XtPointer) HCI_CCZ_SEGMENT_ONE);

        Segment2_button = XtVaCreateManagedWidget ("2",
                xmToggleButtonWidgetClass,      segment,
                XmNselectColor,         hci_get_read_color (WHITE),
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNset,                 False,
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

        XtAddCallback (Segment2_button,
                XmNvalueChangedCallback, Select_segment_cb,
                (XtPointer) HCI_CCZ_SEGMENT_TWO);

        Segment3_button = XtVaCreateManagedWidget ("3",
                xmToggleButtonWidgetClass,      segment,
                XmNselectColor,         hci_get_read_color (WHITE),
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNset,                 False,
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

        XtAddCallback (Segment3_button,
                XmNvalueChangedCallback, Select_segment_cb,
                (XtPointer) HCI_CCZ_SEGMENT_THREE);

        Segment4_button = XtVaCreateManagedWidget ("4",
                xmToggleButtonWidgetClass,      segment,
                XmNselectColor,         hci_get_read_color (WHITE),
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNset,                 False,
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

        XtAddCallback (Segment4_button,
                XmNvalueChangedCallback, Select_segment_cb,
                (XtPointer) HCI_CCZ_SEGMENT_FOUR);

        Segment5_button = XtVaCreateManagedWidget ("5",
                xmToggleButtonWidgetClass,      segment,
                XmNselectColor,         hci_get_read_color (WHITE),
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNset,                 False,
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

        XtAddCallback (Segment5_button,
                XmNvalueChangedCallback, Select_segment_cb,
                (XtPointer) HCI_CCZ_SEGMENT_FIVE);

        Hci_set_segment_button_sensitivities();

        XtManageChild (segment);

        XtVaCreateManagedWidget ("    Mode:",
                xmLabelWidgetClass,     list1_rowcol,
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNfontList,            hci_get_fontlist (LIST),
                XmNalignment,           XmALIGNMENT_CENTER,
                NULL);

        n = 0;

        XtSetArg (arg [n], XmNforeground,       hci_get_read_color (TEXT_FOREGROUND)); n++;
        XtSetArg (arg [n], XmNbackground,       hci_get_read_color (BACKGROUND_COLOR1)); n++;
        XtSetArg (arg [n], XmNfontList, hci_get_fontlist (LIST)); n++;
        XtSetArg (arg [n], XmNorientation,      XmHORIZONTAL); n++;

        mode = XmCreateRadioBox (list1_rowcol,
                "mode", arg, n);

        zoom_button = XtVaCreateManagedWidget ("Zoom",
                xmToggleButtonWidgetClass,      mode,
                XmNselectColor,         hci_get_read_color (WHITE),
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNfontList,            hci_get_fontlist (LIST),
                XmNset,                 False,
                NULL);

        XtAddCallback (zoom_button,
                XmNvalueChangedCallback, Select_mode_cb,
                (XtPointer) HCI_CCZ_ZOOM_MODE);

        sector_button = XtVaCreateManagedWidget ("Sector",
                xmToggleButtonWidgetClass,      mode,
                XmNselectColor,         hci_get_read_color (WHITE),
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNset,                 True,
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

        XtAddCallback (sector_button,
                XmNvalueChangedCallback, Select_mode_cb,
                (XtPointer) HCI_CCZ_SECTOR_MODE);

        XtManageChild (mode);

        XtVaCreateManagedWidget ("  ",
                xmLabelWidgetClass,     list1_rowcol,
                XmNforeground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

        reset_zoom_button = XtVaCreateManagedWidget ("Reset Zoom",
                xmPushButtonWidgetClass,        list1_rowcol,
                XmNforeground,          hci_get_read_color (BUTTON_FOREGROUND),
                XmNbackground,          hci_get_read_color (BUTTON_BACKGROUND),
                XmNfontList,            hci_get_fontlist (LIST),
                XmNalignment,           XmALIGNMENT_CENTER,
                NULL);

        XtAddCallback (reset_zoom_button,
                XmNactivateCallback, Reset_zoom_cb, NULL);

        XtVaCreateManagedWidget ("   Units",
                xmLabelWidgetClass,     list1_rowcol,
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

        n = 0;

        XtSetArg (arg [n], XmNforeground,       hci_get_read_color (TEXT_FOREGROUND)); n++;
        XtSetArg (arg [n], XmNbackground,       hci_get_read_color (BACKGROUND_COLOR1)); n++;
        XtSetArg (arg [n], XmNfontList, hci_get_fontlist (LIST)); n++;
        XtSetArg (arg [n], XmNorientation,      XmHORIZONTAL); n++;

        units = XmCreateRadioBox (list1_rowcol,
                "units_filter", arg, n);

        km_button = XtVaCreateManagedWidget ("km",
                xmToggleButtonWidgetClass,      units,
                XmNselectColor,         hci_get_read_color (WHITE),
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNset,                 False,
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

        XtAddCallback (km_button,
                XmNvalueChangedCallback, Select_units_cb,
                (XtPointer) UNITS_KM);

        nm_button = XtVaCreateManagedWidget ("nm",
                xmToggleButtonWidgetClass,      units,
                XmNselectColor,         hci_get_read_color (WHITE),
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNset,                 True,
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

        XtAddCallback (nm_button,
                XmNvalueChangedCallback, Select_units_cb,
                (XtPointer) UNITS_NM);

        XtManageChild (units);

        XtManageChild (list1_rowcol);

        list2_frame = XtVaCreateManagedWidget ("clutter_list2_frame",
                xmFrameWidgetClass,     list_form,
                XmNtopAttachment,       XmATTACH_WIDGET,
                XmNtopWidget,           list1_rowcol,
                XmNleftAttachment,      XmATTACH_FORM,
                XmNrightAttachment,     XmATTACH_FORM,
                NULL);

        list2_rowcol = XtVaCreateWidget ("clutter_list1_rowcol",
                xmRowColumnWidgetClass, list2_frame,
                XmNorientation,         XmHORIZONTAL,
                XmNnumColumns,          1,
                XmNmarginHeight,        1,
                XmNpacking,             XmPACK_TIGHT,
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                NULL);

        XtVaCreateManagedWidget ("Background: ",
                xmLabelWidgetClass,     list2_rowcol,
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNfontList,            hci_get_fontlist (LIST),
                XmNalignment,           XmALIGNMENT_CENTER,
                NULL);

        n = 0;

        XtSetArg (arg [n], XmNforeground,       hci_get_read_color (TEXT_FOREGROUND)); n++;
        XtSetArg (arg [n], XmNbackground,       hci_get_read_color (BACKGROUND_COLOR1)); n++;
        XtSetArg (arg [n], XmNfontList, hci_get_fontlist (LIST)); n++;
        XtSetArg (arg [n], XmNpacking,  XmPACK_TIGHT); n++;
        XtSetArg (arg [n], XmNorientation,      XmHORIZONTAL); n++;

        background = XmCreateRadioBox (list2_rowcol,
                "background_display", arg, n);

        Background_product_button = XtVaCreateManagedWidget ("Reflectivity",
                xmToggleButtonWidgetClass,      background,
                XmNselectColor,         hci_get_read_color (WHITE),
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNset,                 True,
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

        XtAddCallback (Background_product_button,
                XmNvalueChangedCallback, Background_product_type_cb,
                (XtPointer) HCI_CCZ_BACKGROUND_BREF);

        CFC_product_button = XtVaCreateManagedWidget ("CFC Product",
                xmToggleButtonWidgetClass,      background,
                XmNselectColor,         hci_get_read_color (WHITE),
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNset,                 False,
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

        XtAddCallback (CFC_product_button,
                XmNvalueChangedCallback, Background_product_type_cb,
                (XtPointer) HCI_CCZ_BACKGROUND_CFC);

        Refresh_button = XtVaCreateManagedWidget ("Refresh",
                xmPushButtonWidgetClass,      list2_rowcol,
                XmNforeground,          hci_get_read_color (BUTTON_FOREGROUND),
                XmNbackground,          hci_get_read_color (BUTTON_BACKGROUND),
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

        XtAddCallback (Refresh_button,
                XmNactivateCallback, Refresh_button_cb, NULL);

        XtManageChild (background);
        XtManageChild (list2_rowcol);

/*      Create a row for defining the background product to be          *
 *      displayed beneath the clutter regions graphical data.           */

        Background_product_rowcol = XtVaCreateWidget ("Background_product_rowcol",
                xmRowColumnWidgetClass, list_form,
                XmNorientation,         XmHORIZONTAL,
                XmNnumColumns,          1,
                XmNmarginHeight,        1,
                XmNpacking,             XmPACK_TIGHT,
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNleftAttachment,      XmATTACH_FORM,
                XmNrightAttachment,     XmATTACH_FORM,
                XmNtopAttachment,       XmATTACH_WIDGET,
                XmNtopWidget,           list2_frame,
                NULL);

        XtVaCreateManagedWidget ("Change Background Product",
                xmLabelWidgetClass,     Background_product_rowcol,
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNfontList,            hci_get_fontlist (LIST),
                XmNalignment,           XmALIGNMENT_CENTER,
                NULL);

/*      Use a combo box widget to handle the background product menu.   */

        Background_product = XtVaCreateWidget ("background_product_list",
                xmComboBoxWidgetClass,  Background_product_rowcol,
                XmNforeground,          hci_get_read_color (BUTTON_FOREGROUND),
                XmNbackground,          hci_get_read_color (BUTTON_BACKGROUND),
                XmNfontList,            hci_get_fontlist (LIST),
                XmNcomboBoxType,        XmDROP_DOWN_LIST,
                XmNcolumns,             HCI_BUF_64,
                NULL);

        XtVaGetValues (Background_product,
                XmNlist,        &Background_product_list,
                XmNtextField,   &Background_product_text,
                NULL);

/*      Only allow 5 items to be displayed at a time in the menu.       */

        XtVaSetValues (Background_product_list,
                XmNvisibleItemCount,    5,
                NULL);

        XtAddCallback (Background_product,
                XmNselectionCallback, Background_product_id_cb, NULL);

/*      Build the background product menu.  Get all of the products     *
 *      for the menu from the configuration data for this task.         */

        buf = (char *) calloc (1, ALIGNED_SIZE(sizeof(Hci_ccz_data_t)));

        status = ORPGDA_read (ORPGDAT_HCI_DATA,
                              buf,
                              ALIGNED_SIZE(sizeof(Hci_ccz_data_t)),
                              HCI_CCZ_TASK_DATA_MSG_ID);

        if (status <= 0) {

            HCI_LE_error("Unable to read task configuration data (%d)", status);

            str = XmStringCreateLocalized ("No products defined");

            XmListAddItemUnselected (Background_product_list, str, 0);

            XtSetSensitive( Background_product, False );

            XmStringFree (str);

        } else {

            config_data = (Hci_ccz_data_t *) buf;

            if (HCI_is_low_bandwidth())
                Clutter_display_background_product_id = ORPGPAT_get_prod_id_from_code ((int) config_data->low_product);
            else
                Clutter_display_background_product_id = ORPGPAT_get_prod_id_from_code ((int) config_data->high_product);

            for (i=0;i<config_data->n_products;i++) {

                prod_id = ORPGPAT_get_prod_id_from_code (config_data->product_list[i]);
                Clutter_display_background_product_LUT[i] = prod_id;

                if (prod_id < 0)
                        continue;

                sprintf (Buff,"%s[%d] - %-50s   ",
                        ORPGPAT_get_mnemonic (prod_id),
                        ORPGPAT_get_code (prod_id),
                        ORPGPAT_get_description (prod_id, STRIP_MNEMONIC));

                str = XmStringCreateLocalized (Buff);

                XmListAddItemUnselected (Background_product_list, str, 0);

                if (Clutter_display_background_product_id == prod_id)
                        XmTextSetString (Background_product_text, Buff);

                XmStringFree (str);

            }
        }

/*      Format a new label to the background product radio button.      */

        sprintf (Buff,"%s[%d] - %-50s   ",
                ORPGPAT_get_mnemonic (Clutter_display_background_product_id),
                ORPGPAT_get_code (Clutter_display_background_product_id),
                ORPGPAT_get_description (Clutter_display_background_product_id, STRIP_MNEMONIC));

        str = XmStringCreateLocalized (Buff);

        XtVaSetValues (Background_product_button,
                XmNlabelString,         str,
                NULL);

        XmStringFree (str);

        XtManageChild (Background_product);
        XtManageChild (Background_product_rowcol);

/*      Create a row to take the place of the Background_product_rowcol. */
/*      If the CFC product is chosen, then the area that houses the      */
/*      Background_product_rowcol should become blank. In legacy, this   */
/*      area contained buttons to choose channel type. In ORDA, there are*/
/*      no channel types to choose from, thus the area is blank.         */

        Cfc_rowcol = XtVaCreateWidget ("Cfc_rowcol",
                xmRowColumnWidgetClass, list_form,
                XmNorientation,         XmHORIZONTAL,
                XmNnumColumns,          1,
                XmNmarginHeight,        1,
                XmNpacking,             XmPACK_TIGHT,
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNleftAttachment,      XmATTACH_FORM,
                XmNrightAttachment,     XmATTACH_FORM,
                XmNtopAttachment,       XmATTACH_WIDGET,
                XmNtopWidget,           list2_frame,
                NULL);

/*      Draw a frame around the contents of the segment.                */

        low_segment_frame = XtVaCreateManagedWidget ("low_segment_frame",
                xmFrameWidgetClass,     list_form,
                XmNtopAttachment,       XmATTACH_WIDGET,
                XmNtopWidget,           Background_product_rowcol,
                XmNleftAttachment,      XmATTACH_FORM,
                XmNrightAttachment,     XmATTACH_FORM,
                XmNbottomAttachment,    XmATTACH_FORM,
                NULL);

        segment_form = XtVaCreateWidget ("low_segment_form",
                xmFormWidgetClass,      low_segment_frame,
                XmNforeground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                NULL);

        Delete_button = XtVaCreateManagedWidget ("Delete",
                xmPushButtonWidgetClass,        segment_form,
                XmNforeground,          hci_get_read_color (BUTTON_FOREGROUND),
                XmNbackground,          hci_get_read_color (BUTTON_BACKGROUND),
                XmNfontList,            hci_get_fontlist (LIST),
                XmNalignment,           XmALIGNMENT_CENTER,
                XmNtopAttachment,       XmATTACH_FORM,
                XmNleftAttachment,      XmATTACH_FORM,
                NULL);

        XtAddCallback (Delete_button,
                XmNactivateCallback, Delete_button_cb, NULL);

/*      Label the items in the segment.                                 */

        Segment_label = XtVaCreateManagedWidget ("Elevation Segment 1",
                xmLabelWidgetClass,     segment_form,
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNtopAttachment,       XmATTACH_FORM,
                XmNleftAttachment,      XmATTACH_WIDGET,
                XmNleftWidget,          Delete_button,
                XmNrightAttachment,     XmATTACH_FORM,
                XmNalignment,           XmALIGNMENT_CENTER,
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

        label_rowcol = XtVaCreateWidget ("low_elevation_rowcol",
                xmRowColumnWidgetClass, segment_form,
                XmNorientation,         XmHORIZONTAL,
                XmNnumColumns,          1,
                XmNpacking,             XmPACK_TIGHT,
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNspacing,             1,
                XmNmarginWidth,         0,
                XmNtopAttachment,       XmATTACH_WIDGET,
                XmNtopWidget,           Delete_button,
                XmNleftAttachment,      XmATTACH_FORM,
                XmNrightAttachment,     XmATTACH_FORM,
                NULL);

        sprintf (Buff," Region ");
        text = XtVaCreateManagedWidget ("Region",
                xmTextFieldWidgetClass, label_rowcol,
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNmarginHeight,        2,
                XmNshadowThickness,     2,
                XmNborderWidth,         0,
                XmNmarginWidth,         0,
                XmNhighlightThickness,  1,
                XmNcolumns,             8,
                XmNfontList,            hci_get_fontlist (LIST),
                XmNeditable,            False,
                NULL);

        XmTextSetString (text,Buff);

        sprintf (Buff," Start Azimuth (deg)");
        text = XtVaCreateManagedWidget ("Azimuth 1",
                xmTextFieldWidgetClass, label_rowcol,
                XmNforeground,          hci_get_read_color (EDIT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNfontList,            hci_get_fontlist (LIST),
                XmNcolumns,             20,
                XmNmarginHeight,        2,
                XmNshadowThickness,     2,
                XmNmarginWidth,         0,
                XmNborderWidth,         0,
                XmNeditable,            False,
                NULL);

        XmTextSetString (text,Buff);

        sprintf (Buff," Stop Azimuth (deg)");
        text = XtVaCreateManagedWidget ("Azimuth 2",
                xmTextFieldWidgetClass, label_rowcol,
                XmNforeground,          hci_get_read_color (EDIT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNfontList,            hci_get_fontlist (LIST),
                XmNcolumns,             19,
                XmNmarginHeight,        2,
                XmNshadowThickness,     2,
                XmNmarginWidth,         0,
                XmNborderWidth,         0,
                XmNeditable,            False,
                NULL);

        XmTextSetString (text,Buff);

        if (Units_factor == HCI_KM_TO_KM) {

            sprintf (Buff," Start Range (km)");

        } else {

            sprintf (Buff," Start Range (nm)");

        }

        Start_range_text_box = XtVaCreateManagedWidget ("Range 1",
                xmTextFieldWidgetClass, label_rowcol,
                XmNforeground,          hci_get_read_color (EDIT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNfontList,            hci_get_fontlist (LIST),
                XmNcolumns,             17,
                XmNmarginHeight,        2,
                XmNshadowThickness,     2,
                XmNmarginWidth,         0,
                XmNborderWidth,         0,
                XmNeditable,            False,
                NULL);

        XmTextSetString (Start_range_text_box,Buff);

        if (Units_factor == HCI_KM_TO_KM) {

            sprintf (Buff," Stop Range (km)");

        } else {

            sprintf (Buff," Stop Range (nm)");

        }

        End_range_text_box = XtVaCreateManagedWidget ("Range 2",
                xmTextFieldWidgetClass, label_rowcol,
                XmNforeground,          hci_get_read_color (EDIT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNfontList,            hci_get_fontlist (LIST),
                XmNcolumns,             16,
                XmNmarginHeight,        2,
                XmNshadowThickness,     2,
                XmNmarginWidth,         0,
                XmNborderWidth,         0,
                XmNeditable,            False,
                NULL);

        XmTextSetString (End_range_text_box,Buff);

        Select_code_button = XtVaCreateManagedWidget (" Select Code ",
                xmPushButtonWidgetClass,        label_rowcol,
                XmNforeground,          hci_get_read_color (EDIT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNfontList,            hci_get_fontlist (LIST),
                XmNalignment,           XmALIGNMENT_CENTER,
                XmNmarginHeight,        3,
                XmNshadowThickness,     1,
                XmNborderWidth,         0,
                XmNhighlightThickness,  1,
                NULL);

        XtVaGetValues (Select_code_button,
                XmNwidth,       &select_button_width,
                NULL);

        XtManageChild (label_rowcol);

/*      Use a scrolled window to manage the elements of the segment.    */

        Segment_scroll = XtVaCreateManagedWidget ("clutter_scroll",
                xmScrolledWindowWidgetClass,    segment_form,
                XmNheight,              136,
                XmNscrollingPolicy,     XmAUTOMATIC,
                XmNforeground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNtopAttachment,       XmATTACH_WIDGET,
                XmNtopWidget,           label_rowcol,
                XmNleftAttachment,      XmATTACH_FORM,
                XmNrightAttachment,     XmATTACH_FORM,
                XmNbottomAttachment,    XmATTACH_FORM,
                NULL);

        low_form = XtVaCreateWidget ("low_segment_form",
                xmFormWidgetClass,      Segment_scroll,
                XmNforeground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNhorizontalSpacing,   1,
                XmNverticalSpacing,     1,
                XmNtopAttachment,       XmATTACH_FORM,
                XmNbottomAttachment,    XmATTACH_FORM,
                XmNleftAttachment,      XmATTACH_FORM,
                XmNrightAttachment,     XmATTACH_FORM,
                NULL);

        {
                Widget  hsb;
                Widget  clip;

                XtVaGetValues (Segment_scroll,
                        XmNhorizontalScrollBar, &hsb,
                        XmNclipWindow,          &clip,
                        NULL);

                XtVaSetValues (clip,
                        XmNforeground,  hci_get_read_color (BACKGROUND_COLOR1),
                        XmNbackground,  hci_get_read_color (BACKGROUND_COLOR1),
                        NULL);

                XtUnmanageChild (hsb);
        }

        for (i=0;i<MAX_NUMBER_CLUTTER_ZONES;i++) {

            if (i == 0) {

                Edit_table_rowcol [i] = XtVaCreateWidget ("low_elevation_rowcol",
                        xmRowColumnWidgetClass, low_form,
                        XmNorientation,         XmHORIZONTAL,
                        XmNnumColumns,          1,
                        XmNpacking,             XmPACK_TIGHT,
                        XmNbackground,          hci_get_read_color (BLACK),
                        XmNspacing,             1,
                        XmNmarginWidth,         0,
                        XmNmarginHeight,        0,
                        XmNtopAttachment,       XmATTACH_FORM,
                        XmNleftAttachment,      XmATTACH_FORM,
                        NULL);

            } else {

                Edit_table_rowcol [i] = XtVaCreateWidget ("low_elevation_rowcol",
                        xmRowColumnWidgetClass, low_form,
                        XmNorientation,         XmHORIZONTAL,
                        XmNnumColumns,          1,
                        XmNpacking,             XmPACK_TIGHT,
                        XmNbackground,          hci_get_read_color (BLACK),
                        XmNspacing,             1,
                        XmNmarginWidth,         0,
                        XmNmarginHeight,        0,
                        XmNtopAttachment,       XmATTACH_WIDGET,
                        XmNtopWidget,           Edit_table_rowcol [i-1],
                        XmNleftAttachment,      XmATTACH_FORM,
                        NULL);

            }

            Edit_table_widgets [i].region = XtVaCreateManagedWidget ("",
                xmPushButtonWidgetClass,        Edit_table_rowcol [i],
                XmNforeground,          hci_get_read_color (BUTTON_FOREGROUND),
                XmNbackground,          hci_get_read_color (BUTTON_BACKGROUND),
                XmNuserData,            (XtPointer) -1,
                XmNmarginHeight,        3,
                XmNshadowThickness,     1,
                XmNborderWidth,         0,
                XmNhighlightThickness,  1,
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

            XtAddCallback (Edit_table_widgets [i].region,
                        XmNactivateCallback, Select_region_cb,
                        (XtPointer) NULL);

            sprintf (Buff," %4d  ",i+1);
            str = XmStringCreateLocalized (Buff);
            XtVaSetValues (Edit_table_widgets [i].region,
                XmNlabelString, str,
                NULL);

            XmStringFree (str);

            Edit_table_widgets [i].start_azimuth = XtVaCreateManagedWidget ("",
                xmTextFieldWidgetClass, Edit_table_rowcol [i],
                XmNforeground,          hci_get_read_color (EDIT_FOREGROUND),
                XmNbackground,          hci_get_read_color (EDIT_BACKGROUND),
                XmNcolumns,             20,
                XmNmarginHeight,        2,
                XmNshadowThickness,     2,
                XmNmarginWidth,         0,
                XmNborderWidth,         0,
                XmNhighlightOnEnter,    True,
                XmNuserData,            (XtPointer) -1,
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

            XtAddCallback (Edit_table_widgets [i].start_azimuth,
                XmNmodifyVerifyCallback, hci_verify_unsigned_integer_callback,
                (XtPointer) 3);
            XtAddCallback (Edit_table_widgets [i].start_azimuth,
                XmNfocusCallback, hci_gain_focus_callback,
                NULL);
            XtAddCallback (Edit_table_widgets [i].start_azimuth,
                XmNlosingFocusCallback, Change_azimuth_cb,
                (XtPointer) HCI_CCZ_START_AZIMUTH);
            XtAddCallback (Edit_table_widgets [i].start_azimuth,
                XmNactivateCallback, Change_azimuth_cb,
                (XtPointer) HCI_CCZ_START_AZIMUTH);

            Edit_table_widgets [i].stop_azimuth = XtVaCreateManagedWidget ("",
                xmTextFieldWidgetClass, Edit_table_rowcol [i],
                XmNforeground,          hci_get_read_color (EDIT_FOREGROUND),
                XmNbackground,          hci_get_read_color (EDIT_BACKGROUND),
                XmNcolumns,             19,
                XmNmarginHeight,        2,
                XmNshadowThickness,     2,
                XmNmarginWidth,         0,
                XmNborderWidth,         0,
                XmNhighlightOnEnter,    True,
                XmNuserData,            (XtPointer) -1,
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

            XtAddCallback (Edit_table_widgets [i].stop_azimuth,
                XmNmodifyVerifyCallback, hci_verify_unsigned_integer_callback,
                (XtPointer) 3);
            XtAddCallback (Edit_table_widgets [i].stop_azimuth,
                XmNfocusCallback, hci_gain_focus_callback,
                NULL);
            XtAddCallback (Edit_table_widgets [i].stop_azimuth,
                XmNlosingFocusCallback, Change_azimuth_cb,
                (XtPointer) HCI_CCZ_STOP_AZIMUTH);
            XtAddCallback (Edit_table_widgets [i].stop_azimuth,
                XmNactivateCallback, Change_azimuth_cb,
                (XtPointer) HCI_CCZ_STOP_AZIMUTH);

            Edit_table_widgets [i].start_range = XtVaCreateManagedWidget ("",
                xmTextFieldWidgetClass, Edit_table_rowcol [i],
                XmNforeground,          hci_get_read_color (EDIT_FOREGROUND),
                XmNbackground,          hci_get_read_color (EDIT_BACKGROUND),
                XmNcolumns,             17,
                XmNmarginHeight,        2,
                XmNshadowThickness,     2,
                XmNmarginWidth,         0,
                XmNborderWidth,         0,
                XmNhighlightOnEnter,    True,
                XmNuserData,            (XtPointer) -1,
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

            XtAddCallback (Edit_table_widgets [i].start_range,
                XmNmodifyVerifyCallback, hci_verify_unsigned_integer_callback,
                (XtPointer) 3);
            XtAddCallback (Edit_table_widgets [i].start_range,
                XmNfocusCallback, hci_gain_focus_callback,
                NULL);
            XtAddCallback (Edit_table_widgets [i].start_range,
                XmNlosingFocusCallback, Change_range_cb,
                (XtPointer) HCI_CCZ_START_RANGE);
            XtAddCallback (Edit_table_widgets [i].start_range,
                XmNactivateCallback, Change_range_cb,
                (XtPointer) HCI_CCZ_START_RANGE);

            Edit_table_widgets [i].stop_range = XtVaCreateManagedWidget ("",
                xmTextFieldWidgetClass, Edit_table_rowcol [i],
                XmNforeground,          hci_get_read_color (EDIT_FOREGROUND),
                XmNbackground,          hci_get_read_color (EDIT_BACKGROUND),
                XmNcolumns,             16,
                XmNmarginHeight,        2,
                XmNshadowThickness,     2,
                XmNmarginWidth,         0,
                XmNborderWidth,         0,
                XmNhighlightOnEnter,    True,
                XmNuserData,            (XtPointer) -1,
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

            XtAddCallback (Edit_table_widgets [i].stop_range,
                XmNmodifyVerifyCallback, hci_verify_unsigned_integer_callback,
                (XtPointer) 3);
            XtAddCallback (Edit_table_widgets [i].stop_range,
                XmNfocusCallback, hci_gain_focus_callback,
                NULL);
            XtAddCallback (Edit_table_widgets [i].stop_range,
                XmNlosingFocusCallback, Change_range_cb,
                (XtPointer) HCI_CCZ_STOP_RANGE);
            XtAddCallback (Edit_table_widgets [i].stop_range,
                XmNactivateCallback, Change_range_cb,
                (XtPointer) HCI_CCZ_STOP_RANGE);

            Edit_table_widgets [i].filter = XtVaCreateManagedWidget ("Select Code",
                xmPushButtonWidgetClass,Edit_table_rowcol [i],
                XmNforeground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNwidth,               select_button_width,
                XmNrecomputeSize,       False,
                XmNuserData,            (XtPointer) -1,
                XmNmarginHeight,        3,
                XmNshadowThickness,     1,
                XmNborderWidth,         0,
                XmNhighlightThickness,  1,
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

            XtAddCallback (Edit_table_widgets [i].filter,
                        XmNactivateCallback, Select_filter_cb,
                        NULL);

            XtManageChild (Edit_table_rowcol [i]);

        }

        Update_clutter_regions_table();

        XtManageChild (low_form);
        XtManageChild (segment_form);
        XtManageChild (list_form);

        XtManageChild (form);

/*      Realize top-level widget to define variables
        needed in the following sections of code. */

        XtRealizeWidget (Top_widget);

/*      Define X Window properties of Clutter Drawing area              */

        Clutter_window  = XtWindow (draw);

        Clutter_pixmap  = XCreatePixmap (Clutter_display,
                          Clutter_window,
                          Drawing_area_width,
                          Drawing_area_height,
                          Drawing_area_depth);

/*      Initialize the base data pixmap by writing the current contents *
 *      of the base data window to it.                                  */

        gcv.foreground = hci_get_read_color (PRODUCT_FOREGROUND_COLOR);
        gcv.background = hci_get_read_color (PRODUCT_BACKGROUND_COLOR);
        gcv.line_width = 2;
        gcv.graphics_exposures = FALSE;

        Clutter_gc = XCreateGC (Clutter_display,
                     Clutter_window,
                     GCBackground | GCForeground | GCGraphicsExposures |
                     GCLineWidth,
                     &gcv);

        Hci_set_display_font( SMALL );

/*      Initialize the clutter pixmap by filling it black.              */

        Hci_set_foreground( PRODUCT_BACKGROUND_COLOR );
        Hci_fill_rectangle( 0, 0, Drawing_area_width, Drawing_area_height );

        HCI_PM("Reading Base Reflectivity product");
        
/*      Open the base reflectivity product to be used as a background   *
 *      for defining the clutter censor zones.                          */

        Load_background_product (Clutter_display_background_product_type);
        Display_background_product_and_censor_zones();
        Display_composite_clutter_map ();

/* Register for CCZ DEAU changes. "ccz" is hard-coded, since
   the macro ORPGCCZ_ORDA_ZONES doesn't work. */

        retval = DEAU_UN_register( "ccz", Ccz_deau_cb );

        if( retval < 0 )
        {
          HCI_LE_error("DEAU_UN_register failed: %d", retval);
          HCI_task_exit (HCI_EXIT_FAIL) ;
        }

        Hci_set_control_button_sensitivities();

/* Start HCI loop. */

        HCI_start( Timer_proc, HCI_ONE_SECOND, NO_RESIZE_HCI );

        return 0;
}

/************************************************************************
 *      Description: This function handles expose events.               *
 *                                                                      *
 *      Input:  w - ID of top widget                                    *
 *              client_data - unused                                    *
 *              call_data - unused                                      *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

static void Draw_area_expose_cb( Widget w, XtPointer cl, XtPointer ca )
{
  if( (Clutter_display == (Display *) NULL) ||
      (Clutter_pixmap  == (Pixmap)    NULL) ||
      (Clutter_window  == (Window)    NULL) ||
      (Clutter_gc      == (GC)        NULL) )
  {
    return;
  }

  /* Copy the pixmap to the visible window. */

  Copy_pixmap_to_window();
}


/************************************************************************
 *      Description: This function displays composite clutter maps for  *
 *                   the composite map region.                          *
 *                                                                      *
 *      Input:  NONE                                                    *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

static void Display_composite_clutter_map()
{
  int     beam;
  int     i;
  float   azimuth1, azimuth2;
  int     old_color;
  int     color;
  XPoint  x[12];
  float   sin1, sin2;
  float   cos1, cos2;
  int     center_pixel;
  int     center_scanl;
  int     pixel_radius;
  float   scale_x;
  float   scale_y;
  int max_legend_width, max_string_width;
  int legend_text_x_offset, legend_box_x_offset;
  int legend_y_offset;
  char legend_buf[HCI_BUF_64];
  int font_height;
  int font_y_offset;
  Dimension top_widget_width;

  font_height = Small_fontinfo->ascent + Small_fontinfo->descent;

  /* Set width of Bypass Map area according to width of top widget. */

  XtVaGetValues( Top_widget, XmNwidth, &top_widget_width, NULL );
  if( top_widget_width > Drawing_area_width )
  {
    top_widget_width = Drawing_area_width;
  }
  Composite_display_width = top_widget_width - Clutter_display_width;

  /* Fill Bypass Map area with background color. */

  Hci_set_foreground( BACKGROUND_COLOR1 );
  Hci_fill_rectangle( Clutter_display_width+COLOR_BAR_WIDTH, 0,
                      Composite_display_width, Composite_display_height );

  /* Size of composite circle. Account for color bar. */

  scale_x = (Composite_display_width*0.60)/(HCI_CCZ_GATES*1.75);
  scale_y = -scale_x;
  pixel_radius = scale_x*HCI_CCZ_GATES;
  center_pixel = top_widget_width-(Composite_display_width-COLOR_BAR_WIDTH)/2;
  center_scanl = pixel_radius + LEGEND_SPACE;

  for( beam = 0; beam < HCI_CCZ_RADIALS; beam++ )
  {
    azimuth1 = beam - 0.5;
    azimuth2 = beam + 0.5;

    sin1 = sin( (double) (azimuth1+90)*HCI_DEG_TO_RAD );
    sin2 = sin( (double) (azimuth2+90)*HCI_DEG_TO_RAD );
    cos1 = cos( (double) (azimuth1-90)*HCI_DEG_TO_RAD );
    cos2 = cos( (double) (azimuth2-90)*HCI_DEG_TO_RAD );

    old_color = -1;

    /* Unpack the data at each gate along the beam.  If there is data
       above the lower threshold, display it.  To reduce the number of
       XPolygonFill operations, only paint gate(s) when a color change
       occurs. */

    for( i = 0; i <= HCI_CCZ_GATES; i++ )
    { 
      color = (int) Composite_display_data [beam][i] >> 4;

      switch( color )
      {
        case HCI_CCZ_FILTER_NONE :

          color = Filter_none_color;
          break;

        case HCI_CCZ_FILTER_BYPASS :

          color = Filter_bypass_color;
          break;

        case HCI_CCZ_FILTER_ALL :

          color = Filter_all_bins_color;
          break;

        default:

          color = PRODUCT_FOREGROUND_COLOR;
          break;

      }

      if( color >= 0 )
      {
        /* The value at this gate is not background so process it. */

        if( color != old_color )
        {
          /* If the current value is different from the last
             gate processed, either the last value was back-
             ground or a real value. */

          if( old_color < 0 )
          {
            /* If the last gate was background, then find the
               coordinates of the first two points making up
               the region. */

            x[0].x = i * cos1 * scale_x + center_pixel;
            x[0].y = i * sin1 * scale_y + center_scanl;
            x[1].x = i * cos2 * scale_x + center_pixel;
            x[1].y = i * sin2 * scale_y + center_scanl;
            x[4].x = x[0].x;
            x[4].y = x[0].y;
          }
          else
          {
            /* The last point was not background so find the
               coordinates of the last two points defining the
               region and display it. */

            x[2].x = i * cos2 * scale_x + center_pixel;
            x[2].y = i * sin2 * scale_y + center_scanl;
            x[3].x = i * cos1 * scale_x + center_pixel;
            x[3].y = i * sin1 * scale_y + center_scanl;

            Hci_set_foreground( old_color );
            Hci_fill_polygon( x, 4 );

            x[0].x = x[3].x;
            x[0].y = x[3].y;
            x[1].x = x[2].x;
            x[1].y = x[2].y;
            x[4].x = x[0].x;
            x[4].y = x[0].y;
          }
        }
      }
      else if( old_color >= 0 )
      {
        /* The last gate had a real value so determine the last
           pair of points defining the region and display it. */

        x[2].x = i * cos2 * scale_x + center_pixel;
        x[2].y = i * sin2 * scale_y + center_scanl;
        x[3].x = i * cos1 * scale_x + center_pixel;
        x[3].y = i * sin1 * scale_y + center_scanl;

        Hci_set_foreground( old_color );
        Hci_fill_polygon( x, 4 );

        x[0].x = x[3].x;
        x[0].y = x[3].y;
        x[1].x = x[2].x;
        x[1].y = x[2].y;
        x[4].x = x[0].x;
        x[4].y = x[0].y;
      }

      old_color = color;
    }

    if( color >= 0 )
    {
      /* There are no more gates in the beam but check to see if
         the previous gate had data which needs to be displayed. If
         so, find the last two region coordinates and display it. */

      x[2].x = i * cos2 * scale_x + center_pixel;
      x[2].y = i * sin2 * scale_y + center_scanl;
      x[3].x = i * cos1 * scale_x + center_pixel;
      x[3].y = i * sin1 * scale_y + center_scanl;

      Hci_set_foreground( color );
      Hci_fill_polygon( x, 4 );
    }

    /* Display the segment label beneath the map. */

    Hci_set_foreground( PRODUCT_BACKGROUND_COLOR );

    strcpy( legend_buf, "Clutter Map" );
    max_string_width = XTextWidth( Small_fontinfo,
                                   legend_buf, strlen( legend_buf ) );

    Hci_draw_string( center_pixel-(max_string_width/2),
                     center_scanl+pixel_radius+(1.5*font_height),
                     legend_buf );
  }

  /* Legend color bars below composite circle. Determine offsets
     before drawing anything. The "Bypass Map" label is the longest
     legend label, so use it for the calculations. */

  strcpy( legend_buf, "Bypass Map" );
  max_string_width = XTextWidth( Small_fontinfo,
                                 legend_buf, strlen( legend_buf ) );
  max_legend_width = max_string_width + LEGEND_SPACE + LEGEND_BOX_WIDTH;
  font_y_offset = LEGEND_BOX_HEIGHT - (font_height/2);

  legend_text_x_offset = center_pixel - (max_legend_width/2);
  legend_text_x_offset = center_pixel - max_string_width - LEGEND_SPACE - LEGEND_BOX_WIDTH/2;
  legend_box_x_offset = legend_text_x_offset + max_string_width + LEGEND_SPACE;
  legend_y_offset = LEGEND_BOX_HEIGHT + LEGEND_SPACE;

  /* Bypass Map legend box. */

  Hci_set_foreground( Filter_bypass_color );
  Hci_fill_rectangle( legend_box_x_offset,
                      Composite_display_height-3*legend_y_offset,
                      LEGEND_BOX_WIDTH, LEGEND_BOX_HEIGHT );

  Hci_set_foreground( PRODUCT_FOREGROUND_COLOR );
  Hci_draw_rectangle( legend_box_x_offset,
                      Composite_display_height-3*legend_y_offset,
                      LEGEND_BOX_WIDTH, LEGEND_BOX_HEIGHT );

  /* All Bins legend box. */

  Hci_set_foreground( Filter_all_bins_color );
  Hci_fill_rectangle( legend_box_x_offset,
                      Composite_display_height-2*legend_y_offset,
                      LEGEND_BOX_WIDTH, LEGEND_BOX_HEIGHT );

  Hci_set_foreground( PRODUCT_FOREGROUND_COLOR );
  Hci_draw_rectangle( legend_box_x_offset,
                      Composite_display_height-2*legend_y_offset,
                      LEGEND_BOX_WIDTH, LEGEND_BOX_HEIGHT );

  /* No Filter legend box. */

  Hci_set_foreground( Filter_none_color );
  Hci_fill_rectangle( legend_box_x_offset,
                      Composite_display_height-legend_y_offset,
                      LEGEND_BOX_WIDTH, LEGEND_BOX_HEIGHT );

  Hci_set_foreground( PRODUCT_FOREGROUND_COLOR );
  Hci_draw_rectangle( legend_box_x_offset,
                      Composite_display_height-legend_y_offset,
                      LEGEND_BOX_WIDTH, LEGEND_BOX_HEIGHT );

  Hci_set_foreground( PRODUCT_BACKGROUND_COLOR );

  strcpy( legend_buf, "Bypass Map" );
  Hci_draw_string( legend_text_x_offset,
                   Composite_display_height-(3*legend_y_offset)+font_y_offset,
                   legend_buf );

  strcpy( legend_buf, "All Bins" );
  Hci_draw_string( legend_text_x_offset,
                   Composite_display_height-(2*legend_y_offset)+font_y_offset,
                   legend_buf );

  strcpy( legend_buf, "No Filter" );
  Hci_draw_string( legend_text_x_offset,
                   Composite_display_height-legend_y_offset+font_y_offset,
                   legend_buf );

  /* Display the color code scheme used beneath the maps. */

  Draw_area_expose_cb( NULL, NULL, NULL );
}

/************************************************************************
 *      Description: This function is the callback for the "Close"      *
 *                   button.                                            *
 *                                                                      *
 *      Input:  w - ID of close button                                  *
 *              client_data - unused                                    *
 *              call_data - unused                                      *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

static void Close_button_cb( Widget w, XtPointer cl, XtPointer ca )
{
  HCI_LE_log("Close selected");

  /* If any unsaved edits are detected, display a warning to the
     user and allow them an opportunity to save them first. */

  if( Update_flag == HCI_MODIFIED_FLAG )
  {
    sprintf( Buff, "You did not save your edits.\nDo you want to exit without saving them?" );
    hci_confirm_popup( Top_widget, Buff, Close_button_cb_no_save, NULL );
  }
  else
  {
    HCI_task_exit( HCI_EXIT_SUCCESS );
  }
}


/************************************************************************
 *      Description: This function is activated then the user selects   *
 *                   "Yes" from the Close button warning popup.          *
 *                                                                      *
 *      Input:  w - ID of "Yes" button                                   *
 *              client_data - unused                                    *
 *              call_data - unused                                      *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

static void Close_button_cb_no_save( Widget w, XtPointer cl, XtPointer ca )
{
  HCI_task_exit (HCI_EXIT_SUCCESS);
}

/************************************************************************
 *      Description: This function is activated when the "Delete"       *
 *                   button is selected.  It deletes the currently      *
 *                   highlighted region.                                *
 *                                                                      *
 *      Input:  w - ID of Delete button                                 *
 *              client_data - unused                                    *
 *              call_data - unused                                      *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

static void Delete_button_cb( Widget w, XtPointer cl, XtPointer ca )
{
  int     i;

  /* If there there are clutter regions defined, delete the current one. */

  if( Edit_region >= 0 )
  {
    HCI_LE_log("Deleting region %d", Edit_region);

    /* From the current region, move up all trailing regions one position. */

    for( i = Edit_region; i < Edit_num_regions-1; i++ )
    {
      Edit_data [i].start_azimuth = Edit_data [i+1].start_azimuth;
      Edit_data [i].stop_azimuth  = Edit_data [i+1].stop_azimuth;
      Edit_data [i].start_range   = Edit_data [i+1].start_range;
      Edit_data [i].stop_range    = Edit_data [i+1].stop_range;
      Edit_data [i].segment       = Edit_data [i+1].segment;
      Edit_data [i].select_code   = Edit_data [i+1].select_code;
    }

    /* Unhighlight the button now the entry is deleted. */

     XtVaSetValues( Edit_table_widgets [Edit_row].region,
                    XmNforeground,  hci_get_read_color( BUTTON_FOREGROUND ),
                    XmNbackground,  hci_get_read_color( BUTTON_BACKGROUND ),
                    NULL );

    /* Decrement the number of regions by 1.*/

    Edit_num_regions--;
    Edit_region = -1;
    Update_flag = HCI_MODIFIED_FLAG;
    Clutter_draw();
  }
  else
  {
    sprintf( Buff,"You must select a region before\nit can be deleted!" );
    hci_warning_popup( Top_widget, Buff, NULL );
  }
}

/************************************************************************
 *      Description: This function is activated when the "File"         *
 *                   button is selected.  It opens the Clutter Region   *
 *                   Files window.                                      *
 *                                                                      *
 *      Input:  w - ID of File button                                   *
 *              client_data - unused                                    *
 *              call_data - unused                                      *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

static void File_button_cb( Widget w, XtPointer cl, XtPointer ca )
{
  Widget  label;
  Widget  form;
  Widget  file_rowcol;
  int     i;
  int     n;
  int     in;
  XmStringTable   list_text;
  Arg     args[HCI_BUF_16];
  time_t  tm;
  int     month, day, year;
  int     hour, minute, second;
  char    *file_label;

  /* Check to see if window already opened.  If so, bring the
     window to the top of the window hierarchy and return. */

  if (File_dialog != NULL)
  {
    HCI_Shell_popup( File_dialog );
    return;
  }

  HCI_LE_log("File selected");

  list_text = (XmStringTable) XtMalloc( MAX_CLTR_FILES*sizeof(XmString) );

  in = 0;

  /* For each file defined, create a list entry composed of its
     label and last update time. */

  Num_clutter_files = hci_get_clutter_region_num_files();
  for( i = 0; i < Num_clutter_files; i++ )
  {
    file_label = hci_get_clutter_region_file_label(i);

    tm =hci_get_clutter_region_file_time(i);

    if( tm < 1 )
    { 
      year = 0; month = 0; day = 0; hour = 0; minute = 0; second = 0;
    }
    else
    {
      unix_time( &tm, &year, &month, &day, &hour, &minute, &second );
    }

    sprintf( Buff,"%2.2d/%2.2d/%4.4d  %2.2d:%2.2d:%2.2d  %s",
             month, day, year, hour, minute, second, file_label );

    list_text[in] = XmStringCreateLocalized( Buff );
    in++;
  }

  /* Create a shell widget for the File window. */

  HCI_Shell_init( &File_dialog, "Clutter Regions Files" );

  form = XtVaCreateWidget( "clutter_file_form",
                xmFormWidgetClass, File_dialog,
                XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
                NULL );

  file_rowcol = XtVaCreateWidget( "file_rowcol",
                xmRowColumnWidgetClass, form,
                XmNleftAttachment, XmATTACH_FORM,
                XmNtopAttachment, XmATTACH_FORM,
                XmNrightAttachment, XmATTACH_FORM,
                XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
                XmNpacking, XmPACK_COLUMN,
                XmNorientation, XmHORIZONTAL,
                NULL );

  File_dialog_close_button = XtVaCreateManagedWidget( "  Close  ",
                xmPushButtonWidgetClass,file_rowcol,
                XmNforeground, hci_get_read_color( BUTTON_FOREGROUND ),
                XmNbackground, hci_get_read_color( BUTTON_BACKGROUND ),
                XmNfontList, hci_get_fontlist( LIST ),
                XmNalignment, XmALIGNMENT_CENTER,
                NULL );

  XtAddCallback( File_dialog_close_button, XmNactivateCallback, File_dialog_close_button_cb, NULL );

  File_dialog_new_button = XtVaCreateManagedWidget( "   New   ",
                xmPushButtonWidgetClass, file_rowcol,
                XmNforeground, hci_get_read_color( BUTTON_FOREGROUND ),
                XmNbackground, hci_get_read_color( BUTTON_BACKGROUND ),
                XmNfontList, hci_get_fontlist( LIST ),
                XmNalignment, XmALIGNMENT_CENTER,
                NULL );

  XtAddCallback( File_dialog_new_button,
                 XmNactivateCallback, File_dialog_new_button_cb, NULL );

  File_dialog_save_button = XtVaCreateManagedWidget ("  Save   ",
                xmPushButtonWidgetClass, file_rowcol,
                XmNforeground, hci_get_read_color( BUTTON_FOREGROUND ),
                XmNbackground, hci_get_read_color( BUTTON_BACKGROUND ),
                XmNfontList, hci_get_fontlist( LIST ),
                XmNalignment, XmALIGNMENT_CENTER,
                NULL );

  /* We are assuming as we open this window that the only way the
     Update flag can be set is if a valid password was previously
     entered. */

  XtAddCallback( File_dialog_save_button,
                 XmNactivateCallback, File_dialog_save_button_cb,
                 (XtPointer) 0 );

  File_dialog_save_as_button = XtVaCreateManagedWidget( " Save As ",
                xmPushButtonWidgetClass, file_rowcol,
                XmNforeground, hci_get_read_color( BUTTON_FOREGROUND ),
                XmNbackground, hci_get_read_color( BUTTON_BACKGROUND ),
                XmNfontList, hci_get_fontlist( LIST ),
                XmNalignment, XmALIGNMENT_CENTER,
                NULL );

  XtAddCallback( File_dialog_save_as_button,
                 XmNactivateCallback, File_dialog_save_as_button_cb,
                 (XtPointer) -1 );

  File_dialog_delete_button = XtVaCreateManagedWidget( "  Delete",
                xmPushButtonWidgetClass, file_rowcol,
                XmNforeground, hci_get_read_color( BUTTON_FOREGROUND ),
                XmNbackground, hci_get_read_color( BUTTON_BACKGROUND ),
                XmNfontList, hci_get_fontlist( LIST ),
                XmNalignment, XmALIGNMENT_CENTER,
                NULL );

  XtAddCallback( File_dialog_delete_button,
                XmNactivateCallback, File_dialog_delete_button_cb, NULL );

  File_dialog_open_button = XtVaCreateManagedWidget( "  Open",
                xmPushButtonWidgetClass, file_rowcol,
                XmNforeground, hci_get_read_color( BUTTON_FOREGROUND ),
                XmNbackground, hci_get_read_color( BUTTON_BACKGROUND ),
                XmNfontList, hci_get_fontlist( LIST ),
                XmNalignment, XmALIGNMENT_CENTER,
                NULL );

  XtAddCallback( File_dialog_open_button,
                XmNactivateCallback, File_dialog_open_button_cb, NULL );

  XtManageChild( file_rowcol  );

  label = XtVaCreateManagedWidget( "  Date      Time      Label",
                xmLabelWidgetClass, form,
                XmNleftAttachment, XmATTACH_FORM,
                XmNtopAttachment, XmATTACH_WIDGET,
                XmNtopWidget, file_rowcol,
                XmNrightAttachment, XmATTACH_FORM,
                XmNalignment, XmALIGNMENT_BEGINNING,
                XmNforeground, hci_get_read_color( TEXT_FOREGROUND ),
                XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ),
                XmNfontList, hci_get_fontlist( LIST ),
                NULL );

  /* Create the scrolled files list. */

  n = 0;

  XtSetArg( args [n], XmNvisibleItemCount, VISIBLE_CLUTTER_REGION_FILES );
  n++;
  XtSetArg( args[n], XmNitemCount, in );
  n++;
  XtSetArg( args[n], XmNitems, list_text );
  n++;
  XtSetArg( args[n], XmNleftAttachment, XmATTACH_FORM );
  n++;
  XtSetArg( args[n], XmNtopAttachment, XmATTACH_WIDGET );
  n++;
  XtSetArg( args[n], XmNtopWidget, label );
  n++;
  XtSetArg( args[n], XmNrightAttachment, XmATTACH_FORM );
  n++;
  XtSetArg( args[n], XmNbottomAttachment, XmATTACH_FORM );
  n++;
  XtSetArg( args[n], XmNforeground, hci_get_read_color( TEXT_FOREGROUND ) );
  n++;
  XtSetArg( args[n], XmNbackground, hci_get_read_color( BACKGROUND_COLOR1 ) );
  n++;
  XtSetArg( args[n], XmNfontList, hci_get_fontlist( LIST ) );
  n++;

  File_dialog_list_widget = XmCreateScrolledList( form, "File_dialog_list", args, n );

  XtAddCallback( File_dialog_list_widget,
        XmNbrowseSelectionCallback, File_dialog_list_widget_cb, NULL );

  XtManageChild( File_dialog_list_widget );

  for( i = 0; i < in; i++ )
  {
    XmStringFree( list_text[i] );
  }

  XtFree( (XtPointer) list_text );

  XtManageChild( form );

  XtRealizeWidget( File_dialog );

  Hci_set_control_button_sensitivities();

  /* Force the current file to be selected (highlighted) in the list. */

  XmListSelectPos( File_dialog_list_widget, Clutter_file_index+1, False );

  HCI_Shell_start( File_dialog, NO_RESIZE_HCI );
}

/************************************************************************
 *      Description: This function is activated when a file is          *
 *                   selected from the Clutter Region Files list.       *
 *                                                                      *
 *      Input:  w - ID of File list widget                              *
 *              client_data - unused                                    *
 *              call_data - list data                                   *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

static void File_dialog_list_widget_cb( Widget w, XtPointer cl, XtPointer ca )
{
  XmListCallbackStruct *cbs = (XmListCallbackStruct *) ca;

  if( New_file_flag == HCI_YES_FLAG )
  {
    sprintf( Buff, "A new file buffer has been created, but not\nsaved. Before selecting a different clutter\nfile, you must save the new file buffer\nas a named clutter file or remove it by\nclicking the Undo button. If the new file\nbuffer is removed, the Default clutter\nfile will be displayed.");
    hci_warning_popup( File_dialog, Buff, NULL );
    Update_clutter_files_list();
    return; 
  }

  Selected_file_index = cbs->item_position-1;

  if( Selected_file_index == Clutter_file_index )
  {
    HCI_LE_log( "File %d already selected", Selected_file_index );
  }
  else
  {
    HCI_LE_log( "File %d selected", Selected_file_index );
  }

  Hci_set_control_button_sensitivities();
}

/************************************************************************
 *      Description: This function is activated when the "Open" button  *
 *                   is selected. It opens the highlighted file.        *
 *                                                                      *
 *      Input:  w - unused                                              *
 *              client_data - unused                                    *
 *              call_data - unused                                      *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

static void File_dialog_open_button_cb( Widget w, XtPointer cl, XtPointer ca )
{
  /* If the selected file is the same as the current one,
     do nothing and return. */

  if( Selected_file_index == Clutter_file_index )
  {
    HCI_LE_log( "File %d already opened", Selected_file_index );
  }
  else
  {
    /* A different file was selected so check to see if any edits
       are detected for the current file.  If there are any unsaved
       edits, call the save callback so the user can decide to 
       save them first. */

    HCI_LE_log( "Opening file %d", Selected_file_index );

    if( Update_flag == HCI_MODIFIED_FLAG )
    {
      sprintf( Buff, "Unsaved edits have been detected.\nYou must save or undo recent changes\nbefore opening a different file.");
      hci_warning_popup( File_dialog, Buff, NULL );
    }
    else
    {
      Clutter_file_index = Selected_file_index;
      Load_flag = HCI_YES_FLAG;
    }
  }
}

/************************************************************************
 *      Description: This function is activated when the "Close" button *
 *                   is selected from the Clutter Region Files window.  *
 *                   The window is closed.                              *
 *                                                                      *
 *      Input:  w - ID of Close button                                  *
 *              client_data - unused                                    *
 *              call_data - unused                                      *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

static void File_dialog_close_button_cb( Widget w, XtPointer cl, XtPointer ca )
{
  HCI_LE_log( "File dialog Close selected" );
  HCI_Shell_popdown( File_dialog );
}

/************************************************************************
 *      Description: This function updates the widgets in the clutter   *
 *                   regions table.                                     *
 *                                                                      *
 *      Input:  NONE                                                    *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

static void Update_clutter_regions_table()
{
  int     i;
  int     cut;
  XmString        str;

  cut = -1;

  /* For the current segment, extract all segments from the file
     which match it and display them. */

  for( i = 0; i < Edit_num_regions; i++ )
  {
    if( Edit_segment == Edit_data[i].segment )
    {
      cut++;
      Edit_table_widgets[cut].cut = i;

      XtVaSetValues( Edit_table_widgets[cut].region,
                     XmNuserData, (XtPointer) cut, NULL );

      sprintf( Buff,"%d ",(int) (Edit_data [i].start_azimuth+0.5) );
      XmTextSetString( Edit_table_widgets[cut].start_azimuth, Buff );

      XtVaSetValues( Edit_table_widgets[cut].start_azimuth,
                     XmNuserData, (XtPointer) cut, NULL );

      sprintf( Buff,"%d ",(int) (Edit_data[i].stop_azimuth+0.5) );
      XmTextSetString( Edit_table_widgets [cut].stop_azimuth, Buff );

      XtVaSetValues( Edit_table_widgets[cut].stop_azimuth,
                     XmNuserData, (XtPointer) cut, NULL );

      sprintf( Buff,"%d ",(int) (Edit_data[i].start_range*Units_factor+0.5) );
      XmTextSetString( Edit_table_widgets[cut].start_range, Buff );

      XtVaSetValues( Edit_table_widgets[cut].start_range,
                     XmNuserData, (XtPointer) cut, NULL );

      sprintf( Buff,"%d ",(int) (Edit_data[i].stop_range*Units_factor+0.5) );
      XmTextSetString( Edit_table_widgets[cut].stop_range, Buff );

      XtVaSetValues( Edit_table_widgets[cut].stop_range,
                     XmNuserData, (XtPointer) cut, NULL );

      switch( Edit_data[i].select_code )
      {
        case HCI_CCZ_FILTER_BYPASS :

          sprintf( Buff," Bypass Map" );
          break;

        case HCI_CCZ_FILTER_NONE :

          sprintf( Buff," No Filter " );
          break;

        case HCI_CCZ_FILTER_ALL :

          sprintf( Buff,"  All Bins " );
          break;

        default :

          sprintf( Buff,"  Unknown  " );
          break;
      }

      str = XmStringCreateLocalized( Buff );

      XtVaSetValues( Edit_table_widgets[cut].filter,
                     XmNlabelString, str,
                     XmNuserData,    (XtPointer) cut,
                     XmNforeground,  hci_get_read_color(BUTTON_FOREGROUND),
                     XmNbackground,  hci_get_read_color(BUTTON_BACKGROUND),
                     NULL );

      XmStringFree( str );
      XtManageChild( Edit_table_rowcol[cut] );
    }
  }

  strcpy( Buff,"" );

  /* We need to clear the table beyond what is curently defined. */

  for( i = cut+1; i < MAX_NUMBER_CLUTTER_ZONES; i++ )
  {
    Edit_table_widgets[i].cut = -1;
    XtUnmanageChild( Edit_table_rowcol[i] );
  }
}

/************************************************************************
 *      Description: This function is activated when a "Region" button  *
 *                   is selected from the clutter regions table.        *
 *                                                                      *
 *      Input:  w - ID of selected button                               *
 *              client_data - unused                                    *
 *              call_data - unused                                      *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

static void Select_region_cb( Widget w, XtPointer cl, XtPointer ca )
{
  XtPointer data;
  int cut;
  static int old_cut = 0;

  /* The row number is passed via the widget user data.  Get it. */

  XtVaGetValues( w, XmNuserData, &data, NULL );

  HCI_LE_log( "Region %d selected", (int) data );

  /* Unselect (by changing background color) the previously
     selected table row. */

  XtVaSetValues( Edit_table_widgets[old_cut].region,
                XmNforeground, hci_get_read_color( BUTTON_FOREGROUND ),
                XmNbackground, hci_get_read_color( BUTTON_BACKGROUND ),
                NULL );

  cut = (int) data;

  /* Highlight the newly selected row and update the current
     region pointer. */

  if( cut >= 0 )
  {
    XtVaSetValues( Edit_table_widgets[cut].region,
                XmNbackground, hci_get_read_color( BUTTON_FOREGROUND ),
                XmNforeground, hci_get_read_color( BUTTON_BACKGROUND ),
                NULL );

    Edit_region = Edit_table_widgets[cut].cut;

    old_cut = cut;
  }

  Edit_row = cut;
}

/************************************************************************
 *      Description: This function is activated when a "Select Code"    *
 *                   button is selected from the clutter regions table. *
 *                                                                      *
 *      Input:  w - ID of selected button                               *
 *              client_data - unused                                    *
 *              call_data - unused                                      *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

static void Select_filter_cb( Widget w, XtPointer cl, XtPointer ca )
{
  XtPointer data;
  XmString str;
  int i;

  /* The table row number is passed via user data.  Get it. */

  XtVaGetValues( w, XmNuserData, &data, NULL );

  i = (int) data;

  /* If the row number isn't valid do nothing. */

  if( i < 0){ return; }

  HCI_LE_log( "Select code %d selected", Edit_data[Edit_table_widgets[i].cut].select_code) ;

  /* For the selected button, change the select code label and
     and value to the next selected code in the heirarchy. */

  switch( Edit_data[Edit_table_widgets[i].cut].select_code )
  {
    case HCI_CCZ_FILTER_BYPASS :

      sprintf( Buff, "  All Bins " );
      Edit_data[Edit_table_widgets[i].cut].select_code = HCI_CCZ_FILTER_ALL;
      break;

    case HCI_CCZ_FILTER_NONE :

      sprintf( Buff, " Bypass Map" );
      Edit_data[Edit_table_widgets[i].cut].select_code = HCI_CCZ_FILTER_BYPASS;
      break;

    case HCI_CCZ_FILTER_ALL :

      sprintf( Buff, " No Filter " );
      Edit_data[Edit_table_widgets[i].cut].select_code = HCI_CCZ_FILTER_NONE;
      break;

    default :

      sprintf( Buff, " Bypass Map" );
      Edit_data[Edit_table_widgets[i].cut].select_code = HCI_CCZ_FILTER_BYPASS;
      break;
  }

  str = XmStringCreateLocalized( Buff );
  XtVaSetValues( w, XmNlabelString, str, NULL );
  XmStringFree( str );

  Update_flag = HCI_MODIFIED_FLAG;

  Hci_set_control_button_sensitivities();

  /* Rebuild the composite clutter maps and redisplay them. */

  hci_build_clutter_map( &Composite_display_data[0][0], Edit_data, Edit_segment, Edit_num_regions );
  Display_composite_clutter_map();
}

/************************************************************************
 *      Description: This function is activated when an azimuth angle   *
 *                   field is changed or loses focus in the clutter     *
 *                   regions table.                                     *
 *                                                                      *
 *      Input:  w - ID of selected field                                *
 *              client_data - HCI_CCZ_START_AZIMITH or                  *
 *                            HCI_CCZ_STOP_AZIMUTH                      *
 *              call_data - unused                                      *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

static void Change_azimuth_cb( Widget w, XtPointer cl, XtPointer ca )
{
  XtPointer data;
  char *text;
  int value;
  int i;
  char az_buf[HCI_BUF_16];
  int err_flag;

  /* The table row number is passed via user data.  Get it. */

  XtVaGetValues( w, XmNuserData, &data, NULL );

  i = (int) data;

  /* If an invalid row number is detected, do nothing. */

  if( i < 0 ){ return; }

  /* Get the azimuth value from the widget text. */

  text = XmTextGetString( w );

  /* If a value found, verify that it falls within the allowed range. */

  err_flag = 0;

  if( strlen( text ) )
  {
    sscanf( text,"%d", &value );

    /* If an invalid azimuth was entered, then we need to inform
       the user and reset the value to its previous value. */

    if( (value > HCI_CCZ_MAX_AZIMUTH) || (value < HCI_CCZ_MIN_AZIMUTH) )
    {
      sprintf( Buff, "You entered an invalid value of %d.\nThe valid range is %d to %d.", value, HCI_CCZ_MIN_AZIMUTH, HCI_CCZ_MAX_AZIMUTH );

      err_flag = 1;
    }
    else
    {
      /* Now set the appropriate entry in the clutter censor zone data. */

      switch( (int) cl )
      {
        /* If we are changing the start azimuth, only update
           it if is changed. */

        case HCI_CCZ_START_AZIMUTH:

          if( value == (int) Edit_data[Edit_table_widgets[i].cut].stop_azimuth )
          {
            sprintf( Buff,"You entered an invalid value of %d.\nIt must be different from the other\nregion azimuth.", value );
            err_flag = 1;
          }
          else
          {
            if( Edit_data[Edit_table_widgets[i].cut].start_azimuth != value )
            {
              Edit_data[Edit_table_widgets[i].cut].start_azimuth = value;
              HCI_LE_log( "Azimuth %5.1f selected", value );
              Update_flag = HCI_MODIFIED_FLAG;
              Hci_set_control_button_sensitivities();
            }
          }
          break;

          /* If we are changing the stop azimuth, only update
             it if is changed. */

        case HCI_CCZ_STOP_AZIMUTH:

          if( value == (int) Edit_data[Edit_table_widgets[i].cut].start_azimuth )
          {
            sprintf( Buff,"You entered an invalid value of %d.\nIt must be different from the other\nregion azimuth.", value );
            err_flag = 1;
          }
          else
          {
            if( Edit_data[Edit_table_widgets[i].cut].stop_azimuth != value )
            {
              Edit_data[Edit_table_widgets[i].cut].stop_azimuth = value;
              HCI_LE_log( "Azimuth %5.1f selected", value );
              Update_flag = HCI_MODIFIED_FLAG;
              Hci_set_control_button_sensitivities();
            }
          }
          break;
      }

      /* Rebuild the composite clutter maps and redisplay them. */

      if( !err_flag )
      {
        hci_build_clutter_map( &Composite_display_data[0][0], Edit_data, Edit_segment, Edit_num_regions );
        Display_composite_clutter_map ();
        Display_background_product_and_censor_zones();
        /* Call the expose callback to make them visible. */
        Draw_area_expose_cb( NULL, NULL, NULL );
      }
    }
  }
  else
  {
    /* A blank entry was detected so reset the text widget
       to its previous state. */

    switch( (int) cl )
    {
      case HCI_CCZ_START_AZIMUTH:

        value = Edit_data[Edit_table_widgets[i].cut].start_azimuth;
        break;

      case HCI_CCZ_STOP_AZIMUTH:

        value = Edit_data[Edit_table_widgets[i].cut].stop_azimuth;
        break;

    }

    sprintf( az_buf,"%3d ",value );
    XmTextSetString( w, az_buf );
  }

  if( err_flag )
  {
    /* The widget client data determines if the azimuth is
       a start or stop value. */

    switch( (int) cl )
    {
      case HCI_CCZ_START_AZIMUTH:

        value = Edit_data[Edit_table_widgets[i].cut].start_azimuth;
        break;

      case HCI_CCZ_STOP_AZIMUTH:

        value = Edit_data[Edit_table_widgets[i].cut].stop_azimuth;
        break;

    }

    sprintf( az_buf,"%3d ",value );
    XmTextSetString( w, az_buf );
    hci_warning_popup( Top_widget, Buff, NULL );
  }

  XtFree( text );
}

/************************************************************************
 *      Description: This function is activated when a range field      *
 *                   is changed or loses focus in the clutter regions   *
 *                   table.                                             *
 *                                                                      *
 *      Input:  w - ID of selected field                                *
 *              client_data - HCI_CCZ_START_RANGE or                    *
 *                            HCI_CCZ_STOP_RANGE                        *
 *              call_data - unused                                      *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

static void Change_range_cb( Widget w, XtPointer cl, XtPointer ca )
{
  XtPointer data;
  char *text;
  int num;
  int tmp;
  int i;
  char range_buf[HCI_BUF_16];
  int err_flag;

  /* The table row number is passed via user data. Get it. */

  XtVaGetValues( w, XmNuserData, &data, NULL );

  i = (int) data;

  /* If an invalid row number is detected, do nothing. */

  if( i < 0 ){ return; }

  /* Get the range value from the widget text. */

  text = XmTextGetString( w );

  /* If a value found, verify that it falls within the allowed range. */

  err_flag = 0;

  if( strlen( text ) )
  {
    sscanf( text,"%d", &num );

    tmp = (int) (num / Units_factor + 0.5);

    /* If an ivalid range was entered, then we need to inform
       the user and reset the value to its previous value. */

    if( (tmp > HCI_CCZ_MAX_RANGE) || (tmp < HCI_CCZ_MIN_RANGE) )
    {
      sprintf( Buff, "You entered an invalid value of %d.\nThe valid range is %d to %d.", num, (int) (HCI_CCZ_MIN_RANGE*Units_factor+0.5), (int) (HCI_CCZ_MAX_RANGE*Units_factor+0.5) );
      err_flag = 1;
    }
    else if( ( ((int) cl == HCI_CCZ_START_RANGE) &&
               (tmp >= Edit_data[Edit_table_widgets[i].cut].stop_range) ) ||
             ( ( (int) cl == HCI_CCZ_STOP_RANGE) &&
                 (tmp <= Edit_data[Edit_table_widgets[i].cut].start_range) ) )
    {
      err_flag = 1;

      if( (int) cl == HCI_CCZ_START_RANGE )
      {
        sprintf( Buff, "You entered an invalid start range (%d).\nThe start range must be less than the\nstop range (%d).", num, (int) (Edit_data[Edit_table_widgets[i].cut].stop_range*Units_factor+0.5) );
      }
      else
      {
        sprintf( Buff,"You entered an invalid stop range (%d).\nThe stop range must be greater than the\nstart range (%d).", num, (int) (Edit_data[Edit_table_widgets[i].cut].start_range*Units_factor+0.5) );
      }
    }
    else
    {
      /* Now set the appropriate entry in the clutter censor zone data. */

      switch( (int) cl )
      {
        /* If we are changing the start range,
           only update if it is changed. */

        case HCI_CCZ_START_RANGE:

          if( Edit_data[Edit_table_widgets[i].cut].start_range != tmp )
          {
            Edit_data[ Edit_table_widgets[i].cut].start_range = tmp;
            HCI_LE_log( "Range %d selected", tmp );
            Update_flag = HCI_MODIFIED_FLAG;
            Hci_set_control_button_sensitivities();
          }
          break;

        /* If we are changing the stop range,
           only update it if is changed. */

        case HCI_CCZ_STOP_RANGE:

          if( Edit_data[Edit_table_widgets[i].cut].stop_range != tmp )
          {
            Edit_data[Edit_table_widgets[i].cut].stop_range = tmp;
            HCI_LE_log( "Range %d selected", tmp );
            Update_flag = HCI_MODIFIED_FLAG;
            Hci_set_control_button_sensitivities();
          }
          break;
      }

      /* Rebuild the composite clutter maps and redisplay them. */
      hci_build_clutter_map( &Composite_display_data[0][0], Edit_data, Edit_segment, Edit_num_regions );
      Display_composite_clutter_map();
      Display_background_product_and_censor_zones();
      Draw_area_expose_cb( NULL, NULL, NULL );
    }
  }
  else
  {
    /* A blank entry was detected so reset the text widget to its
       previous state. */

    switch( (int) cl )
    {
      case HCI_CCZ_START_RANGE:

        num = Edit_data[Edit_table_widgets[i].cut].start_range;
        break;

      case HCI_CCZ_STOP_RANGE:

        num = Edit_data[Edit_table_widgets[i].cut].stop_range;
        break;
    }

    sprintf( range_buf,"%d ",(int) (num*Units_factor+0.5) );
    XmTextSetString( w, range_buf );
  }

  if( err_flag )
  {
    /* The widget client data determines if the range is
       a start or stop value. */

    switch( (int) cl )
    {
      case HCI_CCZ_START_RANGE:

        num = Edit_data[Edit_table_widgets[i].cut].start_range;
        break;

      case HCI_CCZ_STOP_RANGE:

        num = Edit_data[Edit_table_widgets[i].cut].stop_range;
        break;
    }

    sprintf( range_buf,"%d ",(int) (num*Units_factor+0.5) );
    XmTextSetString( w, range_buf );
    hci_warning_popup( Top_widget, Buff, NULL );
  }

  XtFree( text );
}

/************************************************************************
 *      Description: This function is activated when the "Undo" button  *
 *                   is selected.  It can also be called explicitly     *
 *                   to force a data reread.                            *
 *                                                                      *
 *      Input:  w - ID of Undo button if Undo button selected; unused   *
 *              client_data - unused                                    *
 *              call_data - unused                                      *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

static void Undo_button_cb( Widget w, XtPointer cl, XtPointer ca )
{
  HCI_LE_log("Reload selected");
  /* If undoing a new file, re-set index to default file. */
  if( New_file_flag == HCI_YES_FLAG)
  {
    New_file_flag = HCI_NO_FLAG;
    Selected_file_index = DEFAULT_FILE_INDEX;
    Clutter_file_index = DEFAULT_FILE_INDEX;
  }
  Load_flag = HCI_YES_FLAG;
}

static void Clutter_load()
{
  int status = 0;

  HCI_LE_log( "Clutter load" );

  /* Create a progress meter window if low bandwidth. */

  HCI_PM( "Loading clutter data" );

  /* Read the clutter regions file message. */

  if( ( status = hci_read_clutter_regions_file() ) <= 0 )
  {
    /* If reading the clutter censor zones message failed,
       create popup and then exit. */
    hci_error_popup( Top_widget, "Error while reading clutter file", NULL );
    HCI_LE_error("hci_read_clutter_regions_file failed (%d)", status);
    return;
  }

  /* Update the edit buffer with data for the current file. */

  Copy_file_to_edit_buffer();

  /* Unset the update flag. */

  Update_flag = HCI_NOT_MODIFIED_FLAG;
}

static void Clutter_draw()
{
  /* Rebuild the composite clutter maps and redisplay them. */

  hci_build_clutter_map( &Composite_display_data[0][0], Edit_data, Edit_segment, Edit_num_regions );
  Display_composite_clutter_map();

  /* Redisplay the clutter region overlays in the display area. */

  Display_background_product_and_censor_zones();

  /* Update the clutter regions table widgets. */

  Update_clutter_regions_table();

  /* Call the expose callback to make them visible. */

  Draw_area_expose_cb( NULL, NULL, NULL );

  /* If the File window is open update the files list. */

  Update_clutter_files_list();

  /* Update file info label. */

  Set_file_info_label();

  Hci_set_control_button_sensitivities();
}

/************************************************************************
 *      Description: This function is activated when the "Save" button  *
 *                   is selected from the File window.                  *
 *                                                                      *
 *      Input:  w - ID of Save button if Undo button selected; unused   *
 *              client_data - unused                                    *
 *              call_data - unused                                      *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

static void File_dialog_save_button_cb( Widget w, XtPointer cl, XtPointer ca )
{
  sprintf( Buff, "Are you sure you want to save your clutter\nregions changes?" );

  hci_confirm_popup( File_dialog, Buff, File_dialog_save_button_cb_accept, File_dialog_save_button_cb_cancel );
}

static void File_dialog_save_button_cb_accept( Widget w, XtPointer cl, XtPointer ca )
{
  Save_flag = HCI_YES_FLAG;
}

static void File_dialog_save_button_cb_cancel( Widget w, XtPointer cl, XtPointer ca )
{
  Save_flag = HCI_NO_FLAG;
}

/************************************************************************
 *      Description: This function handles all mouse input inside the   *
 *                   clutter regions drawing area.                      *
 *                                                                      *
 *      Input:  w - drawing area widget ID                              *
 *              event - X event data                                    *
 *              args  - action data                                     *
 *              num_args - number of arguments in action data           *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

static void Mouse_input( Widget w, XEvent *event, String *args, int *num_args )
{
  static float first_azimuth; 
  static float first_range;
  static float second_azimuth;
  static float second_range;
  static int first_pixel;
  static int first_scanl;
  static int second_pixel;
  static int second_scanl;
  static int button_down = 0;
  float azimuth;
  float range;
  float temp;
  float x;
  float y;
  int pixel;
  int scanl;

  pixel = event->xbutton.x;
  scanl = event->xbutton.y;

  /* If mouse event occurs outside clutter map, ignore it. */

  if( pixel > Clutter_display_width || scanl > Clutter_display_height )
  {
    return;
  }

  /* Compute the X and Y coordinate of the cursor relative to the radar. */

  x = Clutter_display_center_x_offset + (Clutter_display_center_pixel-pixel)/(Clutter_display_zoom_factor*Clutter_display_scale_x);
  y = Clutter_display_center_y_offset + (Clutter_display_center_scanl-scanl)/(Clutter_display_zoom_factor*Clutter_display_scale_y);

  /* Convert the XY coordinate to polar coordinates. */

  azimuth = hci_find_azimuth( (int) -(x*100), (int) (y*100), 0, 0 );
  range  = sqrt( (double)(x*x + y*y) );

  /* If in zoom mode, increase the magnification for a left click
     and decrease the magnification for a right click.  Recenter
     on a middle button click. */

  if( Clutter_display_input_mode == HCI_CCZ_ZOOM_MODE )
  {
    if( !strcmp( args[0], "up1" ) )
    {
      /* Left mouse click */

      HCI_LE_log( "Zooming window" );

      if( Clutter_display_zoom_factor < HCI_CCZ_MAX_ZOOM )
      {
        Clutter_display_center_x_offset = (Clutter_display_center_pixel-pixel)/
                      (Clutter_display_scale_x*Clutter_display_zoom_factor) +
                       Clutter_display_center_x_offset;
        Clutter_display_center_y_offset = (Clutter_display_center_scanl-scanl)/
                      (Clutter_display_scale_y*Clutter_display_zoom_factor) +
                       Clutter_display_center_y_offset;

        Clutter_display_zoom_factor = Clutter_display_zoom_factor*2;
        Display_background_product_and_censor_zones();
        Draw_area_expose_cb( NULL, NULL, NULL );
      }

    }
    else if( !strcmp( args[0], "up2" ) )
    {
      /* Middle mouse click */

      HCI_LE_log("Recentering window");

      if( range <= HCI_CCZ_MAX_RANGE )
      {
        Clutter_display_center_x_offset = (Clutter_display_center_pixel-pixel)/
                        (Clutter_display_scale_x*Clutter_display_zoom_factor) +
                         Clutter_display_center_x_offset;
        Clutter_display_center_y_offset = (Clutter_display_center_scanl-scanl)/
                        (Clutter_display_scale_y*Clutter_display_zoom_factor) +
                         Clutter_display_center_y_offset;
      }

      Display_background_product_and_censor_zones();
      Draw_area_expose_cb( NULL, NULL, NULL );
    }
    else if( !strcmp( args[0], "up3" ) )
    {
      /* Right mouse click */

      HCI_LE_log( "Unzooming window" );

      if( Clutter_display_zoom_factor > 1 )
      {
        if( range <= HCI_CCZ_MAX_RANGE )
        {
          Clutter_display_center_x_offset = (Clutter_display_center_pixel-pixel)/
                       (Clutter_display_scale_x*Clutter_display_zoom_factor) +
                        Clutter_display_center_x_offset;
          Clutter_display_center_y_offset = (Clutter_display_center_scanl-scanl)/
                       (Clutter_display_scale_y*Clutter_display_zoom_factor) +
                        Clutter_display_center_y_offset;
        }

        Clutter_display_zoom_factor = Clutter_display_zoom_factor/2;
        Display_background_product_and_censor_zones();
        Draw_area_expose_cb( NULL, NULL, NULL );
      }
    }
  }
  else
  {
    /* If the left mouse button is pressed, start a new region definition. */

    if( ( !strcmp( args[0], "down1" ) ) &&
        ( (int) range  < HCI_CCZ_MAX_RANGE) )
    {
      /* If we have reached the maximum number of allowed
         ignore the new region. */

      if( Edit_num_regions >= MAX_NUMBER_CLUTTER_ZONES )
      {
        hci_warning_popup( Top_widget, "The maximum number of regions has been reached.\nYou must first delete an existing region before\nadding a new one.", NULL );
        return;
      }

      HCI_LE_log( "Stating new region" );

      first_pixel = pixel;
      first_scanl = scanl;
      button_down = 1;
      first_azimuth = azimuth;
      first_range   = range;
      second_azimuth = first_azimuth;
      second_range   = first_range;
    }
    else if( !strcmp( args[0], "up1" ) && ( button_down == 1 ) )
    {
      /* If the left button is released, then this defines the
         end of the new region definition. */
      HCI_LE_log("Ending new region");

      button_down = 0;
      second_pixel = pixel;
      second_scanl = scanl;
      second_azimuth = azimuth;

      /* Don't allow the range to exceed the maximm. If it
         does, set it to the maximum. */

      if( (int) range > HCI_CCZ_MAX_RANGE ){ range = HCI_CCZ_MAX_RANGE; }

      second_range   = range;

      /* If either the range or azimuth boundaries are
         same then this is not a valid region. */

      if( ( (int) first_range == (int) second_range ) ||
          ( (int) first_azimuth == (int) second_azimuth ) )
      {
        Copy_pixmap_to_window();
        return;
      }

      Update_flag = HCI_MODIFIED_FLAG;
      Hci_set_control_button_sensitivities();

      /* A new region is defined. Update the internal
         data table to include the new region definition
         NOTE: The internal data structure uses floats
               for the azimuth and range data.  However, when
               the data are saved they are converted to
               integer.  So, round the values to the nearest
               whole number. */

      Edit_data[Edit_num_regions].start_azimuth = (float) ((int) (first_azimuth+0.5));
      Edit_data[Edit_num_regions].stop_azimuth  = (float) ((int) (second_azimuth+0.5));

      if( first_range > second_range )
      {
        temp = first_range;
        first_range = second_range;
        second_range = temp;
      }

      if( first_range < 2 )
      {
        first_range = 2;
      }
      else if( first_range > HCI_CCZ_MAX_RANGE )
      {
        first_range = HCI_CCZ_MAX_RANGE;
      }

      if( second_range > HCI_CCZ_MAX_RANGE )
      {
        second_range = HCI_CCZ_MAX_RANGE;
      }
      else if( second_range > HCI_CCZ_MAX_RANGE )
      {
        second_range = HCI_CCZ_MAX_RANGE;
      }

      Edit_data[Edit_num_regions].start_range = (float) ((int) (first_range+0.5));
      Edit_data[Edit_num_regions].stop_range = (float) ((int) (second_range+0.5));
      Edit_data[Edit_num_regions].segment = Edit_segment;
      Edit_data[Edit_num_regions].select_code = Edit_filter;
      Edit_num_regions++;

      hci_build_clutter_map( &Composite_display_data[0][0], Edit_data, Edit_segment, Edit_num_regions );
      Display_composite_clutter_map();
      Update_clutter_regions_table();

      Draw_censor_zone( Clutter_pixmap, PRODUCT_FOREGROUND_COLOR,
                        first_azimuth, second_azimuth,
                        first_range, second_range, Edit_filter );

      Copy_pixmap_to_window();

      /* If the mouse is being moved with the left button down, we
         are defining a new region.  Update the new region display
         as the mouse is being moved. */

    }
    else if( (!strcmp( args[0], "move" ) ) && button_down )
    {
      Copy_pixmap_to_window();

      second_pixel = pixel;
      second_scanl = scanl;
      second_azimuth = azimuth;

      if( (int) range > HCI_CCZ_MAX_RANGE ){ range = HCI_CCZ_MAX_RANGE; }

      second_range = range;

      if( second_azimuth <= first_azimuth )
      {
        second_azimuth = second_azimuth+360;
      }

      Draw_censor_zone( Clutter_window, PRODUCT_FOREGROUND_COLOR,
                        first_azimuth, second_azimuth,
                        first_range, second_range, Edit_filter );
    }
  }

  /* Display the azran of the cursor in the upper left corner of
     the edit window. */

  if( pixel == Clutter_display_width ){ range = HCI_CCZ_MAX_RANGE; }

  Hci_set_foreground( BUTTON_FOREGROUND );
  Hci_set_background( BUTTON_BACKGROUND );
  Hci_set_display_font( LIST );

  if( Units_factor == HCI_KM_TO_KM )
  {
    sprintf( Buff,"(%3d deg,%3d km)", (int) azimuth, (int) range) ;
  }
  else
  {
    sprintf( Buff,"(%3d deg,%3d nm)", (int) azimuth, (int) (range*Units_factor+0.5) );
  }

  Hci_draw_imagestring( 5, 15, Buff );
  Hci_set_display_font( SMALL );
}


/************************************************************************
 *      Description: This function is the called when the user selects  *
 *                   one of the Mode radio buttons.  Changing the mode  *
 *                   affects the interpretation of mouse actions while  *
 *                   the cursor is in the clutter display area.         *
 *                                                                      *
 *      Input:  w - select radio button ID                              *
 *              client_data - clutter mode                              *
 *              call_data - radio button state data                     *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

static void Select_mode_cb( Widget w, XtPointer cl, XtPointer ca )
{
  XmToggleButtonCallbackStruct    *state =
                (XmToggleButtonCallbackStruct *) ca;

  /* If the radio button is set, change the current mode.  The mode
     data is passed as client data. */

  if( state->set )
  {
    Clutter_display_input_mode = (int) cl;
    HCI_LE_log( "Clutter Mode %d selected", Clutter_display_input_mode) ;
  }
}

/************************************************************************
 *      Description: This function is the called when the user selects  *
 *                   one of the Units radio buttons.                    *
 *                                                                      *
 *      Input:  w - select radio button ID                              *
 *              client_data - UNITS_KM, UNITS_NM                        *
 *              call_data - radio button state data                     *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

static void Select_units_cb( Widget w, XtPointer cl, XtPointer ca )
{
  XmToggleButtonCallbackStruct    *state =
                (XmToggleButtonCallbackStruct *) ca;

  /* If the radio button is set, change the current units. The
     units data is passed as client data. */

  if( state->set )
  {
    switch( (int) cl )
    {
      /* If the new units are Kilometers, set the conversion factor
         for the clutter range data and update the range labels. */

      case UNITS_KM :

        Units_factor = HCI_KM_TO_KM;
        sprintf( Buff," Start Range (km) " );
        XmTextSetString( Start_range_text_box,Buff );
        sprintf( Buff," Stop Range (km) " );
        XmTextSetString( End_range_text_box,Buff );
        break;

      /* If the new units are Nautical Miles, set the conversion
         for the clutter range data and update the range labels. */

      case UNITS_NM :

        Units_factor = HCI_KM_TO_NM;
        sprintf( Buff," Start Range (nm) " );
        XmTextSetString( Start_range_text_box,Buff );
        sprintf( Buff," Stop Range (nm) " );
        XmTextSetString( End_range_text_box,Buff );
        break;

    }

    /* Update the clutter regions table so new units are applied
       to all range data and labels. */

    Update_clutter_regions_table();
  }
}

/************************************************************************
 *      Description: This function is the called when the user selects  *
 *                   one of the Segment radio buttons.                  *
 *                                                                      *
 *      Input:  w - select radio button ID                              *
 *              client_data - HCI_CCZ_SEGMENT_ONE or                    *
 *                            HCI_CCZ_SEGMENT_TWO or                    *
 *                            HCI_CCZ_SEGMENT_THREE or                  *
 *                            HCI_CCZ_SEGMENT_FOUR or                   *
 *                            HCI_CCZ_SEGMENT_FIVE                      *
 *              call_data - radio button state data                     *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

static void Select_segment_cb( Widget w, XtPointer cl, XtPointer ca )
{
  int i;
  int cut;
  XmString str;

  XmToggleButtonCallbackStruct    *state =
                (XmToggleButtonCallbackStruct *) ca;

  /* If the radio button is set, change the current segment number
     using data passed as client data. */

  if( state->set )
  {
    HCI_LE_log( "Clutter Segment %d selected", Edit_segment) ;

    /* Update the Segment label widget. */

    switch( (int) cl )
    {
      case HCI_CCZ_SEGMENT_ONE :

        sprintf( Buff,"Elevation Segment 1" );
        str = XmStringCreateLocalized( Buff );
        XtVaSetValues( Segment_label, XmNlabelString, str, NULL );
        XmStringFree (str);
        break;

      case HCI_CCZ_SEGMENT_TWO :

        sprintf (Buff,"Elevation Segment 2");
        str = XmStringCreateLocalized( Buff );
        XtVaSetValues( Segment_label, XmNlabelString, str, NULL );
        XmStringFree (str);
        break;

      case HCI_CCZ_SEGMENT_THREE :

        sprintf (Buff,"Elevation Segment 3");
        str = XmStringCreateLocalized( Buff );
        XtVaSetValues( Segment_label, XmNlabelString, str, NULL );
        XmStringFree (str);
        break;

      case HCI_CCZ_SEGMENT_FOUR :

        sprintf (Buff,"Elevation Segment 4");
        str = XmStringCreateLocalized( Buff );
        XtVaSetValues( Segment_label, XmNlabelString, str, NULL );
        XmStringFree (str);
        break;

      case HCI_CCZ_SEGMENT_FIVE :

        sprintf (Buff,"Elevation Segment 5");
        str = XmStringCreateLocalized( Buff );
        XtVaSetValues( Segment_label, XmNlabelString, str, NULL );
        XmStringFree (str);
        break;
    }

    /* Update the current segment and rebuild Edit_table_widgets table
       based on the new segment. */

    Edit_segment = (int) cl;

    cut = 0;

    for( i = 0; i < Edit_num_regions; i++ )
    {
      if( Edit_segment == Edit_data [i].segment )
      {
        Edit_table_widgets [cut].cut = i;
        cut++;
      }
    }

    Load_background_product( Clutter_display_background_product_type );
    Clutter_draw();
  }
}

/************************************************************************
 *      Description: This function displays a region overlay in the     *
 *                   clutter regions display area.                      *
 *                                                                      *
 *      Input:  display - Display info                                  *
 *              window  - Drawable                                      *
 *              gc      - Graphics Context                              *
 *              color   - overlay color index                           *
 *              first_azimuth  - first region azimuth (deg)             *
 *              second_azimuth - second region azimuth (deg)            *
 *              first_range    - first region range (deg)               *
 *              second_range   - second region range (deg)              *
 *              filter    - operator select code                        *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

static void Draw_censor_zone( Drawable window, int color,
            float first_azimuth, float second_azimuth,
            float first_range, float second_range, int filter )
{
  float angle;
  XPoint x[2000];
  int i;
  int first_arc;

  Hci_set_foreground( color );

  /* We draw the region in a clockwise manner so add 360 to the
     second azimuth angle if it is less than the first. */

  if( second_azimuth < first_azimuth )
  {
    second_azimuth = second_azimuth + 360.0;
  }

  i = 0;

  /* In half degree increments, calculate the end points (in
     cartesian space) for a set of line segments connecting the
     two range values across radials. */

  for( angle = first_azimuth; angle < second_azimuth; angle = angle+0.5 )
  {
    x[i].x = Clutter_display_center_pixel +
                (first_range * cos ((double) (angle-90.0)*HCI_DEG_TO_RAD) +
                Clutter_display_center_x_offset) * (Clutter_display_zoom_factor * Clutter_display_scale_x);
    x[i].y = Clutter_display_center_scanl +
                (first_range * sin ((double) (angle+90.0)*HCI_DEG_TO_RAD) +
                Clutter_display_center_y_offset) * (Clutter_display_zoom_factor * Clutter_display_scale_y);
    i++;
  }

  first_arc = i;

  /* Reverse the process to generate a second set of lines. */

  x[i].x = Clutter_display_center_pixel +
          (first_range * cos((double) (second_azimuth-90.0)*HCI_DEG_TO_RAD) +
          Clutter_display_center_x_offset) * (Clutter_display_zoom_factor * Clutter_display_scale_x);
  x[i].y = Clutter_display_center_scanl +
          (first_range * sin((double) (second_azimuth+90.0)*HCI_DEG_TO_RAD) +
          Clutter_display_center_y_offset) * (Clutter_display_zoom_factor * Clutter_display_scale_y);
  i++;

  for( angle = second_azimuth; angle > first_azimuth; angle = angle-0.5 )
  {
    x[i].x = Clutter_display_center_pixel +
                (second_range * cos((double) (angle-90.0)*HCI_DEG_TO_RAD) +
                Clutter_display_center_x_offset) * (Clutter_display_zoom_factor * Clutter_display_scale_x);
    x[i].y = Clutter_display_center_scanl +
                (second_range * sin((double) (angle+90.0)*HCI_DEG_TO_RAD) +
                Clutter_display_center_y_offset) * (Clutter_display_zoom_factor * Clutter_display_scale_y);
    i++;
  }

  x[i].x = x[0].x;
  x[i].y = x[0].y;
  i++;

  /* Display the region by drawing both sets of lines. */

  angle = fabs ((double)(second_azimuth-first_azimuth));

  if( (angle <   0.5) || (angle > 359.5) )
  {
    Hci_draw_line( window, &x[0], first_arc );
    Hci_draw_line( window, &x[first_arc+1], i-first_arc-2 );
  }
  else
  {
    Hci_draw_line( window, x, i );
  }
}

/************************************************************************
 *      Description: This function displays a background product and    *
 *                   region overlays in the clutter regions display     *
 *                   area.                                              *
 *                                                                      *
 *      Input:  NONE                                                    *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

static void Display_background_product_and_censor_zones()
{
  int i = 0;

  Hci_set_foreground( PRODUCT_BACKGROUND_COLOR );

  Hci_fill_rectangle( 0, 0, Clutter_display_width, Clutter_display_height );

  /* If a background product is open, display it. */

  if( Background_product_status_flag > 0 )
  {
    if( (hci_product_type () == RADIAL_TYPE ) ||
        (hci_product_type () == DHR_TYPE) )
    {
      /* Set clip to only draw in clutter area. */
      Hci_set_clip_rectangle( Clutter_display_width, Clutter_display_height );

      hci_display_radial_product( Clutter_display, Clutter_pixmap, Clutter_gc,
                 0, 0, Clutter_display_width, Clutter_display_height,
                 HCI_CCZ_MAX_RANGE, Clutter_display_center_x_offset,
                 Clutter_display_center_y_offset, Clutter_display_zoom_factor );

      /* Reset clip to allow drawing anywhere. */
      Hci_set_clip_rectangle( Drawing_area_width, Drawing_area_height );

      /* Temporarily set line width to 1 pixel so color bar isn't wide. */
      Hci_set_line_width( 1 );

      /* Display the color bar. */
      hci_display_color_bar( Clutter_display, Clutter_pixmap, Clutter_gc,
                   Clutter_display_width, 0, Clutter_display_height, COLOR_BAR_WIDTH, 10, 0 );

      /* Reset the line width so region overlays will use wide lines. */
      Hci_set_line_width( 2 );

      /* Draw date/time/status labels if not zoomed in. */
      if( Clutter_display_zoom_factor == 1 ){ Draw_labels_on_display(); }
    }
  }
  else
  {
    /* No background product open, so display informative label instead. */
    Draw_no_background_label();
  }

  /* Set clip to only draw in clutter area. */
  Hci_set_clip_rectangle( Clutter_display_width, Clutter_display_height );

  /* Overlay region lines for all defined regions in segment. */
  for( i = 0; i < Edit_num_regions; i++ )
  {
    if( Edit_segment == Edit_data [i].segment )
    {
      Draw_censor_zone( Clutter_pixmap, PRODUCT_FOREGROUND_COLOR,
                Edit_data[i].start_azimuth, Edit_data[i].stop_azimuth,
                Edit_data[i].start_range, Edit_data [i].stop_range, Edit_filter );
    }
  }

  /* Reset clip to allow drawing anywhere. */
  Hci_set_clip_rectangle( Drawing_area_width, Drawing_area_height );
}

/************************************************************************
 *      Description: This function is called when the user selects the  *
 *                   "New" button in the Clutter Region Files window.   *
 *                                                                      *
 *      Input:  w - "New" button ID                                     *
 *              client_data - unused                                    *
 *              call_data - unused                                      *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

static void File_dialog_new_button_cb( Widget w, XtPointer cl, XtPointer ca )
{
  int empty_index;

  HCI_LE_log( "New File selected" );

  if( Update_flag == HCI_MODIFIED_FLAG )
  {
    sprintf( Buff, "Unsaved edits have been detected.\nYou must save or undo recent changes\nbefore creating a new file.");
    hci_warning_popup( File_dialog, Buff, NULL );
    return;
  }
  else if( Num_clutter_files == MAX_CLTR_FILES )
  {
    /* If maximum number of clutter files have been created,
       let user know a new one can't be created. */
    hci_warning_popup( File_dialog,
        "The maximum number of clutter files has been reached.\nYou must first delete an existing file before adding\na new one.", NULL );
    HCI_LE_log("Maximum number of Clutter files created. Cannot create new file." );
    return;
  }

  empty_index = hci_get_clutter_region_num_files();

  New_file_flag = HCI_YES_FLAG;
  Clutter_file_index = empty_index;
  Selected_file_index = empty_index;
  Edit_num_regions = Num_valid_segments;
  Update_flag = HCI_MODIFIED_FLAG;

  if( Num_valid_segments >= 1 )
  {
    /* Define the region for segment 1. */
    Edit_data[0].start_azimuth = 0;
    Edit_data[0].stop_azimuth = 360;
    Edit_data[0].start_range = 2;
    Edit_data[0].stop_range = HCI_CCZ_MAX_RANGE;
    Edit_data[0].select_code = HCI_CCZ_FILTER_BYPASS;
    Edit_data[0].segment = HCI_CCZ_SEGMENT_ONE;
  }

  if( Num_valid_segments >= 2 )
  {
    /* Define the region for segment 2. */
    Edit_data[1].start_azimuth = 0;
    Edit_data[1].stop_azimuth = 360;
    Edit_data[1].start_range = 2;
    Edit_data[1].stop_range = HCI_CCZ_MAX_RANGE;
    Edit_data[1].select_code = HCI_CCZ_FILTER_BYPASS;
    Edit_data[1].segment = HCI_CCZ_SEGMENT_TWO;
  }

  if( Num_valid_segments >= 3 )
  {
    /* Define the region for segment 3. */
    Edit_data[2].start_azimuth = 0;
    Edit_data[2].stop_azimuth = 360;
    Edit_data[2].start_range = 2;
    Edit_data[2].stop_range = HCI_CCZ_MAX_RANGE;
    Edit_data[2].select_code = HCI_CCZ_FILTER_BYPASS;
    Edit_data[2].segment = HCI_CCZ_SEGMENT_THREE;
  }

  if( Num_valid_segments >= 4 )
  {
    /* Define the region for segment 4. */
    Edit_data[3].start_azimuth = 0;
    Edit_data[3].stop_azimuth = 360;
    Edit_data[3].start_range = 2;
    Edit_data[3].stop_range = HCI_CCZ_MAX_RANGE;
    Edit_data[3].select_code = HCI_CCZ_FILTER_BYPASS;
    Edit_data[3].segment = HCI_CCZ_SEGMENT_FOUR;
  }

  if( Num_valid_segments >= 5 )
  {
    /* Define the region for segment 5. */
    Edit_data[4].start_azimuth = 0;
    Edit_data[4].stop_azimuth = 360;
    Edit_data[4].start_range = 2;
    Edit_data[4].stop_range = HCI_CCZ_MAX_RANGE;
    Edit_data[4].select_code = HCI_CCZ_FILTER_BYPASS;
    Edit_data[4].segment = HCI_CCZ_SEGMENT_FIVE;
  }

  Clutter_draw();
}

/************************************************************************
 *      Description: This function is activated when the "Yes" button   *
 *                   is selected from the save verification popup.      *
 *                                                                      *
 *      Input:  w - ID of Yes button selected; unused                   *
 *              client_data - unused                                    *
 *              call_data - unused                                      *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

static void Clutter_file_save()
{
  int i;
  int status;
  time_t tm;
  char *file_label;

  HCI_LE_log( "File Save selected" );

  file_label = hci_get_clutter_region_file_label( Clutter_file_index );

  if( strlen( file_label ) == 0 || strcmp( file_label, "Default" ) == 0 )
  {
    /* No filename or filename is "Default", so must do a "Save As". */
    Save_as_flag = HCI_YES_FLAG;
  }
  else
  {
    /* Get the current time so we can update the update time field. */

    tm = time (NULL);

    /* Update the time stamp field. */

    hci_set_clutter_region_file_time( Clutter_file_index, tm );

    /* Set the clutter properties for the saved file. */

    hci_set_clutter_region_regions( Clutter_file_index, Edit_num_regions );

    for( i = 0; i < Edit_num_regions; i++ )
    {
      hci_set_clutter_region_data( Clutter_file_index,
                         i, HCI_CCZ_START_AZIMUTH, Edit_data[i].start_azimuth );
      hci_set_clutter_region_data( Clutter_file_index,
                         i, HCI_CCZ_STOP_AZIMUTH, Edit_data[i].stop_azimuth );
      hci_set_clutter_region_data( Clutter_file_index,
                         i, HCI_CCZ_START_RANGE, Edit_data[i].start_range );
      hci_set_clutter_region_data( Clutter_file_index,
                         i, HCI_CCZ_STOP_RANGE, Edit_data[i].stop_range);
      hci_set_clutter_region_data( Clutter_file_index,
                         i, HCI_CCZ_SEGMENT, Edit_data[i].segment);
      hci_set_clutter_region_data( Clutter_file_index,
                         i, HCI_CCZ_SELECT_CODE, Edit_data[i].select_code);
    }

    /* If low bandwidth, display a progress meter window. */

    HCI_PM( "Saving Clutter Regions Data" );

    /* Write the updated clutter file data to the clutter LB. */

    status = hci_write_clutter_regions_file();

    /* If the File window is open update the files list. */

    Update_clutter_files_list();

    Update_flag = HCI_NOT_MODIFIED_FLAG;
    Hci_set_control_button_sensitivities();
  }
}

/************************************************************************
 *      Description: This function is called when the user selects the  *
 *                   "Save As" button in the Clutter Region Files       *
 *                   window.                                            *
 *                                                                      *
 *      Input:  w - "Save As" button ID (unused)                        *
 *              client_data - unused                                    *
 *              call_data - unused                                      *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

static void File_dialog_save_as_button_cb( Widget w, XtPointer cl, XtPointer ca )
{
  HCI_LE_log( "File Save As selected" );

  Save_as_flag = HCI_YES_FLAG;
}

static void Clutter_file_save_as()
{
  /* Find the index of the first available entry in the clutter
     regions data buffer for the new file. If a file entry is
     available, then popup a window so a filename can be specified. */

  if( hci_get_clutter_region_num_files() < MAX_CLTR_FILES )
  {
    /* make sure to clear the existing value */
    memset( Edit_file_label, 0, 1 );
    /* If the Save As window is already open, do nothing. */

    if( Save_as_dialog != NULL )
    {
      /* Clear the password text widget. */
      XmTextSetString( Filename_text_box, "" );
      HCI_Shell_popup( Save_as_dialog );
    }
    else
    {
      Create_save_as_dialog();
    }
  }
  else
  {
    Save_as_flag = HCI_NO_FLAG;
    if( Save_as_dialog != NULL ){ HCI_Shell_popdown( Save_as_dialog ); }
    hci_warning_popup( File_dialog,
             "The maximum number of clutter files has been reached.\nYou must first delete an existing file before adding\na new one.", NULL);
    HCI_LE_log("Maximum number of Clutter files created. Cannot \"Save As\"" );
    return;
  }
}

static void Save_as_dialog_cancel_button_cb( Widget w, XtPointer cl, XtPointer ca )
{
  Save_as_flag = HCI_NO_FLAG;
  if( Save_as_dialog != NULL ){ HCI_Shell_popdown( Save_as_dialog ); }
}

static void Save_as_dialog_accept_button_cb( Widget w, XtPointer cl, XtPointer ca )
{
  char *text;
  int len;
  int i;
  char *file_label;
  int file_index_to_use;

  /* Pull filename from box in popup window. */

  text = XmTextGetString( Filename_text_box );
  len = strlen( text );
  if( len >= MAX_LABEL_SIZE ){ len = MAX_LABEL_SIZE - 1; }
  strncpy( Edit_file_label, text, len );
  memset( Edit_file_label+len, 0, 1 );
  XtFree( text );

  file_index_to_use = -1;

  if( strlen( Edit_file_label ) > 0 )
  {
    for( i = 0; i < MAX_CLTR_FILES; i++ )
    {
      file_label = hci_get_clutter_region_file_label(i);
      if( !strcmp( Edit_file_label,file_label ) )
      {
        hci_warning_popup( Save_as_dialog,
            "The filename you specified is already\nused.  Enter a new filename.", NULL);
        break;
      }

      if( strlen( file_label ) == 0 )
      {
        file_index_to_use = i;
        break;
      }
    }
  }
  else
  {
    hci_warning_popup( Save_as_dialog, "Please Enter a valid filename", NULL );
  }

  /* If no files are available in buffer, do nothing and return. */

  if( file_index_to_use < 0 ){ return; }

  /* Increment number of clutter files. */

  Num_clutter_files++;

  /* Set the current clutter file index to the new one. */

  Clutter_file_index = file_index_to_use;

  Selected_file_index=Clutter_file_index;

  /* Set the clutter file label tag. */

  hci_set_clutter_region_file_label( Clutter_file_index, Edit_file_label );

  XmListSelectPos( File_dialog_list_widget, Clutter_file_index+1, False );
  HCI_Shell_popdown( Save_as_dialog );
  New_file_flag = HCI_NO_FLAG;
  Save_flag = HCI_YES_FLAG;
}

/************************************************************************
 *      Description: This function is called when the "Delete" button   *
 *                   is selected from the Clutter Region Files window.  *
 *                                                                      *
 *      Input:  w - Delete button ID                                    *
 *              client_data - unused                                    *
 *              call_data - unused                                      *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

static void File_dialog_delete_button_cb( Widget w, XtPointer cl, XtPointer ca )
{
  int position_count;
  int *position_list;

  /* If no file is selected in the File list, do nothing and return. */

  if( !XmListGetSelectedPos( File_dialog_list_widget, &position_list, &position_count ) )
  {
    return;
  }

  /* A different file was selected so check to see if any edits
     are detected for the current file.  If there are any unsaved
     edits, call the save callback so the user can decide to 
     save them first. */

  HCI_LE_log( "File %d selected for deletion", Selected_file_index );

  /* If the selected file is the default file, don't allow it to be
     deleted.  Popup an information message to convey this to the user. */

  if( Update_flag == HCI_MODIFIED_FLAG && Selected_file_index != Clutter_file_index )
  {
    sprintf( Buff, "Unsaved edits have been detected.\nYou must save or undo recent changes\nbefore deleting a different file.");
    hci_warning_popup( File_dialog, Buff, NULL );
  }
  else
  {
    if( position_list[0] == 1 )
    {
      sprintf( Buff, "You cannot delete the default clutter\nsuppression regions file!" );
      hci_warning_popup( File_dialog, Buff, NULL );
    }
    else
    {
      /* A valid file is selected.  Popup a confirmation window so the
         user can proceed or abort the delete file operation. */

      sprintf( Buff,"You are about to delete the selected\nclutter suppression regions file.\nDo you want to continue?" );
      hci_confirm_popup( File_dialog, Buff, File_dialog_delete_button_cb_accept, NULL );
    }
  }
}

/************************************************************************
 *      Description: This function is called when the "Yes" button      *
 *                   is selected from the Clutter Region Files delete   *
 *                   confirmation popup window.                         *
 *                                                                      *
 *      Input:  w - Delete button ID                                    *
 *              client_data - unused                                    *
 *              call_data - unused                                      *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

static void File_dialog_delete_button_cb_accept( Widget w, XtPointer cl, XtPointer ca )
{
  int status;
  int i;
  int item;
  int j;
  int regions;
  char *file_label;
  int position_count;
  int *position_list;
  time_t tm;

  /* If no file was selected, do nothing and return. */

  if( !XmListGetSelectedPos( File_dialog_list_widget, &position_list, &position_count ) )
  {
    return;
  }

  /* Blank out the current clutter label. */

  strcpy( Edit_file_label,"" );

  /* For each selected file in the list delete it by setting its
     label tag to blank. */

  for( item = 0; item < position_count; item++ )
  {
    XmListDeletePos( File_dialog_list_widget, position_list[item] );

    status = hci_set_clutter_region_file_label( position_list[item]-1,
                                Edit_file_label );

    /* Move up all trailing clutter files in the buffer one position. */

    for( i = position_list[item]; i < MAX_CLTR_FILES; i++ )
    {
      file_label = hci_get_clutter_region_file_label(i);
      status = hci_set_clutter_region_file_label( i-1, file_label );
      tm =hci_get_clutter_region_file_time(i);
      status = hci_set_clutter_region_file_time( i-1, tm );
      regions = hci_get_clutter_region_regions(i);
      hci_set_clutter_region_regions( i-1, regions );

      for( j = 0; j < regions; j++ )
      {
        status = hci_set_clutter_region_data( i-1, j, HCI_CCZ_START_AZIMUTH,
                hci_get_clutter_region_data( i, j, HCI_CCZ_START_AZIMUTH ) );

        status = hci_set_clutter_region_data( i-1, j, HCI_CCZ_STOP_AZIMUTH,
                hci_get_clutter_region_data( i, j, HCI_CCZ_STOP_AZIMUTH ) );

        status = hci_set_clutter_region_data( i-1, j, HCI_CCZ_START_RANGE,
                hci_get_clutter_region_data( i, j, HCI_CCZ_START_RANGE ) );

        status = hci_set_clutter_region_data( i-1, j, HCI_CCZ_STOP_RANGE,
                hci_get_clutter_region_data( i, j, HCI_CCZ_STOP_RANGE ) );

        status = hci_set_clutter_region_data( i-1, j, HCI_CCZ_SELECT_CODE,
                hci_get_clutter_region_data( i, j, HCI_CCZ_SELECT_CODE ) );

        status = hci_set_clutter_region_data( i-1, j, HCI_CCZ_SEGMENT,
                hci_get_clutter_region_data( i, j, HCI_CCZ_SEGMENT ) );
      }
    }

    /* If maximum number of clutter files had been created,
       make sure last label is reset. */

    if( Num_clutter_files == MAX_CLTR_FILES )
    {
      status = hci_set_clutter_region_file_label( MAX_CLTR_FILES-1, Edit_file_label );
    }

    /* Decrement number of clutter files. */

    Num_clutter_files--;
  }

  XtFree( (char *) position_list );

  /* If low bandwidth user, popup a progress meter window. */

  HCI_PM( "Deleting clutter region file" );

  /* Output the update clutter buffer. */

  status = hci_write_clutter_regions_file();

  /* If deleting the file that is currently displayed, then set
     the current clutter file pointer to the default file. */

  if( (Selected_file_index == Clutter_file_index)
                          ||
      (Clutter_file_index == DEFAULT_FILE_INDEX) )
  {
    Clutter_file_index = DEFAULT_FILE_INDEX;
    Selected_file_index = DEFAULT_FILE_INDEX;
    Load_flag = HCI_YES_FLAG;
  }
  XmListSelectPos( File_dialog_list_widget, Clutter_file_index+1, False );

  Hci_set_control_button_sensitivities();
}

/************************************************************************
 *      Description: This function is called when the "Download" button *
 *                   is selected from the Clutter Regions window.       *
 *                                                                      *
 *      Input:  w - Download button ID                                  *
 *              client_data - unused                                    *
 *              call_data - unused                                      *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

static void Download_button_cb( Widget w, XtPointer cl, XtPointer ca )
{
  HCI_LE_log("Download selected");

  /* Pop up a confirmation window for the download operation. */

  if( Update_flag == HCI_MODIFIED_FLAG )
  {
    sprintf( Buff, "Unsaved edits have been detected.\nYou must save or undo recent changes\nbefore downloading to the RDA.");
    hci_warning_popup( Top_widget, Buff, NULL );
  }
  else
  {
    sprintf( Buff, "You are about to download the currently\ndisplayed clutter regions file.  Do you\nwant to continue?" );
    hci_confirm_popup( Top_widget, Buff, Download_button_cb_accept, Download_button_cb_cancel );
  }
}

/************************************************************************
 *      Description: This function is called when the "No" button       *
 *                   is selected from the Clutter Regions download      *
 *                   confirmation popup window.                         *
 *                                                                      *
 *      Input:  w - No button ID                                        *
 *              client_data - unused                                    *
 *              call_data - unused                                      *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

static void Download_button_cb_cancel( Widget w, XtPointer cl, XtPointer ca )
{
  Download_flag = HCI_NO_FLAG;
  HCI_LE_log("Download aborted");
  HCI_display_feedback( "Clutter Regions download aborted" );
}

/************************************************************************
 *      Description: This function is called when the "Yes" button      *
 *                   is selected from the Clutter Regions download      *
 *                   confirmation popup window.                         *
 *                                                                      *
 *      Input:  w - No button ID                                        *
 *              client_data - unused                                    *
 *              call_data - unused                                      *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

static void Download_button_cb_accept( Widget w, XtPointer cl, XtPointer ca )
{
  HCI_LE_log( "Download confirmed" );
  Download_flag = HCI_YES_FLAG;
}

static void Clutter_download()
{
  int filter;
  int all_bins_filtering = 0;
  int i;
  int status;

  /* Get the clutter properties for the file. */

  for( i = 0; i < hci_get_clutter_region_regions( Clutter_file_index ); i++ )
  {
    filter = hci_get_clutter_region_data( Clutter_file_index,
                                              i, HCI_CCZ_SELECT_CODE );

    if( filter == HCI_CCZ_FILTER_ALL )
    {
      all_bins_filtering = 1;
      HCI_LE_log( "\"All Bins\" Filtering Detected" );
      break;
    }
  }

  if( all_bins_filtering )
  {
    sprintf( Buff, "You have downloaded a Clutter Suppression Regions file\ncontaining \"ALL BINS\" filtering.\n\nIf \"ALL BINS\" is required to address clutter return, it is\nsuggested that you DO NOT use an SZ-2 VCP since \"ALL BINS\"\nclutter filtering with SZ-2 VCPs can cause problems with\ndata quality.\n" );
    hci_warning_popup( Top_widget, Buff, NULL );
  }

  /* If low bandwidth user, popup a progress meter window. */

  HCI_PM( "Downloading Clutter Regions" );

  /* Send the download command to the control_rda task so it can
     send the data to the RDA. */

  status = hci_download_clutter_regions_file( Clutter_file_index );
  HCI_display_feedback( "Clutter Regions downloaded" );
}

/************************************************************************
 *      Description: This function updates the clutter region files     *
 *                   list in the Clutter_region Files window.           *
 *                                                                      *
 *      Input:  NONE                                                    *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

static void Update_clutter_files_list()
{
  int     i;
  int     in;
  char    *file_label;
  int     month, day, year;
  int     hour, minute, second;
  time_t  tm;
  XmString        str;

  if( File_dialog == (Widget) NULL ){ return; }

  /* First, delete the current list contents. */

  XmListDeleteAllItems( File_dialog_list_widget );

  /* Next, read the file list and build a menu item for each
     file with a label. */

  in = 0;

  for( i = 0; i < hci_get_clutter_region_num_files(); i++ )
  {
    file_label = hci_get_clutter_region_file_label(i);

    tm =hci_get_clutter_region_file_time(i);

    if( tm < 1 )
    {
      year = 0; month = 0; day = 0; hour = 0; minute = 0; second = 0;
    }
    else
    {
      unix_time( &tm, &year, &month, &day, &hour, &minute, &second );
    }

    sprintf( Buff,"%2.2d/%2.2d/%4.4d  %2.2d:%2.2d:%2.2d  %s",
             month, day, year, hour, minute, second, file_label );

    str = XmStringCreateLocalized( Buff );
    XmListAddItemUnselected( File_dialog_list_widget, str, 0 );
    XmStringFree( str );
  }

  XmListSelectPos( File_dialog_list_widget, Clutter_file_index+1, False );
}

/************************************************************************
 *      Description: This function is called when the "Reset Zoom"      *
 *                   button is selected from the Clutter Regions        *
 *                   window.  The zoom factor is set to 1:1 and the     *
 *                   radar offset is reset to the center of the         *
 *                   display region.                                    *
 *                                                                      *
 *      Input:  w - Reset Zoom button ID                                *
 *              client_data - unused                                    *
 *              call_data - unused                                      *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

static void Reset_zoom_cb( Widget w, XtPointer cl, XtPointer ca )
{
  HCI_LE_log("Reset Zoom selected");

  /* Reset the magnification factor and radar offsets. */

  Clutter_display_zoom_factor = 1;
  Clutter_display_center_x_offset = 0;
  Clutter_display_center_y_offset = 0;

  /* Refresh the data displayed in the display area. */

  Display_background_product_and_censor_zones();

  /* Call the expose callback to make the updates visible. */

  Draw_area_expose_cb( NULL, NULL, NULL );
}

/************************************************************************
 *      Description: This function is called when one of the            *
 *                   Background radio buttons is selected in the        *
 *                   Clutter Regions window.                            *
 *                                                                      *
 *      Input:  w - radio button ID                                     *
 *              client_data - HCI_CCZ_BACKGROUND_BREF or                *
 *                            HCI_CCZ_BACKGROUND_CFC                    *
 *              call_data - toggle button data                          *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

static void Background_product_type_cb( Widget w, XtPointer cl, XtPointer ca )
{
  XmToggleButtonCallbackStruct    *state =
                (XmToggleButtonCallbackStruct *) ca;

  /* If the button is set, get the product and display it. */

  if( state->set )
  {
    /* The background product type is passed via client data. */

    Clutter_display_background_product_type = (int) cl;

    Refresh_background_flag = 1;
  }
}

/************************************************************************
 *      Description: This function is called when the lock button is    *
 *                   selected in the Clutter Regions window.            *
 *                                                                      *
 *      Input: NONE                                                     *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

static int Lock_cb()
{
  if( hci_lock_open() )
  {
    /* Do nothing when user opens password dialog. */
  }
  else if( hci_lock_loca_selected() )
  {
    /* Do nothing when user selects a LOCA. */
  }
  else if( hci_lock_loca_unlocked() )
  {
    if( hci_lock_URC_unlocked() || hci_lock_ROC_unlocked() )
    {
      Unlocked_loca = HCI_YES_FLAG;

      /* Let user know if another user is currently editing this data. */
 
      if( ORPGCCZ_get_edit_status( ORPGCCZ_ORDA_ZONES ) == ORPGEDLOCK_EDIT_LOCKED)
      {
        sprintf( Buff, "Another user is currently editing clutter\nsuppression regions data. Any changes may be\noverwritten by the other user.");
        hci_warning_popup( Top_widget, Buff, NULL );
      }
 
      /* Set edit advisory lock. */
 
      ORPGCCZ_set_edit_lock( ORPGCCZ_ORDA_ZONES );
    }
  }
  else if( hci_lock_close() && Unlocked_loca == HCI_YES_FLAG )
  {
    Unlocked_loca = HCI_NO_FLAG;

    /* Clear edit advisory locks so users won't get warned. */

    ORPGCCZ_clear_edit_lock( ORPGCCZ_ORDA_ZONES );
  }

  Hci_set_control_button_sensitivities();

  return HCI_LOCK_PROCEED;
}

/************************************************************************
 *      Description: This function is called when the "Restore" button  *
 *                   selected in the Clutter Regions window.            *
 *                                                                      *
 *      Input:  w - Restore button ID                                   *
 *              client_data - unused                                    *
 *              call_data - unused                                      *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

static void Restore_button_cb( Widget w, XtPointer cl, XtPointer ca )
{
  /* Popup a confirmation window before changing any data. */

  sprintf( Buff, "You are about to restore the clutter regions\nadaptation data to baseline values.\nDo you want to continue?" );
  hci_confirm_popup( Top_widget, Buff, Restore_button_cb_accept, NULL );
}

/************************************************************************
 *      Description: This function is called when the "Yes" button      *
 *                   selected in the Clutter Regions restore            *
 *                   confirmation popup window.                         *
 *                                                                      *
 *      Input:  w - Yes button ID                                       *
 *              client_data - unused                                    *
 *              call_data - unused                                      *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

static void Restore_button_cb_accept( Widget w, XtPointer cl, XtPointer ca )
{
  Restore_flag = HCI_YES_FLAG;
}

static void Restore_baseline()
{
  int status;

  /* Copy the backup message to the current message. */

  status = ORPGCCZ_baseline_to_default( ORPGCCZ_ORDA_ZONES );

  /* Generate a message based on the success of the copy. Post
     an HCI event to the HCI Control/Status task will display
     the message in the feedback line. */

  if( status < 0 )
  {
    HCI_LE_error( "Unable to restore ORDA CENSOR ZONES" );
    HCI_display_feedback( "Unable to restore baseline Clutter Regions data" );
  }
  else
  {
    HCI_LE_status( "Clutter Regions data restored from baseline" );
    HCI_display_feedback( "Clutter Regions data restored from baseline" );
  }

  Selected_file_index = DEFAULT_FILE_INDEX;
  Clutter_file_index = DEFAULT_FILE_INDEX;
}

/************************************************************************
 *      Description: This function is called when the "Update" button   *
 *                   selected in the Clutter Regions window.            *
 *                                                                      *
 *      Input:  w - Restore button ID                                   *
 *              client_data - unused                                    *
 *              call_data - unused                                      *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

static void Update_button_cb( Widget w, XtPointer cl, XtPointer ca )
{
  /* Display a confirmation prompt before updating anything. */

  sprintf( Buff, "You are about to update the baseline\nClutter Regions adaptation data.\nDo you want to continue?" );
  hci_confirm_popup( Top_widget, Buff, Update_button_cb_accept, NULL );
}

/************************************************************************
 *      Description: This function is called when the "Yes" button      *
 *                   selected in the Clutter Regions update             *
 *                   confirmation popup window.                         *
 *                                                                      *
 *      Input:  w - Yes button ID                                       *
 *              client_data - unused                                    *
 *              call_data - unused                                      *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

static void Update_button_cb_accept( Widget w, XtPointer cl, XtPointer ca )
{
  Update_baseline_flag = HCI_YES_FLAG;
}

static void Update_baseline()
{
  int status;

  /* Copy the current message to the backup message. */

  status = ORPGCCZ_default_to_baseline( ORPGCCZ_ORDA_ZONES );


  /* Generate a message based on the success of the copy. Post
     an HCI event to the HCI Control/Status task will display
     the message in the feedback line. */

  if( status < 0 )
  {
    HCI_LE_error( "Unable to update ORDA BASELINE CENSOR ZONES" );
    HCI_display_feedback( "Unable to update baseline Clutter Regions data" );
  }
  else
  {
    HCI_LE_status( "Clutter Regions baseline data updated" );
    HCI_display_feedback( "Clutter Regions baseline data updated" );
  }
}

/************************************************************************
 *      Description: This function loads the latest specified product   *
 *                   type from the RPG products database.               *
 *                                                                      *
 *      Input:  background_product_type - type of background product    *
 *                                      HCI_CCZ_BACKGROUND_BREF         *
 *                                      HCI_CCZ_BACKGROUND_CFC          *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

static void Load_background_product( int background_product_type )
{
  int     i, j;
  int     products;
  int     retval;
  short   params [6];
  int     julian_seconds;
  int     diff, min_diff;
  int     vol_t_index;

  /* Initialize the parameter buffer. */

  for( i = 0; i < 6;i++ ){ params[i] = 0; }

  /* Check the background product type. */

  switch( background_product_type )
  {
    case HCI_CCZ_BACKGROUND_BREF :

      HCI_LE_log( "Reflectivity Background selected" );

      XtManageChild( Background_product_rowcol );
      XtUnmanageChild( Cfc_rowcol );

      /* Query the RPG products database and retrieve a list of
         the latest products available.  We can then check this
         list to determine the best cut to use for the selected
         segment.  This is far superior than assuming a specific
         cut is available.  This new scheme is VCP independent.

         Lets query the database for the latest two volume times
         NOTE: We query the latest 2 times since the elevation
         cut we want may not yet be available for the latest
         volume time. */

      Query_data[1].field = RPGP_VOLT;
      Query_data[1].value = 0;
      Query_data[0].field = ORPGDBM_MODE;
      Query_data[0].value = ORPGDBM_FULL_SEARCH |
                            ORPGDBM_HIGHEND_SEARCH |
                            ORPGDBM_DISTINCT_FIELD_VALUES;

      products = ORPGDBM_query( Db_info, Query_data, 2, 2 );

      for( i = 0; i < products; i++ ){ Query_time [i] = Db_info[i].vol_t; }

      Background_product_status_flag = 0;
 
      for( i = 0; i < products; i++ )
      {
        /* Check to see if the time of the latest product is
           more than 15 minutes old. If so, we do not want to
           display any data. */

        julian_seconds = (ORPGVST_get_volume_date()-1)*HCI_SECONDS_PER_DAY +
                                      ORPGVST_get_volume_time()/1000;

        if( (julian_seconds - Query_time[i]) > 15*60 )
        {
          Background_product_status_flag = HCI_CCZ_DATA_TIMED_OUT;
          return;
        }

        /* Set up the query parameters for the previous
           time query and get all instances of the current
           background product for that time.  Do a low end
           search so that the smallest cuts are listed first. */

        Query_data[1].field = RPGP_VOLT;
        Query_data[1].value = Query_time[i];
        Query_data[2].field = RPGP_PCODE;
        Query_data[2].value = ORPGPAT_get_code( Clutter_display_background_product_id );
        Query_data[0].field = ORPGDBM_MODE;
        Query_data[0].value = ORPGDBM_EXACT_MATCH |
                              ORPGDBM_ALL_FIELD_VALUES |
                              ORPGDBM_LOWEND_SEARCH;

        retval = ORPGDBM_query( Db_info, Query_data, 3, 50 );

        min_diff = 999;
        vol_t_index = -1;
        for( j = 0; j < retval; j++ )
        {

          if( Edit_segment == HCI_CCZ_SEGMENT_ONE )
          {
            if( Db_info[j].params[2] <= Segment_one_max_elevation )
            {
              diff = Db_info[j].params[2];
              if( diff < min_diff )
              {
                params [2] = Db_info[j].params[2];
                min_diff = diff;
                vol_t_index = j;
              }
            }
          }
          else if( Edit_segment == HCI_CCZ_SEGMENT_TWO )
          {
            if( ( Db_info[j].params[2] > Segment_one_max_elevation )
                                                    &&
                ( Db_info[j].params[2] <= Segment_two_max_elevation ) )
            {
              diff = Db_info[j].params[2] - Segment_one_max_elevation;
              if( diff < min_diff )
              {
                params [2] = Db_info[j].params[2];
                min_diff = diff;
                vol_t_index = j;
              }
            }

          }
          else if( Edit_segment == HCI_CCZ_SEGMENT_THREE )
          {
            if( ( Db_info[j].params[2] > Segment_two_max_elevation )
                                                     &&
                ( Db_info[j].params[2] <= Segment_three_max_elevation ) )
            {
              diff = Db_info[j].params[2] - Segment_two_max_elevation;
              if( diff < min_diff )
              {
                params [2] = Db_info[j].params[2];
                min_diff = diff;
                vol_t_index = j;
              }
            }
          }
          else if( Edit_segment == HCI_CCZ_SEGMENT_FOUR )
          {
            if( ( Db_info[j].params[2] > Segment_three_max_elevation )
                                                    &&
                ( Db_info[j].params[2] <= Segment_four_max_elevation ) )
            {
              diff = Db_info[j].params[2] - Segment_three_max_elevation;
              if( diff < min_diff )
              {
                params [2] = Db_info[j].params[2];
                min_diff = diff;
                vol_t_index = j;
              }
            }
          }
          else if( Edit_segment == HCI_CCZ_SEGMENT_FIVE )
          {
            if( Db_info[j].params[2] > Segment_four_max_elevation )
            {
              diff = Db_info[j].params[2] - Segment_four_max_elevation;
              if( diff < min_diff )
              {
                params [2] = Db_info[j].params[2];
                min_diff = diff;
                vol_t_index = j;
              }
            }
          }
        } /* End of j for loop. */

        if( (min_diff < 999) && (vol_t_index >= 0 ) )
        {
          Background_product_status_flag =
                 hci_load_product_data( Clutter_display_background_product_id,
                 Db_info[vol_t_index].vol_t, params );
          if( Background_product_status_flag > 0 ){ break; }
        }
      } /* End of i for loop */

      break;

    case HCI_CCZ_BACKGROUND_CFC :

      /* If the background product is the CFC product, then we need
         to determine the product from the currently active segment. */

      HCI_LE_log( "CFC Background selected" );

      XtUnmanageChild( Background_product_rowcol );
      XtManageChild( Cfc_rowcol );

      params[0] = ( ONE << Edit_segment );

      Background_product_status_flag =
                      hci_load_product_data( CFCPROD, -1, params );
      break;

  }/* End switch */

  if( Background_product_status_flag == RMT_CANCELLED )
  {
    HCI_task_exit( HCI_EXIT_SUCCESS) ;
  }
}

/************************************************************************
 *      Description: This function is called when the user selects      *
 *                   a background product from the background product   *
 *                   dropdown list.                                     *
 *                                                                      *
 *      Input:  w - List widget ID                                      *
 *              client_data - unused                                    *
 *              call_data - unused                                      *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

static void Background_product_id_cb( Widget w, XtPointer cl, XtPointer ca )
{
  int status;
  XmString str;
  char *buf;

  XmComboBoxCallbackStruct *cbs =
                (XmComboBoxCallbackStruct *) ca;

  /* Set the new background product ID from the background product
     lookup table using the menu item position as the index. */

  Clutter_display_background_product_id = Clutter_display_background_product_LUT[cbs->item_position];

  HCI_LE_status( "Setting new background product ID [%d]",
                 ORPGPAT_get_code( Clutter_display_background_product_id ) );

  /* Format a new label to the background product radio button. */

  sprintf( Buff,"%s[%d] - %-50s   ",
           ORPGPAT_get_mnemonic( Clutter_display_background_product_id ),
           ORPGPAT_get_code( Clutter_display_background_product_id ),
           ORPGPAT_get_description( Clutter_display_background_product_id, STRIP_MNEMONIC ) );

  str = XmStringCreateLocalized( Buff );

  XtVaSetValues( Background_product_button, XmNlabelString, str, NULL );

  XmStringFree( str );

  /* Modify the Clutter Regions Editor task data message so the
     HCI agent can build a new RPS list to force generation of
     the new background product. */

  buf = (char *) calloc( 1, ALIGNED_SIZE( sizeof( Hci_ccz_data_t ) ) );

  status = ORPGDA_read( ORPGDAT_HCI_DATA, buf,
                        ALIGNED_SIZE( sizeof( Hci_ccz_data_t ) ),
                        HCI_CCZ_TASK_DATA_MSG_ID );

  if( status <= 0 )
  {
    HCI_LE_error( "Unable to read task configuration data (%d)", status) ;
  }
  else
  {
    Hci_ccz_data_t *config_data = (Hci_ccz_data_t *) buf;

    if( HCI_is_low_bandwidth() )
    {
      config_data->low_product = ORPGPAT_get_code( Clutter_display_background_product_id );
    }
    else
    {
      config_data->high_product = ORPGPAT_get_code( Clutter_display_background_product_id );
    }

    status = ORPGDA_write( ORPGDAT_HCI_DATA, buf,
                           ALIGNED_SIZE( sizeof( Hci_ccz_data_t ) ),
                           HCI_CCZ_TASK_DATA_MSG_ID );

    if( status <= 0 )
    {
      HCI_LE_error( "Unable to update task configuration data (%d)", status );
    }
    else
    {
      HCI_LE_status("Background product changed [%d]", Clutter_display_background_product_id);
    }
  }

  Refresh_background_flag = 1;
}

/************************************************************************
 *      Description: This function is called when the user wants to     *
 *                   refresh the background product.                    *
 ************************************************************************/

static void Refresh_button_cb( Widget w, XtPointer cl, XtPointer ca )
{
  Refresh_background_flag = 1;
}

/************************************************************************
 *      Description: This function is called to copy the clutter file   *
 *                   data to the edit buffer.                           *
 *                                                                      *
 *      Input:  NONE                                                    *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

static void Copy_file_to_edit_buffer()
{
  int count, segment;
  int i;

  /* Get the number of edit regions in this file. */

  Edit_num_regions = hci_get_clutter_region_regions (Clutter_file_index);

  count = 0;
  for( i = 0; i < Edit_num_regions; i++ )
  {
    segment = hci_get_clutter_region_data( Clutter_file_index, i, HCI_CCZ_SEGMENT );

    /* Remove invalid segments. */
    if( segment > Num_valid_segments ){ continue; }

    Edit_data[count].start_azimuth = (float) hci_get_clutter_region_data(
                              Clutter_file_index, i, HCI_CCZ_START_AZIMUTH );
    Edit_data[count].stop_azimuth = (float) hci_get_clutter_region_data(
                              Clutter_file_index, i, HCI_CCZ_STOP_AZIMUTH );
    Edit_data[count].start_range = (float) hci_get_clutter_region_data(
                              Clutter_file_index, i, HCI_CCZ_START_RANGE );
    Edit_data[count].stop_range = (float) hci_get_clutter_region_data(
                              Clutter_file_index, i, HCI_CCZ_STOP_RANGE );
    Edit_data[count].segment = segment;
    Edit_data[count].select_code = hci_get_clutter_region_data(
                              Clutter_file_index, i, HCI_CCZ_SELECT_CODE );
    count++;
  }

  Edit_num_regions = count;
}

/************************************************************************
 *      Description: This function is called when the rda adaptation    *
 *                   data buffer is updated.                            *
 ************************************************************************/

static void Rda_adaptation_updated( int f, LB_id_t m, int i, void *a )
{
  Rda_adapt_updated_flag = HCI_YES_FLAG;
}

/************************************************************************
 *      Description: This function is called when the rda adaptation    *
 *                   data buffer is updated.                            *
 ************************************************************************/

static void Read_rda_adaptation()
{
  char rda_adapt_msg[ sizeof( ORDA_adpt_data_msg_t ) ];
  ORDA_adpt_data_msg_t *rda_adapt_data;
  int status = -1;
  float scale = 10.0;
  int temp_int = -1;
  static int initialized_rda_adaptation_data = 0;
  static int prev_num_valid_segments = -1;

  /* Read message in LB */

  status = ORPGDA_read( ORPGDAT_RDA_ADAPT_DATA,
                        ( char * ) rda_adapt_msg,
                        sizeof( ORDA_adpt_data_msg_t ),
                        ORPGDAT_RDA_ADAPT_MSG_ID ); 

  if( status > 0 )
  {
    rda_adapt_data = (ORDA_adpt_data_msg_t *) rda_adapt_msg;
    temp_int = Num_valid_segments; 
    Num_valid_segments = rda_adapt_data->rda_adapt.nbr_el_segments;

    /* If number of segments has changed, let user know. */
    /* Otherwise, read in segments and proceed. */

    if( Num_valid_segments < HCI_CCZ_MIN_NUM_SEGS )
    {
      sprintf( Buff, "The number of segments from the RDA has changed from %d to %d which is fewer than the allowed minimum.\nThis gui cannot continue and will exit.", prev_num_valid_segments, Num_valid_segments );
      hci_warning_popup( Top_widget, Buff, NULL );
      HCI_LE_error("Number of RDA segments (%d) is less than allowed minimum",
            Num_valid_segments );
      HCI_task_exit( HCI_EXIT_FAIL );
    }
    else if( Num_valid_segments > HCI_CCZ_MAX_NUM_SEGS )
    {
      sprintf( Buff, "The number of segments from the RDA has changed from %d to %d which is more than the allowed maximum.\nThis gui cannot continue and will exit.", prev_num_valid_segments, Num_valid_segments );
      hci_warning_popup( Top_widget, Buff, NULL );
      HCI_LE_error("Number of RDA segments (%d) is more than allowed maximum",
            Num_valid_segments );
      HCI_task_exit( HCI_EXIT_FAIL );
    }

    if( ( Num_valid_segments != prev_num_valid_segments ) &&
        ( initialized_rda_adaptation_data ) )
    {
      sprintf( Buff, "The number of segments from the RDA has changed from %d to %d.\nTo use the new segments, close and re-open this gui.", prev_num_valid_segments, Num_valid_segments );
      hci_warning_popup( Top_widget, Buff, NULL );
      Num_valid_segments = temp_int;
    }
    else
    {
      prev_num_valid_segments = Num_valid_segments;

      if( Num_valid_segments > 1 )
        Segment_one_max_elevation = rda_adapt_data->rda_adapt.seg1lim * scale;

      if( Num_valid_segments > 2 )
        Segment_two_max_elevation = rda_adapt_data->rda_adapt.seg2lim * scale;

      if( Num_valid_segments > 3 )
        Segment_three_max_elevation = rda_adapt_data->rda_adapt.seg3lim * scale;

      if( Num_valid_segments > 4 )
        Segment_four_max_elevation = rda_adapt_data->rda_adapt.seg4lim * scale;
    }
    initialized_rda_adaptation_data = 1;
  }
  else
  {
    HCI_LE_error("Unable to read RDA adaptation data (%d)", status );
    HCI_task_exit( HCI_EXIT_FAIL );
  }
}

/************************************************************************
 *      Description: This function is used to regularly check for       *
 *                   changes to the system.                             *
 *                                                                      *
 *      Input:  NONE                                                    *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

static void Timer_proc()
{
  static time_t previous_RDA_Status_time = 0;
         time_t current_RDA_Status_time = 0;
         int    current_RDA_channel_number = 0;

  /* If the RDA configuration changes, call function to display
     popup window and exit. */

   if( ORPGRDA_get_rda_config( NULL ) != ORPGRDA_ORDA_CONFIG )
   {
     Rda_config_change();
   }

  if( Initialization_error_flag != NO_INIT_ERROR )
  {
    if( Initialization_error_flag & READ_CLUTTER_FAILED )
    {
      hci_error_popup( Top_widget, "Failure while reading clutter file information.\nUnable to continue.", Close_button_cb );
    }
    else if( Initialization_error_flag & READ_DOWNLOAD_INFO_FAILED )
    {
      hci_warning_popup( Top_widget, "Unable to read information for the previously\ndownloaded clutter file. The Default clutter\nfile will be displayed.", NULL );
    }
    else if( Initialization_error_flag & DOWNLOAD_FILE_DELETED )
    {
      hci_warning_popup( Top_widget, "Previously downloaded clutter file does not match any\ncurrent clutter files. Either part of the downloaded\nfile was rejected or the file has since been deleted.\nThe Default clutter file will be displayed.", NULL );
    }
    Initialization_error_flag = NO_INIT_ERROR;
  }

  if( System_type == HCI_NWSR_SYSTEM )
  {
    current_RDA_Status_time = ORPGRDA_status_update_time();
    if( current_RDA_Status_time != previous_RDA_Status_time )
    {
      previous_RDA_Status_time = current_RDA_Status_time;
      current_RDA_channel_number = ORPGRDA_channel_num();
      if( current_RDA_channel_number != Channel_number )
      {
        HCI_LE_log( "NWSR channel updated from %d to %d",
                    Channel_number, current_RDA_channel_number );
        Channel_number = current_RDA_channel_number;
        Set_download_info_label();
      }
    }
  }

  if( CCZ_updated_flag == HCI_YES_FLAG )
  {
    HCI_LE_log( "CCZ updated" );
    CCZ_updated_flag = HCI_NO_FLAG;
    hci_read_clutter_download_info();
    Set_download_info_label();
    Load_flag = HCI_YES_FLAG;
  }

  if( Rda_adapt_updated_flag == HCI_YES_FLAG )
  {
    HCI_LE_log( "RDA adaptation data updated" );
    Rda_adapt_updated_flag = HCI_NO_FLAG;
    Read_rda_adaptation();
    Hci_set_segment_button_sensitivities();
  }

  if( Save_flag == HCI_YES_FLAG )
  {
    HCI_LE_log( "Save flag" );
    Save_flag = HCI_NO_FLAG;
    Clutter_file_save();
  }

  if( Save_as_flag == HCI_YES_FLAG )
  {
    HCI_LE_log( "Save as flag" );
    Save_as_flag = HCI_NO_FLAG;
    Clutter_file_save_as();
  }

  if( Restore_flag == HCI_YES_FLAG )
  {
    HCI_LE_log( "Restore flag" );
    Restore_flag = HCI_NO_FLAG;
    Restore_baseline();
    Load_flag = HCI_YES_FLAG;
  }
 
  if( Update_baseline_flag == HCI_YES_FLAG )
  {
    HCI_LE_log( "Update baseline flag" );
    Update_baseline_flag = HCI_NO_FLAG;
    Update_baseline();
  }

  if( Load_flag == HCI_YES_FLAG )
  {
    Load_flag = HCI_NO_FLAG;
    Clutter_load();
    Clutter_draw();
  }

  if( Download_flag == HCI_YES_FLAG )
  {
    Download_flag = HCI_NO_FLAG;
    Clutter_download();
  }

  if( Refresh_background_flag )
  {
    Refresh_background_flag = 0;
    Load_background_product( Clutter_display_background_product_type );
    Display_background_product_and_censor_zones();
    Draw_area_expose_cb( NULL, NULL, NULL );
  }
}

/************************************************************************
 *  Description: This function is called when the CCZ DEAU data         *
 *               is updated.                                            *
 *                                                                      *
 *  Input:  NONE                                                        *
 *  Output: NONE                                                        *
 *  Return: NONE                                                        *
 ************************************************************************/

static void Ccz_deau_cb()
{
  CCZ_updated_flag = HCI_YES_FLAG;
}

/************************************************************************
 *  Description: This function is called when the RDA configuration     *
 *         changes.                                                     *
 *                                                                      *
 *  Input:  NONE                                                        *
 *  Output: NONE                                                        *
 *  Return: NONE                                                        *
 ************************************************************************/

static void Rda_config_change()
{
   if( Top_widget != (Widget) NULL && !Config_change_popup )
   {
     Config_change_popup = 1; /* Indicate popup has been launched. */
     hci_rda_config_change_popup();
   }
   else
   {
     return;
   }
}

/************************************************************************
 *      Description: Determine time the bypass map was created.         *
 ************************************************************************/

static void Time_bypass_map_created( int *yr, int *mo, int *dy, int *hr, int *mn )
{
  int temp;

  calendar_date( ORPGRDA_get_status(RS_BPM_GEN_DATE), dy, mo, yr );

  temp = ORPGRDA_get_status(RS_BPM_GEN_TIME);
  *hr = temp/60;
  *mn = temp%60;
}

/************************************************************************
 *      Description: Determine time the clutter_filter map was created. *
 ************************************************************************/

static void Time_clutter_map_created( int *yr, int *mo, int *dy, int *hr, int *mn )
{
  int temp;

  calendar_date( ORPGRDA_get_status(RS_NWM_GEN_DATE), dy, mo, yr );

  temp = ORPGRDA_get_status(RS_NWM_GEN_TIME);
  *hr = temp/60;
  *mn = temp%60;
}

/************************************************************************
 *      Description: Determine time the product was created.            *
 ************************************************************************/

static void Time_product_created( int *yr, int *mo, int *dy, int *hr, int *mn )
{
  int temp;

  calendar_date( hci_product_date(), dy, mo, yr );

  temp = hci_product_time();
  *hr = temp/3600;
  *mn = (temp - (*hr*3600))/60;
}

/************************************************************************
 *      Description: Draw labels on product/display.                    *
 ************************************************************************/

static void Draw_labels_on_display()
{
  int month, day, year, hour, minute;
  int string_width;

  Time_product_created( &year, &month, &day, &hour, &minute );

  Hci_set_foreground( WHITE );

  sprintf( Buff, "%02d/%02d/%02d %02d:%02dZ", month, day, year, hour, minute );

  Hci_draw_string( 5, (Clutter_display_height-10), Buff );

  strcpy( Buff, "PRODUCT" );

  Hci_draw_string( 5, (Clutter_display_height-25), Buff );

  Time_bypass_map_created( &year, &month, &day, &hour, &minute );

  sprintf( Buff,"%02d/%02d/%02d %02d:%02dZ", month, day, year, hour, minute );

  string_width = XTextWidth( Small_fontinfo,
                             Buff, strlen( Buff ) ) + LABEL_OFFSET;

  Hci_draw_string( (Clutter_display_width-string_width), (Clutter_display_height-10), Buff );

  sprintf( Buff,"BYPASS MAP" );

  string_width = XTextWidth( Small_fontinfo,
                             Buff, strlen( Buff ) ) + LABEL_OFFSET;

  Hci_draw_string( (Clutter_display_width-string_width), (Clutter_display_height-25), Buff );

  if( ( ORPGRDA_get_status( ORPGRDA_CMD ) >> Edit_segment ) & 0x1 )
  {
    sprintf( Buff, "CMD" );

    string_width = XTextWidth( Small_fontinfo,
                               Buff, strlen( Buff ) ) + LABEL_OFFSET;

    Hci_draw_string( (Clutter_display_width-string_width), (Clutter_display_height-40), Buff );
  }

  Time_clutter_map_created( &year, &month, &day, &hour, &minute );

  sprintf( Buff, "%02d/%02d/%02d %02d:%02dZ", month, day, year, hour, minute );

  string_width = XTextWidth( Small_fontinfo,
                             Buff, strlen( Buff ) ) + LABEL_OFFSET;

  Hci_draw_string( (Clutter_display_width-string_width), 30, Buff );

  sprintf( Buff, "CLUTTER REGIONS" );

  string_width = XTextWidth( Small_fontinfo,
                             Buff, strlen( Buff ) ) + LABEL_OFFSET;

  Hci_draw_string( (Clutter_display_width-string_width), 15, Buff );
}

/************************************************************************
 *      Description: Draw label when background product unavailable.    *
 ************************************************************************/

static void Draw_no_background_label()
{
  int string_width;

  /* Fill in strip to right of display and left of Clutter Map with
     background color. */

  Hci_set_foreground( BACKGROUND_COLOR1 );
  Hci_fill_rectangle( Clutter_display_width, 0, COLOR_BAR_WIDTH, Clutter_display_height );

  /* Message to display. */

  if (Background_product_status_flag == HCI_CCZ_DATA_TIMED_OUT)
  {
    sprintf( Buff, "Background Product Data Expired" );
  }
  else
  {
    sprintf( Buff, "Background Product Data Not Available" );
  }

  Hci_set_display_font( LARGE );

  Hci_set_foreground( PRODUCT_FOREGROUND_COLOR );

  string_width = XTextWidth( Large_fontinfo, Buff, strlen( Buff ) );

  Hci_draw_string( (Clutter_display_width/2-string_width/2), (Clutter_display_height/3), Buff );

  Hci_set_display_font( SMALL );
}

static void Copy_pixmap_to_window()
{
  /* Copy the pixmap to the visible window so that any parts of the
     window which may have been obscured by another window will be
     visible. */
 
  XCopyArea( Clutter_display, Clutter_pixmap, Clutter_window, Clutter_gc,
             0, 0, Drawing_area_width, Drawing_area_height, 0, 0 );
}

static void Hci_set_foreground( int color )
{
  XSetForeground( Clutter_display, Clutter_gc, hci_get_read_color( color ) );
}

static void Hci_set_background( int color )
{
  XSetBackground( Clutter_display, Clutter_gc, hci_get_read_color( color ) );
}

static void Hci_set_display_font( int font )
{
  XSetFont( Clutter_display, Clutter_gc, hci_get_font( font ) );
}

static void Hci_set_line_width( int line_width )
{
  XSetLineAttributes( Clutter_display, Clutter_gc, line_width,
                      LineSolid, CapButt, JoinMiter );
}

static void Hci_fill_polygon( XPoint *x, int num_points )
{
  XFillPolygon( Clutter_display, Clutter_pixmap, Clutter_gc,
                x, num_points, Convex, CoordModeOrigin );
}

static void Hci_fill_rectangle( int x, int y, int width, int height )
{
  XFillRectangle( Clutter_display, Clutter_pixmap, Clutter_gc,
                  x, y, width, height );
}

static void Hci_draw_string( int x, int y, char *buf )
{
  XDrawString( Clutter_display, Clutter_pixmap, Clutter_gc,
               x, y, buf, strlen( buf ) );
}

static void Hci_draw_rectangle( int x, int y, int width, int height )
{
  XDrawRectangle( Clutter_display, Clutter_pixmap, Clutter_gc,
                  x, y, width, height );
}

static void Hci_draw_line( Drawable window, XPoint *x, int num_points )
{
  XDrawLines( Clutter_display, window, Clutter_gc,
              &x[0], num_points, CoordModeOrigin );
}

static void Hci_draw_imagestring( int x, int y, char *buf )
{
  XDrawImageString( Clutter_display, Clutter_window, Clutter_gc,
                    x, y, buf, strlen( buf ) );
}

static void Hci_set_control_button_sensitivities()
{
  /* Desensitize the Undo button and sensitize the Update and
     Restore buttons (we assume that the window is unlocked). */

  if( Update_flag == HCI_NOT_MODIFIED_FLAG )
  {
    XtSetSensitive( Undo_button, False ); 
    if( Unlocked_loca == HCI_YES_FLAG )
    {
      XtSetSensitive( Update_button, True ); 
      XtSetSensitive( Restore_button, True ); 
    }
    else
    {
      XtSetSensitive( Update_button, False ); 
      XtSetSensitive( Restore_button, False ); 
    }
  }
  else
  {
    XtSetSensitive( Undo_button, True ); 
    XtSetSensitive( Update_button, False ); 
    XtSetSensitive( Restore_button, False ); 
  }

  /* If the File window is open, set button sensitivities there. */

  if( File_dialog != (Widget) NULL )
  {
    if( Unlocked_loca == HCI_NO_FLAG )
    {
      XtSetSensitive( File_dialog_delete_button, False );
      XtSetSensitive( File_dialog_save_button, False );
    }
    else
    {
      if( (Selected_file_index == Clutter_file_index) && (New_file_flag == HCI_YES_FLAG) )
      {
        XtSetSensitive( File_dialog_delete_button, False );
      }
      else
      {
        XtSetSensitive( File_dialog_delete_button, True );
      }
      if( Update_flag == HCI_MODIFIED_FLAG &&
          New_file_flag == HCI_NO_FLAG &&
          Clutter_file_index != DEFAULT_FILE_INDEX &&
          Selected_file_index == Clutter_file_index )
      {
        XtSetSensitive( File_dialog_save_button, True );
      }
      else
      {
        XtSetSensitive( File_dialog_save_button, False );
      }
    }

    if( Selected_file_index == Clutter_file_index )
    {
      XtSetSensitive( File_dialog_open_button, False );
      XtSetSensitive( File_dialog_save_as_button, True );
    }
    else
    {
      XtSetSensitive( File_dialog_open_button, True );
      XtSetSensitive( File_dialog_save_as_button, False );
    }

    if( New_file_flag == HCI_YES_FLAG )
    {
      XtSetSensitive( File_dialog_new_button, False );
    }
    else
    {
      XtSetSensitive( File_dialog_new_button, True );
    }

  }

  /* Only sensitize download button if Wideband is connected */

  if( ORPGRDA_get_wb_status( ORPGRDA_WBLNSTAT ) != RS_CONNECTED )
  {
    XtSetSensitive( Download_button, False );
  }
  else
  {
    XtSetSensitive( Download_button, True );
  }
}

static void Hci_set_segment_button_sensitivities()
{
  if( Num_valid_segments < 1 )
  {
    XtSetSensitive( Segment1_button, False );
    XtSetSensitive( Segment2_button, False );
    XtSetSensitive( Segment3_button, False );
    XtSetSensitive( Segment4_button, False );
    XtSetSensitive( Segment5_button, False );
  }
  else if( Num_valid_segments < 2 )
  {
    XtSetSensitive( Segment1_button, True );
    XtSetSensitive( Segment2_button, False );
    XtSetSensitive( Segment3_button, False );
    XtSetSensitive( Segment4_button, False );
    XtSetSensitive( Segment5_button, False );
  }
  else if( Num_valid_segments < 3 )
  {
    XtSetSensitive( Segment1_button, True );
    XtSetSensitive( Segment2_button, True );
    XtSetSensitive( Segment3_button, False );
    XtSetSensitive( Segment4_button, False );
    XtSetSensitive( Segment5_button, False );
  }
  else if( Num_valid_segments < 4 )
  {
    XtSetSensitive( Segment1_button, True );
    XtSetSensitive( Segment2_button, True );
    XtSetSensitive( Segment3_button, True );
    XtSetSensitive( Segment4_button, False );
    XtSetSensitive( Segment5_button, False );
  }
  else if( Num_valid_segments < 5 )
  {
    XtSetSensitive( Segment1_button, True );
    XtSetSensitive( Segment2_button, True );
    XtSetSensitive( Segment3_button, True );
    XtSetSensitive( Segment4_button, True );
    XtSetSensitive( Segment5_button, False );
  }
  else
  {
    XtSetSensitive( Segment1_button, True );
    XtSetSensitive( Segment2_button, True );
    XtSetSensitive( Segment3_button, True );
    XtSetSensitive( Segment4_button, True );
    XtSetSensitive( Segment5_button, True );
  }
}

static void Set_file_info_label()
{
  int month, day, year, hour, minute, second;
  char *filename;
  time_t tm;
  XmString str;
  int i;

  filename = hci_get_clutter_region_file_label( Clutter_file_index );

  if( filename == NULL ||
      ( strlen( filename ) == 0 && New_file_flag == HCI_YES_FLAG ) )
  {
    sprintf( Buff, "            Unknown            " );
  }
  else
  {
    /* Center align filename. */
    if( strlen(filename) < MAX_LABEL_SIZE )
    {
      for( i=0; i<(MAX_LABEL_SIZE -strlen(filename))/2; i++ )
      {
        Buff[i]=' ';
      }
      sprintf( &Buff[i], "%s", filename );
      for( i=i+strlen(filename); i<MAX_LABEL_SIZE; i++ )
      {
        Buff[i]=' ';
      }
    }
    else
    {
      strncpy( Buff, filename, MAX_LABEL_SIZE-1 );
    }
    Buff[MAX_LABEL_SIZE-1]='\0';
  }

  str = XmStringCreateLocalized( Buff );
  XtVaSetValues( Currently_displayed_file_label, XmNlabelString, str, NULL );
  XmStringFree( str );

  tm = hci_get_clutter_region_file_time( Clutter_file_index );

  if( tm < 1 )
  {
    sprintf( Buff,"       Unknown        " );
  }
  else
  {
    unix_time( &tm, &year, &month, &day, &hour, &minute, &second );

    sprintf( Buff,"%2.2d/%2.2d/%4.4d %2.2d:%2.2d:%2.2d UT",
             month, day, year, hour, minute, second);
  }

  str = XmStringCreateLocalized( Buff );
  XtVaSetValues( Time_last_modified_label, XmNlabelString, str, NULL );
  XmStringFree( str );
}

static void Create_save_as_dialog()
{
  Widget form;
  Widget rowcol;
  Widget label;
  Widget save_as_dialog_accept_button;
  Widget save_as_dialog_cancel_button;

  HCI_Shell_init( &Save_as_dialog, "Clutter Region Filename" );

  form = XtVaCreateWidget( "File_dialog_save_as_form",
                xmFormWidgetClass,    Save_as_dialog,
                XmNbackground,        hci_get_read_color( BACKGROUND_COLOR1 ),
                NULL );

  rowcol = XtVaCreateWidget( "file_save_as_rowcol",
                xmRowColumnWidgetClass, form,
                XmNleftAttachment,    XmATTACH_FORM,
                XmNtopAttachment,     XmATTACH_FORM,
                XmNrightAttachment,   XmATTACH_FORM,
                XmNpacking,           XmPACK_COLUMN,
                XmNorientation,       XmHORIZONTAL,
                XmNbackground,        hci_get_read_color( BACKGROUND_COLOR1 ),
                NULL );

  save_as_dialog_cancel_button = XtVaCreateManagedWidget( "Cancel",
                xmPushButtonWidgetClass,rowcol,
                XmNforeground,        hci_get_read_color( BUTTON_FOREGROUND ),
                XmNbackground,        hci_get_read_color( BUTTON_BACKGROUND ),
                XmNfontList,          hci_get_fontlist( LIST ),
                NULL );

  XtAddCallback( save_as_dialog_cancel_button, XmNactivateCallback, Save_as_dialog_cancel_button_cb, NULL );

  save_as_dialog_accept_button = XtVaCreateManagedWidget( "Accept",
                xmPushButtonWidgetClass,rowcol,
                XmNforeground,        hci_get_read_color( BUTTON_FOREGROUND ),
                XmNbackground,        hci_get_read_color( BUTTON_BACKGROUND ),
                XmNfontList,          hci_get_fontlist( LIST ),
                NULL );

  XtAddCallback( save_as_dialog_accept_button, XmNactivateCallback, Save_as_dialog_accept_button_cb, NULL );

  XtManageChild( rowcol );

  label = XtVaCreateManagedWidget( "Label: ",
                xmLabelWidgetClass,    form,
                XmNforeground,         hci_get_read_color( TEXT_FOREGROUND ),
                XmNbackground,         hci_get_read_color( BACKGROUND_COLOR1 ),
                XmNfontList,           hci_get_fontlist( LIST ),
                XmNleftAttachment,     XmATTACH_FORM,
                XmNtopAttachment,      XmATTACH_WIDGET,
                XmNtopWidget,          rowcol,
                XmNbottomAttachment,   XmATTACH_FORM,
                NULL );

  Filename_text_box = XtVaCreateManagedWidget( "save_as_text",
                xmTextFieldWidgetClass, form,
                XmNbackground,          hci_get_read_color( EDIT_BACKGROUND ),
                XmNforeground,          hci_get_read_color( EDIT_FOREGROUND ),
                XmNcolumns,             MAX_LABEL_SIZE-1,
                XmNmaxLength,           MAX_LABEL_SIZE-1,
                XmNmarginHeight,        2,
                XmNshadowThickness,     2,
                XmNmarginWidth,         0,
                XmNborderWidth,         0,
                XmNhighlightOnEnter,    True,
                XmNfontList,            hci_get_fontlist( LIST ),
                XmNleftAttachment,      XmATTACH_WIDGET,
                XmNleftWidget,          label,
                XmNtopAttachment,       XmATTACH_WIDGET,
                XmNtopWidget,           rowcol,
                XmNbottomAttachment,    XmATTACH_FORM,
                XmNrightAttachment,     XmATTACH_FORM,
                NULL );

  XtAddCallback( Filename_text_box, XmNactivateCallback, Save_as_dialog_accept_button_cb, NULL );

  XtManageChild( form );
  HCI_Shell_start( Save_as_dialog, NO_RESIZE_HCI );
}

/************************************************************************
 *      Description: This function sets the Download info labels.	*
 ************************************************************************/

static void Set_download_info_label()
{
  int month, day, year, hour, minute, second;
  char *filename;
  time_t tm;
  XmString str;
  int i;

  filename = hci_get_clutter_download_file_name( Channel_number );

  if( strlen( filename ) == 0 )
  {
    sprintf( Buff, "            Unknown            " );
  }
  else
  {
    /* Center align filename. */
    if( strlen(filename) < MAX_LABEL_SIZE )
    {
      for( i=0; i<(MAX_LABEL_SIZE -strlen(filename))/2; i++ )
      {
        Buff[i]=' ';
      }
      sprintf( &Buff[i], "%s", filename );
      for( i=i+strlen(filename); i<MAX_LABEL_SIZE; i++ )
      {
        Buff[i]=' ';
      }
    }
    else
    {
      strncpy( Buff, filename, MAX_LABEL_SIZE-1 );
    }
    Buff[MAX_LABEL_SIZE-1]='\0';
  }

  str = XmStringCreateLocalized( Buff );
  XtVaSetValues( Last_downloaded_file_label, XmNlabelString, str, NULL );
  XmStringFree( str );

  tm = hci_get_clutter_download_time( Channel_number );

  if( tm < 1 )
  {
    sprintf( Buff,"       Unknown        " );
  }
  else
  {
    unix_time( &tm, &year, &month, &day, &hour, &minute, &second );

    sprintf( Buff,"%2.2d/%2.2d/%4.4d %2.2d:%2.2d:%2.2d UT",
             month, day, year, hour, minute, second);
  }

  str = XmStringCreateLocalized( Buff );
  XtVaSetValues( Time_last_downloaded_label, XmNlabelString, str, NULL );
  XmStringFree( str );
}

/************************************************************************
 *      Description: This function sets clip area.
 ************************************************************************/

static void Hci_set_clip_rectangle( int width, int height )
{
  XRectangle clip_rectangle;

  clip_rectangle.x = 0;
  clip_rectangle.y = 0;
  clip_rectangle.width = width;
  clip_rectangle.height = height;

  XSetClipRectangles( Clutter_display, Clutter_gc, 0, 0, &clip_rectangle, 1, Unsorted );
}

