/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/03/25 13:51:41 $
 * $Id: prod_display.h,v 1.7 2014/03/25 13:51:41 steves Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */
/* prod_display.h */


#ifndef _PROD_DISPLAY_H_
#define _PROD_DISPLAY_H_

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include "global.h"
#include "packet_definitions.h"


/* #define MAX_PRODUCTS 16000 */

#define HEADER_ID_OFFSET   (96+12)

/* the following are defined in global.h */
/*           96 bytes skips over internal header   */
/* #define THRESHOLD_OFFSET  (96+60) */
/* #define THR_01_OFFSET (96+60)     */
/* #define THR_02_OFFSET (96+62)     */
/* #define THR_03_OFFSET (96+64)     */
/* #define THR_04_OFFSET (96+66)     */
/* #define THR_05_OFFSET (96+68)     */
/* #define THR_06_OFFSET (96+70)     */
/* #define THR_07_OFFSET (96+72)     */
/* #define THR_08_OFFSET (96+74)     */
/* #define THR_09_OFFSET (96+76)     */
/* #define THR_10_OFFSET (96+78)     */
/* #define THR_11_OFFSET (96+80)     */
/* #define THR_12_OFFSET (96+82)     */
/* #define THR_13_OFFSET (96+84)     */
/* #define THR_14_OFFSET (96+86)     */
/* #define THR_15_OFFSET (96+88)     */
/* #define THR_16_OFFSET (96+90)     */
/* #define PDP_01_OFFSET (96+52)     */
/* #define PDP_02_OFFSET (96+54)     */
/* #define PDP_03_OFFSET (96+58)     */
/* #define PDP_04_OFFSET (96+92)     */
/* #define PDP_05_OFFSET (96+94)     */
/* #define PDP_06_OFFSET (96+96)     */
/* #define PDP_07_OFFSET (96+98)     */
/* #define PDP_08_OFFSET (96+100)    */
/* #define PDP_09_OFFSET (96+102)    */
/* #define PDP_10_OFFSET (96+104)    */

extern Widget id_label;

extern Display *display;
extern Window window;
extern Widget screen_1, screen_2;
extern Widget screen_3;
extern screen_data *sd1, *sd2, *sd;
extern screen_data *sd3;

extern Widget zoom_opt1, zoom_opt2, zoom_opt3;
extern Widget zoombutnormal1, zoombutnormal2, zoombutnormal3;


extern int verbose_flag;

extern int overlay_flag;

extern GC gc;
extern Dimension pwidth, pheight; 
extern Dimension  barwidth, sidebarwidth, sidebarheight;
extern Pixel black_color, white_color, grey_color;

extern int  map_flag1, az_line_flag1, range_ring_flag1;
extern int  map_flag2, az_line_flag2, range_ring_flag2;
extern int  map_flag3, az_line_flag3, range_ring_flag3;
extern int ctr_loc_icon_visible_flag;


/* preference information */
extern assoc_array_i *product_res;
extern assoc_array_s *product_names;
extern assoc_array_i *digital_legend_flag;
extern assoc_array_s *legend_units;
extern assoc_array_s *digital_legend_file;
extern assoc_array_s *dig_legend_file_2;
extern assoc_array_s *icao_list;
extern assoc_array_s *short_prod_names;
extern assoc_array_i *radar_type_list;
extern assoc_array_s *radar_names;
extern assoc_array_i *msg_type_list;
extern assoc_array_i *elev_flag; /* CVG 9.3 - added elevation flag */

extern assoc_array_s *product_mnemonics;

extern char **resolution_name_list;

extern assoc_array_s *configured_palette;
extern assoc_array_s *config_palette_2;



/* some of the following are temporary comments included as info only */
/* some should be declared locally once implementation choices made */

#define TOT_BAR_HT 512    /* Total color bar height is 512 pixels maximum     */
#define MAX_BAR_HT 28     /* Maximum single bar height= 28 pixels  */
#define MIN_FLAG_HT 12    /* Minimum bar height for flag values = 12 pixels   */


/* declared in cvg.h */
extern int palette_size;  /* the number of colors in the palette */
extern int global_palette_size_1;
extern int global_palette_size_2;
extern int global_palette_size_3;
extern int global_packet_list[100];
extern int global_packet_list_2[100];
extern int global_packet_list_3[100];

extern XColor global_display_colors_1[256];
extern XColor global_display_colors_2[256];
extern XColor global_display_colors_3[256];


extern char config_dir[255];
/* CVG 9.0 - added for locally setting up a palette */
extern XColor display_colors[256];

