/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:52:37 $
 * $Id: global.h,v 1.13 2009/05/15 17:52:37 ccalvert Exp $
 * $Revision: 1.13 $
 * $State: Exp $
 */
/* global.h */
/* global definitions and includes for main task*/

#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include "assoc_array_i.h"
#include "assoc_array_s.h"
#include <prod_gen_msg.h>
#include <product.h>
#include <Xm/Xm.h>


#include "global2.h"


/* global unit conversion factors - click.c and packet_28radial.c */
/*  more precise than ORPG DEG_TO_RADIAN */
#define CVG_RAD_TO_DEG  (180.0/3.14159265) 
#define CVG_KM_TO_NM 0.539957 /*  more precise than the ORPG KM_TO_NM  0.53996 */



/* CVG 9.0 - SIZE OF PIXMAP AND DRAWABLE FOR PRODUCT (NOT INCLUDE LEGEND */
#define LARGE_IMG 1840
#define SMALL_IMG 768




/* possible selections of screen type */
#define SCREEN_1    1
#define SCREEN_2    2
#define SCREEN_3    3


/* flag to modify legend block drawing logic */
#define LEGEND_FRAME 0   /* not used */
#define PRODUCT_FRAME 1
#define PREFS_FRAME 3


/* flag to state result of loading the product */
#define GOOD_LOAD 1
#define CONFIG_ERROR -1
#define BLK_LEN_ERROR -2
#define PARSE_ERROR -3

/* - flag to state error in generic legend file */
#define GOOD_LEGEND 1
#define LEGEND_ERROR -1

/* types of selections from the packet select dialog */
#define SELECT_NONE   0
#define SELECT_LAYER  1
#define SELECT_PACKET 2
#define SELECT_ALL    3

/* types of "last_image" that can be plotted. we keep track of this
 * for click info processing, memory cleaning, and overlay logic
 * using files: callbacks.c, click.c, dispatcher.c, gui_display.c,
 * packet_X.c, prod_display.c and symbology_block.c
 */
/* a second purpose is with the "this_image" screen variable which
 * is used to help determine the correct value of the overlay_flag.
 * using files: callbacks.c, gui_display.c, symbology_block.c, and
 * indirectly packetselect.c 
 */
#define NO_IMAGE            0
#define RASTER_IMAGE        1
#define PRECIP_ARRAY_IMAGE  2
#define RASTER_NON_GEO      3  
#define DIGITAL_IMAGE      10
#define RLE_IMAGE          11 
#define GENERIC_RADIAL     21
#define GENERIC_GRID       22 /*  not yet implemented */
#define OTHER_2D_IMAGE     99 /*  used for precip rate array with "this_image" */


/************** ADDED FOR GENERIC PRODUCT DISPLAY *********************/
#define MAX_N_COLOR 128   /* maximum number of colors that can be used */
#define MAX_N_LABEL 30
typedef char label_string[10];   



/* classes of products we can deal with */
#define GEOGRAPHIC_PRODUCT      0
#define NON_GEOGRAPHIC_PRODUCT  1
#define STANDALONE_TABULAR      2
#define RADAR_CODED_MESSAGE     3
#define TEXT_MESSAGE            4
#define UNKNOWN_MESSAGE       999

/* types of headers an ICD product can have */
#define HEADER_NONE    0
#define HEADER_WMO     1
#define HEADER_PRE_ICD 2
#define AUTO_DETECT -1

/* types of CVG raw data */
#define CVG_RAW_DATA_RADIAL 0
#define CVG_RAW_DATA_RASTER 1

/* online help files */
#define HELP_FILE_PREFS         "prefs.help"
#define HELP_FILE_FILE_SELECT   "fileselect.help"
#define HELP_FILE_PDLB_SELECT   "pdlbselect.help"
#define HELP_FILE_ILB_SELECT    "ilbselect.help"
#define HELP_FILE_DISK_SELECT   "diskfile.help"
#define HELP_FILE_PNG_OUT       "pngout.help"
#define HELP_FILE_GIF_OUT       "gifout.help"
#define HELP_FILE_PACKET_SELECT "packetselect.help"
#define HELP_FILE_MAIN_WIN      "main.help"
#define HELP_FILE_DISPLAY_WIN   "screen.help"
#define HELP_FILE_PROD_SPEC     "prod_config.help"
#define HELP_FILE_SITE_SPEC     "site_spec_info.help"
#define HELP_FILE_ANIM_OPT      "animation_opt.help"

