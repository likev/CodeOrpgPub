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
 * $Author: jeffs $
 * $Locker:  $
 * $Date: 2014/03/18 16:55:30 $
 * $Id: hci_clutter_censor_zones_editor_legacy.c,v 1.31 2014/03/18 16:55:30 jeffs Exp $
 * $Revision: 1.31 $
 * $State: Exp $
 */

/*      HCI include files               */

#include <hci.h>
#include <hci_clutter_censor_zones_legacy.h>

/*      Local Constants                 */

#define ONE                              1
#define HCI_CCZ_DATA_TIMED_OUT          -2
#define COLOR_BAR_WIDTH			100

/*      Macro for the number of visible entries in the clutter  *
 *      files list.                                             */

#define VISIBLE_CLUTTER_REGION_FILES    10 

/*      Macros for each notch width option      */

#define LOW                       0
#define MEDIUM                    1
#define HIGH                      2

/*      Macros for the input mode of the clutter regions        *
 *      display region.                                         */

#define HCI_CCZ_ZOOM_MODE         0
#define HCI_CCZ_SECTOR_MODE       1

/*      Macros for the background product type displayed in     *
 *      the clutter regions display region.                     */

#define HCI_CCZ_BACKGROUND_BREF   0
#define HCI_CCZ_BACKGROUND_CFC    1

/*      Maximum zoom factor for the clutter regions sector      *
 *      display area.                                           */

#define HCI_CCZ_MAX_ZOOM         32

/*      Macros to identify the range units used.                */

enum {UNITS_KM, UNITS_NM};

/*      Macro for converting kilometers to kilometers (1).      */

#define HCI_KM_TO_KM                1.0

/*      Macros for defining the elevation limits (scaled by 10) for the *
 *      low and high segments.                                          */

#define HCI_LOW_SEGMENT_MAX_ELEVATION   19
#define HCI_HIGH_SEGMENT_MAX_ELEVATION  45

/*      Global Variables                                                */

/*                            Currently active segment (low or high)    */

short   Edit_segment        = HCI_CCZ_SEGMENT_LOW;

/*                            Currently active channel (surveillance    *
 *                            or Doppler.                               */

short   Edit_channel        = HCI_CCZ_SURV_CHANNEL;

/*                            Currently active clutter region.          */

short   Edit_region_current = -1;

/*                            Currently active filter (bypass map,      *
 *                            all bins, or none).                       */

short   Edit_select_code    = HCI_CCZ_FILTER_ALL;

/*                            Currently active Doppler notch width      *
 *                            (add 1 to macro defs).                    */

short   Edit_dopl_level     = LOW+1;

/*                            Currently active Surveillance notch width *
 *                            (add 1 to macro defs).                    */

short   Edit_surv_level     = LOW+1;

/*                            Pixmap Drawable associated with the       *
 *                            clutter regions display area.             */

Pixmap  Clutter_edit_pixmap = (Pixmap) NULL;

/*                            Buffer used to temporarily store active   *
 *                            clutter filename.                         */

char    Clutter_label [MAX_LABEL_SIZE];
static int config_change_popup = 0; /* RDA config change popup flag. */

/*      Global definitions for various widgets.                         */

Widget          Top_widget           = (Widget) NULL;
Widget          List_widget          = (Widget) NULL;
Widget          Open_dialog          = (Widget) NULL;
Widget          Save_as_dialog       = (Widget) NULL;
Widget          Download_label       = (Widget) NULL;
Widget          Cfc_rowcol           = (Widget) NULL;
Widget          Segment_label        = (Widget) NULL;
Widget          Lock_widget          = (Widget) NULL;
Widget          Download_button      = (Widget) NULL;
Widget          Delete_button        = (Widget) NULL;
Widget          Restore_button       = (Widget) NULL;
Widget          Update_button        = (Widget) NULL;
Widget          Undo_button          = (Widget) NULL;
Widget          New_file_button      = (Widget) NULL;
Widget          Delete_file_button   = (Widget) NULL;
Widget          Save_file_button     = (Widget) NULL;
Widget          Save_file_as_button  = (Widget) NULL;
Widget          Segment_scroll       = (Widget) NULL;
Widget          Label_dialog         = (Widget) NULL;
Widget          Accept_button        = (Widget) NULL;

/*      Definitions for background product selection combo box widgets  */

#define MAX_BACKGROUND_PRODUCTS_IN_LIST 128     /* Max number of product
                                        entries in background product list */

Widget          Background_product_rowcol = (Widget) NULL;
Widget          Background_product        = (Widget) NULL;
Widget          Background_product_text   = (Widget) NULL;
Widget          Background_product_list   = (Widget) NULL;
Widget          Background_product_button = (Widget) NULL;
int             Background_product_LUT [MAX_BACKGROUND_PRODUCTS_IN_LIST];

/*      Structure defining widgets in clutter regions table.    */

typedef struct {

        int     cut;
        Widget  sector;
        Widget  start_azimuth;
        Widget  stop_azimuth;
        Widget  start_range;
        Widget  stop_range;
        Widget  select_code;
        Widget  dopl;
        Widget  surv;

} Hci_clutter_widgets_t;

Hci_clutter_widgets_t   Cut  [MAX_NUMBER_CLUTTER_ZONES+1];
Widget                  Cut_rowcol [MAX_NUMBER_CLUTTER_ZONES+1];

Widget          Ran1_text = (Widget) NULL;
Widget          Ran2_text = (Widget) NULL;

/*      Global X Properties.            */

Display         *Clutter_display;
Window          Clutter_window;
Pixmap          Clutter_pixmap;
GC              Clutter_gc;

int             Clutter_mode   = HCI_CCZ_SECTOR_MODE;   /* The current  *
                                         * input mode in the sector     *
                                         * display area.                */
int             Clutter_background = HCI_CCZ_BACKGROUND_BREF;   /* The  *
                                         * current product type used    *
                                         * as background in the sector  *
                                         * display area.                */
int             Clutter_zoom   = 1;     /* Current sector display area  *
                                         * magnification.               */
Dimension       Clutter_width  = 800;   /* Width of the clutter regions *
                                         * display region.              */
Dimension       Clutter_height = 460;   /* Height of the clutter regions*
                                         * display region.              */
unsigned int    Clutter_depth;          /* Depth (bits) of the window   */
Dimension       Bypass_map_width;       /* Width of the composite map   *
                                         * display area (pixels).       */
Dimension       Bypass_map_height;      /* Height of the composite map  *
                                         * display area (scanlines).    */
Dimension       Sector_width;           /* Width of the sector data     *
                                         * display area (pixels).       */
Dimension       Sector_height;          /* Height of the sector data    *
                                         * display area (scanlines).    */
unsigned char   Clutter_map [HCI_CCZ_RADIALS+1][HCI_CCZ_GATES+1]; /*
                                         * Composite map data built     *
                                         * from sector data.            */
Pixel           Bypass_colors [HIGH+1]; /* Color table for all bypass   *
                                         * map filter notch widths.     */
Pixel           All_colors    [HIGH+1]; /* Color table for all bins     *
                                         * filter notch widths.         */
Pixel           Filter_colors [HIGH+1]; /* Color table for currently    *
                                         * active filter option.        */
Boolean         Update_flag = False;    /* Flag to indicate when edits  *
                                         * are detected.                */
Boolean         Close_flag  = False;    /* Flag to indicate when window *
                                         * is to be closed.             */
float           Scale_x =  460.0/(2*HCI_CCZ_MAX_RANGE); /* Scale factor *
                                         * used in conversion of pixels *
                                         * to azran in the sector       *
                                         * display area.                */
float           Scale_y = -(460.0/(2*HCI_CCZ_MAX_RANGE)); /* Scale      *
                                         * factor used in conversion of *
                                         * scanlines to azran in the    *
                                         * sector display area.         */
int             Center_pixel = 230;     /* Center pixel coordinate in   *
                                         * the sector display area.     */
int             Center_scanl = 230;     /* Center scanline coordinate   *
                                         * in the sector display area.  */
float           Center_x_offset = 0;    /* X offset of the radar (km)   *
                                         * to the center of the sector  *
                                         * display area.                */
float           Center_y_offset = 0;    /* Y offset of the radar (km)   *
                                         * to the center of the sector  *
                                         * display area.                */
int             Background_display_flag = 0; /* Flag to indicate wheter *
                                         * a product for the selected   *
                                         * type is available for        *
                                         * display.                     */
Dimension       Select_button_width = 0; /* Width (pixels) of the       *
                                          * select code and notch width *
                                          * buttons in the sector data  *
                                          * table.                      */
float           Units_factor = HCI_KM_TO_NM; /* Units conversion factor to  *
                                          * to be applied to range      *
                                          * data.                       */
int             Background_product_id = 2;      /* Product code *
                                          * of selected background      *
                                          * product for sector display  *
                                          * area.                       */

int		Unlocked_loca = HCI_NO_FLAG;

/*                      Edit buffer containing clutter sector data      *
 *                      for currently selected clutter file.            */

Hci_clutter_data_t      Edit [MAX_NUMBER_CLUTTER_ZONES+1];

int                     Edit_regions = 0;       /* Number of clutter    *
                                          * regions defined for current *
                                          * file.                       */
int                     Edit_row     = 0;       /* Current selected row */
int                     Clutter_file = 0;       /* Currently selected   *
                                          * clutter file index.         */
int                     New_file     = 0;       /* Index of file when   *
                                          * new file selected.          */
int                     New_flag     = 0;       /* Set to 1 when        *
                                          * new file created.           */
int                     Download_flag = 0; /* Set to 1 when download is *
                                              active */

ORPGDBM_query_data_t    Query_data [16];   /* product query parameters  *
                                              for background product.   */
RPG_prod_rec_t          Db_info [50];      /* product query results     */
int                     Query_time [50];   /* product query times       */
int			reload_flag = 0;

char    Buff [256];     /* Shared buffer for labels/etc.                */
        
char    Lbw_title[20] = "";     /* Label to be added to title when in   *
                                 * low bandwidth mode.                  */

/*      Definitions for translation table to be used for mouse events   *
 *      in sector display area.                                         */

String  Clutter_translations =
        "<PtrMoved>:    clutter_censor_zones_input(move) \n\
        <Btn1Down>:     clutter_censor_zones_input(down1)  ManagerGadgetArm() \n\
        <Btn1Up>:       clutter_censor_zones_input(up1)    ManagerGadgetActivate() \n\
        <Btn1Motion>:   clutter_censor_zones_input(motion1) ManagerGadgetButtonMotion() \n\
        <Btn2Down>:     clutter_censor_zones_input(down2)  ManagerGadgetArm() \n\
        <Btn2Up>:       clutter_censor_zones_input(up2)    ManagerGadgetActivate() \n\
        <Btn2Motion>:   clutter_censor_zones_input(motion2) ManagerGadgetButtonMotion() \n\
        <Btn3Down>:     clutter_censor_zones_input(down3)  ManagerGadgetArm() \n\
        <Btn3Up>:       clutter_censor_zones_input(up3)    ManagerGadgetActivate() \n\
        <Btn3Motion>:   clutter_censor_zones_input(motion3) ManagerGadgetButtonMotion()";

/*      Local prototypes                                                */

void    accept_clutter_restore_baseline  (Widget w,
                 XtPointer client_data, XtPointer call_data);
void    accept_clutter_update_baseline  (Widget w,
                 XtPointer client_data, XtPointer call_data);
void    change_file_label_callback (Widget w,
                XtPointer client_data, XtPointer call_data);
void    close_and_save (Widget w,
                XtPointer client_data, XtPointer call_data);
void    close_and_no_save (Widget w,
                XtPointer client_data, XtPointer call_data);
void    clutter_restore_baseline (Widget w,
                 XtPointer client_data, XtPointer call_data);
void    clutter_update_baseline  (Widget w,
                 XtPointer client_data, XtPointer call_data);
int     destroy_callback ();
void    hci_change_ccz_azimuth_callback (Widget w,
                XtPointer client_data, XtPointer call_data);
void    hci_change_ccz_range_callback (Widget w,
                XtPointer client_data, XtPointer call_data);
void    hci_close_clutter_file_window (Widget w,
                XtPointer client_data, XtPointer call_data);
void    hci_clutter_close_callback (Widget w,
                XtPointer client_data, XtPointer call_data);
void    hci_clutter_delete_callback (Widget w,
                XtPointer client_data, XtPointer call_data);
void    hci_clutter_download_callback (Widget w,
                XtPointer client_data, XtPointer call_data);
void    hci_clutter_download_confirm_ok (Widget w,
                XtPointer client_data, XtPointer call_data);
void    hci_clutter_download_confirm_no (Widget w,
                XtPointer client_data, XtPointer call_data);
void    hci_clutter_download_confirm_ok_save (Widget w,
                XtPointer client_data, XtPointer call_data);
void    hci_clutter_download_confirm_no_save (Widget w,
                XtPointer client_data, XtPointer call_data);
void    hci_clutter_edit_cancel     (Widget w,
                XtPointer client_data, XtPointer call_data);
void    hci_clutter_expose_callback (Widget w,
                XtPointer client_data, XtPointer call_data);
void    hci_clutter_file_callback (Widget w,
                XtPointer client_data, XtPointer call_data);
void    hci_clutter_file_delete_confirm_ok (Widget w,
                XtPointer client_data, XtPointer call_data);
void    hci_clutter_file_new_callback (Widget w,
                XtPointer client_data, XtPointer call_data);
void    hci_clutter_file_save_callback (Widget w,
                XtPointer client_data, XtPointer call_data);
void    hci_clutter_file_save_as_callback (Widget w,
                XtPointer client_data, XtPointer call_data);
void    hci_clutter_file_save_as_window_close (Widget w,
                XtPointer client_data, XtPointer call_data);
void    hci_clutter_file_save_as_window_ok (Widget w,
                XtPointer client_data, XtPointer call_data);
void    hci_clutter_file_save_yes_callback (Widget w,
                XtPointer client_data, XtPointer call_data);
void    hci_clutter_file_save_no_callback (Widget w,
                XtPointer client_data, XtPointer call_data);
void    hci_clutter_reload_callback (Widget w,
                XtPointer client_data, XtPointer call_data);
void    hci_clutter_region_delete   (Widget w,
                XtPointer client_data, XtPointer call_data);
void    hci_ccz_background_callback (Widget w,
                XtPointer client_data, XtPointer call_data);
void    hci_ccz_channel_callback (Widget w,
                XtPointer client_data, XtPointer call_data);
void    hci_ccz_reset_zoom_callback (Widget w,
                XtPointer client_data, XtPointer call_data);
void    hci_save_as_label_callback (Widget w,
                XtPointer client_data, XtPointer call_data);
void    hci_select_code_callback  (Widget w,
                XtPointer client_data, XtPointer call_data);
void    hci_select_clutter_file (Widget w,
                XtPointer client_data, XtPointer call_data);
void    hci_select_dopl_callback    (Widget w,
                XtPointer client_data, XtPointer call_data);
void    hci_select_mode_callback (Widget w,
                XtPointer client_data, XtPointer call_data);
void    hci_select_sector_callback (Widget w,
                XtPointer client_data, XtPointer call_data);
void    hci_select_segment_callback (Widget w,
                XtPointer client_data, XtPointer call_data);
void    hci_select_select_code_callback  (Widget w,
                XtPointer client_data, XtPointer call_data);
void    hci_select_surv_callback    (Widget w,
                XtPointer client_data, XtPointer call_data);
void    hci_select_units_callback (Widget w,
                XtPointer client_data, XtPointer call_data);
void    background_product_callback (Widget w,
                XtPointer client_data, XtPointer call_data);

void    hci_clutter_download ();
void    hci_display_clutter_map     ();
void    hci_load_clutter_background_product (int type);
void    hci_refresh_censor_zone_sectors ();
void    hci_update_clutter_sector_widgets ();

void    clutter_censor_zones_input (Widget w, XEvent *event,
                String *args, int *num_args);
void    display_censor_zone_sector (Display *display, Drawable window,
                        GC gc,               Pixel color,
                        float first_azimuth, float second_azimuth,
                        float first_range,   float second_range,
                        int select_code);
int     hci_clutter_censor_zones_lock ();
void    hci_display_radial_product (Display *display, Drawable window,
                        GC gc,               int pixel,
                        int scanl,           int width,
                        int height,          int max_range,
                        float x_offset,      float y_offset,
                        int zoom);
float   hci_find_azimuth (int first_pixel, int first_scanl,
                int center_pixel, int center_scanl);
int     hci_load_product_data (int pcode, int vol_t, short *params);
void    hci_update_file_list ();
void	hci_ccz_timer_proc();
void	hci_ccz_deau_callback();
void	rda_config_change();

/************************************************************************
 *      Description: This is the main routine for the HCI Clutter       *
 *                   Suppression Regions Editor task.                   *
 *                                                                      *
 *      Input:  argc - number of command line arguments                 *
 *              argv - string containing command line arguments         *
 *      Output: NONE                                                    *
 *      Return: exit code                                               *
 ************************************************************************/