/* cvg 9.1 - set to TRUE in colors.h if there is a configured palette for a 2-d array */
int display_color_bars;


/*  globals used in reading legend file and displaying legend blocks */
/*  reduces number of parameters to subroutines */
int font_height = 12;  /* this may not always be true */ 

int x_legend_start;
int y_legend_start;

/* actually a local variable in two separate functions */
/* unsigned long max_num_levels; */ /*  based upon array type, Methods 1, 2, 3, 5 */

/* tot_num_lvls if needed for generic method, */
/* is calculated from maximum value           */
extern unsigned long tot_num_lvls;/* Number of data levels incl flags, Methods 1, 2, 3*/
                                  /* used in DEBUG print */

/* declared in prod_disp_legend.h */
extern int num_lead_flags;  /*  all Methods */  /* used in DEBUG print */
extern int num_trail_flags; /*  all Methods */  /* used in DEBUG print */
extern int leg_min_val;       /*  min data value - Method 1 only */
extern int leg_inc;           /*  increment between values, Method 1 only */
extern int leg_min_val_scale; /*  scale of min data value, Method 1 only */
extern int leg_inc_scale;     /*  scale of increment, Method 1 only */
extern unsigned short thresh[16]; 





/* ////////////////////////////////////////////////////// */



extern Widget compare_shell;
extern int compare_num;

extern int orpg_build_i;

/*  prototypes ----  */

void plot_image(int screen_num, int add_history);

/* CVG 9.0 - added no_blocks_flag */
void display_legend(Drawable canvas, int legend_offset, int no_blocks_flag, 
                                                              int long_display_flag);
void display_product_info();

/* CVG 9.0 - added "local_color" parameter */
void display_legend_blocks(Drawable canvas, int x, int in_y, int local_color, 
                                                               int frame_location);
/* CVG 9.0 - added */
int open_legend_block_palette(int prod_id, int frame_loc);



extern void display_calculated_dig_labels(Drawable canvas, int h,
                                 int x, int y, int frame_loc);
extern void display_explicit_dig_labels(Drawable canvas, int h,
                                 int x, int y, int frame_loc);
extern void draw_digital_color_bars(Drawable canvas, int h,
                                 int x, int y, int frame_loc);
                                 

extern int read_generic_legend(FILE *leg_file, char *generic_legend_filename, 
                                 int method, int frame_location);
/* CVG 9.0 - added prod_id param */
extern void display_generic_legend(Drawable canvas, int n_val_hgt, int x_in, int y_in,
                                 int method, int prod_id, int frame_location);


extern int read_digital_legend(FILE *leg_file, char *digital_legend_filename, 
                                               int method, int frame_location);
extern void display_rle_labels(Drawable canvas, int h_in, int x_in, int y_in,  
                             int frame_location);
extern void draw_rle_color_bars(Drawable canvas, int h_in, int x_in, int y_in,  
                             int frame_location);

void add_or_page_gab(int screen);
void add_tab(int screen);

extern short get_elev_ind(char *bufptr, int orpg_build);
 
extern void dispatch_packet_type(int packet, int offset, int replay);

extern int transfer_packet_code(unsigned int val);
extern float res_index_to_res(int res);

extern int temporary_set_screen_res_index(float res);

extern short read_half(char *buffer,int *offset);

/* CVG 9.0 - changed function name from setup_colors */
extern void open_config_or_default_palette(int packet_type, int legend_flag);
/* CVG 9.0 - added */
extern int open_default_palette(int packet_type);

extern void setup_palette(FILE *the_palette_file, int packet_type, int legend_block);
extern void read_to_eol(FILE *list_file, char *buf);

extern void draw_az_lines(int screen);
extern void draw_range_rings(int screen);
extern void display_map(int screen);
extern void clear_history(history_info **hist, int *hist_size);
extern void save_current_state_to_history(history_info **hist, int *hist_size);
extern void delete_packet_16();
extern void delete_packet_17();
extern void delete_raster();
extern void delete_radial_rle();

extern void delete_generic_radial();
extern void read_to_eol(FILE *list_file, char *buf);

extern void clear_pixmap(int screen_num);
extern void legend_show_pixmap(int screen_num);
extern void legend_clear_pixmap(int screen_num);

extern void legend_copy_pixmap(int screen_num);

extern float res_index_to_res(int res);

extern	char		*_88D_date_to_string (short date);
extern  char            *_88D_secs_to_string(int time);
extern void julian_date( int year, int month, int day, int *julian_date );
extern void calendar_date (short date,int *dd, int *dm,	int *dy );

extern int test_for_icd(short div, short ele, short vol, int silent);

extern void display_compare_product();

#endif 