/* orpg build of product data */
#define BUILD_4_AND_EARLIER 4
#define BUILD_5 5
#define BUILD_6 6
#define BUILD_7 7
#define BUILD_8 8 /*  and later */


/* product db list filter values */
#define FILTER_PROD_ID 1
#define FILTER_MNEMONIC 2
#define FILTER_P_CODE 3


/* selection of area component label format */
#define AREA_LBL_NONE 1
#define AREA_LBL_MNEMONIC 2
#define AREA_LBL_COMP_NUM 3
#define AREA_LBL_BOTH 4
/* selection of area component symbol type */
#define AREA_SYM_POINT 0
#define AREA_SYM_CIRCLE 1
#define AREA_SYM_SEGMENTED_CIRCLE 2
#define AREA_SYM_DIAMOND 3
#define AREA_SYM_SQUARE 4
/* selection of area component line type */
#define AREA_LINE_SOLID 1
#define AREA_LINE_DASH_CLEAR 2
#define AREA_LINE_DASH_BLACK 3
#define AREA_LINE_DOTTED 4


/*************************************************************************/
/* animation initialization type */
#define ANIM_NO_INIT 0  
#define ANIM_FULL_INIT 1 /*  displaying product, clearing screen, and  */
                         /*  if ANIM_NO_INIT when setting time series */
#define ANIM_CHANGE_MODE_INIT 2 /*  set when changing animation mode */
#define ANIM_CHANGE_MODE_NEW_TS_INIT 3 /*  set if user sets new new */
                         /*  base vol an loop after change mode, and */
                         /*  set when changing animation mode from AUTO_UPDATE */
                         /*  to TIME_SERIES */
#define ANIM_UPDATE_LIST_INIT 4 /*  set when updating sorted product list */

/* structure for animation variables */
#define MAX_HIST_SZ 20

typedef struct {
    
    int anim_type;
    int loop_size;
    int stop_anim;
    
    int reset_ts_flag;
    int reset_es_flag;
    int reset_au_flag;
    
    int anim_hist_size; /*  init to 0 */
    
    int es_first_index;
    int es_last_index;
    
    int ts_current_first_index; /* first and last volume message of the    */
    int ts_current_last_index;  /* currently displayed product. Set during */
                                /* time series initiation and when finding */
                                /* the next base product                   */
   
    int first_index_num; /* product db list index of first message lower volume */
    int last_index_num;  /* product db list index of last message upper volume */
    
    unsigned int lower_vol_num; /* sequence number of lower limit image */
    unsigned int lower_vol_time; /* int date-time of lower limit image */
    
    unsigned int upper_vol_num; /* sequence number of upper limit image */
    unsigned int upper_vol_time; /* int date-time of upper limit image */
                                 /* only set to -1 in anim.c and cvg.c */

    short prod_ids[MAX_HIST_SZ];
    short elev_nums[MAX_HIST_SZ];  /* used by time series and elevation series  */
                                   /* animation, modified by elevation animation */
    short list_index[MAX_HIST_SZ];
    
} anim_data;



/* contains the contents of the 16 threshold fields to be */
/* used in future custom display of header contents       */
typedef struct {
    unsigned short us01;
    unsigned short us02;
    unsigned short us03;
    unsigned short us04;
    unsigned short us05;
    unsigned short us06;
    unsigned short us07;
    unsigned short us08;
    unsigned short us09;
    unsigned short us10;
    unsigned short us11;
    unsigned short us12;
    unsigned short us13;
    unsigned short us14;
    unsigned short us15;
    unsigned short us16;
    short          s01;
    short          s02;
    short          s03;
    short          s04;
    short          s05;
    short          s06;
    short          s07;
    short          s08;
    short          s09;
    short          s10;
    short          s11;
    short          s12;
    short          s13;
    short          s14;
    short          s15;
    short          s16;
    unsigned int ui01;
    unsigned int ui02;
    unsigned int ui03;
    unsigned int ui04;
    unsigned int ui05;
    unsigned int ui06;
    unsigned int ui07;
    unsigned int ui08;
    unsigned int ui09;
    unsigned int ui10;
    unsigned int ui11;
    unsigned int ui12;
    unsigned int ui13;
    unsigned int ui14;
    unsigned int ui15;
    int          i01;
    int          i02;
    int          i03;
    int          i04;
    int          i05;
    int          i06;
    int          i07;
    int          i08;
    int          i09;
    int          i10;
    int          i11;
    int          i12;
    int          i13;
    int          i14;
    int          i15;
    float      f01;
    float      f02;
    float      f03;
    float      f04;
    float      f05;
    float      f06;
    float      f07;
    float      f08;
    float      f09;
    float      f10;
    float      f11;
    float      f12;
    float      f13;
    float      f14;
    float      f15;
} disp_threshold_t;