int
main (
int     argc,
char    *argv []
)
{
        Arg             arg [10];
        XGCValues       gcv;
        XtActionsRec    actions;
        int             i;
        int             status;
        XmString        str;
        int             n;

/*      Define the set of widgets used to define the elements of the    *
 *      clutter censor zone edit window.                                */

        Widget          form;
        Widget          control_frame;
        Widget          control_rowcol;
        Widget          clutter_rowcol;
        Widget          button;
        Widget          draw;
        Widget          list_frame;
        Widget          list_form;
        Widget          list1_rowcol;
        Widget          list2_rowcol;
        Widget          list2_frame;
        Widget          lock_form;
        Widget          low_segment_frame;
        Widget          segment_form;
        Widget          label_rowcol;
        Widget          low_form;
        Widget          segment;
        Widget          units;
        Widget          mode;
        Widget          background;
        Widget          text;

        time_t  tm;
        int     month, day, year;
        int     hour, minute, second;
        XRectangle      clip_rectangle;

        int     retval;
        int     prod_id;

        char    *buf;
        Hci_ccz_data_t  *config_data;
       
	/* Initialize HCI. */

	HCI_init( argc, argv, HCI_CCZ_TASK );

        Top_widget = HCI_get_top_widget();
        Clutter_display = HCI_get_display();
	HCI_set_destroy_callback( destroy_callback );

        Clutter_depth = XDefaultDepth( Clutter_display,
                        DefaultScreen( Clutter_display ) );

        hci_initialize_product_colors (Clutter_display, HCI_get_colormap());
        
/*      Use a form widget to manage placement of various widgets        *
 *      in the window.  The objects are:  a rowcolumn widget at the     *
 *      top which contains all of the file control functions, a         *
 *      drawingarea widget in which basedata and cluttermap data are    *
 *      displayed and clutter regions defined, a rowcolumn widget       *
 *      which contains buttons for region operations, and two scrolled  *
 *      windows which contain the definitions of each clutter censor    *
 *      zone.                                                           */

        form = XtVaCreateWidget ("clutter_censor_zones_form",
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

        Sector_width      = Clutter_height;
        Sector_height     = Clutter_height;
        Bypass_map_width  = Clutter_width-Sector_width-90;
        Bypass_map_height = Clutter_height;
        
/*      Read the clutter censor zone data from the clutter data lb      */

        status = hci_read_clutter_regions_file (); 
        if (status == RMT_CANCELLED)
           HCI_task_exit(HCI_EXIT_SUCCESS);

/*      If the clutter censor zones message was read OK then initialize *
 *      the widgets using data from the last downloaded file.           */

        if (status > 0) {

            char        *file_label;

            Clutter_file = hci_get_clutter_region_download_file ();
            New_file     = hci_get_clutter_region_download_file ();

/*          If the last downloaded clutter file no longer exists, then  *
 *          start with the default file.                                */

            file_label = hci_get_clutter_region_file_label (Clutter_file);

            if (strlen (file_label) == 0) {

                Clutter_file = 0;
                New_file     = 0;

            }

            Edit_regions = hci_get_clutter_region_regions (Clutter_file);

            for (i=0;i<MAX_NUMBER_CLUTTER_ZONES;i++) {

                Edit [i].start_azimuth = (float) hci_get_clutter_region_data (
                                                Clutter_file,
                                                i,
                                                HCI_CCZ_START_AZIMUTH);
                Edit [i].stop_azimuth  = (float) hci_get_clutter_region_data (
                                                Clutter_file,
                                                i,
                                                HCI_CCZ_STOP_AZIMUTH);
                Edit [i].start_range   = (float) hci_get_clutter_region_data (
                                                Clutter_file,
                                                i,
                                                HCI_CCZ_START_RANGE);
                Edit [i].stop_range    = (float) hci_get_clutter_region_data (
                                                Clutter_file,
                                                i,
                                                HCI_CCZ_STOP_RANGE);
                Edit [i].segment       = hci_get_clutter_region_data (
                                                Clutter_file,
                                                i,
                                                HCI_CCZ_SEGMENT);
                Edit [i].select_code   = hci_get_clutter_region_data (
                                                Clutter_file,
                                                i,
                                                HCI_CCZ_SELECT_CODE);
                Edit [i].dopl          = hci_get_clutter_region_data (
                                                Clutter_file,
                                                i,
                                                HCI_CCZ_DOPPL_LEVEL);
                Edit [i].surv          = hci_get_clutter_region_data (
                                                Clutter_file,
                                                i,
                                                HCI_CCZ_SURV_LEVEL);

            }

/*      If the clutter censor zones message was not read OK then exit.  */

        } else {

            HCI_LE_error("Read clutter regions message failed (%d)", status);
            HCI_task_exit(HCI_EXIT_FAIL);

        }

        if (HCI_is_low_bandwidth())
           strcpy(Lbw_title, " <LB> ");
        else
           strcpy(Lbw_title, "");

        tm =hci_get_clutter_region_file_time (Clutter_file);

        if (tm < 1) {

            sprintf (Buff,"Clutter Regions - File: %s %s: Last Modified: Unknown ",
                hci_get_clutter_region_file_label (Clutter_file), Lbw_title);

        } else {

            unix_time (&tm,
               &year,
               &month,
               &day,
               &hour,
               &minute,
               &second);

            sprintf (Buff,"Clutter Regions - File:%s %s: Last Modified %2.2d/%2.2d/%4.4d  %2.2d:%2.2d:%2.2d",
                hci_get_clutter_region_file_label (Clutter_file), Lbw_title, 
                month, day, year, hour, minute, second);

        }

/*      Add redundancy information if site FAA redundant        */

        if (HCI_get_system() == HCI_FAA_SYSTEM) {

            sprintf (Buff,"%s - (FAA:%d)", Buff,
                ORPGRED_channel_num (ORPGRED_MY_CHANNEL));

        }

/*      Initialize the colors used to display clutter filter info       */

        Bypass_colors [LOW]    = hci_get_read_color (GREEN);
        Bypass_colors [MEDIUM] = hci_get_read_color (YELLOW);
        Bypass_colors [HIGH]   = hci_get_read_color (ORANGE);
        Filter_colors [LOW]    = hci_get_read_color (GREEN);
        Filter_colors [MEDIUM] = hci_get_read_color (YELLOW);
        Filter_colors [HIGH]   = hci_get_read_color (ORANGE);
        All_colors    [LOW]    = hci_get_read_color (BLUE);
        All_colors    [MEDIUM] = hci_get_read_color (CYAN);
        All_colors    [HIGH]   = hci_get_read_color (DARKSEAGREEN);

/*      Initialize the clutter suppression region data structure        */

        Edit_region_current = -1;

/*      Build a clutter map using the priority definitions in the       *
 *      RDA/RPG ICD.                                                    */

        hci_build_clutter_map (&Clutter_map[0][0],
                        Edit,
                        Edit_segment,
                        Edit_regions);


/*      Define a rowcolumn widget to contain file main file control     *
 *      functions.                                                      */

        control_rowcol = XtVaCreateWidget ("clutter_control_rowcol",
                xmRowColumnWidgetClass, control_frame,
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNorientation,         XmHORIZONTAL,
                XmNmarginHeight,        1,
                NULL);

        button = XtVaCreateManagedWidget ("Close",
                xmPushButtonWidgetClass,        control_rowcol,
                XmNforeground,          hci_get_read_color (BUTTON_FOREGROUND),
                XmNbackground,          hci_get_read_color (BUTTON_BACKGROUND),
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

        XtAddCallback (button,
                XmNactivateCallback, hci_clutter_close_callback, NULL);

        button = XtVaCreateManagedWidget ("File",
                xmPushButtonWidgetClass,        control_rowcol,
                XmNforeground,          hci_get_read_color (BUTTON_FOREGROUND),
                XmNbackground,          hci_get_read_color (BUTTON_BACKGROUND),
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

        XtAddCallback (button,
                XmNactivateCallback, hci_clutter_file_callback, NULL);

        Undo_button = XtVaCreateManagedWidget ("Undo",
                xmPushButtonWidgetClass,        control_rowcol,
                XmNforeground,          hci_get_read_color (BUTTON_FOREGROUND),
                XmNbackground,          hci_get_read_color (BUTTON_BACKGROUND),
                XmNfontList,            hci_get_fontlist (LIST),
                XmNsensitive,           False,
                NULL);

        XtAddCallback (Undo_button,
                XmNactivateCallback, hci_clutter_reload_callback, NULL);

        Download_button = XtVaCreateManagedWidget ("Download",
                xmPushButtonWidgetClass,        control_rowcol,
                XmNforeground,          hci_get_read_color (BUTTON_FOREGROUND),
                XmNbackground,          hci_get_read_color (BUTTON_BACKGROUND),
                XmNfontList,            hci_get_fontlist (LIST),
                XmNsensitive,           True,
                NULL);

        XtAddCallback (Download_button,
                XmNactivateCallback, hci_clutter_download_callback, NULL);

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
                XmNsensitive,           False,
                NULL);

        XtAddCallback (Restore_button,
                XmNactivateCallback, clutter_restore_baseline, NULL);

        Update_button = XtVaCreateManagedWidget ("Update",
                xmPushButtonWidgetClass,        control_rowcol,
                XmNforeground,          hci_get_read_color (BUTTON_FOREGROUND),
                XmNbackground,          hci_get_read_color (BUTTON_BACKGROUND),
                XmNfontList,            hci_get_fontlist (LIST),
                XmNsensitive,           False,
                NULL);

        XtAddCallback (Update_button,
                XmNactivateCallback, clutter_update_baseline, NULL);

        Download_label = XtVaCreateManagedWidget (" ",
                xmLabelWidgetClass,     control_rowcol,
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

        tm =hci_get_clutter_region_download_time (Clutter_file);

        unix_time (&tm,
               &year,
               &month,
               &day,
               &hour,
               &minute,
               &second);

        if (((month  >    0) && (month  <   13)) &&
            ((day    >    0) && (day    <   32)) &&
            ((year   > 1988) && (year   < 2100)) &&
            ((hour   >=   0) && (hour   <   24)) &&
            ((minute >=   0) && (minute <   60)) &&
            ((second >=   0) && (second <   60))) {

            sprintf (Buff,"Last Download %2.2d/%2.2d/%4.4d  %2.2d:%2.2d:%2.2d",
                month, day, year, hour, minute, second);

        } else {

            sprintf (Buff,"Last Download Time Unknown        ");

        }

        str = XmStringCreateLocalized (Buff);
        XtVaSetValues (Download_label,
                XmNlabelString, str,
                NULL);
        XmStringFree (str);
        XtManageChild (control_rowcol);

/*      Create the lock widget for password protection.                 */

        lock_form = XtVaCreateWidget ("lock_form",
                xmFormWidgetClass,      control_rowcol,
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNtopAttachment,       XmATTACH_FORM,
                XmNrightAttachment,     XmATTACH_FORM,
                NULL);

        Lock_widget = hci_lock_widget( lock_form, hci_clutter_censor_zones_lock, HCI_LOCA_URC | HCI_LOCA_ROC );

        XtManageChild (lock_form);

/*      Define the main drawingarea widget for displaying reflectivity  *
 *      and sector data in the left 2/3 and the resultant clutter       *
 *      filter map for both channels in the right 1/3.                  */

	clutter_rowcol = XtVaCreateManagedWidget ("clutter_rowcol",
		xmRowColumnWidgetClass,	form,
		XmNbackground,		hci_get_read_color (BACKGROUND_COLOR1),
		XmNorientation,		XmHORIZONTAL,
		XmNcolumns,		1,
		XmNpacking,		XmPACK_TIGHT,
                XmNtopAttachment,	XmATTACH_WIDGET,
                XmNtopWidget,		control_rowcol,
                XmNleftAttachment,	XmATTACH_FORM,
                XmNrightAttachment,	XmATTACH_FORM,
                NULL);

        actions.string = "clutter_censor_zones_input";
        actions.proc   = (XtActionProc) clutter_censor_zones_input;
        XtAppAddActions (HCI_get_appcontext(), &actions, 1);

        draw = XtVaCreateWidget ("clutter_drawing_area",
                xmDrawingAreaWidgetClass,       clutter_rowcol,
                XmNwidth,               Clutter_width,
                XmNheight,              Clutter_height,
                XmNbackground,          hci_get_read_color (PRODUCT_BACKGROUND_COLOR),
                XmNleftAttachment,      XmATTACH_FORM,
                XmNrightAttachment,     XmATTACH_FORM,
                XmNtopAttachment,       XmATTACH_WIDGET,
                XmNtopWidget,           control_frame,
                XmNtranslations,        XtParseTranslationTable (Clutter_translations),
                NULL);

        XtAddCallback (draw,
                XmNexposeCallback, hci_clutter_expose_callback, NULL);

        XtManageChild (draw);

/*      Define two forms which contain the clutter censor zone defs     *
 *      for both segments.                                              */

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

        button = XtVaCreateManagedWidget ("Low",
                xmToggleButtonWidgetClass,      segment,
                XmNselectColor,         hci_get_read_color (WHITE),
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNset,                 True,
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

        XtAddCallback (button,
                XmNvalueChangedCallback, hci_select_segment_callback,
                (XtPointer) HCI_CCZ_SEGMENT_LOW);

        button = XtVaCreateManagedWidget ("High",
                xmToggleButtonWidgetClass,      segment,
                XmNselectColor,         hci_get_read_color (WHITE),
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNset,                 False,
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

        XtAddCallback (button,
                XmNvalueChangedCallback, hci_select_segment_callback,
                (XtPointer) HCI_CCZ_SEGMENT_HIGH);

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

        button = XtVaCreateManagedWidget ("Zoom",
                xmToggleButtonWidgetClass,      mode,
                XmNselectColor,         hci_get_read_color (WHITE),
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNfontList,            hci_get_fontlist (LIST),
                XmNset,                 False,
                NULL);

        XtAddCallback (button,
                XmNvalueChangedCallback, hci_select_mode_callback,
                (XtPointer) HCI_CCZ_ZOOM_MODE);

        button = XtVaCreateManagedWidget ("Sector",
                xmToggleButtonWidgetClass,      mode,
                XmNselectColor,         hci_get_read_color (WHITE),
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNset,                 True,
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

        XtAddCallback (button,
                XmNvalueChangedCallback, hci_select_mode_callback,
                (XtPointer) HCI_CCZ_SECTOR_MODE);

        XtManageChild (mode);

        XtVaCreateManagedWidget ("  ",
                xmLabelWidgetClass,     list1_rowcol,
                XmNforeground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

        button = XtVaCreateManagedWidget ("Reset Zoom",
                xmPushButtonWidgetClass,        list1_rowcol,
                XmNforeground,          hci_get_read_color (BUTTON_FOREGROUND),
                XmNbackground,          hci_get_read_color (BUTTON_BACKGROUND),
                XmNfontList,            hci_get_fontlist (LIST),
                XmNalignment,           XmALIGNMENT_CENTER,
                NULL);

        XtAddCallback (button,
                XmNactivateCallback, hci_ccz_reset_zoom_callback, NULL);

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

        button = XtVaCreateManagedWidget ("km",
                xmToggleButtonWidgetClass,      units,
                XmNselectColor,         hci_get_read_color (WHITE),
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNset,                 False,
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

        XtAddCallback (button,
                XmNvalueChangedCallback, hci_select_units_callback,
                (XtPointer) UNITS_KM);

        button = XtVaCreateManagedWidget ("nm",
                xmToggleButtonWidgetClass,      units,
                XmNselectColor,         hci_get_read_color (WHITE),
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNset,                 True,
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

        XtAddCallback (button,
                XmNvalueChangedCallback, hci_select_units_callback,
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

        Background_product_button = XtVaCreateManagedWidget ("Reflectivity  ",
                xmToggleButtonWidgetClass,      background,
                XmNselectColor,         hci_get_read_color (WHITE),
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNset,                 True,
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

        XtAddCallback (Background_product_button,
                XmNvalueChangedCallback, hci_ccz_background_callback,
                (XtPointer) HCI_CCZ_BACKGROUND_BREF);

        button = XtVaCreateManagedWidget ("CFC Product",
                xmToggleButtonWidgetClass,      background,
                XmNselectColor,         hci_get_read_color (WHITE),
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNset,                 False,
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

        XtAddCallback (button,
                XmNvalueChangedCallback, hci_ccz_background_callback,
                (XtPointer) HCI_CCZ_BACKGROUND_CFC);

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
                XmNcolumns,             64,
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
                XmNselectionCallback, background_product_callback, NULL);

/*      Build the background product menu.  Get all of the products     *
 *      for the menu from the configuration data for this task.         */

        buf = (char *) calloc (1, ALIGNED_SIZE(sizeof(Hci_ccz_data_t)));

        status = ORPGDA_read (ORPGDAT_HCI_DATA,
                              buf,
                              ALIGNED_SIZE(sizeof(Hci_ccz_data_t)),
                              HCI_CCZ_TASK_DATA_MSG_ID);

        if (status <= 0) {

            HCI_LE_error("Unable to read task configuration data [%d]", status);

            str = XmStringCreateLocalized ("Not products defined");

            XmListAddItemUnselected (Background_product_list, str, 0);

            XtVaSetValues (Background_product,
                XmNsensitive,   False,
                NULL);

            XmStringFree (str);

        } else {

            config_data = (Hci_ccz_data_t *) buf;

            if (HCI_is_low_bandwidth())
                Background_product_id = ORPGPAT_get_prod_id_from_code ((int) config_data->low_product);
            else
                Background_product_id = ORPGPAT_get_prod_id_from_code ((int) config_data->high_product);

            for (i=0;i<config_data->n_products;i++) {

                prod_id = ORPGPAT_get_prod_id_from_code (config_data->product_list[i]);
                Background_product_LUT[i] = prod_id;

                if (prod_id < 0)
                        continue;

                sprintf (Buff,"%s[%d] - %-48s   ",
                        ORPGPAT_get_mnemonic (prod_id),
                        ORPGPAT_get_code (prod_id),
                        ORPGPAT_get_description (prod_id, STRIP_MNEMONIC));

                str = XmStringCreateLocalized (Buff);

                XmListAddItemUnselected (Background_product_list, str, 0);

                if (Background_product_id == prod_id)
                        XmTextSetString (Background_product_text, Buff);

                XmStringFree (str);

            }
        }

/*      Format a new label to the background product radio button.      */

        sprintf (Buff,"%s[%d] - %-48s   ",
                ORPGPAT_get_mnemonic (Background_product_id),
                ORPGPAT_get_code (Background_product_id),
                ORPGPAT_get_description (Background_product_id, STRIP_MNEMONIC));

        str = XmStringCreateLocalized (Buff);

        XtVaSetValues (Background_product_button,
                XmNlabelString,         str,
                NULL);

        XmStringFree (str);

        XtManageChild (Background_product);
        XtManageChild (Background_product_rowcol);

/*      Create a row for defining the channel type of the CFC product   *
 *      displayed beneath the clutter regions graphical data.           */

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

        XtVaCreateManagedWidget ("blank space",
                xmLabelWidgetClass,     Cfc_rowcol,
                XmNforeground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNfontList,            hci_get_fontlist (LIST),
                XmNalignment,           XmALIGNMENT_CENTER,
                NULL);

        XtVaCreateManagedWidget ("CFC Channel:",
                xmLabelWidgetClass,     Cfc_rowcol,
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

        background = XmCreateRadioBox (Cfc_rowcol,
                "cfc_channel", arg, n);

        button = XtVaCreateManagedWidget ("Doppler",
                xmToggleButtonWidgetClass,      background,
                XmNselectColor,         hci_get_read_color (WHITE),
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNset,                 False,
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

        XtAddCallback (button,
                XmNvalueChangedCallback, hci_ccz_channel_callback,
                (XtPointer) HCI_CCZ_DOPL_CHANNEL);

        button = XtVaCreateManagedWidget ("Surveillance",
                xmToggleButtonWidgetClass,      background,
                XmNselectColor,         hci_get_read_color (WHITE),
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNset,                 True,
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

        XtAddCallback (button,
                XmNvalueChangedCallback, hci_ccz_channel_callback,
                (XtPointer) HCI_CCZ_SURV_CHANNEL);

        XtManageChild (background);
        XtManageChild (Cfc_rowcol);

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
                XmNsensitive,           True,
                XmNtopAttachment,       XmATTACH_FORM,
                XmNleftAttachment,      XmATTACH_FORM,
                NULL);

        XtAddCallback (Delete_button,
                XmNactivateCallback, hci_clutter_region_delete, NULL);

/*      Label the items in the segment.                                 */

        Segment_label = XtVaCreateManagedWidget ("Low Elevation Segment",
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

        sprintf (Buff,"Region");
        text = XtVaCreateManagedWidget ("Region",
                xmTextFieldWidgetClass, label_rowcol,
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNmarginHeight,        2,
                XmNshadowThickness,     2,
                XmNborderWidth,         0,
                XmNmarginWidth,         0,
                XmNhighlightThickness,  1,
                XmNcolumns,             6,
                XmNfontList,            hci_get_fontlist (LIST),
                XmNeditable,            False,
                NULL);

        XmTextSetString (text,Buff);

        sprintf (Buff,"Azi1 (deg)");
        text = XtVaCreateManagedWidget ("Azimuth 1",
                xmTextFieldWidgetClass, label_rowcol,
                XmNforeground,          hci_get_read_color (EDIT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNfontList,            hci_get_fontlist (LIST),
                XmNcolumns,             10,
                XmNmarginHeight,        2,
                XmNshadowThickness,     2,
                XmNmarginWidth,         0,
                XmNborderWidth,         0,
                XmNeditable,            False,
                NULL);

        XmTextSetString (text,Buff);

        sprintf (Buff,"Azi2 (deg)");
        text = XtVaCreateManagedWidget ("Azimuth 2",
                xmTextFieldWidgetClass, label_rowcol,
                XmNforeground,          hci_get_read_color (EDIT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNfontList,            hci_get_fontlist (LIST),
                XmNcolumns,             10,
                XmNmarginHeight,        2,
                XmNshadowThickness,     2,
                XmNmarginWidth,         0,
                XmNborderWidth,         0,
                XmNeditable,            False,
                NULL);

        XmTextSetString (text,Buff);

        if (Units_factor == HCI_KM_TO_KM) {

            sprintf (Buff,"Ran1 (km)");

        } else {

            sprintf (Buff,"Ran1 (nm)");

        }

        Ran1_text = XtVaCreateManagedWidget ("Range 1",
                xmTextFieldWidgetClass, label_rowcol,
                XmNforeground,          hci_get_read_color (EDIT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNfontList,            hci_get_fontlist (LIST),
                XmNcolumns,             10,
                XmNmarginHeight,        2,
                XmNshadowThickness,     2,
                XmNmarginWidth,         0,
                XmNborderWidth,         0,
                XmNeditable,            False,
                NULL);

        XmTextSetString (Ran1_text,Buff);

        if (Units_factor == HCI_KM_TO_KM) {

            sprintf (Buff,"Ran2 (km)");

        } else {

            sprintf (Buff,"Ran2 (nm)");

        }

        Ran2_text = XtVaCreateManagedWidget ("Range 2",
                xmTextFieldWidgetClass, label_rowcol,
                XmNforeground,          hci_get_read_color (EDIT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNfontList,            hci_get_fontlist (LIST),
                XmNcolumns,             10,
                XmNmarginHeight,        2,
                XmNshadowThickness,     2,
                XmNmarginWidth,         0,
                XmNborderWidth,         0,
                XmNeditable,            False,
                NULL);

        XmTextSetString (Ran2_text,Buff);

        button = XtVaCreateManagedWidget ("Select Code",
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

        XtVaGetValues (button,
                XmNwidth,       &Select_button_width,
                NULL);

        XtVaCreateManagedWidget ("Dopl Chan",
                xmPushButtonWidgetClass,        label_rowcol,
                XmNforeground,          hci_get_read_color (EDIT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNfontList,            hci_get_fontlist (LIST),
                XmNalignment,           XmALIGNMENT_CENTER,
                XmNwidth,               Select_button_width,
                XmNrecomputeSize,       False,
                XmNmarginHeight,        3,
                XmNshadowThickness,     1,
                XmNborderWidth,         0,
                XmNhighlightThickness,  1,
                NULL);

        XtVaCreateManagedWidget ("Surv Chan",
                xmPushButtonWidgetClass,        label_rowcol,
                XmNforeground,          hci_get_read_color (EDIT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNfontList,            hci_get_fontlist (LIST),
                XmNwidth,               Select_button_width,
                XmNrecomputeSize,       False,
                XmNmarginHeight,        3,
                XmNshadowThickness,     1,
                XmNborderWidth,         0,
                XmNhighlightThickness,  1,
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

                Cut_rowcol [i] = XtVaCreateWidget ("low_elevation_rowcol",
                        xmRowColumnWidgetClass, low_form,
                        XmNorientation,         XmHORIZONTAL,
                        XmNnumColumns,          1,
                        XmNpacking,             XmPACK_TIGHT,
                        XmNbackground,          hci_get_read_color (BLACK),
                        XmNspacing,             0,
                        XmNmarginWidth,         0,
                        XmNmarginHeight,        0,
                        XmNtopAttachment,       XmATTACH_FORM,
                        XmNleftAttachment,      XmATTACH_FORM,
                        NULL);

            } else {

                Cut_rowcol [i] = XtVaCreateWidget ("low_elevation_rowcol",
                        xmRowColumnWidgetClass, low_form,
                        XmNorientation,         XmHORIZONTAL,
                        XmNnumColumns,          1,
                        XmNpacking,             XmPACK_TIGHT,
                        XmNbackground,          hci_get_read_color (BLACK),
                        XmNspacing,             0,
                        XmNmarginWidth,         0,
                        XmNmarginHeight,        0,
                        XmNtopAttachment,       XmATTACH_WIDGET,
                        XmNtopWidget,           Cut_rowcol [i-1],
                        XmNleftAttachment,      XmATTACH_FORM,
                        NULL);

            }

            Cut [i].sector = XtVaCreateManagedWidget ("",
                xmPushButtonWidgetClass,        Cut_rowcol [i],
                XmNforeground,          hci_get_read_color (BUTTON_FOREGROUND),
                XmNbackground,          hci_get_read_color (BUTTON_BACKGROUND),
                XmNuserData,            (XtPointer) -1,
                XmNmarginHeight,        3,
                XmNshadowThickness,     1,
                XmNborderWidth,         0,
                XmNhighlightThickness,  1,
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

            XtAddCallback (Cut [i].sector,
                        XmNactivateCallback, hci_select_sector_callback,
                        (XtPointer) NULL);

            sprintf (Buff," %3d  ",i+1);
            str = XmStringCreateLocalized (Buff);
            XtVaSetValues (Cut [i].sector,
                XmNlabelString, str,
                NULL);

            XmStringFree (str);

            Cut [i].start_azimuth = XtVaCreateManagedWidget ("",
                xmTextFieldWidgetClass, Cut_rowcol [i],
                XmNforeground,          hci_get_read_color (EDIT_FOREGROUND),
                XmNbackground,          hci_get_read_color (EDIT_BACKGROUND),
                XmNcolumns,             10,
                XmNmarginHeight,        2,
                XmNshadowThickness,     2,
                XmNmarginWidth,         0,
                XmNborderWidth,         0,
                XmNhighlightOnEnter,    True,
                XmNuserData,            (XtPointer) -1,
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

            XtAddCallback (Cut [i].start_azimuth,
                XmNmodifyVerifyCallback, hci_verify_unsigned_integer_callback,
                (XtPointer) 3);
            XtAddCallback (Cut [i].start_azimuth,
                XmNfocusCallback, hci_gain_focus_callback,
                NULL);
            XtAddCallback (Cut [i].start_azimuth,
                XmNlosingFocusCallback, hci_change_ccz_azimuth_callback,
                (XtPointer) HCI_CCZ_START_AZIMUTH);
            XtAddCallback (Cut [i].start_azimuth,
                XmNactivateCallback, hci_change_ccz_azimuth_callback,
                (XtPointer) HCI_CCZ_START_AZIMUTH);

            Cut [i].stop_azimuth = XtVaCreateManagedWidget ("",
                xmTextFieldWidgetClass, Cut_rowcol [i],
                XmNforeground,          hci_get_read_color (EDIT_FOREGROUND),
                XmNbackground,          hci_get_read_color (EDIT_BACKGROUND),
                XmNcolumns,             10,
                XmNmarginHeight,        2,
                XmNshadowThickness,     2,
                XmNmarginWidth,         0,
                XmNborderWidth,         0,
                XmNhighlightOnEnter,    True,
                XmNuserData,            (XtPointer) -1,
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

            XtAddCallback (Cut [i].stop_azimuth,
                XmNmodifyVerifyCallback, hci_verify_unsigned_integer_callback,
                (XtPointer) 3);
            XtAddCallback (Cut [i].stop_azimuth,
                XmNfocusCallback, hci_gain_focus_callback,
                NULL);
            XtAddCallback (Cut [i].stop_azimuth,
                XmNlosingFocusCallback, hci_change_ccz_azimuth_callback,
                (XtPointer) HCI_CCZ_STOP_AZIMUTH);
            XtAddCallback (Cut [i].stop_azimuth,
                XmNactivateCallback, hci_change_ccz_azimuth_callback,
                (XtPointer) HCI_CCZ_STOP_AZIMUTH);

            Cut [i].start_range = XtVaCreateManagedWidget ("",
                xmTextFieldWidgetClass, Cut_rowcol [i],
                XmNforeground,          hci_get_read_color (EDIT_FOREGROUND),
                XmNbackground,          hci_get_read_color (EDIT_BACKGROUND),
                XmNcolumns,             10,
                XmNmarginHeight,        2,
                XmNshadowThickness,     2,
                XmNmarginWidth,         0,
                XmNborderWidth,         0,
                XmNhighlightOnEnter,    True,
                XmNuserData,            (XtPointer) -1,
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

            XtAddCallback (Cut [i].start_range,
                XmNmodifyVerifyCallback, hci_verify_unsigned_integer_callback,
                (XtPointer) 3);
            XtAddCallback (Cut [i].start_range,
                XmNfocusCallback, hci_gain_focus_callback,
                NULL);
            XtAddCallback (Cut [i].start_range,
                XmNlosingFocusCallback, hci_change_ccz_range_callback,
                (XtPointer) HCI_CCZ_START_RANGE);
            XtAddCallback (Cut [i].start_range,
                XmNactivateCallback, hci_change_ccz_range_callback,
                (XtPointer) HCI_CCZ_START_RANGE);

            Cut [i].stop_range = XtVaCreateManagedWidget ("",
                xmTextFieldWidgetClass, Cut_rowcol [i],
                XmNforeground,          hci_get_read_color (EDIT_FOREGROUND),
                XmNbackground,          hci_get_read_color (EDIT_BACKGROUND),
                XmNcolumns,             10,
                XmNmarginHeight,        2,
                XmNshadowThickness,     2,
                XmNmarginWidth,         0,
                XmNborderWidth,         0,
                XmNhighlightOnEnter,    True,
                XmNuserData,            (XtPointer) -1,
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

            XtAddCallback (Cut [i].stop_range,
                XmNmodifyVerifyCallback, hci_verify_unsigned_integer_callback,
                (XtPointer) 3);
            XtAddCallback (Cut [i].stop_range,
                XmNfocusCallback, hci_gain_focus_callback,
                NULL);
            XtAddCallback (Cut [i].stop_range,
                XmNlosingFocusCallback, hci_change_ccz_range_callback,
                (XtPointer) HCI_CCZ_STOP_RANGE);
            XtAddCallback (Cut [i].stop_range,
                XmNactivateCallback, hci_change_ccz_range_callback,
                (XtPointer) HCI_CCZ_STOP_RANGE);

            Cut [i].select_code = XtVaCreateManagedWidget ("Select Code",
                xmPushButtonWidgetClass,Cut_rowcol [i],
                XmNforeground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNwidth,               Select_button_width,
                XmNrecomputeSize,       False,
                XmNuserData,            (XtPointer) -1,
                XmNmarginHeight,        3,
                XmNshadowThickness,     1,
                XmNborderWidth,         0,
                XmNhighlightThickness,  1,
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

            XtAddCallback (Cut [i].select_code,
                        XmNactivateCallback, hci_select_code_callback,
                        NULL);

            Cut [i].dopl = XtVaCreateManagedWidget ("Dopl Chan",
                xmPushButtonWidgetClass,Cut_rowcol [i],
                XmNforeground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNwidth,               Select_button_width,
                XmNrecomputeSize,       False,
                XmNuserData,            (XtPointer) -1,
                XmNmarginHeight,        3,
                XmNshadowThickness,     1,
                XmNborderWidth,         0,
                XmNhighlightThickness,  1,
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

            XtAddCallback (Cut [i].dopl,
                        XmNactivateCallback, hci_select_dopl_callback,
                        NULL);

            Cut [i].surv = XtVaCreateManagedWidget ("Surv Chan",
                xmPushButtonWidgetClass,Cut_rowcol [i],
                XmNforeground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNwidth,               Select_button_width,
                XmNrecomputeSize,       False,
                XmNuserData,            (XtPointer) -1,
                XmNmarginHeight,        3,
                XmNshadowThickness,     1,
                XmNborderWidth,         0,
                XmNhighlightThickness,  1,
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

            XtAddCallback (Cut [i].surv,
                        XmNactivateCallback, hci_select_surv_callback,
                        NULL);

            XtManageChild (Cut_rowcol [i]);

        }

        hci_update_clutter_sector_widgets ();

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
                          Clutter_width,
                          Clutter_height,
                          Clutter_depth);

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

        XSetFont (Clutter_display,
                  Clutter_gc,
                  hci_get_font (SMALL));

/*      Initialize the clutter pixmap by filling it black.              */

        XSetForeground (Clutter_display,
                        Clutter_gc,
                        hci_get_read_color (PRODUCT_BACKGROUND_COLOR));
        XFillRectangle (Clutter_display,
                        Clutter_pixmap,
                        Clutter_gc,
                        0, 0,
                        Clutter_width,
                        Clutter_height);
        

        HCI_PM("Reading Base Reflectivity product");
        
/*      Open the base reflectivity product to be used as a background   *
 *      for defining the clutter censor zones.                          */

        hci_load_clutter_background_product (Clutter_background);

        if (Background_display_flag == RMT_CANCELLED)
           HCI_task_exit(HCI_EXIT_SUCCESS);

/*      If a problem occurred reading the reflectivity data then leave  *
 *      the background blank.  Otherwise, display the reflectivity data */

        if (Background_display_flag > 0) {

            if ((hci_product_type () == RADIAL_TYPE) ||
                (hci_product_type () == DHR_TYPE)) {

                hci_display_radial_product (
                        Clutter_display,
                        Clutter_pixmap,
                        Clutter_gc,
                        0,
                        0,
                        Sector_width,
                        Sector_height,
                        HCI_CCZ_MAX_RANGE,
                        Center_x_offset,
                        Center_y_offset,
                        Clutter_zoom);

                XSetLineAttributes (Clutter_display,
                        Clutter_gc,
                        1,
                        LineSolid,
                        CapButt,
                        JoinMiter);

                hci_display_color_bar (Clutter_display,
                        Clutter_pixmap,
                        Clutter_gc,
                        Sector_width,
                        0,
                        (Sector_height),
                        COLOR_BAR_WIDTH,
                        10,
                        0);

                XSetLineAttributes (Clutter_display,
                        Clutter_gc,
                        2,
                        LineSolid,
                        CapButt,
                        JoinMiter);

            }

        } else {

            XSetForeground (Clutter_display,
                            Clutter_gc,
                            hci_get_read_color (BACKGROUND_COLOR1));

            XFillRectangle (Clutter_display,
                            Clutter_pixmap,
                            Clutter_gc,
                            Sector_width,
                            0,
                            90,
                            Sector_height);

            XSetForeground (Clutter_display,
                            Clutter_gc,
                            hci_get_read_color (BLACK));

            if (Background_display_flag == HCI_CCZ_DATA_TIMED_OUT) {

                sprintf (Buff,"Background Product Data Expired");

            } else {

                sprintf (Buff,"Background Product Data Not Available");

            }

            XSetFont (Clutter_display,
                      Clutter_gc,
                      hci_get_font (LARGE));

            XSetForeground (Clutter_display,
                            Clutter_gc,
                            hci_get_read_color (PRODUCT_FOREGROUND_COLOR));

            XDrawString (Clutter_display,
                         Clutter_pixmap,
                         Clutter_gc,
                         (int) (Sector_width/2 - 150),
                         (int) (Sector_height/2 - 50),
                         Buff,
                         strlen(Buff));

            XSetFont (Clutter_display,
                      Clutter_gc,
                      hci_get_font (SMALL));

        }

        XtPopup (Top_widget, XtGrabNone);

        hci_display_clutter_map ();

/*      Set the clip rectangle so we can only draw inside the clutter   *
 *      sector region.                                                  */

        clip_rectangle.x      = 0;
        clip_rectangle.y      = 0;
        clip_rectangle.width  = Sector_width;
        clip_rectangle.height = Clutter_height;

        XSetClipRectangles (Clutter_display,
                            Clutter_gc,
                            0,
                            0,
                            &clip_rectangle,
                            1,
                            Unsorted);

        for (i=0;i<Edit_regions;i++) {

            if (Edit_segment == Edit [i].segment) {

                display_censor_zone_sector (
                        Clutter_display,
                        Clutter_pixmap,
                        Clutter_gc,
                        hci_get_read_color (PRODUCT_FOREGROUND_COLOR),
                        Edit [i].start_azimuth,
                        Edit [i].stop_azimuth,
                        Edit [i].start_range,
                        Edit [i].stop_range,
                        Edit_select_code);

            }
        }

/*      Reset the clip rectangle so we can draw elsewhere (i.e., the    *
 *      composite clutter maps.                                         */

        clip_rectangle.x      = 0;
        clip_rectangle.y      = 0;
        clip_rectangle.width  = Clutter_width;
        clip_rectangle.height = Clutter_height;

        XSetClipRectangles (Clutter_display,
                            Clutter_gc,
                            0,
                            0,
                            &clip_rectangle,
                            1,
                            Unsorted);

        XtUnmanageChild (Cfc_rowcol);

/*	Register for CCZ DEAU changes. I had to hard-code "ccz", since	*
 *	the macro ORPGCCZ_LEGACY_ZONES doesn't work.			*/

	retval = DEAU_UN_register( "ccz",
	                           hci_ccz_deau_callback );

	if( retval < 0 )
	{
	  HCI_LE_error("DEAU_UN_register failed: %d", retval);
	  HCI_task_exit (HCI_EXIT_FAIL) ;
	}

/*	Start HCI loop.							*/

	HCI_start( hci_ccz_timer_proc, HCI_ONE_SECOND, NO_RESIZE_HCI );

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

void
hci_clutter_expose_callback (
Widget          w,
XtPointer       client_data,
XtPointer       call_data
)
{
        if ((Clutter_display == (Display *) NULL) ||
            (Clutter_pixmap  == (Pixmap)    NULL) ||
            (Clutter_window  == (Window)    NULL) ||
            (Clutter_gc      == (GC)        NULL)) {

            return;

        }

/*      Copy the pixmap to the visible window so that any parts of the  *
 *      window which may have been obscured by another window will be   *
 *      visible.                                                        */

        XCopyArea (Clutter_display,
                   Clutter_pixmap,
                   Clutter_window,
                   Clutter_gc,
                   0, 0,
                   Clutter_width,
                   Clutter_height,
                   0, 0);
}


/************************************************************************
 *      Description: This function displays composite clutter maps for  *
 *                   both channels in the compositemap region.          *
 *                                                                      *
 *      Input:  NONE                                                    *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

void
hci_display_clutter_map ()
{
        int     beam;
        int     i;
        float   azimuth1, azimuth2;
        int     old_color;
        int     color;
        int     notch_width;
        XPoint  x [12];
        float   sin1, sin2;
        float   cos1, cos2;
        int     center_pixel;
        int     center_scanl;
        float   scale_x;
        float   scale_y;

/*      For each beam in the product, display it using the product      *
 *      value as a unique color in the display.                         */

        XSetForeground (Clutter_display, Clutter_gc,
                hci_get_read_color (BACKGROUND_COLOR1));

        XFillRectangle (Clutter_display,
                        Clutter_pixmap,
                        Clutter_gc,
                        Sector_width+90, 0,
                        Bypass_map_width,
                        Bypass_map_height);

        scale_x = (Bypass_map_width-50)/(HCI_CCZ_GATES*2.5);
        scale_y = -scale_x;

        for (beam=0;beam<HCI_CCZ_RADIALS;beam++) {

            azimuth1 = beam - 0.5;
            azimuth2 = beam + 0.5;

            sin1 = sin ((double) (azimuth1+90)*HCI_DEG_TO_RAD);
            sin2 = sin ((double) (azimuth2+90)*HCI_DEG_TO_RAD);
            cos1 = cos ((double) (azimuth1-90)*HCI_DEG_TO_RAD);
            cos2 = cos ((double) (azimuth2-90)*HCI_DEG_TO_RAD);

/*      Repeat for both Doppler and Surveillance channels               */
/*      Do Doppler channel first.  It goes in the upper left of the     *
 *      display window.                                                 */

            old_color = -1;

/*      unpack the data at each gate along the beam.  If there is data  *
 *      above the lower threshold, display it.  To reduce the number of *
 *      XPolygonFill operations, only paint gate(s) when a color change *
 *      occurs.                                                         */

            center_pixel = Sector_width + 90 + Bypass_map_width/2;
            center_scanl = Bypass_map_height/5;

            for (i=0;i<=HCI_CCZ_GATES;i++) { 

                color = (int) Clutter_map [beam][i] >> 4;
                notch_width = (int) (Clutter_map [beam][i] >>2) & 3;

                switch (color) {

                    case HCI_CCZ_FILTER_NONE :

                        color = hci_get_read_color (PRODUCT_BACKGROUND_COLOR);
                        break;

                    case HCI_CCZ_FILTER_BYPASS :

                        if (notch_width < 1) {

                            color = hci_get_read_color (PURPLE);

                        } else {

                            color = Bypass_colors [notch_width-1];

                        }

                        break;

                    case HCI_CCZ_FILTER_ALL :

                        if (notch_width < 1) {

                            color = hci_get_read_color (PURPLE);

                        } else {

                            color = All_colors [notch_width-1];

                        }

                        break;

                    default:

                        color = hci_get_read_color (PRODUCT_FOREGROUND_COLOR);
                        break;

                }

                if (color >= 0) {

/*              The value at this gate is not background so process it  */

                    if (color != old_color) {

/*                  If the current value is different from the last     *
 *                  gate processed, either the last value was back-     *
 *                  ground or a real value.                             */

                        if (old_color < 0) {

/*                      If the last gate was background, then find the  *
 *                      coordinates of the first two points making up   *
 *                      the sector.                                     */

                            x[0].x = i * cos1 * scale_x +
                                     center_pixel;
                            x[0].y = i * sin1 * scale_y +
                                     center_scanl;
                            x[1].x = i * cos2 * scale_x +
                                     center_pixel;
                            x[1].y = i * sin2 * scale_y +
                                     center_scanl;
                            x[4].x = x[0].x;
                            x[4].y = x[0].y;

                        } else {

/*                      The last point was not background so find the   *
 *                      coordinates of the last two points defining the *
 *                      sector and display it.                          */

                            x[2].x = i * cos2 * scale_x +
                                     center_pixel;
                            x[2].y = i * sin2 * scale_y +
                                     center_scanl;
                            x[3].x = i * cos1 * scale_x +
                                     center_pixel;
                            x[3].y = i * sin1 * scale_y +
                                     center_scanl;

                            XSetForeground (Clutter_display,
                                            Clutter_gc,
                                            old_color);
                            XFillPolygon (Clutter_display,
                                          Clutter_pixmap,
                                          Clutter_gc,
                                          x, 4,
                                          Convex,
                                          CoordModeOrigin);

                            x[0].x = x[3].x;
                            x[0].y = x[3].y;
                            x[1].x = x[2].x;
                            x[1].y = x[2].y;
                            x[4].x = x[0].x;
                            x[4].y = x[0].y;

                        }

                    }

                } else if (old_color >= 0) {

/*              The last gate had a real value so determine the last    *
 *              pair of points defining the sector and display it.      */

                    x[2].x = i * cos2 * scale_x +
                             center_pixel;
                    x[2].y = i * sin2 * scale_y +
                             center_scanl;
                    x[3].x = i * cos1 * scale_x +
                             center_pixel;
                    x[3].y = i * sin1 * scale_y +
                             center_scanl;

                    XSetForeground (Clutter_display,
                                    Clutter_gc,
                                    old_color);
                    XFillPolygon (Clutter_display,
                                  Clutter_pixmap,
                                  Clutter_gc,
                                  x, 4,
                                  Convex,
                                  CoordModeOrigin);

                    x[0].x = x[3].x;
                    x[0].y = x[3].y;
                    x[1].x = x[2].x;
                    x[1].y = x[2].y;
                    x[4].x = x[0].x;
                    x[4].y = x[0].y;

                }

                old_color = color;

            }

            if (color     >= 0) {

/*          There are no more gates in the beam but check to see if     *
 *          the previous gate had data which needs to be displayed.  If *
 *          so, find the last two sector coordinates and display it.    */

                x[2].x = i * cos2 * scale_x +
                         center_pixel;
                x[2].y = i * sin2 * scale_y +
                         center_scanl;
                x[3].x = i * cos1 * scale_x +
                         center_pixel;
                x[3].y = i * sin1 * scale_y +
                         center_scanl;

                XSetForeground (Clutter_display,
                                Clutter_gc,
                                color);
                XFillPolygon (Clutter_display,
                              Clutter_pixmap,
                              Clutter_gc,
                              x, 4,
                              Convex,
                              CoordModeOrigin);

            }

/*      Display the segment label beneath the map.                      */

        XSetForeground (Clutter_display,
                        Clutter_gc,
                        hci_get_read_color (PRODUCT_BACKGROUND_COLOR));

        XDrawString (Clutter_display,
                     Clutter_pixmap,
                     Clutter_gc,
                     (int) (Sector_width + 90 + Bypass_map_width/2 - 40),
                     (int) ((2*Bypass_map_height)/5),
                     "Doppler Channel",
                     15);

/*      Now go ahead and create the display for the surveillance        *
 *      channel.  It goes in the lower right of the display window.     */

            old_color = -1;

/*      unpack the data at each gate along the beam.  If there is data  *
 *      above the lower threshold, display it.  To reduce the number of *
 *      XPolygonFill operations, only paint gate(s) when a color change *
 *      occurs.                                                         */

            center_scanl = (3*Bypass_map_height)/5 + 15;

            for (i=0;i<=HCI_CCZ_GATES;i++) { 

                color = (int) Clutter_map [beam][i] >> 4;
                notch_width = (int) Clutter_map [beam][i] & 3;

                switch (color) {

                    case HCI_CCZ_FILTER_NONE :

                        color = hci_get_read_color (PRODUCT_BACKGROUND_COLOR);
                        break;

                    case HCI_CCZ_FILTER_BYPASS :

                        if (notch_width < 1) {

                            color = hci_get_read_color (PURPLE);

                        } else {

                            color = Bypass_colors [notch_width-1];

                        }

                        break;

                    case HCI_CCZ_FILTER_ALL :

                        if (notch_width < 1) {

                            color = hci_get_read_color (PURPLE);

                        } else {

                            color = All_colors [notch_width-1];

                        }

                        break;

                    default:

                        color = hci_get_read_color (PRODUCT_FOREGROUND_COLOR);
                        break;

                }

                if (color >= 0) {

/*              The value at this gate is not background so process it  */

                    if (color != old_color) {

/*                  If the current value is different from the last     *
 *                  gate processed, either the last value was back-     *
 *                  ground or a real value.                             */

                        if (old_color < 0) {

/*                      If the last gate was background, then find the  *
 *                      coordinates of the first two points making up   *
 *                      the sector.                                     */

                            x[0].x = i * cos1 * scale_x +
                                     center_pixel;
                            x[0].y = i * sin1 * scale_y +
                                     center_scanl;
                            x[1].x = i * cos2 * scale_x +
                                     center_pixel;
                            x[1].y = i * sin2 * scale_y +
                                     center_scanl;
                            x[4].x = x[0].x;
                            x[4].y = x[0].y;

                        } else {

/*                      The last point was not background so find the   *
 *                      coordinates of the last two points defining the *
 *                      sector and display it.                          */

                            x[2].x = i * cos2 * scale_x +
                                     center_pixel;
                            x[2].y = i * sin2 * scale_y +
                                     center_scanl;
                            x[3].x = i * cos1 * scale_x +
                                     center_pixel;
                            x[3].y = i * sin1 * scale_y +
                                     center_scanl;

                            XSetForeground (Clutter_display,
                                            Clutter_gc,
                                            old_color);
                            XFillPolygon (Clutter_display,
                                          Clutter_pixmap,
                                          Clutter_gc,
                                          x, 4,
                                          Convex,
                                          CoordModeOrigin);

                            x[0].x = x[3].x;
                            x[0].y = x[3].y;
                            x[1].x = x[2].x;
                            x[1].y = x[2].y;
                            x[4].x = x[0].x;
                            x[4].y = x[0].y;

                        }

                    }

                } else if (old_color >= 0) {

/*              The last gate had a real value so determine the last    *
 *              pair of points defining the sector and display it.      */

                    x[2].x = i * cos2 * scale_x +
                             center_pixel;
                    x[2].y = i * sin2 * scale_y +
                             center_scanl;
                    x[3].x = i * cos1 * scale_x +
                             center_pixel;
                    x[3].y = i * sin1 * scale_y +
                             center_scanl;

                    XSetForeground (Clutter_display,
                                    Clutter_gc,
                                    old_color);
                    XFillPolygon (Clutter_display,
                                  Clutter_pixmap,
                                  Clutter_gc,
                                  x, 4,
                                  Convex,
                                  CoordModeOrigin);

                    x[0].x = x[3].x;
                    x[0].y = x[3].y;
                    x[1].x = x[2].x;
                    x[1].y = x[2].y;
                    x[4].x = x[0].x;
                    x[4].y = x[0].y;

                }

                old_color = color;

            }

            if (color     >= 0) {

/*          There are no more gates in the beam but check to see if     *
 *          the previous gate had data which needs to be displayed.  If *
 *          so, find the last two sector coordinates and display it.    */

                x[2].x = i * cos2 * scale_x +
                         center_pixel;
                x[2].y = i * sin2 * scale_y +
                         center_scanl;
                x[3].x = i * cos1 * scale_x +
                         center_pixel;
                x[3].y = i * sin1 * scale_y +
                         center_scanl;

                XSetForeground (Clutter_display,
                                Clutter_gc,
                                color);
                XFillPolygon (Clutter_display,
                              Clutter_pixmap,
                              Clutter_gc,
                              x, 4,
                              Convex,
                              CoordModeOrigin);

            }
        }

/*      Display the segment label beneath the map.                      */

        XSetForeground (Clutter_display,
                        Clutter_gc,
                        hci_get_read_color (PRODUCT_BACKGROUND_COLOR));

        XDrawString (Clutter_display,
                     Clutter_pixmap,
                     Clutter_gc,
                     (int) (Sector_width + 90 + Bypass_map_width/2 - 45),
                     (int) ((4*Bypass_map_height)/5 + 15),
                     "Surveillance Channel",
                     20);

        XSetForeground (Clutter_display, Clutter_gc,
                hci_get_read_color (PURPLE));
        XFillRectangle (Clutter_display,
                    Clutter_pixmap,
                    Clutter_gc,
                    Sector_width + 85 + 76,
                    Bypass_map_height - 45,
                    40,
                    20);
        XSetForeground (Clutter_display, Clutter_gc,
                hci_get_read_color (PRODUCT_FOREGROUND_COLOR));
        XDrawRectangle (Clutter_display,
                    Clutter_pixmap,
                    Clutter_gc,
                    Sector_width + 85 + 76,
                    Bypass_map_height - 45,
                    40,
                    20);

        for (i=0;i<3;i++) {

            XSetForeground (Clutter_display, Clutter_gc,
                Bypass_colors [i]);
            XFillRectangle (Clutter_display,
                            Clutter_pixmap,
                            Clutter_gc,
                            Sector_width + 85 + 116 + i*40,
                            Bypass_map_height - 45,
                            40,
                            20);
            XSetForeground (Clutter_display, Clutter_gc,
                hci_get_read_color (PRODUCT_FOREGROUND_COLOR));
            XDrawRectangle (Clutter_display,
                            Clutter_pixmap,
                            Clutter_gc,
                            Sector_width + 85 + 116 + i*40,
                            Bypass_map_height - 45,
                            40,
                            20);
            XSetForeground (Clutter_display, Clutter_gc,
                All_colors [i]);
            XFillRectangle (Clutter_display,
                            Clutter_pixmap,
                            Clutter_gc,
                            Sector_width + 85 + 116 + i*40,
                            Bypass_map_height - 25,
                            40,
                            20);
            XSetForeground (Clutter_display, Clutter_gc,
                hci_get_read_color (PRODUCT_FOREGROUND_COLOR));
            XDrawRectangle (Clutter_display,
                            Clutter_pixmap,
                            Clutter_gc,
                            Sector_width + 85 + 116 + i*40,
                            Bypass_map_height - 25,
                            40,
                            20);
        }

        XSetForeground (Clutter_display, Clutter_gc,
                hci_get_read_color (PRODUCT_BACKGROUND_COLOR));

        XDrawString (Clutter_display,
                     Clutter_pixmap,
                     Clutter_gc,
                     Sector_width + 90,
                     Bypass_map_height - 25,
                     "Bypass Map",
                     10);

        XDrawString (Clutter_display,
                     Clutter_pixmap,
                     Clutter_gc,
                     Sector_width + 90,
                     Bypass_map_height - 5,
                     "All Bins",
                     8);

        XDrawString (Clutter_display,
                     Clutter_pixmap,
                     Clutter_gc,
                     Sector_width + 85 +76,
                     Bypass_map_height - 50,
                     " NWM ",
                     5);

        XDrawString (Clutter_display,
                     Clutter_pixmap,
                     Clutter_gc,
                     Sector_width + 85 +116,
                     Bypass_map_height - 50,
                     " Low ",
                     5);

        XDrawString (Clutter_display,
                     Clutter_pixmap,
                     Clutter_gc,
                     Sector_width + 85 + 156,
                     Bypass_map_height - 50,
                     " Med ",
                     5);

        XDrawString (Clutter_display,
                     Clutter_pixmap,
                     Clutter_gc,
                     Sector_width + 85 + 196,
                     Bypass_map_height - 50,
                     " High",
                     5);

/*      Display the color code scheme used beneath the maps.            */

        hci_clutter_expose_callback ((Widget) NULL,
                        (XtPointer) NULL,
                        (XtPointer) NULL);
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

void
hci_clutter_close_callback (
Widget          w,
XtPointer       client_data,
XtPointer       call_data
)
{
	char buf[HCI_BUF_128];

        HCI_LE_log("Close selected");

        Close_flag = True;

/*      If any unsaved edits are detected, display a warning to the     *
 *      user and allow them an opportunity to save them first.          */

        if (Update_flag) {

            sprintf( buf, "You did not save your edits.  Do\nyou want to save them?" );
            hci_confirm_popup( Top_widget, buf, close_and_save, close_and_no_save );

        } else {

            HCI_task_exit (HCI_EXIT_SUCCESS);

        }
}


/************************************************************************
 *      Description: This function is activated then the user selects   *
 *                   "No" from the Close button warning popup.          *
 *                                                                      *
 *      Input:  w - ID of "No" button                                   *
 *              client_data - unused                                    *
 *              call_data - unused                                      *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

void
close_and_no_save (
Widget          w,
XtPointer       client_data,
XtPointer       call_data
)
{
        HCI_task_exit (HCI_EXIT_SUCCESS);
}

/************************************************************************
 *      Description: This function is activated then the user selects   *
 *                   "Yes" from the Close button warning popup.         *
 *                                                                      *
 *      Input:  w - ID of "Yes" button                                  *
 *              client_data - unused                                    *
 *              call_data - unused                                      *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

void
close_and_save (
Widget          w,
XtPointer       client_data,
XtPointer       call_data
)
{
        hci_clutter_file_save_yes_callback (w,
                        (XtPointer) NULL,
                        (XtPointer) NULL);
}

/************************************************************************
 *      Description: This function is the destroy callback for the      *
 *                   main window.                                       *
 *                                                                      *
 *      Input:  w - ID of top widget                                    *
 *              client_data - unused                                    *
 *              call_data - unused                                      *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

int
destroy_callback ()
{
        XFreePixmap (Clutter_display,
                     Clutter_pixmap);

        return HCI_OK_TO_EXIT;
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

void
hci_clutter_region_delete (
Widget          w,
XtPointer       client_data,
XtPointer       call_cata
)
{
        int     i;

/*      If there there are clutter regions defined, delete the current  *
 *      one.                                                            */

        if (Edit_region_current >= 0) {

            HCI_LE_log("Deleting region %d", Edit_region_current);

/*          From the current region, move up all trailing regions one   *
 *          position.                                                   */

            for (i=Edit_region_current;i<Edit_regions-1;i++) {

                Edit [i].start_azimuth = Edit [i+1].start_azimuth;
                Edit [i].stop_azimuth  = Edit [i+1].stop_azimuth;
                Edit [i].start_range   = Edit [i+1].start_range;
                Edit [i].stop_range    = Edit [i+1].stop_range;
                Edit [i].segment       = Edit [i+1].segment;
                Edit [i].select_code   = Edit [i+1].select_code;
                Edit [i].dopl          = Edit [i+1].dopl;
                Edit [i].surv          = Edit [i+1].surv;

            }

/*          Unhighlight the button now the entry is deleted.            */

            XtVaSetValues (Cut [Edit_row].sector,
                XmNforeground,  hci_get_read_color (BUTTON_FOREGROUND),
                XmNbackground,  hci_get_read_color (BUTTON_BACKGROUND),
                NULL);

/*          Decrement the number of regions by 1.                       */

            Edit_regions--;

/*          If the update flag is not set, set it, sensitize the Undo   *
 *          button and desensitize the Restore and Update buttons.      */

            if (!Update_flag) {

                XtVaSetValues (Undo_button,
                        XmNsensitive,   True,
                        NULL);
                XtVaSetValues (Update_button,
                        XmNsensitive,   False,
                        NULL);
                XtVaSetValues (Restore_button,
                        XmNsensitive,   False,
                        NULL);

/*              If the File window is open, sensitize the Save button.  */

                if (Open_dialog != (Widget) NULL) {

                    if (Unlocked_loca == HCI_YES_FLAG) {

                        XtVaSetValues (Save_file_button,
                                XmNsensitive,   True,
                                NULL);

                    }
                }
            }

            Update_flag = True;

            Edit_region_current = -1;

/*          Rebuild the composite clutter maps.                         */

            hci_build_clutter_map (&Clutter_map[0][0],
                        Edit,
                        Edit_segment,
                        Edit_regions);

/*          Display the updated composite clutter maps.                 */

            hci_display_clutter_map ();

/*          Update the clutter region overlay in the display area.      */

            hci_refresh_censor_zone_sectors ();

/*          Update the clutter table.                                   */

            hci_update_clutter_sector_widgets ();

/*          Call the expose callback so the window is updated from the  *
 *          background pixmap (everything is drawn into the background  *
 *          pixmap so all updates appear at once).                      */

            hci_clutter_expose_callback ((Widget) NULL,
                        (XtPointer) NULL,
                        (XtPointer) NULL);

        } else {

            sprintf (Buff,"You must select a region before\nit can be deleted!");
            hci_warning_popup( Top_widget, Buff, NULL );

        }
}

/************************************************************************
 *      Description: This function is activated when the "Close"        *
 *                   button is selected.  It terminates the task.       *
 *                                                                      *
 *      Input:  w - ID of Close button                                  *
 *              client_data - unused                                    *
 *              call_data - unused                                      *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

void
hci_clutter_edit_cancel (
Widget          w,
XtPointer       client_data,
XtPointer       call_data
)
{

        HCI_task_exit (HCI_EXIT_SUCCESS);
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

void
hci_clutter_file_callback (
Widget          w,
XtPointer       client_data,
XtPointer       call_data
)
{
        Widget  label;
        Widget  bottom_label;
        Widget  form;
        Widget  file_rowcol;
        Widget  button;
        int     i;
        int     n;
        int     in;
        XmStringTable   list_text;
        Arg     args [16];
        time_t  tm;
        int     month, day, year;
        int     hour, minute, second;
        char    *file_label;

/*      Check to see if window already openned.  If so, bring the       *
 *      window to the top of the window hierarchy and return.           */

        if (Open_dialog != NULL)
	{
          HCI_Shell_popup( Open_dialog );
          return;
        }

        HCI_LE_log("File selected");

        list_text = (XmStringTable) XtMalloc (MAX_CLTR_FILES *
                        sizeof (XmString));

        in = 0;

/*      For each file defined, create a list entry composed of its      *
 *      label and last update time.                                     */

        for (i=0;i<MAX_CLTR_FILES;i++) {

            file_label = hci_get_clutter_region_file_label (i);
            if (strlen (file_label) > 0) {

                tm =hci_get_clutter_region_file_time (i);

                if (tm < 1) {

                    year   = 0;
                    month  = 0;
                    day    = 0;
                    hour   = 0;
                    minute = 0;
                    second = 0;

                } else {

                    unix_time (&tm,
                       &year,
                       &month,
                       &day,
                       &hour,
                       &minute,
                       &second);

                }

                sprintf (Buff,"%2.2d/%2.2d/%4.4d  %2.2d:%2.2d:%2.2d  %s",
                        month, day, year, hour, minute, second,
                        file_label);

                list_text [in] = XmStringCreateLocalized (Buff);

                in++;

            }
        }

/*      Create a shell widget for the File window.                      */

	HCI_Shell_init( &Open_dialog, "Clutter Region Files" );

        form = XtVaCreateWidget ("clutter_file_form",
                xmFormWidgetClass,      Open_dialog,
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                NULL);

        file_rowcol = XtVaCreateWidget ("file_rowcol",
                xmRowColumnWidgetClass, form,
                XmNleftAttachment,      XmATTACH_FORM,
                XmNtopAttachment,       XmATTACH_FORM,
                XmNrightAttachment,     XmATTACH_FORM,
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNpacking,             XmPACK_COLUMN,
                XmNorientation,         XmHORIZONTAL,
                NULL);

        button = XtVaCreateManagedWidget ("  Close  ",
                xmPushButtonWidgetClass,file_rowcol,
                XmNforeground,          hci_get_read_color (BUTTON_FOREGROUND),
                XmNbackground,          hci_get_read_color (BUTTON_BACKGROUND),
                XmNfontList,            hci_get_fontlist (LIST),
                XmNalignment,           XmALIGNMENT_CENTER,
                NULL);

        XtAddCallback (button,
                XmNactivateCallback,  hci_close_clutter_file_window, NULL);

        New_file_button = XtVaCreateManagedWidget ("   New   ",
                xmPushButtonWidgetClass, file_rowcol,
                XmNforeground,          hci_get_read_color (BUTTON_FOREGROUND),
                XmNbackground,          hci_get_read_color (BUTTON_BACKGROUND),
                XmNfontList,            hci_get_fontlist (LIST),
                XmNalignment,           XmALIGNMENT_CENTER,
                XmNsensitive,           True,
                NULL);

        XtAddCallback (New_file_button,
                XmNactivateCallback, hci_clutter_file_new_callback,
                NULL);

        Save_file_button = XtVaCreateManagedWidget ("  Save   ",
                xmPushButtonWidgetClass, file_rowcol,
                XmNforeground,          hci_get_read_color (BUTTON_FOREGROUND),
                XmNbackground,          hci_get_read_color (BUTTON_BACKGROUND),
                XmNfontList,            hci_get_fontlist (LIST),
                XmNalignment,           XmALIGNMENT_CENTER,
                XmNsensitive,           False,
                NULL);

/*      We are assuming as we open this window that the only way the    *
 *      Update flag can be set is if a valid password was previously    *
 *      entered.                                                        */

        if (Update_flag) {

            if (Unlocked_loca == HCI_YES_FLAG) {

                XtVaSetValues (Save_file_button,
                        XmNsensitive,   True,
                        NULL);

            }
        }

        XtAddCallback (Save_file_button,
                XmNactivateCallback, hci_clutter_file_save_callback,
                (XtPointer) 0);

        Save_file_as_button = XtVaCreateManagedWidget (" Save As ",
                xmPushButtonWidgetClass, file_rowcol,
                XmNforeground,          hci_get_read_color (BUTTON_FOREGROUND),
                XmNbackground,          hci_get_read_color (BUTTON_BACKGROUND),
                XmNfontList,            hci_get_fontlist (LIST),
                XmNalignment,           XmALIGNMENT_CENTER,
                XmNsensitive,           True,
                NULL);

        XtAddCallback (Save_file_as_button,
                XmNactivateCallback, hci_clutter_file_save_as_callback,
                (XtPointer) -1);

        Delete_file_button = XtVaCreateManagedWidget ("Delete",
                xmPushButtonWidgetClass,        file_rowcol,
                XmNforeground,          hci_get_read_color (BUTTON_FOREGROUND),
                XmNbackground,          hci_get_read_color (BUTTON_BACKGROUND),
                XmNfontList,            hci_get_fontlist (LIST),
                XmNalignment,           XmALIGNMENT_CENTER,
                XmNsensitive,           False,
                NULL);

        XtAddCallback (Delete_file_button,
                XmNactivateCallback, hci_clutter_delete_callback, NULL);

        XtManageChild (file_rowcol);

        bottom_label = XtVaCreateManagedWidget ("Double click to select file",
                xmLabelWidgetClass,     form,
                XmNleftAttachment,      XmATTACH_FORM,
                XmNrightAttachment,     XmATTACH_FORM,
                XmNbottomAttachment,    XmATTACH_FORM,
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

        label = XtVaCreateManagedWidget ("  Date      Time      Label",
                xmLabelWidgetClass,     form,
                XmNleftAttachment,      XmATTACH_FORM,
                XmNtopAttachment,       XmATTACH_WIDGET,
                XmNtopWidget,           file_rowcol,
                XmNrightAttachment,     XmATTACH_FORM,
                XmNalignment,           XmALIGNMENT_BEGINNING,
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

/*      Create the scrolled files list.                                 */

        n = 0;

        XtSetArg (args [n], XmNvisibleItemCount, VISIBLE_CLUTTER_REGION_FILES); n++;
        XtSetArg (args [n], XmNitemCount,        in);              n++;
        XtSetArg (args [n], XmNitems,            list_text);       n++;
        XtSetArg (args [n], XmNleftAttachment,   XmATTACH_FORM);   n++;
        XtSetArg (args [n], XmNtopAttachment,    XmATTACH_WIDGET); n++;
        XtSetArg (args [n], XmNtopWidget,        label);           n++;
        XtSetArg (args [n], XmNrightAttachment,  XmATTACH_FORM);   n++;
        XtSetArg (args [n], XmNbottomAttachment, XmATTACH_WIDGET); n++;
        XtSetArg (args [n], XmNbottomWidget,     bottom_label);    n++;
        XtSetArg (args [n], XmNforeground,       hci_get_read_color (TEXT_FOREGROUND)); n++;
        XtSetArg (args [n], XmNbackground,       hci_get_read_color (BACKGROUND_COLOR1)); n++;
        XtSetArg (args [n], XmNfontList,        hci_get_fontlist (LIST)); n++;

        List_widget =  XmCreateScrolledList (form,
                "Clutter_region_files_list",
                args,
                n);

        XtAddCallback (List_widget,
                XmNsingleSelectionCallback,  hci_select_clutter_file, NULL);
        XtAddCallback (List_widget,
                XmNdefaultActionCallback,  hci_select_clutter_file, NULL);

        XtManageChild (List_widget);

        for (i=0;i<in;i++) {

            XmStringFree (list_text [i]);

        }

        XtFree ((XtPointer) list_text);

        XtManageChild (form);

        XtRealizeWidget (Open_dialog);

/*      If the window is password unlocked, then sensitize the New,     *
 *      Delete, and Save As buttons.                                    */

        if (Unlocked_loca == HCI_YES_FLAG) {

            XtVaSetValues (Delete_file_button,
                XmNsensitive,   True,
                NULL);

        }

/*      Force the current file to be selected (highlighted) in the list */

        XmListSelectPos (List_widget, Clutter_file+1, False);

	HCI_Shell_start( Open_dialog, NO_RESIZE_HCI );
}

/************************************************************************
 *      Description: This function is activated when a file is          *
 *                   selected from the Clutter Region Files list.  It   *
 *                   opens the selected file.                           *
 *                                                                      *
 *      Input:  w - ID of File list widget                              *
 *              client_data - unused                                    *
 *              call_data - list data                                   *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

void
hci_select_clutter_file (
Widget          w,
XtPointer       client_data,
XtPointer       call_data
)
{
        XmListCallbackStruct    *cbs = (XmListCallbackStruct *) call_data;

        New_file     = cbs->item_position-1;

        HCI_LE_log("File %d selected", Clutter_file);

/*      If the selected file is the same as the current one, do nothing *
 *      and return.                                                     */

        if (New_file == Clutter_file) {

            return;

        } else {

/*          A different file was selected so check to see if any edits  *
 *          are detected for the current file.  If there are anu unsaved*
 *          edits, call the save callback so the user can decide to     *
 *          save them first.                                            */

            if (Update_flag) {

                if (Unlocked_loca == HCI_YES_FLAG) {

                    hci_clutter_file_save_callback (w,
                        (XtPointer) 1,
                        (XtPointer) NULL);

                }

            } else {

/*          No edits were detected so change the current file pointer   *
 *          and load the data from the selected file.                   */

                Clutter_file = New_file;

                hci_clutter_reload_callback ((Widget) NULL,
                                         (XtPointer) NULL,
                                         (XtPointer) NULL);

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

void
hci_close_clutter_file_window (
Widget          w,
XtPointer       client_data,
XtPointer       call_data
)
{
        HCI_LE_log("File Close selected");
	HCI_Shell_popdown( Open_dialog );
}

/************************************************************************
 *      Description: This function updates the widgets in the clutter   *
 *                   regions table.                                     *
 *                                                                      *
 *      Input:  NONE                                                    *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

void
hci_update_clutter_sector_widgets ()
{
        int     i;
        int     cut;
        int     color = -1;
        XmString        str;

/*      If the window is unlocked by the URC or ROC user, then we       *
 *      want to set the edit flag to True and the background color      *
 *      for text widgets to the edit background color.                  */

        if (Unlocked_loca == HCI_YES_FLAG) {

/*          If any edits are detected, the Restore and Update           *
 *          buttons shoud be desensitized.                              */

            if (Update_flag) {

                XtVaSetValues (Restore_button,
                        XmNsensitive,   False,
                        NULL);
                XtVaSetValues (Update_button,
                        XmNsensitive,   False,
                        NULL);

            } else {

                XtVaSetValues (Restore_button,
                        XmNsensitive,   True,
                        NULL);
                XtVaSetValues (Update_button,
                        XmNsensitive,   True,
                        NULL);

            }

        } else {

/*          Since the window is locked, the Restore, Update, Save,      *
 *          and Delete File buttons should be desensitized.             */

            XtVaSetValues (Restore_button,
                XmNsensitive,   False,
                NULL);
            XtVaSetValues (Update_button,
                XmNsensitive,   False,
                NULL);

        }

        cut = -1;

/*      For the current segment, extract all segments from the file     *
 *      which match it and display them.                                */

        for (i=0;i<Edit_regions;i++) {

            if (Edit_segment == Edit [i].segment) {

                cut++;
                Cut [cut].cut = i;

                XtVaSetValues (Cut [cut].sector,
                    XmNuserData,        (XtPointer) cut,
                    NULL);

                sprintf (Buff,"%d ",(int) (Edit [i].start_azimuth+0.5));
                XmTextSetString (Cut [cut].start_azimuth, Buff);

                XtVaSetValues (Cut [cut].start_azimuth,
                    XmNuserData,        (XtPointer) cut,
                    NULL);

                sprintf (Buff,"%d ",(int) (Edit [i].stop_azimuth+0.5));
                XmTextSetString (Cut [cut].stop_azimuth, Buff);

                XtVaSetValues (Cut [cut].stop_azimuth,
                    XmNuserData,        (XtPointer) cut,
                    NULL);

                sprintf (Buff,"%d ",(int) (Edit [i].start_range*Units_factor+0.5));
                XmTextSetString (Cut [cut].start_range, Buff);

                XtVaSetValues (Cut [cut].start_range,
                    XmNuserData,        (XtPointer) cut,
                    NULL);

                sprintf (Buff,"%d ",(int) (Edit [i].stop_range*Units_factor+0.5));
                XmTextSetString (Cut [cut].stop_range, Buff);

                XtVaSetValues (Cut [cut].stop_range,
                    XmNuserData,        (XtPointer) cut,
                    NULL);

                switch (Edit [i].select_code) {

                    case HCI_CCZ_FILTER_BYPASS :

                        sprintf (Buff," Bypass Map");
                        break;

                    case HCI_CCZ_FILTER_NONE :

                        sprintf (Buff,"    None   ");
                        break;

                    case HCI_CCZ_FILTER_ALL :

                        sprintf (Buff,"  All Bins ");
                        break;

                    default :

                        sprintf (Buff,"     ??    ");
                        break;

                }

                str = XmStringCreateLocalized (Buff);

                XtVaSetValues (Cut [cut].select_code,
                        XmNlabelString, str,
                        XmNuserData,    (XtPointer) cut,
                        XmNforeground,  hci_get_read_color (BUTTON_FOREGROUND),
                        XmNbackground,  hci_get_read_color (BUTTON_BACKGROUND),
                        NULL);

                XmStringFree (str);

/*              Determine the color table to use for the filter notch   *
 *              width value.                                            */

                switch (Edit [i].dopl-1) {

                    case LOW :

                        sprintf (Buff,"   Low   ");

                        switch (Edit [i].select_code) {

                            case HCI_CCZ_FILTER_BYPASS :

                                color = Bypass_colors [LOW];
                                break;

                            case HCI_CCZ_FILTER_NONE :

                                color = hci_get_read_color (BACKGROUND_COLOR1);
                                break;
        
                            case HCI_CCZ_FILTER_ALL :

                                color = All_colors [LOW];
                                break;

                        }

                        break;

                    case MEDIUM :

                        sprintf (Buff,"  Medium ");

                        switch (Edit [i].select_code) {

                            case HCI_CCZ_FILTER_BYPASS :

                                color = Bypass_colors [MEDIUM];
                                break;

                            case HCI_CCZ_FILTER_NONE :

                                color = hci_get_read_color (BACKGROUND_COLOR1);
                                break;
        
                            case HCI_CCZ_FILTER_ALL :

                                color = All_colors [MEDIUM];
                                break;

                        }
                        break;

                    case HIGH :

                        sprintf (Buff,"   High  ");

                        switch (Edit [i].select_code) {

                            case HCI_CCZ_FILTER_BYPASS :

                                color = Bypass_colors [HIGH];
                                break;

                            case HCI_CCZ_FILTER_NONE :

                                color = hci_get_read_color (BACKGROUND_COLOR1);
                                break;
        
                            case HCI_CCZ_FILTER_ALL :

                                color = All_colors [HIGH];
                                break;

                        }
                        break;

                    default :

                        sprintf (Buff,"     ??    ");
                        break;

                }

                str = XmStringCreateLocalized (Buff);

                if (Edit [i].select_code == HCI_CCZ_FILTER_NONE) {

                    XtVaSetValues (Cut [cut].dopl,
                        XmNlabelString, str,
                        XmNbackground,  color,
                        XmNforeground,  color,
                        XmNuserData,    (XtPointer) cut,
                        XmNsensitive,   False,
                        NULL);

                } else {

                    XtVaSetValues (Cut [cut].dopl,
                        XmNlabelString, str,
                        XmNbackground,  color,
                        XmNforeground,  hci_get_read_color (EDIT_FOREGROUND),
                        XmNuserData,    (XtPointer) cut,
                        XmNsensitive,   True,
                        NULL);

                }

                XmStringFree (str);

                switch (Edit [i].surv-1) {

                    case LOW :

                        sprintf (Buff,"   Low   ");

                        switch (Edit [i].select_code) {

                            case HCI_CCZ_FILTER_BYPASS :

                                color = Bypass_colors [LOW];
                                break;

                            case HCI_CCZ_FILTER_NONE :

                                color = hci_get_read_color (BACKGROUND_COLOR1);
                                break;

                            case HCI_CCZ_FILTER_ALL :

                                color = All_colors [LOW];
                                break;

                        }
                        break;

                    case MEDIUM :

                        sprintf (Buff,"  Medium ");

                        switch (Edit [i].select_code) {

                            case HCI_CCZ_FILTER_BYPASS :

                                color = Bypass_colors [MEDIUM];
                                break;

                            case HCI_CCZ_FILTER_NONE :

                                color = hci_get_read_color (BACKGROUND_COLOR1);
                                break;
        
                            case HCI_CCZ_FILTER_ALL :

                                color = All_colors [MEDIUM];
                                break;

                        }
                        break;

                    case HIGH :

                        sprintf (Buff,"   High  ");

                        switch (Edit [i].select_code) {

                            case HCI_CCZ_FILTER_BYPASS :

                                color = Bypass_colors [HIGH];
                                break;

                            case HCI_CCZ_FILTER_NONE :

                                color = hci_get_read_color (BACKGROUND_COLOR1);
                                break;
        
                            case HCI_CCZ_FILTER_ALL :

                                color = All_colors [HIGH];
                                break;

                        }
                        break;

                    default :

                        sprintf (Buff,"     ??    ");
                        break;

                }

                str = XmStringCreateLocalized (Buff);

                if (Edit [i].select_code == HCI_CCZ_FILTER_NONE) {

                    XtVaSetValues (Cut [cut].surv,
                        XmNlabelString, str,
                        XmNbackground,  color,
                        XmNforeground,  color,
                        XmNuserData,    (XtPointer) cut,
                        XmNsensitive,   False,
                        NULL);

                } else {

                    XtVaSetValues (Cut [cut].surv,
                        XmNlabelString, str,
                        XmNbackground,  color,
                        XmNforeground,  hci_get_read_color (EDIT_FOREGROUND),
                        XmNuserData,    (XtPointer) cut,
                        XmNsensitive,   True,
                        NULL);

                }

                XmStringFree (str);

                XtManageChild (Cut_rowcol [cut]);

            }
        }

        strcpy (Buff,"");

/*      We need to clear the table beyond what is curently defined      */

        for (i=cut+1;i<MAX_NUMBER_CLUTTER_ZONES;i++) {

            Cut [i].cut = -1;

            XtUnmanageChild (Cut_rowcol [i]);

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

void
hci_select_sector_callback (
Widget          w,
XtPointer       client_data,
XtPointer       call_data
)
{
        XtPointer       data;
        int             cut;
static  int             old_cut = 0;

/*      The row number is passed via the widget user data.  Get it.     */

        XtVaGetValues (w,
                XmNuserData,    &data,
                NULL);

        HCI_LE_log("Sector %d selected", (int) data);

/*      Unselect (by changing background color) the previously selected *
 *      table row.                                                      */

        XtVaSetValues (Cut[old_cut].sector,
                XmNforeground,  hci_get_read_color (BUTTON_FOREGROUND),
                XmNbackground,  hci_get_read_color (BUTTON_BACKGROUND),
                NULL);

        cut = (int) data;

/*      Highlight the newly selected row and update the current region  *
 *      pointer.                                                        */

        if (cut >= 0) {

            XtVaSetValues (Cut[cut].sector,
                XmNbackground,  hci_get_read_color (BUTTON_FOREGROUND),
                XmNforeground,  hci_get_read_color (BUTTON_BACKGROUND),
                NULL);

            Edit_region_current = Cut[cut].cut;

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

void
hci_select_code_callback (
Widget          w,
XtPointer       client_data,
XtPointer       call_data
)
{
        XtPointer       data;
        XmString        str;
        int             i;
        int             num;

/*      The table row number is passed via user data.  Get it.  */

        XtVaGetValues (w,
                XmNuserData,    &data,
                NULL);

        i = (int) data;

/*      If the row number isn't valid do nothing.               */

        if (i < 0) {

            return;

        }

        HCI_LE_log("Select code %d selected", Edit [Cut [i].cut].select_code);

/*      For the selected button, change the select code label and       *
 *      and value to the next selected code in the heirarchy.           */

        switch (Edit [Cut [i].cut].select_code) {

            case HCI_CCZ_FILTER_BYPASS :

                sprintf (Buff,"  All Bins ");
                Edit [Cut [i].cut].select_code = HCI_CCZ_FILTER_ALL;

                Filter_colors [LOW]    = All_colors [LOW];
                Filter_colors [MEDIUM] = All_colors [MEDIUM];
                Filter_colors [HIGH]   = All_colors [HIGH];

                num = Edit [Cut [i].cut].surv-1;

                XtVaSetValues (Cut [i].surv,
                        XmNbackground,  Filter_colors [num],
                        XmNforeground,  hci_get_read_color (TEXT_FOREGROUND),
                        XmNsensitive,   True,
                        NULL);

                num = Edit [Cut [i].cut].dopl-1;

                XtVaSetValues (Cut [i].dopl,
                        XmNbackground,  Filter_colors [num],
                        XmNforeground,  hci_get_read_color (TEXT_FOREGROUND),
                        XmNsensitive,   True,
                        NULL);

                break;

            case HCI_CCZ_FILTER_NONE :

                sprintf (Buff," Bypass Map");
                Edit [Cut [i].cut].select_code = HCI_CCZ_FILTER_BYPASS;

                Filter_colors [LOW]    = Bypass_colors [LOW];
                Filter_colors [MEDIUM] = Bypass_colors [MEDIUM];
                Filter_colors [HIGH]   = Bypass_colors [HIGH];

                num = Edit [Cut [i].cut].surv-1;

                XtVaSetValues (Cut [i].surv,
                        XmNbackground,  Filter_colors [num],
                        XmNforeground,  hci_get_read_color (TEXT_FOREGROUND),
                        XmNsensitive,   True,
                        NULL);

                num = Edit [Cut [i].cut].dopl-1;

                XtVaSetValues (Cut [i].dopl,
                        XmNbackground,  Filter_colors [num],
                        XmNforeground,  hci_get_read_color (TEXT_FOREGROUND),
                        XmNsensitive,   True,
                        NULL);
                break;

            case HCI_CCZ_FILTER_ALL :

                sprintf (Buff,"    None   ");
                Edit [Cut [i].cut].select_code = HCI_CCZ_FILTER_NONE;

                Filter_colors [LOW]    = hci_get_read_color (SEAGREEN);
                Filter_colors [MEDIUM] = hci_get_read_color (SEAGREEN);
                Filter_colors [HIGH]   = hci_get_read_color (SEAGREEN);

                XtVaSetValues (Cut [i].surv,
                        XmNbackground,  hci_get_read_color (BACKGROUND_COLOR1),
                        XmNforeground,  hci_get_read_color (BACKGROUND_COLOR1),
                        XmNsensitive,   False,
                        NULL);

                XtVaSetValues (Cut [i].dopl,
                        XmNbackground,  hci_get_read_color (BACKGROUND_COLOR1),
                        XmNforeground,  hci_get_read_color (BACKGROUND_COLOR1),
                        XmNsensitive,   False,
                        NULL);
                break;

            default :

                sprintf (Buff," Bypass Map");
                Edit [Cut [i].cut].select_code = HCI_CCZ_FILTER_BYPASS;

                Filter_colors [LOW]    = Bypass_colors [LOW];
                Filter_colors [MEDIUM] = Bypass_colors [MEDIUM];
                Filter_colors [HIGH]   = Bypass_colors [HIGH];

                num = Edit [Cut [i].cut].surv-1;

                XtVaSetValues (Cut [i].surv,
                        XmNbackground,  Filter_colors [num],
                        XmNforeground,  hci_get_read_color (TEXT_FOREGROUND),
                        XmNsensitive,   True,
                        NULL);

                num = Edit [Cut [i].cut].dopl-1;

                XtVaSetValues (Cut [i].dopl,
                        XmNbackground,  Filter_colors [num],
                        XmNforeground,  hci_get_read_color (TEXT_FOREGROUND),
                        XmNsensitive,   True,
                        NULL);

                break;

        }

        str = XmStringCreateLocalized (Buff);

        XtVaSetValues (w,
                XmNlabelString, str,
                NULL);

        XmStringFree (str);

/*      If no previous edits are detected, set the update flag and      *
 *      sensitize the Undo button and desensitize the Update and        *
 *      Restore buttons.                                                */

        if (!Update_flag) {

            XtVaSetValues (Undo_button,
                XmNsensitive,   True,
                NULL);
            XtVaSetValues (Update_button,
                XmNsensitive,   False,
                NULL);
            XtVaSetValues (Restore_button,
                XmNsensitive,   False,
                NULL);

/*          If the File window is open sensitize the Save button.       */

            if (Open_dialog != (Widget) NULL) {

                if (Unlocked_loca == HCI_YES_FLAG) {

                    XtVaSetValues (Save_file_button,
                        XmNsensitive,   True,
                        NULL);

                }
            }
        }

        Update_flag = True;

/*      Rebuild the composite clutter maps and redisplay them.          */

        hci_build_clutter_map (&Clutter_map[0][0],
                        Edit,
                        Edit_segment,
                        Edit_regions);

        hci_display_clutter_map ();

}

/************************************************************************
 *      Description: This function is activated when a "Dopl Chan"      *
 *                   button is selected from the clutter regions table. *
 *                                                                      *
 *      Input:  w - ID of selected button                               *
 *              client_data - unused                                    *
 *              call_data - unused                                      *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

void
hci_select_dopl_callback (
Widget          w,
XtPointer       client_data,
XtPointer       call_data
)
{
        XtPointer       data;
        XmString        str;
        int             i;
        Pixel           color = -1;

/*      The table row number is passed via user data.  Get it.  */

        XtVaGetValues (w,
                XmNuserData,    &data,
                NULL);

        i = (int) data;

/*      If an invalid row number is detected, do nothing.       */

        if (i < 0) {

            return;

        }

/*      First determine the select code type and set the colors         *
 *      accordingly.                                                    */

        HCI_LE_log("Doppler Level %d selected", Edit[Cut [i].cut].select_code);

        switch (Edit [Cut [i].cut].select_code) {

            case HCI_CCZ_FILTER_BYPASS :

                Filter_colors [LOW]    = Bypass_colors [LOW];
                Filter_colors [MEDIUM] = Bypass_colors [MEDIUM];
                Filter_colors [HIGH]   = Bypass_colors [HIGH];
                break;

            case HCI_CCZ_FILTER_ALL:

                Filter_colors [LOW]    = All_colors [LOW];
                Filter_colors [MEDIUM] = All_colors [MEDIUM];
                Filter_colors [HIGH]   = All_colors [HIGH];
                break;

            case HCI_CCZ_FILTER_NONE:

                Filter_colors [LOW]    = hci_get_read_color (SEAGREEN);
                Filter_colors [MEDIUM] = hci_get_read_color (SEAGREEN);
                Filter_colors [HIGH]   = hci_get_read_color (SEAGREEN);
                break;

            default:

/*      If an unknown select code was detected, do nothing and return.  */

                return;

        }

/*      Change the notch width value to the next sequential one.        */

        switch (Edit [Cut [i].cut].dopl-1) {

            case LOW :

                sprintf (Buff,"  Medium ");
                Edit [Cut [i].cut].dopl = MEDIUM+1;
                color = Filter_colors [MEDIUM];
                break;

            case MEDIUM :

                sprintf (Buff,"   High  ");
                Edit [Cut [i].cut].dopl = HIGH+1;
                color = Filter_colors [HIGH];
                break;

            case HIGH :

                sprintf (Buff,"   Low   ");
                Edit [Cut [i].cut].dopl = LOW+1;
                color = Filter_colors [LOW];
                break;

        }

        str = XmStringCreateLocalized (Buff);

        XtVaSetValues (w,
                XmNlabelString, str,
                XmNbackground,  color,
                NULL);

        XmStringFree (str);

/*      If no previous edits are detected, set the update flag and      *
 *      sensitize the Undo button and desensitize the Update and        *
 *      Restore buttons.                                                */

        if (!Update_flag) {

            XtVaSetValues (Undo_button,
                XmNsensitive,   True,
                NULL);
            XtVaSetValues (Update_button,
                XmNsensitive,   False,
                NULL);
            XtVaSetValues (Restore_button,
                XmNsensitive,   False,
                NULL);

/*          If the File window is open sensitize the Save button.       */

            if (Open_dialog != (Widget) NULL) {

                if (Unlocked_loca == HCI_YES_FLAG) {

                    XtVaSetValues (Save_file_button,
                        XmNsensitive,   True,
                        NULL);

                }
            }
        }

        Update_flag = True;

/*      Rebuild the composite clutter maps and redisplay them.          */

        hci_build_clutter_map (&Clutter_map[0][0],
                        Edit,
                        Edit_segment,
                        Edit_regions);

        hci_display_clutter_map ();

}

/************************************************************************
 *      Description: This function is activated when a "Surv Chan"      *
 *                   button is selected from the clutter regions table. *
 *                                                                      *
 *      Input:  w - ID of selected button                               *
 *              client_data - unused                                    *
 *              call_data - unused                                      *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

void
hci_select_surv_callback (
Widget          w,
XtPointer       client_data,
XtPointer       call_data
)
{
        XtPointer       data;
        XmString        str;
        int             i;
        Pixel           color = -1;

/*      The table row number is passed via user data.  Get it.  */

        XtVaGetValues (w,
                XmNuserData,    &data,
                NULL);

        i = (int) data;

/*      If an invalid row number is detected, do nothing.       */

        if (i < 0) {

            return;

        }

/*      First determine the select code type and set the colors         *
 *      accordingly.                                                    */

        HCI_LE_log("Surveilance Level %d selected", Edit[Cut[i].cut].select_code);

        switch (Edit [Cut [i].cut].select_code) {

            case HCI_CCZ_FILTER_BYPASS :

                Filter_colors [LOW]    = Bypass_colors [LOW];
                Filter_colors [MEDIUM] = Bypass_colors [MEDIUM];
                Filter_colors [HIGH]   = Bypass_colors [HIGH];
                break;

            case HCI_CCZ_FILTER_ALL:

                Filter_colors [LOW]    = All_colors [LOW];
                Filter_colors [MEDIUM] = All_colors [MEDIUM];
                Filter_colors [HIGH]   = All_colors [HIGH];
                break;

            case HCI_CCZ_FILTER_NONE:

                Filter_colors [LOW]    = hci_get_read_color (SEAGREEN);
                Filter_colors [MEDIUM] = hci_get_read_color (SEAGREEN);
                Filter_colors [HIGH]   = hci_get_read_color (SEAGREEN);
                break;

            default:

/*      If an unknown select code was detected, do nothing and return.  */

                return;

        }

/*      Change the notch width value to the next sequential one.        */

        switch (Edit [Cut [i].cut].surv-1) {

            case LOW :

                sprintf (Buff,"  Medium ");
                Edit [Cut [i].cut].surv = MEDIUM+1;
                color = Filter_colors [MEDIUM];
                break;

            case MEDIUM :

                sprintf (Buff,"   High  ");
                Edit [Cut [i].cut].surv = HIGH+1;
                color = Filter_colors [HIGH];
                break;

            case HIGH :

                sprintf (Buff,"   Low   ");
                Edit [Cut [i].cut].surv = LOW+1;
                color = Filter_colors [LOW];
                break;

        }

        str = XmStringCreateLocalized (Buff);

        XtVaSetValues (w,
                XmNlabelString, str,
                XmNbackground,  color,
                NULL);

        XmStringFree (str);

/*      If no previous edits are detected, set the update flag and      *
 *      sensitize the Undo button and desensitize the Update and        *
 *      Restore buttons.                                                */

        if (!Update_flag) {

            XtVaSetValues (Undo_button,
                XmNsensitive,   True,
                NULL);
            XtVaSetValues (Update_button,
                XmNsensitive,   False,
                NULL);
            XtVaSetValues (Restore_button,
                XmNsensitive,   False,
                NULL);

/*          If the File window is open sensitize the Save button.       */

            if (Open_dialog != (Widget) NULL) {

                if (Unlocked_loca == HCI_YES_FLAG) {

                    XtVaSetValues (Save_file_button,
                        XmNsensitive,   True,
                        NULL);

                }
            }
        }

        Update_flag = True;

/*      Rebuild the composite clutter maps and redisplay them.          */

        hci_build_clutter_map (&Clutter_map[0][0],
                        Edit,
                        Edit_segment,
                        Edit_regions);

        hci_display_clutter_map ();
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

void
hci_change_ccz_azimuth_callback (
Widget          w,
XtPointer       client_data,
XtPointer       call_data
)
{
        XtPointer       data;
        char            *text;
        int             value;
        int             i;
        char            buf [16];
        int             err_flag;

/*      The table row number is passed via user data.  Get it.  */

        XtVaGetValues (w,
                XmNuserData,    &data,
                NULL);

        i = (int) data;

/*      If an invalid row number is detected, do nothing.       */

        if (i < 0) {

            return;

        }

/*      Get the azimuth value from the widget text.             */

        text = XmTextGetString (w);

/*      If a value found, verify that it falls within the allowed       *
 *      range.                                                          */

        err_flag = 0;

        if (strlen (text)) {

            sscanf (text,"%d",&value);

/*          If an invalid azimuth was entered, then we need to inform   *
 *          the user and reset the value to its previous value.         */

            if ((value > HCI_CCZ_MAX_AZIMUTH) ||
                (value < HCI_CCZ_MIN_AZIMUTH)) {

                sprintf (Buff,"You entered an invalid value of %d.\nThe valid range is %d to %d.",
                        value, HCI_CCZ_MIN_AZIMUTH, HCI_CCZ_MAX_AZIMUTH);

                err_flag = 1;

/*          A valid azimuth angle was entered, so accept it as long as  *
 *          it is different from the other azimuth.                     */

            } else {

/*          Now set the appropriate entry in the clutter censor zone    *
 *          data.                                                       */

                switch ((int) client_data) {

/*                  If we are changing the start azimuth, only update   *
 *                  it if is changed.                                   */

                    case HCI_CCZ_START_AZIMUTH:

                        if (value == (int) Edit [Cut [i].cut].stop_azimuth) {

                            sprintf (Buff,"You entered an invalid value of %d.\nIt must be different from the other\nsector azimuth.", value);

                            err_flag = 1;

                        } else {

                            if (Edit [Cut [i].cut].start_azimuth != value) {

                                Edit [Cut [i].cut].start_azimuth = value;

                                HCI_LE_log("Azimuth %5.1f selected", value);

/*                          If this is the first edit, then we need to  *
 *                          set the update flag, sensitize the Undo     *
 *                          button and desensitize the Restore and      *
 *                          Update buttons.                             */

                                if (!Update_flag) {

                                    XtVaSetValues (Undo_button,
                                        XmNsensitive,   True,
                                        NULL);
                                    XtVaSetValues (Update_button,
                                        XmNsensitive,   False,
                                        NULL);
                                    XtVaSetValues (Restore_button,
                                        XmNsensitive,   False,
                                        NULL);

/*                              If the File window is open sensitize    *
 *                              the Save button.                        */

                                    if (Open_dialog != (Widget) NULL) {

                                        if (Unlocked_loca == HCI_YES_FLAG) {

                                            XtVaSetValues (Save_file_button,
                                                XmNsensitive,   True,
                                                NULL);

                                        }
                                    }
                                }

                                Update_flag = True;

                            }
                        }

                        break;

/*                  If we are changing the stop azimuth, only update    *
 *                  it if is changed.                                   */

                    case HCI_CCZ_STOP_AZIMUTH:

                        if (value == (int) Edit [Cut [i].cut].start_azimuth) {

                            sprintf (Buff,"You entered an invalid value of %d.\nIt must be different from the other\nsector azimuth.", value);

                            err_flag = 1;

                        } else {

                            if (Edit [Cut [i].cut].stop_azimuth != value) {

                                Edit [Cut [i].cut].stop_azimuth = value;

                                HCI_LE_log("Azimuth %5.1f selected", value);

/*                          If this is the first edit, then we need to  *
 *                          set the update flag, sensitize the Undo     *
 *                          button and desensitize the Restore and      *
 *                          Update buttons.                             */

                                if (!Update_flag) {

                                    XtVaSetValues (Undo_button,
                                        XmNsensitive,   True,
                                        NULL);
                                    XtVaSetValues (Update_button,
                                        XmNsensitive,   False,
                                        NULL);
                                    XtVaSetValues (Restore_button,
                                        XmNsensitive,   False,
                                        NULL);

/*                              If the File window is open sensitize    *
 *                              the Save button.                        */

                                    if (Open_dialog != (Widget) NULL) {

                                        if (Unlocked_loca == HCI_YES_FLAG) {

                                            XtVaSetValues (Save_file_button,
                                                XmNsensitive,   True,
                                                NULL);

                                        }
                                    }
                                }

                                Update_flag = True;

                            }
                        }

                        break;

                }

/*              Rebuild the composite clutter maps and redisplay        *
 *              them.                                                   */

                if (!err_flag) {

                    hci_build_clutter_map (&Clutter_map[0][0],
                        Edit,
                        Edit_segment,
                        Edit_regions);

                    hci_display_clutter_map ();
                    hci_refresh_censor_zone_sectors ();

/*              Call the expose callback to make them visible.          */

                    hci_clutter_expose_callback ((Widget) NULL,
                        (XtPointer) NULL,
                        (XtPointer) NULL);

                }
            }

/*      A blank entry was detected so reset the text widget to its      *
 *      previous state.                                                 */

        } else {

            switch ((int) client_data) {

                case HCI_CCZ_START_AZIMUTH:

                    value = Edit [Cut [i].cut].start_azimuth;
                    break;

                case HCI_CCZ_STOP_AZIMUTH:

                    value = Edit [Cut [i].cut].stop_azimuth;
                    break;

            }

            sprintf (buf,"%3d ",value);
            XmTextSetString (w, buf);

        }

        if (err_flag) {

/*          The widget client data determines if the azimuth is *
 *          a start or stop value.                                      */

            switch ((int) client_data) {

                case HCI_CCZ_START_AZIMUTH:

                    value = Edit [Cut [i].cut].start_azimuth;
                    break;

                case HCI_CCZ_STOP_AZIMUTH:

                    value = Edit [Cut [i].cut].stop_azimuth;
                    break;

            }

            sprintf (buf,"%3d ",value);
            XmTextSetString (w, buf);
            hci_warning_popup (Top_widget, Buff,NULL);

        }

        XtFree (text);
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

void
hci_change_ccz_range_callback (
Widget          w,
XtPointer       client_data,
XtPointer       call_data
)
{
        XtPointer       data;
        char            *text;
        int             num;
        int             tmp;
        int             i;
        char            buf [16];
        int             err_flag;

/*      The table row number is passed via user data.  Get it.  */

        XtVaGetValues (w,
                XmNuserData,    &data,
                NULL);

        i = (int) data;

/*      If an invalid row number is detected, do nothing.       */

        if (i < 0) {

            return;

        }

/*      Get the range value from the widget text.               */

        text = XmTextGetString (w);

/*      If a value found, verify that it falls within the allowed       *
 *      range.                                                          */

        err_flag = 0;

        if (strlen (text)) {

            sscanf (text,"%d",&num);

            tmp = (int) (num / Units_factor + 0.5);

/*          If an ivalid range was entered, then we need to inform      *
 *          the user and reset the value to its previous value.         */

            if ((tmp > HCI_CCZ_MAX_RANGE) ||
                (tmp < HCI_CCZ_MIN_RANGE)) {

                sprintf (Buff,"You entered an invalid value of %d.\nThe valid range is %d to %d.",
                        num,
                        (int) (HCI_CCZ_MIN_RANGE*Units_factor+0.5),
                        (int) (HCI_CCZ_MAX_RANGE*Units_factor+0.5));

                err_flag = 1;

            } else if ((((int) client_data == HCI_CCZ_START_RANGE) &&
                        (tmp >= Edit [Cut [i].cut].stop_range)) ||
                       (((int) client_data == HCI_CCZ_STOP_RANGE) &&
                        (tmp <= Edit [Cut [i].cut].start_range))) {

                err_flag = 1;

                if ((int) client_data == HCI_CCZ_START_RANGE) {

                    sprintf (Buff,"You entered an invalid start range (%d).\nThe start range must be less than the\nstop range (%d).", num,
                        (int) (Edit [Cut [i].cut].stop_range*Units_factor+0.5));

                } else {

                    sprintf (Buff,"You entered an invalid stop range (%d).\nThe stop range must be greater than the\nstart range (%d).", num,
                        (int) (Edit [Cut [i].cut].start_range*Units_factor+0.5));

                }

/*          A valid range was entered, so accept it.            */

            } else {

/*              Now set the appropriate entry in the clutter censor     *
 *              zone data.                                              */

                switch ((int) client_data) {

/*                  If we are changing the start range, only update     *
 *                  it if is changed.                                   */

                    case HCI_CCZ_START_RANGE:

                        if (Edit [ Cut [i].cut].start_range != tmp) {

                            Edit [ Cut [i].cut].start_range = tmp;

                            HCI_LE_log("Range %d selected", tmp);

/*                          If this is the first edit, then we need to  *
 *                          set the update flag, sensitize the Undo     *
 *                          button and desensitize the Restore and      *
 *                          Update buttons.                             */

                            if (!Update_flag) {

                                XtVaSetValues (Undo_button,
                                        XmNsensitive,   True,
                                        NULL);
                                XtVaSetValues (Update_button,
                                        XmNsensitive,   False,
                                        NULL);
                                XtVaSetValues (Restore_button,
                                        XmNsensitive,   False,
                                        NULL);

/*                              If the File window is open sensitize    *
 *                              the Save button.                        */

                                if (Open_dialog != (Widget) NULL) {

                                    if (Unlocked_loca == HCI_YES_FLAG) {

                                        XtVaSetValues (Save_file_button,
                                                XmNsensitive,   True,
                                                NULL);

                                    }
                                }
                            }

                            Update_flag = True;

                        }

                        break;

/*                  If we are changing the stop range, only update      *
 *                  it if is changed.                                   */

                    case HCI_CCZ_STOP_RANGE:

                        if (Edit [ Cut [i].cut].stop_range != tmp) {

                            Edit [ Cut [i].cut].stop_range = tmp;

                            HCI_LE_log("Range %d selected", tmp);

/*                          If this is the first edit, then we need to  *
 *                          set the update flag, sensitize the Undo     *
 *                          button and desensitize the Restore and      *
 *                          Update buttons.                             */

                            if (!Update_flag) {

                                XtVaSetValues (Undo_button,
                                        XmNsensitive,   True,
                                        NULL);
                                XtVaSetValues (Update_button,
                                        XmNsensitive,   False,
                                        NULL);
                                XtVaSetValues (Restore_button,
                                        XmNsensitive,   False,
                                        NULL);

/*                              If the File window is open sensitize    *
 *                              the Save button.                        */

                                if (Open_dialog != (Widget) NULL) {

                                    if (Unlocked_loca == HCI_YES_FLAG) {

                                        XtVaSetValues (Save_file_button,
                                                XmNsensitive,   True,
                                                NULL);

                                    }
                                }
                            }

                            Update_flag = True;

                        }

                        break;

                }

/*              Rebuild the composite clutter maps and redisplay        *
 *              them.                                                   */

                hci_build_clutter_map (&Clutter_map[0][0],
                        Edit,
                        Edit_segment,
                        Edit_regions);

                hci_display_clutter_map ();
                hci_refresh_censor_zone_sectors ();

/*              Call the expose callback to make them visible.          */

                hci_clutter_expose_callback ((Widget) NULL,
                        (XtPointer) NULL,
                        (XtPointer) NULL);

            }

/*      A blank entry was detected so reset the text widget to its      *
 *      previous state.                                                 */

        } else {

            switch ((int) client_data) {

                case HCI_CCZ_START_RANGE:

                    num = Edit [Cut [i].cut].start_range;
                    break;

                case HCI_CCZ_STOP_RANGE:

                    num = Edit [Cut [i].cut].stop_range;
                    break;

            }

            sprintf (buf,"%d ",(int) (num*Units_factor+0.5));
            XmTextSetString (w, buf);

        }

        if (err_flag) {

/*          The widget client data determines if the range is           *
 *          a start or stop value.                                      */

            switch ((int) client_data) {

                case HCI_CCZ_START_RANGE:

                    num = Edit [Cut [i].cut].start_range;
                    break;

                case HCI_CCZ_STOP_RANGE:

                    num = Edit [Cut [i].cut].stop_range;
                    break;

            }

            sprintf (buf,"%d ",(int) (num*Units_factor+0.5));
            XmTextSetString (w, buf);
            hci_warning_popup (Top_widget, Buff,NULL);

        }

        XtFree (text);
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

void
hci_clutter_reload_callback (
Widget          w,
XtPointer       client_data,
XtPointer       call_data
)
{
        int     status;
        int     i;
        int     month, day, year;
        int     hour, minute, second;
        time_t  tm;

        HCI_LE_log("Reload selected");

/*      Create a progress meter window if low bandwidth.        */

        HCI_PM("Loading clutter data");

/*      Read the clutter regions file message.                  */

        status = hci_read_clutter_regions_file ();

/*      Get the number of regions defined for the current file. */

        Edit_regions = hci_get_clutter_region_regions (Clutter_file);

/*      Update the edit buffer with data for the current file.  */

        for (i=0;i<MAX_NUMBER_CLUTTER_ZONES;i++) {

            Edit [i].start_azimuth = (float) hci_get_clutter_region_data (
                                                Clutter_file,
                                                i,
                                                HCI_CCZ_START_AZIMUTH);
            Edit [i].stop_azimuth  = (float) hci_get_clutter_region_data (
                                                Clutter_file,
                                                i,
                                                HCI_CCZ_STOP_AZIMUTH);
            Edit [i].start_range   = (float) hci_get_clutter_region_data (
                                                Clutter_file,
                                                i,
                                                HCI_CCZ_START_RANGE);
            Edit [i].stop_range    = (float) hci_get_clutter_region_data (
                                                Clutter_file,
                                                i,
                                                HCI_CCZ_STOP_RANGE);
            Edit [i].segment       = hci_get_clutter_region_data (
                                                Clutter_file,
                                                i,
                                                HCI_CCZ_SEGMENT);
            Edit [i].select_code   = hci_get_clutter_region_data (
                                                Clutter_file,
                                                i,
                                                HCI_CCZ_SELECT_CODE);
            Edit [i].dopl          = hci_get_clutter_region_data (
                                                Clutter_file,
                                                i,
                                                HCI_CCZ_DOPPL_LEVEL);
            Edit [i].surv          = hci_get_clutter_region_data (
                                                Clutter_file,
                                                i,
                                                HCI_CCZ_SURV_LEVEL);

        }

/*      Get the update time stamp for the current file and build a      *
 *      label to be displayed as the new window title.                  */

        tm = hci_get_clutter_region_file_time (Clutter_file);

        if (tm < 1) {

            sprintf (Buff,"Clutter Regions - File:%s %s: Last Modified: Unknown ",
                hci_get_clutter_region_file_label (Clutter_file), Lbw_title);

        } else {

            unix_time (&tm,
                   &year,
                   &month,
                   &day,
                   &hour,
                   &minute,
                   &second);

            sprintf (Buff,"Clutter Regions - File:%s %s: Last Modified %2.2d/%2.2d/%4.4d  %2.2d:%2.2d:%2.2d",
                hci_get_clutter_region_file_label (Clutter_file), Lbw_title, 
                month, day, year, hour, minute, second);

        }

/*      Add redundancy information if site FAA redundant        */

        if (HCI_get_system() == HCI_FAA_SYSTEM) {

            sprintf (Buff,"%s - (FAA:%d)", Buff,
                ORPGRED_channel_num (ORPGRED_MY_CHANNEL));

        }

/*      Update the window title with the new label.             */

        XtVaSetValues (Top_widget,
                XmNtitle, Buff,
                NULL);

/*      Rebuild the composite clutter maps and redisplay them.  */

        hci_build_clutter_map (&Clutter_map[0][0],
                        Edit,
                        Edit_segment,
                        Edit_regions);

        hci_display_clutter_map ();

/*      Redisplay the clutter sector overlays in the display area.      */

        hci_refresh_censor_zone_sectors ();

/*      Update the clutter regions table widgets.               */

        hci_update_clutter_sector_widgets ();

/*      Call the expose callback to make them visible.          */

        hci_clutter_expose_callback ((Widget) NULL,
                        (XtPointer) NULL,
                        (XtPointer) NULL);

/*      If the File window is open update the files list.       */

        if (Open_dialog != (Widget) NULL) {

            hci_update_file_list ();

        }

/*      Desensitize the Undo button and sensitize the Update and        *
 *      Restore buttons (we assume that the window is unlocked).        */

        XtVaSetValues (Undo_button,
                XmNsensitive,   False,
                NULL);

        if (Unlocked_loca == HCI_YES_FLAG) {

            XtVaSetValues (Update_button,
                XmNsensitive,   True,
                NULL);
            XtVaSetValues (Restore_button,
                XmNsensitive,   True,
                NULL);

        }

/*      Unset the update flag.                                          */

        Update_flag = 0;

/*      If the File window is open desensitize the Save button.         */

        if (Open_dialog != (Widget) NULL) {

            if (Unlocked_loca == HCI_YES_FLAG) {

                XtVaSetValues (Save_file_button,
                        XmNsensitive,   False,
                        NULL);

            }
        }

        if (New_flag) {

                hci_clutter_file_new_callback ((Widget) NULL,
                        (XtPointer) NULL, (XtPointer) NULL);

        }
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

void
hci_clutter_file_save_callback (
Widget          w,
XtPointer       client_data,
XtPointer       call_data
)
{
        char buf[HCI_BUF_128];

/*      There are two ways this function was activated.  Build a        *
 *      verification message based on the method.                       */

        if (New_file == Clutter_file) {

/*          The Save button was selected.       */

            if (Unlocked_loca == HCI_NO_FLAG) {

                sprintf( buf, "You made edits but did not save them.\nDo you want to save them?" );

            } else {

                sprintf( buf, "Are you sure you want to save your clutter\nregions changes?" );

            }

        } else {

/*          The user selected a different file  */

            sprintf( buf, "You are changing clutter files but did\nnot save edits to the previous file.\nDo you want to save them?" );

        }

        hci_confirm_popup( Top_widget, buf, hci_clutter_file_save_yes_callback, hci_clutter_file_save_no_callback );

}

/************************************************************************
 *      Description: This function is activated when the "No" button    *
 *                   is selected from the save verification popup.      *
 *                                                                      *
 *      Input:  w - ID of No button selected; unused                    *
 *              client_data - unused                                    *
 *              call_data - unused                                      *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

void
hci_clutter_file_save_no_callback (
Widget          w,
XtPointer       client_data,
XtPointer       call_data
)
{

/*      If the close flag is set then exit the task.                    */

        if (Close_flag) {

            HCI_task_exit (HCI_EXIT_SUCCESS);

/*      The close flag is not set.                                      */

        } else {

/*      If cancelling out of a new file operation we need to undo what  *
 *      was done and reset the active file to the Default one.  If the  *
 *      user wants to create another new file then they need to select  *
 *      the New button again.                                           */

            if (New_flag || Unlocked_loca == HCI_NO_FLAG) {

                if (Unlocked_loca == HCI_YES_FLAG) {

                    New_flag     = 0;
                    Clutter_file = 0;

                }

                hci_clutter_reload_callback ((Widget) NULL,
                         (XtPointer) NULL,
                         (XtPointer) NULL);

/*              Desensitize the Save button in case the window was      *
 *              previously openned.                                     */

                if (Open_dialog != (Widget) NULL) {

                    if (Unlocked_loca == HCI_YES_FLAG) {

                        XtVaSetValues (Save_file_button,
                                XmNsensitive,   False,
                                NULL);

                    }
                }

            } else {


/*          If a new file was selected, then we must discard any        *
 *          unsaved edits for the previous file.                        */

                if (New_file != Clutter_file) {

                    Clutter_file = New_file;

/*                  Reset the file pointer and reload new data.         */

                    hci_clutter_reload_callback ((Widget) NULL,
                                         (XtPointer) NULL,
                                         (XtPointer) NULL);

                }
            }
        }
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

void
clutter_censor_zones_input (
Widget          w,
XEvent          *event,
String          *args,
int             *num_args
)
{
        static float    first_azimuth; 
        static float    first_range;
        static float    second_azimuth;
        static float    second_range;
        static int      first_pixel;
        static int      first_scanl;
        static int      second_pixel;
        static int      second_scanl;
        static int      button_down = 0;
        float           azimuth;
        float           range;
        float           temp;
        float           x;
        float           y;
        XRectangle      clip_rectangle;
        int             pixel;
        int             scanl;

        pixel = event->xbutton.x;
        scanl = event->xbutton.y;

/*	If mouse event occurs outside clutter map, ignore it.           */

	if( pixel > Sector_width || scanl > Sector_height )
	{
	  return;
	}

/*      Compute the X and Y coordinate of the cursor relative to the    *
 *      radar.                                                          */

        x = Center_x_offset + 
                (Center_pixel - pixel) / (Clutter_zoom * Scale_x);
        y = Center_y_offset +
                (Center_scanl - scanl) / (Clutter_zoom * Scale_y);

/*      Convert the XY coordinate to polar coordinates.                 */

        azimuth = hci_find_azimuth (
                        (int) -(x*100),
                        (int) (y*100),
                        0,
                        0);

        range  = sqrt ((double)(x*x + y*y));

/*      If the cursor is outside the display area then restrict the     *
 *      pixel coordinate to the left edge of the area.  We do not have  *
 *      to worry about the scanline coordinate since the area extends   *
 *      vertically to cover the entire drawing area for the widget.  We *
 *      do have to worry about the right edgae of the area since the    *
 *      drawing area also includes the composite clutter map.  We could *
 *      have avoided this check if we had used separate drawing area    *
 *      widgets for the display area and the composite clutter map.     */

        if (pixel > Sector_width) {

            pixel = Sector_width;

        }

/*      If in zoom mode, increase the magnification for a left click    *
 *      and decrease the magnification for a right click.  Recenter     *
 *      on a middle button click.                                       */

        if (Clutter_mode == HCI_CCZ_ZOOM_MODE) {

            if (!strcmp (args[0], "up1")) {     /* Left mouse click */

                HCI_LE_log("Zooming window");

                if (Clutter_zoom < HCI_CCZ_MAX_ZOOM) {

                    if (range <= HCI_CCZ_MAX_RANGE) {

                        Center_x_offset = (Center_pixel - pixel) /
                                          (Scale_x * Clutter_zoom) +
                                           Center_x_offset;

                        Center_y_offset = (Center_scanl - scanl) /
                                          (Scale_y * Clutter_zoom) +
                                           Center_y_offset;

                    }

                    Clutter_zoom = Clutter_zoom*2;

                    hci_refresh_censor_zone_sectors ();

                    hci_clutter_expose_callback ((Widget) NULL,
                            (XtPointer) NULL,
                            (XtPointer) NULL);

                }

            } else if (!strcmp (args[0], "up2")) { /* Middle mouse click */

                HCI_LE_log("Recentering window");

                if (range <= HCI_CCZ_MAX_RANGE) {

                    Center_x_offset = (Center_pixel - pixel) /
                                      (Scale_x * Clutter_zoom) +
                                       Center_x_offset;

                    Center_y_offset = (Center_scanl - scanl) /
                                      (Scale_y * Clutter_zoom) +
                                       Center_y_offset;

                }

                hci_refresh_censor_zone_sectors ();

                hci_clutter_expose_callback ((Widget) NULL,
                            (XtPointer) NULL,
                            (XtPointer) NULL);

            } else if (!strcmp (args[0], "up3")) { /* Right mouse click */

                HCI_LE_log("Unzooming window");

                if (Clutter_zoom > 1) {

                    if (range <= HCI_CCZ_MAX_RANGE) {

                        Center_x_offset = (Center_pixel - pixel) /
                                          (Scale_x * Clutter_zoom) +
                                           Center_x_offset;

                        Center_y_offset = (Center_scanl - scanl) /
                                          (Scale_y * Clutter_zoom) +
                                           Center_y_offset;

                    }

                    Clutter_zoom = Clutter_zoom/2;

                    hci_refresh_censor_zone_sectors ();

                    hci_clutter_expose_callback ((Widget) NULL,
                            (XtPointer) NULL,
                            (XtPointer) NULL);

                }
            }

/*      else, in sector mode                                            */

        } else {

/*          If the left mouse button is pressed, start a new sector     *
 *          definition.                                                 */

            if ((!strcmp (args[0], "down1")) &&
                ((int) range  < HCI_CCZ_MAX_RANGE)) {

                if (Edit_regions > MAX_NUMBER_CLUTTER_ZONES) {

                    hci_warning_popup (Top_widget,
                        "The maximum number of regions has been reached.\nYou must first delete an existing region before\nadding a new one.", NULL);
                    return;

                }

                HCI_LE_log("Stating new sector");

                first_pixel = pixel;
                first_scanl = scanl;

                button_down = 1;

                first_azimuth = azimuth;
                first_range   = range;

                second_azimuth = first_azimuth;
                second_range   = first_range;

/*          If the left button is released, then this defines the       *
 *          end of the new sector definition.                           */

            } else if (!strcmp (args[0], "up1") && (button_down == 1)) {

                HCI_LE_log("Ending new sector");
                button_down = 0;

                second_pixel = pixel;
                second_scanl = scanl;

                second_azimuth = azimuth;

/*              Don't allow the range to exceed the maximm.  If it      *
 *              does, set it to the maximum.                            */

                if ((int) range > HCI_CCZ_MAX_RANGE) {

                    range = HCI_CCZ_MAX_RANGE;

                }

                second_range   = range;

/*              If either the range or azimuth boundaries are   *
 *              same then this is not a valid region.           */

                if (((int) first_range   == (int) second_range) ||
                    ((int) first_azimuth == (int) second_azimuth)) {

                    XCopyArea (Clutter_display,
                            Clutter_pixmap,
                            Clutter_window,
                            Clutter_gc,
                            0, 0,
                            Clutter_width,
                            Clutter_height,
                            0, 0);

                    return;

                }

/*              If this is the first edit during this session,  *
 *              sensitize the various control buttons.          */

                if (!Update_flag) {

                    XtVaSetValues (Undo_button,
                        XmNsensitive,   True,
                        NULL);
                    XtVaSetValues (Update_button,
                        XmNsensitive,   False,
                        NULL);
                    XtVaSetValues (Restore_button,
                        XmNsensitive,   False,
                        NULL);

                    if (Open_dialog != (Widget) NULL) {

                        if (Unlocked_loca == HCI_YES_FLAG) {

                            XtVaSetValues (Save_file_button,
                                XmNsensitive,   True,
                                NULL);

                        }
                    }
                }

                Update_flag = True;

/*              If we have reached the maximum number of allowed        *
 *              ignore the new region.                          */

                if (Edit_regions > MAX_NUMBER_CLUTTER_ZONES) {

                    return;

                } else {

/*              A new region is defined.  Update the internal   *
 *              data table to include the new sector definition */

/*              NOTE: The internal data structure uses floats   *
 *              for the azimuth and range data.  However, when  *
 *              the data are saved they are converted to        *
 *              integer.  So, round the values to the nearest   *
 *              whole number.                                   */

                    Edit [Edit_regions].start_azimuth =
                                (float) ((int) (first_azimuth+0.5));
                    Edit [Edit_regions].stop_azimuth  =
                                (float) ((int) (second_azimuth+0.5));

                    if (first_range > second_range) {

                        temp         = first_range;
                        first_range  = second_range;
                        second_range = temp;

                    }

                    if (first_range < 2) {

                        first_range = 2;

                    } else if (first_range > HCI_CCZ_MAX_RANGE) {

                        first_range = HCI_CCZ_MAX_RANGE;

                    }

                    if (second_range > HCI_CCZ_MAX_RANGE) {

                        second_range = HCI_CCZ_MAX_RANGE;

                    } else if (second_range > HCI_CCZ_MAX_RANGE) {

                        second_range = HCI_CCZ_MAX_RANGE;

                    }

                    Edit [Edit_regions].start_range =
                                (float) ((int) (first_range+0.5));
                    Edit [Edit_regions].stop_range  =
                                (float) ((int) (second_range+0.5));

                    Edit [Edit_regions].segment     = Edit_segment;
                    Edit [Edit_regions].select_code = Edit_select_code;

                    Edit [Edit_regions].surv        = Edit_surv_level;
                    Edit [Edit_regions].dopl        = Edit_dopl_level;

                    Edit_regions++;

                    hci_build_clutter_map (&Clutter_map[0][0],
                                        Edit,
                                        Edit_segment,
                                        Edit_regions);

                    hci_display_clutter_map ();
                    hci_update_clutter_sector_widgets ();

/*                  Define a clip region to include only the sector     *
 *                  region.                                             */

                    clip_rectangle.x      = 0;
                    clip_rectangle.y      = 0;
                    clip_rectangle.width  = Sector_width;
                    clip_rectangle.height = Clutter_height;

                    XSetClipRectangles (Clutter_display,
                            Clutter_gc,
                            0,
                            0,
                            &clip_rectangle,
                            1,
                            Unsorted);

                    display_censor_zone_sector (
                            Clutter_display,
                            Clutter_pixmap,
                            Clutter_gc,
                            hci_get_read_color (PRODUCT_FOREGROUND_COLOR),
                            first_azimuth,
                            second_azimuth,
                            first_range,
                            second_range,
                            Edit_select_code);

/*                  Reset the clip region.                              */

                    clip_rectangle.x      = 0;
                    clip_rectangle.y      = 0;
                    clip_rectangle.width  = Clutter_width;
                    clip_rectangle.height = Clutter_height;

                    XSetClipRectangles (Clutter_display,
                            Clutter_gc,
                            0,
                            0,
                            &clip_rectangle,
                            1,
                            Unsorted);

                    XCopyArea (Clutter_display,
                            Clutter_pixmap,
                            Clutter_window,
                            Clutter_gc,
                            0, 0,
                            Clutter_width,
                            Clutter_height,
                            0, 0);

                }

/*      If the mouse is being moved with the left button down, we       *
 *      are defining a new sector.  Update the new sector display       *
 *      as the mouse is being moved.                                    */

            } else if ((!strcmp (args[0], "move")) &&
                       (button_down)) {

                XCopyArea (Clutter_display,
                        Clutter_pixmap,
                        Clutter_window,
                        Clutter_gc,
                        0, 0,
                        Clutter_width,
                        Clutter_height,
                        0, 0);

                second_pixel = pixel;
                second_scanl = scanl;

                second_azimuth = azimuth;

                if ((int) range > HCI_CCZ_MAX_RANGE) {

                    range = HCI_CCZ_MAX_RANGE;

                }

                second_range   = range;

                if (second_azimuth <= first_azimuth)
                    second_azimuth = second_azimuth+360;

/*              Define a clip region to include only the sector *
 *              region.                                         */

                clip_rectangle.x      = 0;
                clip_rectangle.y      = 0;
                clip_rectangle.width  = Sector_width;
                clip_rectangle.height = Clutter_height;

                XSetClipRectangles (Clutter_display,
                            Clutter_gc,
                            0,
                            0,
                            &clip_rectangle,
                            1,
                            Unsorted);

                display_censor_zone_sector (
                        Clutter_display,
                        Clutter_window,
                        Clutter_gc,
                        hci_get_read_color (PRODUCT_FOREGROUND_COLOR),
                        first_azimuth,
                        second_azimuth,
                        first_range,
                        second_range,
                        Edit_select_code);

/*              Reset the clip region.                          */

                clip_rectangle.x      = 0;
                clip_rectangle.y      = 0;
                clip_rectangle.width  = Clutter_width;
                clip_rectangle.height = Clutter_height;

                XSetClipRectangles (Clutter_display,
                            Clutter_gc,
                            0,
                            0,
                            &clip_rectangle,
                            1,
                            Unsorted);
                
            }
        }

/*      Display the azran of the cursor in the upper left corner of     *
 *      the edit window.                                                */

        if (pixel == Sector_width)
                range = HCI_CCZ_MAX_RANGE;

        XSetForeground (Clutter_display,
                        Clutter_gc,
                        hci_get_read_color (BUTTON_FOREGROUND));
        XSetBackground (Clutter_display,
                        Clutter_gc,
                        hci_get_read_color (BUTTON_BACKGROUND));

        XSetFont (Clutter_display,
                  Clutter_gc,
                  hci_get_font (LIST));

        if (Units_factor == HCI_KM_TO_KM) {

            sprintf (Buff,"(%3d deg,%3d km)", (int) azimuth, (int) range);

        } else {

            sprintf (Buff,"(%3d deg,%3d nm)", (int) azimuth, (int) (range*Units_factor+0.5));

        }

        XDrawImageString (Clutter_display,
                          Clutter_window,
                          Clutter_gc,
                          5,
                          15,
                          Buff,
                          strlen (Buff));

        XSetFont (Clutter_display,
                  Clutter_gc,
                  hci_get_font (SMALL));
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

void
hci_select_mode_callback (
Widget          w,
XtPointer       client_data,
XtPointer       call_data
)
{
        XmToggleButtonCallbackStruct    *state =
                (XmToggleButtonCallbackStruct *) call_data;

/*      If the radio button is set, change the current mode.  The mode  *
 *      data is passed as client data.                                  */

        if (state->set) {

            Clutter_mode = (int) client_data;

            HCI_LE_log("Clutter Mode %d selected", Clutter_mode);

        }
}

/*      The following function is called when one of the Units radio    *
 *      buttons is selected.  The units can be NM or KM.                */

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

void
hci_select_units_callback (
Widget          w,
XtPointer       client_data,
XtPointer       call_data
)
{
        XmToggleButtonCallbackStruct    *state =
                (XmToggleButtonCallbackStruct *) call_data;

/*      If the radio button is set, change the current units.  The      *
 *      units data is passed as client data.                            */

        if (state->set) {

            switch ((int) client_data) {

/*              If the new units are Kilometers, set the conversion     *
 *              factor for the clutter range data and update the range  *
 *              labels.                                                 */

                case UNITS_KM :

                    Units_factor = HCI_KM_TO_KM;

                    sprintf (Buff,"Ran1 (km)");
                    XmTextSetString (Ran1_text,Buff);
                    sprintf (Buff,"Ran2 (km)");
                    XmTextSetString (Ran2_text,Buff);

                    break;

/*              If the new units are Nautical Miles, set the conversion *
 *              factor for the clutter range data and update the range  *
 *              labels.                                                 */

                case UNITS_NM :

                    Units_factor = HCI_KM_TO_NM;

                    sprintf (Buff,"Ran1 (Nm)");
                    XmTextSetString (Ran1_text,Buff);
                    sprintf (Buff,"Ran2 (Nm)");
                    XmTextSetString (Ran2_text,Buff);

                    break;

            }

/*          Update the clutter regions table so new units are applied   *
 *          to all range data and labels.                               */

            hci_update_clutter_sector_widgets ();

        }
}

/************************************************************************
 *      Description: This function is the called when the user selects  *
 *                   one of the Segment radio buttons.                  *
 *                                                                      *
 *      Input:  w - select radio button ID                              *
 *              client_data - HCI_CCZ_SEGMENT_HIGH or                   *
 *                            HCI_CCZ_SEGMENT_LOW                       *
 *              call_data - radio button state data                     *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

void
hci_select_segment_callback (
Widget          w,
XtPointer       client_data,
XtPointer       call_data
)
{
        int     i;
        int     cut;
        char    buf [32];
        XmString        str;

        XmToggleButtonCallbackStruct    *state =
                (XmToggleButtonCallbackStruct *) call_data;

/*      If the radio button is set, change the current segment number   *
 *      using data passed as client data.                               */

        if (state->set) {

            HCI_LE_log("Clutter Segment %d selected", Edit_segment);

/*          Update the Segmnent label widget.                           */

            switch ((int) client_data) {

                case HCI_CCZ_SEGMENT_LOW :

                    sprintf (buf,"Low Elevation Segment");
                    str = XmStringCreateLocalized (buf);

                    XtVaSetValues (Segment_label,
                        XmNlabelString, str,
                        NULL);

                    XmStringFree (str);
                    break;

                case HCI_CCZ_SEGMENT_HIGH :

                    sprintf (buf,"High Elevation Segment");
                    str = XmStringCreateLocalized (buf);

                    XtVaSetValues (Segment_label,
                        XmNlabelString, str,
                        NULL);

                    XmStringFree (str);
                    break;

            }

/*          Update the current segment and rebuild Cut table based on   *
 *          new segment.                                                */

            Edit_segment = (int) client_data;

            cut = 0;

            for (i=0;i<Edit_regions;i++) {

                if (Edit_segment == Edit [i].segment) {

                    Cut [cut].cut = i;
                    cut++;

                }
            }

/*          Rebuild the composite clutter maps and display them.        */

            hci_build_clutter_map (&Clutter_map[0][0],
                        Edit,
                        Edit_segment,
                        Edit_regions);

            hci_display_clutter_map ();

/*          If low bandwidth, popup a progress meter before reading     *
 *          new background product data.                                */

            HCI_PM("Updating background product data");

            hci_load_clutter_background_product (Clutter_background);
            
/*          Refresh the sector overlay data in the dispay area.         */

            hci_refresh_censor_zone_sectors ();

/*          Update the clutter regions table.                           */

            hci_update_clutter_sector_widgets ();

/*          Call the expose function to make the window updates         *
 *          visible.                                                    */

            hci_clutter_expose_callback ((Widget) NULL,
                        (XtPointer) NULL,
                        (XtPointer) NULL);

        }
}

/*      The following function displays a sector overlay in the *
 *      clutter clutter display area.                           */

/************************************************************************
 *      Description: This function displays a sector overlay in the     *
 *                   clutter regions display area.                      *
 *                                                                      *
 *      Input:  display - Display info                                  *
 *              window  - Drawable                                      *
 *              gc      - Graphics Context                              *
 *              color   - overlay color index                           *
 *              first_azimuth  - first sector azimuth (deg)             *
 *              second_azimuth - second sector azimuth (deg)            *
 *              first_range    - first sector range (deg)               *
 *              second_range   - second sector range (deg)              *
 *              select_code    - operator select code                   *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

void
display_censor_zone_sector (
Display         *display,       /* Display property of window.  */
Drawable        window,         /* Drawable of window.          */
GC              gc,             /* Graphics Context to use.     */
Pixel           color,          /* Color for overlay.           */
float           first_azimuth,  /* First sector azimuth (deg).  */
float           second_azimuth, /* Second sector azimuth (deg). */
float           first_range,    /* First sector range.          */
float           second_range,   /* Second sector range.         */
int             select_code     /* Select code of sector.       */
)
{
        float   angle;
        XPoint  x [2000];
        int     i;
        int     first_arc;

        XSetForeground (display,
                        gc,
                        color);

/*      We draw the sector in a clockwise manner so add 360 to the      *
 *      second azimuth angle if it is less than the first.              */

        if (second_azimuth < first_azimuth) {

            second_azimuth = second_azimuth + 360.0;

        }

        i = 0;

/*      In half degree increments, calculate the end points (in         *
 *      cartesian space) for a set of line segments connecting the      *
 *      two range values across radials.                                */

        for (angle=first_azimuth;angle<second_azimuth;angle=angle+0.5) {

            x[i].x = Center_pixel +
                        (first_range * cos ((double) (angle-90.0)*HCI_DEG_TO_RAD) +
                        Center_x_offset) * (Clutter_zoom * Scale_x);
            x[i].y = Center_scanl +
                        (first_range * sin ((double) (angle+90.0)*HCI_DEG_TO_RAD) +
                        Center_y_offset) * (Clutter_zoom * Scale_y);
            i++;

        }

        first_arc = i;

/*      Reverse the process to generate a second set of lines.          */

        x[i].x = Center_pixel +
                    (first_range * cos ((double) (second_azimuth-90.0)*HCI_DEG_TO_RAD) +
                    Center_x_offset) * (Clutter_zoom * Scale_x);
        x[i].y = Center_scanl +
                    (first_range * sin ((double) (second_azimuth+90.0)*HCI_DEG_TO_RAD) +
                    Center_y_offset) * (Clutter_zoom * Scale_y);
        i++;

        for (angle=second_azimuth;angle>first_azimuth;angle=angle-0.5) {

            x[i].x = Center_pixel +
                        (second_range * cos ((double) (angle-90.0)*HCI_DEG_TO_RAD) +
                        Center_x_offset) * (Clutter_zoom * Scale_x);
            x[i].y = Center_scanl +
                        (second_range * sin ((double) (angle+90.0)*HCI_DEG_TO_RAD) +
                        Center_y_offset) * (Clutter_zoom * Scale_y);
            i++;

        }

        x[i].x = x[0].x;
        x[i].y = x[0].y;
        i++;

/*      Display the sector by drawing both sets of lines.       */

        angle = fabs ((double)(second_azimuth-first_azimuth));

        if ((angle <   0.5) ||
            (angle > 359.5)) {

            XDrawLines (display,
                        window,
                        gc,
                        &x[0],
                        first_arc,
                        CoordModeOrigin);

            XDrawLines (display,
                        window,
                        gc,
                        &x[first_arc+1],
                        i-first_arc-2,
                        CoordModeOrigin);

        } else {

            XDrawLines (display,
                        window,
                        gc,
                        x,
                        i,
                        CoordModeOrigin);

        }
}

/************************************************************************
 *      Description: This function displays a background product and    *
 *                   sector overlays in the clutter regions display     *
 *                   area.                                              *
 *                                                                      *
 *      Input:  NONE                                                    *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

void
hci_refresh_censor_zone_sectors ()
{
        int     i;
        XRectangle      clip_rectangle;

/*      Define a clip rectangle around the clutter display area so      *
 *      we don't get any entraneous data displayed outside of it.       */

        clip_rectangle.x      = 0;
        clip_rectangle.y      = 0;
        clip_rectangle.width  = Sector_width+90;
        clip_rectangle.height = Sector_height;

        XSetClipRectangles (Clutter_display,
                            Clutter_gc,
                            0,
                            0,
                            &clip_rectangle,
                            1,
                            Unsorted);

        XSetForeground (Clutter_display,
                        Clutter_gc,
                        hci_get_read_color (PRODUCT_BACKGROUND_COLOR));

        XFillRectangle (Clutter_display,
                        Clutter_pixmap,
                        Clutter_gc,
                        0, 0,
                        Sector_width,
                        Sector_height);

/*      If a background product is open, display it.            */

        if (Background_display_flag > 0) {

            if ((hci_product_type () == RADIAL_TYPE) ||
                (hci_product_type () == DHR_TYPE)) {

                hci_display_radial_product (
                        Clutter_display,
                        Clutter_pixmap,
                        Clutter_gc,
                        0,
                        0,
                        Sector_width,
                        Sector_height,
                        HCI_CCZ_MAX_RANGE,
                        Center_x_offset,
                        Center_y_offset,
                        Clutter_zoom);

/*              Temporarily set the line width to 1 pixel so our        *
 *              color bar borders aren't wide.                          */

                XSetLineAttributes (Clutter_display,
                        Clutter_gc,
                        1,
                        LineSolid,
                        CapButt,
                        JoinMiter);

/*              Display the color bar.                          */

                hci_display_color_bar (Clutter_display,
                        Clutter_pixmap,
                        Clutter_gc,
                        Sector_width,
                        0,
                        (Sector_height),
                        COLOR_BAR_WIDTH,
                        10,
                        0);

/*              Reset the line width so sector overlays will use        *
 *              wide lines.                                             */

                XSetLineAttributes (Clutter_display,
                        Clutter_gc,
                        2,
                        LineSolid,
                        CapButt,
                        JoinMiter);

            }

/*      No background product is open so display an informative label   *
 *      instead.                                                        */

        } else {

            XSetForeground (Clutter_display,
                            Clutter_gc,
                            hci_get_read_color (BACKGROUND_COLOR1));

            XFillRectangle (Clutter_display,
                            Clutter_pixmap,
                            Clutter_gc,
                            Sector_width,
                            0,
                            90,
                            Sector_height);

            XSetForeground (Clutter_display,
                            Clutter_gc,
                            hci_get_read_color (BLACK));

            if (Background_display_flag == HCI_CCZ_DATA_TIMED_OUT) {

                sprintf (Buff,"Background Product Data Expired");

            } else {

                sprintf (Buff,"Background Product Data Not Available");

            }

            XSetFont (Clutter_display,
                      Clutter_gc,
                      hci_get_font (LARGE));

            XSetForeground (Clutter_display,
                            Clutter_gc,
                            hci_get_read_color (PRODUCT_FOREGROUND_COLOR));

            XDrawString (Clutter_display,
                         Clutter_pixmap,
                         Clutter_gc,
                         (int) (Sector_width/2 - 150),
                         (int) (Sector_height/2 - 50),
                         Buff,
                         strlen(Buff));

            XSetFont (Clutter_display,
                      Clutter_gc,
                      hci_get_font (SMALL));

        }

/*      Reset the clip rectangle so we can draw elsewhere (i.e., the    *
 *      composite clutter maps.                                         */

        clip_rectangle.x      = 0;
        clip_rectangle.y      = 0;
        clip_rectangle.width  = Sector_width;
        clip_rectangle.height = Clutter_height;

        XSetClipRectangles (Clutter_display,
                            Clutter_gc,
                            0,
                            0,
                            &clip_rectangle,
                            1,
                            Unsorted);

/*      Overlay sector lines for all defined regions in segment.        */

        for (i=0;i<Edit_regions;i++) {

            if (Edit_segment == Edit [i].segment) {

                display_censor_zone_sector (
                        Clutter_display,
                        Clutter_pixmap,
                        Clutter_gc,
                        hci_get_read_color (PRODUCT_FOREGROUND_COLOR),
                        Edit [i].start_azimuth,
                        Edit [i].stop_azimuth,
                        Edit [i].start_range,
                        Edit [i].stop_range,
                        Edit_select_code);

           }
        }

/*      Reset the clip rectangle so we can draw elsewhere (i.e., the    *
 *      composite clutter maps.                                         */

        clip_rectangle.x      = 0;
        clip_rectangle.y      = 0;
        clip_rectangle.width  = Clutter_width;
        clip_rectangle.height = Clutter_height;

        XSetClipRectangles (Clutter_display,
                            Clutter_gc,
                            0,
                            0,
                            &clip_rectangle,
                            1,
                            Unsorted);
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

void
hci_clutter_file_new_callback (
Widget          w,
XtPointer       client_data,
XtPointer       call_data
)
{
        int     i;
        char    *file_label;

        HCI_LE_log("New File selected");

/*      Go through the clutter regions data buffer and locate the first *
 *      undefined file entry (label will be undefined).                 */

        for (i=0;i<MAX_CLTR_FILES;i++) {

            file_label = hci_get_clutter_region_file_label (i);

            if (strlen (file_label) == 0) {

/*              Label for buffer entry undefined so initialize the new  *
 *              file.                                                   */

                New_flag     = 1;
                Clutter_file = i;
                Edit_regions = 2;
/*              Define the region for segment 1.                        */

                Edit [0].start_azimuth = 0;
                Edit [0].stop_azimuth = 360;
                Edit [0].start_range = 2;
                Edit [0].stop_range = HCI_CCZ_MAX_RANGE;
                Edit [0].select_code = HCI_CCZ_FILTER_BYPASS;
                Edit [0].segment = HCI_CCZ_SEGMENT_LOW;
                Edit [0].dopl = HIGH+1;
                Edit [0].surv = MEDIUM+1;

/*              Define the region for segment 2.                        */

                Edit [1].start_azimuth = 0;
                Edit [1].stop_azimuth = 360;
                Edit [1].start_range = 2;
                Edit [1].stop_range = HCI_CCZ_MAX_RANGE;
                Edit [1].select_code = HCI_CCZ_FILTER_BYPASS;
                Edit [1].segment = HCI_CCZ_SEGMENT_HIGH;
                Edit [1].dopl = HIGH+1;
                Edit [1].surv = MEDIUM+1;

/*              If the edit flag is not set, set it, sensitize the Undo *
 *              button, desensitize the Restore and Update buttons, and *
 *              sensitize the Save button.                              */

                if (!Update_flag) {

                    XtVaSetValues (Undo_button,
                        XmNsensitive,   True,
                        NULL);
                    XtVaSetValues (Update_button,
                        XmNsensitive,   False,
                        NULL);
                    XtVaSetValues (Restore_button,
                        XmNsensitive,   False,
                        NULL);

                    if (Open_dialog != (Widget) NULL) {

                        if (Unlocked_loca == HCI_YES_FLAG) {

                            XtVaSetValues (Save_file_button,
                                XmNsensitive,   True,
                                NULL);

                        }
                    }
                }

                Update_flag  = True;

/*              Build new composite clutter maps and display them.      */

                hci_build_clutter_map (&Clutter_map[0][0],
                        Edit,
                        Edit_segment,
                        Edit_regions);

                hci_display_clutter_map ();

/*              Redisplay data in the clutter display area.             */

                hci_refresh_censor_zone_sectors ();

/*              Update the clutter table widgets.                       */

                hci_update_clutter_sector_widgets ();

/*              Call the expose callback to make the updates visible.   */

                hci_clutter_expose_callback ((Widget) NULL,
                        (XtPointer) NULL,
                        (XtPointer) NULL);

                sprintf (Buff,"Clutter Regions - File:<> %s: Last Modified:         ", Lbw_title);

/*              Add redundancy information if site FAA redundant        */

                if (HCI_get_system() == HCI_FAA_SYSTEM) {

                    sprintf (Buff,"%s - (FAA:%d)", Buff,
                                ORPGRED_channel_num (ORPGRED_MY_CHANNEL));

                }

/*              Update the window title with the new label.             */

                XtVaSetValues (Top_widget,
                        XmNtitle, Buff,
                        NULL);

                break;

            }
        }
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

void
hci_clutter_file_save_yes_callback (
Widget          w,
XtPointer       client_data,
XtPointer       call_data
)
{
        int     i;
        int     status;
        time_t  tm;
        int     month, day, year;
        int     hour, minute, second;
        char    *file_label;

        HCI_LE_log("File Save selected");

/*      If the File window is open desensitize the Save button.         */

        if (Open_dialog != (Widget) NULL) {

            XtVaSetValues (Save_file_button,
                XmNsensitive,   False,
                NULL);
        }

/*      If a file label hasn't been defined, call the save as function  *
 *      to prompt the user for a filename before saving.                */

        file_label = hci_get_clutter_region_file_label (Clutter_file);

        if (strlen (file_label) == 0) {

            hci_clutter_file_save_as_callback (w,
                        (XtPointer) -1,
                        (XtPointer) -1);

        } else {

/*      If a file label hast been defined but the file is the default   *
 *      file, call the save as function so a new filename can be        *
 *      specified.                                                      */

            if (!strcmp (file_label,"Default")) {

                hci_clutter_file_save_as_callback (w,
                        (XtPointer) -1,
                        (XtPointer) -1);

            } else {

/*              Get the current time so we can update the update time   *
 *              field.                                                  */

                tm = time (NULL);

/*              Update the time stamp field.                            */

                hci_set_clutter_region_file_time (Clutter_file, tm);

/*              Set the clutter properties for the saved file.          */

                hci_set_clutter_region_regions (Clutter_file, Edit_regions);

                for (i=0;i<Edit_regions;i++) {

                    hci_set_clutter_region_data (Clutter_file,
                                        i,
                                        HCI_CCZ_START_AZIMUTH,
                                        Edit [i].start_azimuth);
                    hci_set_clutter_region_data (Clutter_file,
                                        i,
                                        HCI_CCZ_STOP_AZIMUTH,
                                        Edit [i].stop_azimuth);
                    hci_set_clutter_region_data (Clutter_file,
                                        i,
                                        HCI_CCZ_START_RANGE,
                                        Edit [i].start_range);
                    hci_set_clutter_region_data (Clutter_file,
                                        i,
                                        HCI_CCZ_STOP_RANGE,
                                        Edit [i].stop_range);
                    hci_set_clutter_region_data (Clutter_file,
                                        i,
                                        HCI_CCZ_SEGMENT,
                                        Edit [i].segment);
                    hci_set_clutter_region_data (Clutter_file,
                                        i,
                                        HCI_CCZ_SELECT_CODE,
                                        Edit [i].select_code);
                    hci_set_clutter_region_data (Clutter_file,
                                        i,
                                        HCI_CCZ_DOPPL_LEVEL,
                                        Edit [i].dopl);
                    hci_set_clutter_region_data (Clutter_file,
                                        i,
                                        HCI_CCZ_SURV_LEVEL,
                                        Edit [i].surv);

                }

/*              If low bandwidth, display a progress meter window.      */

                HCI_PM("Saving Clutter Regions Data");

/*              Write the updated clutter file data to the clutter LB.  */

                status = hci_write_clutter_regions_file ();

/*              If the close flag is set, exit the task.                */

                if (Close_flag) {

                    HCI_task_exit (HCI_EXIT_SUCCESS);

                } else {

/*                  If the File window is open update the files list.   */

                    if (Open_dialog != (Widget) NULL) {

                        hci_update_file_list ();

                    }
                }

/*              If this function was called because someone selected a  *
 *              new file from the files list, set the current file      *
 *              pointer to the selected file and refresh the display    *
 *              with the new file data.                                 */

                if (New_file != Clutter_file) {

                    Clutter_file = New_file;

                    hci_clutter_reload_callback ((Widget) NULL,
                                         (XtPointer) NULL,
                                         (XtPointer) NULL);

                }

/*              Update the time stamp in the main window title.         */

                unix_time (&tm,
                   &year,
                   &month,
                   &day,
                   &hour,
                   &minute,
                   &second);

                sprintf (Buff,"Clutter Regions - File:%s %s: Last Modified %2.2d/%2.2d/%4.4d  %2.2d:%2.2d:%2.2d",
                    hci_get_clutter_region_file_label (Clutter_file), Lbw_title, 
                    month, day, year, hour, minute, second);

/*              Add redundancy information if site FAA redundant        */

                if (HCI_get_system() == HCI_FAA_SYSTEM) {

                    sprintf (Buff,"%s - (FAA:%d)", Buff,
                        ORPGRED_channel_num (ORPGRED_MY_CHANNEL));

                }

                XtVaSetValues (Top_widget,
                        XmNtitle, Buff,
                        NULL);

/*              Clear the update flag, desensitize the Undo button      *
 *              and sensitize the Restore and Update buttons.           */

                Update_flag = False;

                XtVaSetValues (Undo_button,
                        XmNsensitive,   False,
                        NULL);
                XtVaSetValues (Update_button,
                        XmNsensitive,   True,
                        NULL);
                XtVaSetValues (Restore_button,
                        XmNsensitive,   True,
                        NULL);

/*              If the download flag is set we want to also download    *
 *              the saved file to the RDA.                              */

                if (Download_flag) {

                    hci_clutter_download (w, client_data, call_data);

                }
            }
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

void
hci_clutter_file_save_as_callback (
Widget          w,
XtPointer       client_data,
XtPointer       call_data
)
{
        int     in;
        int     i;
        char    *file_label;
        Widget  form;
        Widget  rowcol;
        Widget  button;
        Widget  label;
        static Widget  text;

        HCI_LE_log("File Save As selected");

/*      If the Save As window is already open, do nothing.      */

        if (Save_as_dialog != NULL)
	{
	  /* Clear the password text widget. */
	  XmTextSetString( text, "" );
	  HCI_Shell_popup( Save_as_dialog );
        }
	else
	{
	  HCI_Shell_init( &Save_as_dialog, "Clutter Region Filename" );

          form = XtVaCreateWidget ("clutter_file_save_as_form",
                xmFormWidgetClass,      Save_as_dialog,
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                NULL);

          rowcol = XtVaCreateWidget ("file_save_as_rowcol",
                xmRowColumnWidgetClass, form,
                XmNleftAttachment,      XmATTACH_FORM,
                XmNtopAttachment,       XmATTACH_FORM,
                XmNrightAttachment,     XmATTACH_FORM,
                XmNpacking,             XmPACK_COLUMN,
                XmNorientation,         XmHORIZONTAL,
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                NULL);

          button = XtVaCreateManagedWidget ("Cancel",
                xmPushButtonWidgetClass,rowcol,
                XmNforeground,          hci_get_read_color (BUTTON_FOREGROUND),
                XmNbackground,          hci_get_read_color (BUTTON_BACKGROUND),
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

          XtAddCallback (button,
                XmNactivateCallback,  hci_clutter_file_save_as_window_close, client_data);

          Accept_button = XtVaCreateManagedWidget ("Accept",
                xmPushButtonWidgetClass,rowcol,
                XmNforeground,          hci_get_read_color (BUTTON_FOREGROUND),
                XmNbackground,          hci_get_read_color (BUTTON_BACKGROUND),
                XmNfontList,            hci_get_fontlist (LIST),
                NULL);

          XtAddCallback (Accept_button,
                XmNactivateCallback,  hci_clutter_file_save_as_window_ok, client_data);

          XtManageChild (rowcol);

          label = XtVaCreateManagedWidget ("Label: ",
                xmLabelWidgetClass,     form,
                XmNforeground,          hci_get_read_color (TEXT_FOREGROUND),
                XmNbackground,          hci_get_read_color (BACKGROUND_COLOR1),
                XmNfontList,            hci_get_fontlist (LIST),
                XmNleftAttachment,      XmATTACH_FORM,
                XmNtopAttachment,       XmATTACH_WIDGET,
                XmNtopWidget,           rowcol,
                XmNbottomAttachment,    XmATTACH_FORM,
                NULL);

          text = XtVaCreateManagedWidget ("save_as_text",
                xmTextFieldWidgetClass, form,
                XmNbackground,          hci_get_read_color (EDIT_BACKGROUND),
                XmNforeground,          hci_get_read_color (EDIT_FOREGROUND),
                XmNcolumns,             31,
                XmNmaxLength,           31,
                XmNmarginHeight,        2,
                XmNshadowThickness,     2,
                XmNmarginWidth,         0,
                XmNborderWidth,         0,
                XmNhighlightOnEnter,    True,
                XmNfontList,            hci_get_fontlist (LIST),
                XmNleftAttachment,      XmATTACH_WIDGET,
                XmNleftWidget,          label,
                XmNtopAttachment,       XmATTACH_WIDGET,
                XmNtopWidget,           rowcol,
                XmNbottomAttachment,    XmATTACH_FORM,
                XmNrightAttachment,     XmATTACH_FORM,
                NULL);

          XtAddCallback (text,
                XmNlosingFocusCallback, hci_save_as_label_callback,
                NULL);
      
          XtAddCallback (text,
                XmNactivateCallback, hci_save_as_label_callback,
                (XtPointer)-1);

          XtManageChild (form);
	  HCI_Shell_start( Save_as_dialog, NO_RESIZE_HCI );
	}

/*      Find the index of the first available entry in the clutter      *
 *      regions data buffer for the new file.                           */

        in = 0;

        for (i=0;i<MAX_CLTR_FILES;i++) {

            file_label = hci_get_clutter_region_file_label (i);

            if (strlen (file_label) > 0) {

                in++;

            }
        }

/*      If a file entry is available, then popup a window so a filename *
 *      can be specified.                                               */

        if (in < MAX_CLTR_FILES) {

            /* make sure to clear the existing value */
            memset( Clutter_label, 0, 1 );

        }
        else
        {
          HCI_Shell_popdown( Save_as_dialog );
          hci_warning_popup (Top_widget,
             "The maximum number of clutter files has been reached.\nYou must first delete an existing file before adding\na new one.", NULL);
          HCI_LE_log("Maximum number of Clutter files created. Cannot \"Save As\"" );
          return;
        }
}

/************************************************************************
 *      Description: This function is called after the user enters a    *
 *                   new name in the Clutter Region Files Save As       *
 *                   window.                                            *
 *                                                                      *
 *      Input:  w - save as edit box widget ID                          *
 *              client_data - used to indicate that enter is pushed     *
 *              call_data - unused                                      *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

void
hci_save_as_label_callback (
Widget          w,
XtPointer       client_data,
XtPointer       call_data
)
{
        char    *text;
        int     len;

        text = XmTextGetString (w);

        len = strlen (text);

        if (len >= MAX_LABEL_SIZE)
            len = MAX_LABEL_SIZE-1;

        strncpy (Clutter_label, text, len);
        memset (Clutter_label+len, 0, 1);

        XtFree (text);
       /* when enter is pushed make sure to call accept */
        if ( (int)client_data < 0)
        {
          hci_clutter_file_save_as_window_ok(w, client_data, call_data);
        }
}

/************************************************************************
 *      Description: This function is called after the user selects the *
 *                   "Accept" button in the Clutter Region Files Save   *
 *                   As window.                                         *
 *                                                                      *
 *      Input:  w - Accept button ID                                    *
 *              client_data - unused                                    *
 *              call_data - unused                                      *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

void
hci_clutter_file_save_as_window_ok (
Widget          w,
XtPointer       client_data,
XtPointer       call_data
)
{
        int     i;
        int     status;
        time_t  tm;
        int     month, day, year;
        int     hour, minute, second;
        char    *file_label;
        int     clutter_file;

/*      Set the label of the first available clutter file in buffer.    */

        New_flag = 0;

        if ((int) client_data < 0) {

            clutter_file = -1;

            if (strlen (Clutter_label) > 0) {

                for (i=0;i<MAX_CLTR_FILES;i++) {

                    file_label = hci_get_clutter_region_file_label (i);
                if (!strcmp (Clutter_label,file_label)) {

                    hci_warning_popup (Top_widget,
                        "The filename you specified is already\nused.  Enter a new filename.", NULL);

                    break;

                }

                    if (strlen (file_label) == 0) {

                        clutter_file = i;
                        break;

                    }
                }
            }
            else
            {
                    hci_warning_popup (Top_widget,
                        "Please Enter a valid filename", NULL);
            }

/*          If no files are available in buffer, do nothing and return. */

            if (clutter_file < 0) {

                return;

            }

/*          Get the current system time.                                */

            tm = time (NULL);

/*          Set the current clutter file index to the new one.          */

            Clutter_file = clutter_file;

/*          Update the clutter file update time tag.                    */

            hci_set_clutter_region_file_time (Clutter_file, tm);

/*          Set the number of regions defined in the current file.      */

            hci_set_clutter_region_regions (Clutter_file, Edit_regions);

/*          Set the clutter file label tag.                             */

            hci_set_clutter_region_file_label (Clutter_file,
                                Clutter_label);

/*          Set the regions data.                                       */

            for (i=0;i<Edit_regions;i++) {

                hci_set_clutter_region_data (Clutter_file,
                                        i,
                                        HCI_CCZ_START_AZIMUTH,
                                        Edit [i].start_azimuth);
                hci_set_clutter_region_data (Clutter_file,
                                        i,
                                        HCI_CCZ_STOP_AZIMUTH,
                                        Edit [i].stop_azimuth);
                hci_set_clutter_region_data (Clutter_file,
                                        i,
                                        HCI_CCZ_START_RANGE,
                                        Edit [i].start_range);
                hci_set_clutter_region_data (Clutter_file,
                                        i,
                                        HCI_CCZ_STOP_RANGE,
                                        Edit [i].stop_range);
                hci_set_clutter_region_data (Clutter_file,
                                        i,
                                        HCI_CCZ_SEGMENT,
                                        Edit [i].segment);
                hci_set_clutter_region_data (Clutter_file,
                                        i,
                                        HCI_CCZ_SELECT_CODE,
                                        Edit [i].select_code);
                hci_set_clutter_region_data (Clutter_file,
                                        i,
                                        HCI_CCZ_DOPPL_LEVEL,
                                        Edit [i].dopl);
                hci_set_clutter_region_data (Clutter_file,
                                        i,
                                        HCI_CCZ_SURV_LEVEL,
                                        Edit [i].surv);

            }
        }

/*      If directed by client data, output the updated data.    */

        if ((int) client_data < 0) {

/*          If low bandwidth user, popup a progress meter window.       */

            HCI_PM("Saving clutter region data");

            status = hci_write_clutter_regions_file ();

            if (Close_flag) {

                HCI_task_exit (HCI_EXIT_SUCCESS);

            }

        }

/*      Update the window label update time.                            */

        unix_time (&tm,
                   &year,
                   &month,
                   &day,
                   &hour,
                   &minute,
                   &second);

        sprintf (Buff,"Clutter Regions - File:%s %s: Last Modified %2.2d/%2.2d/%4.4d  %2.2d:%2.2d:%2.2d",
        hci_get_clutter_region_file_label (Clutter_file), Lbw_title, 
                        month, day, year, hour, minute, second);

/*      Add redundancy information if site FAA redundant        */

        if (HCI_get_system() == HCI_FAA_SYSTEM) {

            sprintf (Buff,"%s - (FAA:%d)", Buff,
                        ORPGRED_channel_num (ORPGRED_MY_CHANNEL));

        }

        XtVaSetValues (Top_widget,
                XmNtitle, Buff,
                NULL);

/*      If the File window is open update the files list.               */

        if (Open_dialog != (Widget) NULL) {

            hci_update_file_list ();
            XmListSelectPos (List_widget, Clutter_file+1, False);

        }

        HCI_Shell_popdown(Save_as_dialog);

/*      Clear the update flag, desensitize the Undo button, and         *
 *      sensitize the Restore and Update buttons.                       */

        Update_flag = 0;

        XtVaSetValues (Undo_button,
                XmNsensitive,   False,
                NULL);

        if (Unlocked_loca == HCI_YES_FLAG) {

            XtVaSetValues (Update_button,
                XmNsensitive,   True,
                NULL);
            XtVaSetValues (Restore_button,
                XmNsensitive,   True,
                NULL);

        }

/*      If the download flag is set we want to also download    *
 *      the saved file to the RDA.                              */

        if (Download_flag) {

            hci_clutter_download (w, client_data, call_data);

        }
}

/************************************************************************
 *      Description: This function is called after the user selects the *
 *                   "Close" button in the Clutter Region Files Save    *
 *                   As window.                                         *
 *                                                                      *
 *      Input:  w - Close button ID                                     *
 *              client_data - unused                                    *
 *              call_data - unused                                      *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

void
hci_clutter_file_save_as_window_close (
Widget          w,
XtPointer       client_data,
XtPointer       call_data
)
{

/*      If cancelling out of a new file operation we need to undo what  *
 *      was done and reset the active file to the Default one.  If the  *
 *      user wants to create another new file then they need to select  *
 *      the New button again.                                           */

        if (New_flag || (Unlocked_loca == HCI_NO_FLAG)) {

            if (Unlocked_loca == HCI_YES_FLAG) {

                New_flag     = 0;
                Clutter_file = 0;

            }

            hci_clutter_reload_callback ((Widget) NULL,
                                         (XtPointer) NULL,
                                         (XtPointer) NULL);

        }

/*      Clear the download flag in case we are aborting a download      *
 *      operation.                                                      */

        if (Download_flag) {

            Download_flag = 0;

            sprintf (Buff,"Clutter Regions Download aborted");
        
/*          Post an HCI event so the HCI Control/Status task will       *
 *          display the message in the Feedback line.                   */

	    HCI_display_feedback( Buff );
        }

	HCI_Shell_popdown( Save_as_dialog );        
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

void
hci_clutter_delete_callback (
Widget          w,
XtPointer       client_data,
XtPointer       call_data
)
{
        int     position_count;
        int     *position_list;

/*      If no file is selected in the File list, do nothing and return. */

        if (!XmListGetSelectedPos (List_widget, &position_list, &position_count)) {

            return;

        }

        HCI_LE_log("File Delete selected");

/*      If the selected file is the default file, don't allow it to     *
 *      be deleted.  Popup an information message to convey this to the *
 *      user.                                                           */

        if (position_list [0] == 1) {

            sprintf (Buff,"You cannot delete the default clutter\nsuppression regions file!");
            hci_warning_popup( Top_widget, Buff, NULL );

/*      A valid file is selected.  Popup a confirmation window so the   *
 *      user can proceed or abort the delete file operation.            */

        } else {

            sprintf (Buff,"You are about to delete the selected\nclutter suppression regions file.\nDo you want to continue?");
            hci_confirm_popup( Top_widget, Buff, hci_clutter_file_delete_confirm_ok, NULL );

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

void
hci_clutter_file_delete_confirm_ok (
Widget          w,
XtPointer       client_data,
XtPointer       call_data
)
{
        int     status;
        time_t  tm;
        int     i;
        int     item;
        int     j;
        int     regions;
        char    *file_label;
        int     month, day, year;
        int     hour, minute, second;
        int     position_count;
        int     *position_list;

/*      If no file was selected, do nothing and return.         */

        if (!XmListGetSelectedPos (List_widget, &position_list, &position_count)) {

            return;

        }

/*      Blank out the current clutter label.                    */

        strcpy (Clutter_label,"");

/*      For each selected file in the list delete it by setting its     *
 *      label tag to blank.                                             */

        for (item=0;item<position_count;item++) {

            XmListDeletePos (List_widget, position_list [item]);

            status = hci_set_clutter_region_file_label (position_list [item]-1,
                                Clutter_label);

/*          Move up all trailing clutter files in the buffer one        *
 *          position.                                                   */

            for (i=position_list [item];i<MAX_CLTR_FILES;i++) {

                file_label = hci_get_clutter_region_file_label (i);

                status = hci_set_clutter_region_file_label (i-1,
                                file_label);
                tm =hci_get_clutter_region_file_time (i);
                status = hci_set_clutter_region_file_time (i-1,
                                tm);

                regions = hci_get_clutter_region_regions (i);

                hci_set_clutter_region_regions (i-1,
                                regions);

                for (j=0;j<regions;j++) {

                    status = hci_set_clutter_region_data (
                                        i-1,
                                        j,
                                        HCI_CCZ_START_AZIMUTH,
                                        hci_get_clutter_region_data (
                                                i,
                                                j,
                                                HCI_CCZ_START_AZIMUTH));

                    status = hci_set_clutter_region_data (
                                        i-1,
                                        j,
                                        HCI_CCZ_STOP_AZIMUTH,
                                        hci_get_clutter_region_data (
                                                i,
                                                j,
                                                HCI_CCZ_STOP_AZIMUTH));

                    status = hci_set_clutter_region_data (
                                        i-1,
                                        j,
                                        HCI_CCZ_START_RANGE,
                                        hci_get_clutter_region_data (
                                                i,
                                                j,
                                                HCI_CCZ_START_RANGE));

                    status = hci_set_clutter_region_data (
                                        i-1,
                                        j,
                                        HCI_CCZ_STOP_RANGE,
                                        hci_get_clutter_region_data (
                                                i,
                                                j,
                                                HCI_CCZ_STOP_RANGE));

                    status = hci_set_clutter_region_data (
                                        i-1,
                                        j,
                                        HCI_CCZ_SELECT_CODE,
                                        hci_get_clutter_region_data (
                                                i,
                                                j,
                                                HCI_CCZ_SELECT_CODE));

                    status = hci_set_clutter_region_data (
                                        i-1,
                                        j,
                                        HCI_CCZ_SEGMENT,
                                        hci_get_clutter_region_data (
                                                i,
                                                j,
                                                HCI_CCZ_SEGMENT));

                    status = hci_set_clutter_region_data (
                                        i-1,
                                        j,
                                        HCI_CCZ_DOPPL_LEVEL,
                                        hci_get_clutter_region_data (
                                                i,
                                                j,
                                                HCI_CCZ_DOPPL_LEVEL));

                    status = hci_set_clutter_region_data (
                                        i-1,
                                        j,
                                        HCI_CCZ_SURV_LEVEL,
                                        hci_get_clutter_region_data (
                                                i,
                                                j,
                                                HCI_CCZ_SURV_LEVEL));

                }
            }
        }

        XtFree ( (char *) position_list);

/*      If low bandwidth user, popup a progress meter window.   */

        HCI_PM("Deleting clutter region file");

/*      Output the update clutter buffer.                       */

        status = hci_write_clutter_regions_file ();

/*      Set the current clutter file pointer to the default file.       */

        Clutter_file = 0;

        XmListSelectPos (List_widget, Clutter_file+1, False);

        Edit_regions = hci_get_clutter_region_regions (Clutter_file);

        for (i=0;i<MAX_NUMBER_CLUTTER_ZONES;i++) {

            Edit [i].start_azimuth = (float) hci_get_clutter_region_data (
                                                Clutter_file,
                                                i,
                                                HCI_CCZ_START_AZIMUTH);
            Edit [i].stop_azimuth  = (float) hci_get_clutter_region_data (
                                                Clutter_file,
                                                i,
                                                HCI_CCZ_STOP_AZIMUTH);
            Edit [i].start_range   = (float) hci_get_clutter_region_data (
                                                Clutter_file,
                                                i,
                                                HCI_CCZ_START_RANGE);
            Edit [i].stop_range    = (float) hci_get_clutter_region_data (
                                                Clutter_file,
                                                i,
                                                HCI_CCZ_STOP_RANGE);
            Edit [i].segment       = hci_get_clutter_region_data (
                                                Clutter_file,
                                                i,
                                                HCI_CCZ_SEGMENT);
            Edit [i].select_code   = hci_get_clutter_region_data (
                                                Clutter_file,
                                                i,
                                                HCI_CCZ_SELECT_CODE);
            Edit [i].dopl          = hci_get_clutter_region_data (
                                                Clutter_file,
                                                i,
                                                HCI_CCZ_DOPPL_LEVEL);
            Edit [i].surv          = hci_get_clutter_region_data (
                                                Clutter_file,
                                                i,
                                                HCI_CCZ_SURV_LEVEL);

        }

/*      If the File window is open, update the files list.      */

        if (Open_dialog != (Widget) NULL) {

            hci_update_file_list ();

        }

/*      Build and display new composite clutter maps.           */

        hci_build_clutter_map (&Clutter_map[0][0],
                        Edit,
                        Edit_segment,
                        Edit_regions);

        hci_display_clutter_map ();

/*      Update the clutter sector overlays in the display area. */

        hci_refresh_censor_zone_sectors ();

/*      Update the clutter table widgets.                       */

        hci_update_clutter_sector_widgets ();

/*      Update the update time part of the label at the top of the      *
 *      clutter regions window.                                         */

        tm =hci_get_clutter_region_file_time (Clutter_file);

        if (tm < 1) {

            sprintf (Buff,"Clutter Regions - File:%s %s: Last Modified Unknown",
                hci_get_clutter_region_file_label (Clutter_file), Lbw_title);

        } else {

            unix_time (&tm,
               &year,
               &month,
               &day,
               &hour,
               &minute,
               &second);

            sprintf (Buff,"Clutter Regions - File:%s %s: Last Modified %2.2d/%2.2d/%4.4d  %2.2d:%2.2d:%2.2d",
                hci_get_clutter_region_file_label (Clutter_file), Lbw_title, 
                month, day, year, hour, minute, second);

        }

/*      Add redundancy information if site FAA redundant        */

        if (HCI_get_system() == HCI_FAA_SYSTEM) {

            sprintf (Buff,"%s - (FAA:%d)", Buff,
                ORPGRED_channel_num (ORPGRED_MY_CHANNEL));

        }

        XtVaSetValues (Top_widget,
                XmNtitle,               Buff,
                NULL);

/*      Call the expose callback to make the updates visible.           */

        hci_clutter_expose_callback ((Widget) NULL,
                        (XtPointer) NULL,
                        (XtPointer) NULL);
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

void
hci_clutter_download_callback (
Widget          w,
XtPointer       client_data,
XtPointer       call_data
)
{
        char buf[HCI_BUF_128];

        HCI_LE_log("Download selected");

        Download_flag = 1;

/*      Pop up a confirmation window for the download operation.        */

        sprintf( buf, "You are about to download the currently\ndisplayed clutter regions file.  Do you\nwant to continue?" );
        hci_confirm_popup( Top_widget, buf, hci_clutter_download_confirm_ok, hci_clutter_download_confirm_no );
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

void
hci_clutter_download_confirm_no (
Widget          w,
XtPointer       client_data,
XtPointer       call_data
)
{
/*      Do nothing.     */

        HCI_LE_log("Download aborted");

        Download_flag = 0;

        sprintf (Buff,"Clutter Regions Download aborted");
        
/*      Post an HCI event so the HCI Control/Status task will display   *
 *      the message in the Feedback line.                               */

	HCI_display_feedback( Buff );
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

void
hci_clutter_download_confirm_ok (
Widget          w,
XtPointer       client_data,
XtPointer       call_data
)
{
        char buf[HCI_BUF_256];

/*      If the update flag is set, first popup a window allowing the    *
 *      user to save edits before downloading.                          */

        if (Update_flag) {

            sprintf( buf, "You made edits but did not save them.\nYou must save edits for them to be\ndownloaded to the RDA.  Do you want\nto save them and download or abort?");
            hci_confirm_popup( Top_widget, buf, hci_clutter_download_confirm_ok_save, hci_clutter_download_confirm_no_save );

/*      No unsaved edits were detected so download the file to the      *
 *      RDA.                                                            */

        } else {

            hci_clutter_download (w, client_data, call_data);

        }
}

/************************************************************************
 *      Description: This function is called when the "No" button       *
 *                   is selected from the Clutter Regions download      *
 *                   and save confirmation popup window.                *
 *                                                                      *
 *      Input:  w - No button ID                                        *
 *              client_data - unused                                    *
 *              call_data - unused                                      *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

void
hci_clutter_download_confirm_no_save (
Widget          w,
XtPointer       client_data,
XtPointer       call_data
)
{
/*      Don't save edits before downloading to the RDA.                 */

        HCI_LE_log("Download aborted");

        Download_flag = 0;

        sprintf (Buff,"Clutter Regions Download aborted");
        
/*      Post an HCI event so the HCI Control/Status task will display   *
 *      the message in the Feedback line.                               */

	HCI_display_feedback( Buff );
}

/************************************************************************
 *      Description: This function is called when the "Yes" button      *
 *                   is selected from the Clutter Regions download      *
 *                   and save confirmation popup window.                *
 *                                                                      *
 *      Input:  w - No button ID                                        *
 *              client_data - unused                                    *
 *              call_data - unused                                      *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

void
hci_clutter_download_confirm_ok_save (
Widget          w,
XtPointer       client_data,
XtPointer       call_data
)
{
/*      Save the edits first.                                           */

/*      If the window is unlocked then we can use the save function.    *
 *      If not, then we must use the save as function.                  */

        if (Unlocked_loca == HCI_YES_FLAG) {

            hci_clutter_file_save_yes_callback (w,
                        (XtPointer) NULL,
                        (XtPointer) NULL);

        } else {

            hci_clutter_file_save_as_callback (w,
                        (XtPointer) -1,
                        (XtPointer) -1);

        }
}

/*      The following function requests that the current clutter        *
 *      regions file is downloaded to the RDA.                          */

void
hci_clutter_download ()
{
        int     status;
        int     month, day, year;
        int     hour, minute, second;
        time_t  tm;
        XmString        str;

        Download_flag = 0;

/*      If low bandwidth user, popup a progress meter window.           */

        HCI_PM("Downloading Clutter Regions");

/*      Send the download command to the control_rda task so it can     *
 *      send the data to the RDA.                                       */

        status = hci_download_clutter_regions_file (Clutter_file);

/*      Update the last download time label.                            */

        tm =hci_get_clutter_region_download_time (Clutter_file);

        unix_time (&tm,
               &year,
               &month,
               &day,
               &hour,
               &minute,
               &second);

        if (((month  >    0) && (month  <   13)) &&
            ((day    >    0) && (day    <   32)) &&
            ((year   > 1988) && (year   < 2100)) &&
            ((hour   >=   0) && (hour   <   24)) &&
            ((minute >=   0) && (minute <   60)) &&
            ((second >=   0) && (second <   60))) {

            sprintf (Buff,"Last Download %2.2d/%2.2d/%4.4d  %2.2d:%2.2d:%2.2d",
                month, day, year, hour, minute, second);

        } else {

            sprintf (Buff,"Last Download Time Unknown        ");

        }

        str = XmStringCreateLocalized (Buff);
        XtVaSetValues (Download_label,
                XmNlabelString, str,
                NULL);
        XmStringFree (str);
}

/************************************************************************
 *      Description: This function updates the clutter region files     *
 *                   list in the Clutter_region Files window.           *
 *                                                                      *
 *      Input:  NONE                                                    *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

void
hci_update_file_list (
)
{
        int     i;
        int     in;
        char    *file_label;
        int     month, day, year;
        int     hour, minute, second;
        time_t  tm;
        XmString        str;

/*      First, delete the current list contents.                */

        XmListDeleteAllItems (List_widget);

/*      Next, read the file list and build a menu item for each *
 *      file with a label.                                      */

        in = 0;

        for (i=0;i<MAX_CLTR_FILES;i++) {

            file_label = hci_get_clutter_region_file_label (i);

            if (strlen (file_label) > 0) {

                tm =hci_get_clutter_region_file_time (i);

                if (tm < 1) {

                    year   = 0;
                    month  = 0;
                    day    = 0;
                    hour   = 0;
                    minute = 0;
                    second = 0;

                } else {

                    unix_time (&tm,
                       &year,
                       &month,
                       &day,
                       &hour,
                       &minute,
                       &second);

                }

                sprintf (Buff,"%2.2d/%2.2d/%4.4d  %2.2d:%2.2d:%2.2d  %s",
                        month, day, year, hour, minute, second,
                        file_label);

                str = XmStringCreateLocalized (Buff);
                XmListAddItemUnselected (List_widget, str, 0);
                XmStringFree (str);

            } else {

                break;

            }
        }

        XmListSelectPos (List_widget, Clutter_file+1, False);
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

void
hci_ccz_reset_zoom_callback (
Widget          w,
XtPointer       client_data,
XtPointer       call_data
)
{

        HCI_LE_log("Reset Zoom selected");

/*      Reset the magnification factor and radar offsets.       */

        Clutter_zoom = 1;
        Center_x_offset = 0;
        Center_y_offset = 0;

/*      Refresh the data displayed in the display area.         */

        hci_refresh_censor_zone_sectors ();

/*      Call the expose callback to make the updates visible.   */

        hci_clutter_expose_callback ((Widget) NULL,
                        (XtPointer) NULL,
                        (XtPointer) NULL);
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

void
hci_ccz_background_callback (
Widget          w,
XtPointer       client_data,
XtPointer       call_data
)
{
        XmToggleButtonCallbackStruct    *state =
                (XmToggleButtonCallbackStruct *) call_data;

/*      If the button is set, get the product and display it.           */

        if (state->set) {

/*          The background product type is passed via client data.      */

            Clutter_background = (int) client_data;

            hci_load_clutter_background_product (Clutter_background);

/*          Update the data in the data display area.                   */

            hci_refresh_censor_zone_sectors ();

/*          Update the clutter table widgets.                           */

            hci_update_clutter_sector_widgets ();

/*          Call the expose callback to make updates visible.           */

            hci_clutter_expose_callback ((Widget) NULL,
                        (XtPointer) NULL,
                        (XtPointer) NULL);

        }
}

/************************************************************************
 *      Description: This function is called when one of the CFC        *
 *                   Channel radio buttons is selected in the Clutter   *
 *                   Regions window.                                    *
 *                                                                      *
 *      Input:  w - radio button ID                                     *
 *              client_data - channel number                            *
 *              call_data - toggle button data                          *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

void
hci_ccz_channel_callback (
Widget          w,
XtPointer       client_data,
XtPointer       call_data
)
{
        int     i;
        short   params [6];
        XmToggleButtonCallbackStruct    *state =
                (XmToggleButtonCallbackStruct *) call_data;

/*      If the button is set, then process the new selection.           */

        if (state->set) {

/*          Get the channel number from client data.                    */

            Edit_channel = (int) client_data;

            HCI_LE_log("Channel %d selected", Edit_channel);

/*          Since the channel select buttons are only visible when the  *
 *          CFC product is selected as the background, determine which  *
 *          CFC product is needed and open it.                          */

            for (i=0;i<6;i++)
                params [i] = 0;

            params [0] = (Edit_channel-1) + (ONE<<Edit_segment);

            Background_display_flag =
                hci_load_product_data (CFCPROD,
                                       -1,
                                       params);

/*          Update the data displayed in the display area.              */

            hci_refresh_censor_zone_sectors ();

/*          Update the clutter table widgets.                           */

            hci_update_clutter_sector_widgets ();

/*          Call the expose callback to make updates visible.           */

            hci_clutter_expose_callback ((Widget) NULL,
                        (XtPointer) NULL,
                        (XtPointer) NULL);

        }
}

/************************************************************************
 *      Description: This function is called when the lock button is    *
 *                   selected in the Clutter Regions window.            *
 *                                                                      *
 *      Input:  w - lock button ID                                      *
 *              security_level - security state data                    *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

int
hci_clutter_censor_zones_lock ()
{
  if( hci_lock_open() )
  {
    /* Do nothing when user opens password dialog. */
  }
  else if( hci_lock_loca_selected() )
  {
    /* When the user selects a LOCA, update
       any color/sensitivity changes. */
    if( hci_lock_URC_selected() || hci_lock_ROC_selected() )
    {
      hci_update_clutter_sector_widgets ();
    }
  }
  else if( hci_lock_loca_unlocked() )
  {
    if( hci_lock_URC_unlocked() || hci_lock_ROC_unlocked() )
    {
      Unlocked_loca = HCI_YES_FLAG;

      /* Let user know if another user is currently editing this data. */

      if( ORPGCCZ_get_edit_status( ORPGCCZ_LEGACY_ZONES ) == ORPGEDLOCK_EDIT_LOCKED)
      {
        sprintf( Buff, "Another user is currently editing clutter\nsuppression regions data. Any changes may be\noverwritten by the other user.");
        hci_warning_popup( Top_widget, Buff, NULL );
      }

      /* Set edit advisory lock. */

      ORPGCCZ_set_edit_lock( ORPGCCZ_LEGACY_ZONES );
    }

    /* If the File window is open, sensitize the New,
       Delete, and Save As buttons. */

    if( Open_dialog != (Widget) NULL )
    {
      XtVaSetValues( Delete_file_button, XmNsensitive, True, NULL );
    }
  }
  else if( hci_lock_close() && Unlocked_loca == HCI_YES_FLAG )
  {
    Unlocked_loca = HCI_NO_FLAG;

    /* Clear any advisory edit locks so other users won't
       get warned about another user editing the data. */

    ORPGCCZ_clear_edit_lock( ORPGCCZ_LEGACY_ZONES );

    /* If the update flag is set (indicating unsaved edits),
       set the new file pointer to the current one. */

    if( Update_flag )
    {
      New_file = Clutter_file;
      hci_clutter_file_save_callback( Top_widget, (XtPointer) 1, NULL );
    }

    /* If the File window is open, desensitize the Save As,
       New, and Delete buttons.*/

    if( Open_dialog != (Widget) NULL )
    {
      XtVaSetValues( Delete_file_button, XmNsensitive, False, NULL );
    }
  }

  /* Update the clutter table widgets. */

  hci_update_clutter_sector_widgets ();

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

void
clutter_restore_baseline (
Widget          w,
XtPointer       client_data,
XtPointer       call_data
)
{
        char buf[HCI_BUF_128];

/*      Popup a confirmation window before changing any data.           */

        sprintf( buf, "You are about to restore the clutter regions\nadaptation data to baseline values.\nDo you want to continue?" );
        hci_confirm_popup( Top_widget, buf, accept_clutter_restore_baseline, NULL );
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

void
accept_clutter_restore_baseline (
Widget          w,
XtPointer       client_data,
XtPointer       call_data
)
{
        int     status;
        int     i;
        time_t  tm;
        int     month, day, year;
        int     hour, minute, second;

/*      Copy the backup message to the current message.                 */

        status = ORPGCCZ_baseline_to_default( ORPGCCZ_LEGACY_ZONES );

/*      Generate a message based on the success of the copy.            */

        if (status < 0) {

            HCI_LE_error("Unable to restore LEGACY CENSOR ZONES");
            sprintf (Buff,"Unable to restore baseline Clutter Regions data");

        } else {

            HCI_LE_status("Clutter Regions data restored from baseline");
            sprintf (Buff,"Clutter Regions data restored from baseline");
        
        }

/*      Post an HCI event so the HCI Control/Status task will display   *
 *      the message in the Feedback line.                               */

	HCI_display_feedback( Buff );

/*      Reset the current file pointer to the default file.             */

        Clutter_file = 0;
        New_file     = 0;

/*      If low bandwidth, popup a progress meter window.                */

        HCI_PM("Reloading clutter data");

/*      Read the updated clutter data from file.                        */

        status = hci_read_clutter_regions_file ();

/*      Replace the data in the current edit buffer with the new data   */

        Edit_regions = hci_get_clutter_region_regions (Clutter_file);

        for (i=0;i<MAX_NUMBER_CLUTTER_ZONES;i++) {

            Edit [i].start_azimuth = (float) hci_get_clutter_region_data (
                                                Clutter_file,
                                                i,
                                                HCI_CCZ_START_AZIMUTH);
            Edit [i].stop_azimuth  = (float) hci_get_clutter_region_data (
                                                Clutter_file,
                                                i,
                                                HCI_CCZ_STOP_AZIMUTH);
            Edit [i].start_range   = (float) hci_get_clutter_region_data (
                                                Clutter_file,
                                                i,
                                                HCI_CCZ_START_RANGE);
            Edit [i].stop_range    = (float) hci_get_clutter_region_data (
                                                Clutter_file,
                                                i,
                                                HCI_CCZ_STOP_RANGE);
            Edit [i].segment       = hci_get_clutter_region_data (
                                                Clutter_file,
                                                i,
                                                HCI_CCZ_SEGMENT);
            Edit [i].select_code   = hci_get_clutter_region_data (
                                                Clutter_file,
                                                i,
                                                HCI_CCZ_SELECT_CODE);
            Edit [i].dopl          = hci_get_clutter_region_data (
                                                Clutter_file,
                                                i,
                                                HCI_CCZ_DOPPL_LEVEL);
            Edit [i].surv          = hci_get_clutter_region_data (
                                                Clutter_file,
                                                i,
                                                HCI_CCZ_SURV_LEVEL);

        }

/*      Build and displayy composite clutter maps.              */

        hci_build_clutter_map (&Clutter_map[0][0],
                        Edit,
                        Edit_segment,
                        Edit_regions);

        hci_display_clutter_map ();

/*      Update the clutter display area.                        */

        hci_refresh_censor_zone_sectors ();

/*      Update the clutter table widgets.                       */

        hci_update_clutter_sector_widgets ();

/*      Call the expose callback to make updates visible.       */

        hci_clutter_expose_callback ((Widget) NULL,
                        (XtPointer) NULL,
                        (XtPointer) NULL);

/*      If the File window is open, update the files list.      */

        if (Open_dialog != (Widget) NULL) {

            hci_update_file_list ();

        }

/*      Update the time stamp in the main window title.         */

        tm = hci_get_clutter_region_file_time (Clutter_file);

        if (tm < 1) {

            sprintf (Buff,"Clutter Regions - File:%s %s: Last Modified: Unknown ",
                hci_get_clutter_region_file_label (Clutter_file), Lbw_title);

        } else {

            unix_time (&tm,
                   &year,
                   &month,
                   &day,
                   &hour,
                   &minute,
                   &second);

            sprintf (Buff,"Clutter Regions - File:%s %s: Last Modified %2.2d/%2.2d/%4.4d  %2.2d:%2.2d:%2.2d",
                hci_get_clutter_region_file_label (Clutter_file), Lbw_title, 
                month, day, year, hour, minute, second);

        }

/*      Add redundancy information if site FAA redundant        */

        if (HCI_get_system() == HCI_FAA_SYSTEM) {

            sprintf (Buff,"%s - (FAA:%d)", Buff,
                ORPGRED_channel_num (ORPGRED_MY_CHANNEL));

        }

        XtVaSetValues (Top_widget,
                XmNtitle, Buff,
                NULL);

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

void
clutter_update_baseline (
Widget          w,
XtPointer       client_data,
XtPointer       call_data
)
{
        char buf[HCI_BUF_128];

/*      Display a confirmation prompt before updating anything.         */

        sprintf( buf, "You are about to update the baseline\nClutter Regions adaptation data.\nDo you want to continue?" );
        hci_confirm_popup( Top_widget, buf, accept_clutter_update_baseline, NULL );
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

void
accept_clutter_update_baseline (
Widget          w,
XtPointer       client_data,
XtPointer       call_data
)
{
        int     status;

/*      Copy the current message to the backup message.                 */

	status = ORPGCCZ_default_to_baseline( ORPGCCZ_LEGACY_ZONES );


/*      Generate a message based on the success of the copy.            */

        if (status < 0) {

            HCI_LE_error("Unable to update LEGACY BASELINE CENSOR ZONES");
            sprintf (Buff,"Unable to update baseline Clutter Regions data");

        } else {

            HCI_LE_status("Clutter Regions baseline data updated");
            sprintf (Buff,"Clutter Regions baseline data updated");

        }

/*      Post an HCI event so the HCI Control/Status task will display   *
 *      the message in the Feedback line.                               */

	HCI_display_feedback( Buff );
}

/************************************************************************
 *      Description: This function loads the latest specified product   *
 *                   type from the RPG products database.               *
 *                                                                      *
 *      Input:  background_type - type of background product            *
 *                                      HCI_CCZ_BACKGROUND_BREF         *
 *                                      HCI_CCZ_BACKGROUND_CFC          *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

void
hci_load_clutter_background_product (
int     background_type
)
{
        int     i, j;
        int     products;
        int     retval;
        short   params [6];
        int     julian_seconds;

/*      Initialize the parameter buffer.                        */

        for (i=0;i<6;i++)
            params [i] = 0;

/*      Check the background product type.                              */

        switch (background_type) {

            case HCI_CCZ_BACKGROUND_BREF :

/*              If a reflectivity product is selected, detemine the     *
 *              cut from the active segment (0.5 degree cut for the     *
  *             low swgment and the 2.4 degree cut for the high segment */

                HCI_LE_log("Reflectivity Background selected");

                XtManageChild (Background_product_rowcol);
                XtUnmanageChild (Cfc_rowcol);

/*              Query the RPG products database and retrieve a list of  *
 *              the latest products available.  We can then check this  *
 *              list to determine the best cut to use for the selected  *
 *              segment.  This is far superior than assuming a specific *
 *              cut is available.  This new scheme is VCP independent.  */

/*              Lets query the database for the latest two volume times *
 *              NOTE: We query the latest 2 times since the elevation   *
 *              cut we want may not yet be available for the latest     *
 *              volume time.                                            */

                Query_data[1].field = RPGP_VOLT;
                Query_data[1].value = 0;
                Query_data[0].field = ORPGDBM_MODE;
                Query_data[0].value = ORPGDBM_FULL_SEARCH |
                                      ORPGDBM_HIGHEND_SEARCH |
                                      ORPGDBM_DISTINCT_FIELD_VALUES;

                products = ORPGDBM_query (Db_info,
                                          Query_data,
                                          2,
                                          2);

                for (i=0;i<products;i++)
                    Query_time [i] = Db_info[i].vol_t;

/*              For the low segment we need to use the lowest elevation *
 *              cut below 2 degrees.                                    */

                Background_display_flag = 0;
 
                for (i=0;i<products;i++) {

/*                  Check to see if the time of the latest product is   *
 *                  more than 15 minutes old.  If so, we do not want to *
 *                  display any data.                                   */

                    julian_seconds = (ORPGVST_get_volume_date()-1)*
                                      HCI_SECONDS_PER_DAY +
                                      ORPGVST_get_volume_time()/1000;

                    if ((julian_seconds - Query_time[i]) > 15*60) {

                        Background_display_flag = HCI_CCZ_DATA_TIMED_OUT;
                        return;

                    }

/*                  Set up the query parameters for the previous        *
 *                  time query and get all instances of the current     *
 *                  background product for that time.  Do a low end     *
 *                  search so that the smallest cuts are listed first.  */

                    Query_data[1].field = RPGP_VOLT;
                    Query_data[1].value = Query_time[i];
                    Query_data[2].field = RPGP_PCODE;
                    Query_data[2].value = ORPGPAT_get_code(Background_product_id);
                    Query_data[0].field = ORPGDBM_MODE;
                    Query_data[0].value = ORPGDBM_EXACT_MATCH |
                                          ORPGDBM_ALL_FIELD_VALUES |
                                          ORPGDBM_LOWEND_SEARCH;

                    retval = ORPGDBM_query (Db_info,
                                         Query_data,
                                         3,
                                         50);

                    for (j=0;j<retval;j++) {

                        if (Edit_segment == HCI_CCZ_SEGMENT_LOW) {

                            if (Db_info[j].params[2] <= HCI_LOW_SEGMENT_MAX_ELEVATION) {
                        
                                params [2] = Db_info[j].params[2];

                                Background_display_flag = 
                                    hci_load_product_data (Background_product_id,
                                                   Db_info[j].vol_t,
                                                   params);

                                break;

                            }

                        } else {

                            if ((Db_info[j].params[2] > HCI_LOW_SEGMENT_MAX_ELEVATION) &&
                                (Db_info[j].params[2] <= HCI_HIGH_SEGMENT_MAX_ELEVATION)) {
                        
                                params [2] = Db_info[j].params[2];

                                Background_display_flag = 
                                    hci_load_product_data (Background_product_id,
                                                   Db_info[j].vol_t,
                                                   params);

                                break;

                            }
                        }
                    }
                        
                    if (Background_display_flag > 0)
                        break;

                }

                break;

/*          If the background product is the CFC product, then we       *
 *          need to determine the product from the currently active     *
 *          segment and channel.                                        */

            case HCI_CCZ_BACKGROUND_CFC :

                HCI_LE_log("CFC Background selected");

                XtUnmanageChild (Background_product_rowcol);
                XtManageChild (Cfc_rowcol);

                params [0] = (Edit_channel-1) + (ONE<<Edit_segment);

                Background_display_flag =
                        hci_load_product_data (CFCPROD,
                                       -1,
                                       params);

                break;

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

void
background_product_callback (
Widget          w,
XtPointer       client_data,
XtPointer       call_data
)
{
        int             status;
        XmString        str;
        char            *buf;

        XmComboBoxCallbackStruct *cbs =
                (XmComboBoxCallbackStruct *) call_data;

/*      Set the new background product ID from the background product   *
 *      lookup table using the menu item position as the index.         */

        Background_product_id = Background_product_LUT[cbs->item_position];

        HCI_LE_status("Setting new background product ID [%d]",
                ORPGPAT_get_code (Background_product_id));

/*      Format a new label to the background product radio button.      */

        sprintf (Buff,"%s[%d] - %-48s   ",
                ORPGPAT_get_mnemonic (Background_product_id),
                ORPGPAT_get_code (Background_product_id),
                ORPGPAT_get_description (Background_product_id, STRIP_MNEMONIC));

        str = XmStringCreateLocalized (Buff);

        XtVaSetValues (Background_product_button,
                XmNlabelString,         str,
                NULL);

        XmStringFree (str);

/*      Modify the Clutter Regions Editor task data message so the      *
 *      HCI agent can build a new RPS list to force generation of       *
 *      the new background product.                                     */

        buf = (char *) calloc (1, ALIGNED_SIZE(sizeof(Hci_ccz_data_t)));

        status = ORPGDA_read (ORPGDAT_HCI_DATA,
                              buf,
                              ALIGNED_SIZE(sizeof(Hci_ccz_data_t)),
                              HCI_CCZ_TASK_DATA_MSG_ID);

        if (status <= 0) {

            HCI_LE_error("Unable to read task configuration data [%d]", status);

        } else {

            Hci_ccz_data_t      *config_data;

            config_data = (Hci_ccz_data_t *) buf;

            if (HCI_is_low_bandwidth())
                config_data->low_product = ORPGPAT_get_code (Background_product_id);
            else
                config_data->high_product = ORPGPAT_get_code (Background_product_id);

            status = ORPGDA_write (ORPGDAT_HCI_DATA,
                              buf,
                              ALIGNED_SIZE(sizeof(Hci_ccz_data_t)),
                              HCI_CCZ_TASK_DATA_MSG_ID);

            if (status <= 0) {

                HCI_LE_error("Unable to update task configuration data [%d]", status);

            } else {

                HCI_LE_status("Background product changed [%d]", Background_product_id);

            }
        }

        hci_load_clutter_background_product (Clutter_background);

/*      Update the data in the data display area.                       */

        hci_refresh_censor_zone_sectors ();

/*      Update the clutter table widgets.                               */

        hci_update_clutter_sector_widgets ();

/*      Call the expose callback to make updates visible.               */

        hci_clutter_expose_callback ((Widget) NULL,
                        (XtPointer) NULL,
                        (XtPointer) NULL);

}

/************************************************************************
 *      Description: This function is used to regularly check for       *
 *                   changes to the system.                             *
 *                                                                      *
 *      Input:  NONE                                                    *
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

void hci_ccz_timer_proc()
{
/* If the RDA configuration changes, call function to display
   popup window and exit. */

   if( ORPGRDA_get_rda_config( NULL ) != ORPGRDA_LEGACY_CONFIG )
   {
     rda_config_change();
   }

  if( reload_flag )
  {
    reload_flag = 0;
    hci_clutter_reload_callback( ( Widget ) NULL,
                                 ( XtPointer ) NULL,
                                 ( XtPointer ) NULL );
  }
}

/************************************************************************
 *      Description: This function is called when the CCZ DEAU data	*
 *                   is updated.					*
 *                                                                      *
 *      Input:  NONE							*
 *      Output: NONE                                                    *
 *      Return: NONE                                                    *
 ************************************************************************/

void
hci_ccz_deau_callback ()
{
  reload_flag = 1;
}

/************************************************************************
 *  Description: This function is called when the RDA configuration     *
 *         changes.                                                     *
 *                                                                      *
 *  Input:  NONE                                                        *
 *  Output: NONE                                                        *
 *  Return: NONE                                                        *
 ************************************************************************/

void rda_config_change()
{
   if( Top_widget != (Widget) NULL && !config_change_popup )
   {
     config_change_popup = 1; /* Indicate popup has been launched. */
     hci_rda_config_change_popup();
   }
   else
   {
     return;
   }
}