#define THRESHOLD_OFFSET  (96+60) /* 96 bytes skips over internal header */
#define THR_01_OFFSET (96+60)
#define THR_02_OFFSET (96+62)
#define THR_03_OFFSET (96+64)
#define THR_04_OFFSET (96+66)
#define THR_05_OFFSET (96+68)
#define THR_06_OFFSET (96+70)
#define THR_07_OFFSET (96+72)
#define THR_08_OFFSET (96+74)
#define THR_09_OFFSET (96+76)
#define THR_10_OFFSET (96+78)
#define THR_11_OFFSET (96+80)
#define THR_12_OFFSET (96+82)
#define THR_13_OFFSET (96+84)
#define THR_14_OFFSET (96+86)
#define THR_15_OFFSET (96+88)
#define THR_16_OFFSET (96+90)



/* contains the contents of the 10 parameter fields to be */
/* used in future custom display of header contents       */
typedef struct {
    unsigned short us01;
    unsigned short us02;
    unsigned short us03;
    unsigned short us04;
    unsigned short us05;
    unsigned short us06;
    unsigned short us07;
    unsigned short us08;
    unsigned short us09;
    unsigned short us10;
    short          s01;
    short          s02;
    short          s03;
    short          s04;
    short          s05;
    short          s06;
    short          s07;
    short          s08;
    short          s09;
    short          s10;
    unsigned int ui01;
    unsigned int ui02;
    unsigned int ui03;
    unsigned int ui04;
    unsigned int ui05;
    unsigned int ui06;
    unsigned int ui07;
    unsigned int ui08;
    unsigned int ui09;
    int          i01;
    int          i02;
    int          i03;
    int          i04;
    int          i05;
    int          i06;
    int          i07;
    int          i08;
    int          i09;
    float      f01;
    float      f02;
    float      f03;
    float      f04;
    float      f05;
    float      f06;
    float      f07;
    float      f08;
    float      f09;
} dep_parameter_t;

#define PDP_01_OFFSET (96+52)  /* 96 bytes skips over internal header */
#define PDP_02_OFFSET (96+54)
#define PDP_03_OFFSET (96+58)
#define PDP_04_OFFSET (96+92)
#define PDP_05_OFFSET (96+94)
#define PDP_06_OFFSET (96+96)
#define PDP_07_OFFSET (96+98)
#define PDP_08_OFFSET (96+100)
#define PDP_09_OFFSET (96+102)
#define PDP_10_OFFSET (96+104)



/*************************************************************************/
/* the following structures are used during the display of the */
/* radial/raster products and for the mouse click data info    */
/* using files: click.c, gui_display.c, and corresponding packet.c */


/*  populated by read_generic_legend and parse packets   */
/*  used by click and display generic legend             */
/*      this is being replaced by the new scale offset method    */
/*      which permits use of dynamic parameters in the product   */

#define NO_DECODE 0
#define FILE_PARAM 1  /* use the OFFSET SCALE parameters in the legend file */
#define PROD_PARAM 2  /* use the OFFSET SCALE parameters in the threshold levels */
#define LEG_VELOCITY 3/* velocity, use OFFSET SCALE in the file plus th.s01 */

/*          The following data are used during packet and legend display      */
/*          but local file variables are used in the preferences preview pane.*/
typedef struct {
    /* the following are set when reading legend configuration file */
    int          n_leg_colors;     /* Method 4, 5, 6 */
    int          decode_flag;      /* added CVG 8.7 Method 5 */
    
    float        leg_Scale;        /* added CVG 8.7 Method 5 */
    float        leg_Offset;       /* added CVG 8.7 Method 5 */
    unsigned int max_level;        /* added CVG 8.7 Method 5 */
    int          n_l_flags;        /* Method 1, 2, 3 ,4, 5, 6 */
    int          n_t_flags;        /* Method 1, 2, 3 ,4, 5, 6 */
    /* values from threshold level fields if dynamic scale offset used */
    /* the following are set during decoding of packet 16 and radial component */
    float        prod_Scale;       /* added CVG 8.7 Method 5 */
    float        prod_Offset;      /* added CVG 8.7 Method 5 */
    unsigned int prod_max_level;   /* added CVG 8.7 Method 5 */
    int          prod_n_l_flags;   /* added CVG 8.7 Method 5 */
    int          prod_n_t_flags;   /* added CVG 8.7 Method 5 */
    /* Retained to support original methods and reduce the number */
    /* of times the legend file is read.                          */
    unsigned int tot_n_levels;  /* Method 1, 2, 3 */
    int          leg_min_val;   /* Method 1 */
    int          leg_incr;      /* Method 1 */
    int          min_val_scale; /* Method 1 */
    int          incr_scale;    /* Method 1 */
} decode_params_t;

/* FUTURE WORK: to support methods 4 and 6, we will need unions for min and max:
 *              in order to calculate bar heights
 */
/*        union { 
 *           unsigned int ui;
 *           int          si;
 *           double       dd;
 *        } min;
 *
 *        union { 
 *           unsigned int ui;
 *           int          si;
 *           double       dd;
 *        } max;
 */


/*  populated by read_generic_legend  */
/*          The following data are used during packet and legend display      */
/*          but local file variables are used in the preferences preview pane.*/
typedef struct {
    /*  data level assigned colors */
    union {
       unsigned int ui_dl[MAX_N_COLOR+1]; /* also supports unsigned short & char */
       int          si_dl[MAX_N_COLOR+1]; /* also supports short & char */
       double       d_dl[MAX_N_COLOR+1];  /* also supports type float */
    } data;
    /*  corresponding colors */
    int          dl_clr[MAX_N_COLOR+1]; 
    /* used to minimize how many times the legend file is read   */
    label_string dl_label[MAX_N_COLOR+1]; /* corresponding threshold label */
} color_assign_t;



/* stores decoded digital radial data for data packet 16 */
typedef struct {
   int              number_of_radials;
   unsigned short **radial_data;
   int              data_elements;
   float           *azimuth_angle;
   float           *azimuth_width;
   float            range_interval;

   decode_params_t  decode;        /*  used by data click and generic legend */
   color_assign_t   color;         /*  used by generic method to assign colors */
} radial_dig;


/* stores decoded RLE radial data for data packet AF1F */
typedef struct {
   int              number_of_radials;
   /* variable for prod 59 decoding */
   unsigned short index_to_first_range_bin;
   unsigned short **radial_data;
   int              data_elements;
   float           *azimuth_angle;
   float           *azimuth_width;
   float            range_interval;
} radial_rle;


/* stores decoded raster data for data packet BA07*/
typedef struct {
   int              number_of_rows;
   int              number_of_columns;
   int              x_start;
   int              x_scale;
   int              y_start;
   int              y_scale;
   unsigned short **raster_data;
} raster_rle;


/* stores decoded raster precip data for data packet 17 */
typedef struct {
   int              number_of_rows;
   int              number_of_columns;
   unsigned short **raster_data;

   decode_params_t  decode;        /*  used by data click and generic legend */
   color_assign_t   color;         /*  used by generic method to assign colors */
} digital_precip_data;



enum d_type {
    DATA_UBYTE = 1,
    DATA_BYTE,
    DATA_USHORT,
    DATA_SHORT,
    DATA_UINT,
    DATA_INT,
    DATA_FLOAT,
    DATA_DOUBLE
};
typedef enum d_type  generic_d_type;


/* stores generic radial data arrays for generic component 41 */
typedef struct {
   float            range_interval;    /*  horizontal resolution */
   int              data_elements;     /*  number of bins in radial       */
   int              number_of_radials;   
   float           *azimuth_angle;     /*  leading azimuth for CVG */
   float           *azimuth_width;
   generic_d_type   type_data;     /*  used by data click and generic legend */
   decode_params_t  decode;        /*  used by data click and legend display*/
   color_assign_t   color;         /*  used by generic method to assign colors */
   union {   
       unsigned char   **ubyte_d;  /*  if type_data is DATA_UBYTE */
       unsigned short  **ushort_d; /*  if type_data is DATA_USHORT */
       unsigned int    **uint_d;   /*  if type_data is DATA_UINT */
       char            **byte_d;   /*  if type_data is DATA_BYTE */
       short           **short_d;  /*  if type_data is DATA_SHORT */
       int             **int_d;    /*  if type_data is DATA_INT */
       float           **float_d;  /*  if type_data is DATA_FLOAT */
       double          **double_d; /*  if type_data is DATA_DOUBLE */
   } data;
} generic_radial_data;



/*************************************************************************/
/* information about the displayable data availible in the current product */
/* used to avoid having to reload the product when being replotted         */
typedef struct 
{
  int  num_packets;  /* number of data elements in the arrays */
  int  index;        /* the offset of this layer in the packet select dialog box
              * it equals num_of_previous_packets + num_of_previous_layers */
  int *codes;        /* the packet codes of each packet */
  int *offsets;      /* each packet's offset into the current icd_product */
} layer_info;

/* information about the images that have previously been plotted, used to */
/* replot the image (via replay_history) for whatever reason               */
typedef struct
{
  /* the elements here are all copies of the similarly named screen data elements */
    char *icd_product;    /* data loaded from disk in ICD format */
    void *generic_prod_data;  /* the deserialized generic product structure */
                              /* must be freed with cvg_RPGP_product_free() */
    layer_info *layers;   /* info about the displayable parts of the product */
    int num_layers;
    /* selection made by the use in the packet select dialog */
    int packet_select_type, packet_select, layer_select;
} history_info;


/*************************************************************************/
/* screen specific info */
typedef struct {
    char *icd_product;     /* the currently loaded product data (byte-swapped) */
    void *generic_prod_data;  /* the deserialized generic product structure */
                              /* must be freed with cvg_RPGP_product_free() */
    /*  WE ASSUME THAT THERE IS ONLY ONE PACKET 28 IN A PRODUCT */
    int packet_28_offset;  /* offset in icd_product to packet 28 (if it exists)*/
    
    history_info* history; /* info needed to redisplay what's been displayed */
    int history_size;      /* Number of display actions since last screen clear */
    
    char *lb_filename;     /* full path of the linear buffer where the product */
                           /* is from.  ULL if product loaded from binary file. */

    Pixmap pixmap;         /* the offscreen image associated with this screen */
    Pixmap legend_pixmap;

    int this_image;        /* type (radial/raster) of the product just loaded */
                           /* set by skip_over_packet() and used by        */
                           /* check_overlay() to set overlay_flag          */
    int last_image;        /* type (radial/raster), in history, of the product */
                           /* displayed (plotted) on this screen               */
                           /* only one (radial/raster) product can be in  */
                           /* history - i.e., one displayed at a time.     */  
                           /* used in numerous places to determine behavior*/    
                           
    Widget tab_window;     /* the window for the TAB for this product  */
    
    int gab_displayed;     /* set whether a gab has been displayed, for export */
                           /* purposes NOTE: set when gab displayed but NOT used */
    int gab_page;          /* which page of the GAB is currently being displayed */
    
    int resolution;        /* resolution index for the current base image */
                           /* Generic radial products do not use this method of  */
                           /* obtaining the product resolion.  A temporary fix   */
                           /* is to reset the resolution field during display.   */
                           /* This is required so overlay products can get the   */
                           /* correct resolution                                 */
    
    float scale_factor;    /* selected zoom factor restricted to a series of*/
                           /* 2:1 or 1:2 ratios.                            */
                           
    int x_center_offset;   /* relative to the radar location, the offset to */
    int y_center_offset;   /* the center of the image as displayed.  The    */
                           /* offset is in "number of pixels / scale_factor" */
                           /* in order to keep the same geographic location */
                           /* centered when changing the zoom factor.       */
    
    Widget base_info;      /*info about the first displayed image (not overlays)*/
    Widget base_info_2;

    Widget data_display;   /* info about the location in the image was clicked */
    Widget data_display_2;

    Widget prod_descript;  /* displays product description */

    char rad_id[5];        /* 4-letter icao id of last base product displayed */
    float prev_lat;        /* Lat Lon of the previous product displayed */
    float prev_lon;
    
    int last_plotted;      /* whether the last product loaded was plotted */
                           /*          we may no longer use this variable */
                           
    layer_info *layers;    /* info about the displayable parts of the product */
    int num_layers;
    
    /* last selection made by the user in the packet select dialog */
    int packet_select_type, packet_select, layer_select;

    /* the folowing is filled out when the applicable 2D array packet is decoded */
    /* used to return information when the mouse is clicked on the plotted image */
    /* and to replot the image (via replay history) for whatever reason          */
    /* only one of these structures is populated at any one time                 */
    raster_rle  *raster;
    radial_rle  *radial;
    radial_dig     *dhr;
    digital_precip_data *digital_precip;
    generic_radial_data *gen_rad;
    
} screen_data;

#endif
