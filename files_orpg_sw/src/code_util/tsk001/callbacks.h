/* callbacks.h */

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:52:32 $
 * $Id: callbacks.h,v 1.12 2009/05/15 17:52:32 ccalvert Exp $
 * $Revision: 1.12 $
 * $State: Exp $
 */


#ifndef _CALLBACKS_H_
#define _CALLBACKS_H_

#include <X11/IntrinsicP.h>
#include <Xm/Xm.h>
#include <Xm/Label.h>
#include <Xm/DrawingA.h>
#include <Xm/ScrollBar.h>
#include <Xm/ScrolledW.h>
#include <Xm/RowColumn.h>
#include <Xm/MessageB.h>
#include <Xm/Form.h>
#include <Xm/XpmP.h>
#include <stdio.h>

#include <stdlib.h>
#include <string.h>



#include "packet_definitions.h"
#include "global.h"
#include "logos.h"




/*  pid of reade_db child */
extern pid_t new_pid;

extern int disp_width, disp_height;  /*  size of X display  */
extern int spacexy, screen1x, screen1y, screen2x, screen2y;
extern int screen3x, screen3y;

extern int x_legend_start;
extern int y_legend_start;

extern GC gc;
extern	Display	*display;
extern Dimension pwidth, pheight; 
extern Dimension width, height;
extern Widget shell;
extern Widget screen_1, screen_2;
extern Widget screen_3;

extern Widget port1, port2;
extern Widget port3;

extern Widget legend_frame_1, legend_frame_2;
extern Widget legend_frame_3;

extern int selected_screen;
extern Dimension  barwidth, sidebarwidth, sidebarheight;
extern Pixel black_color, white_color, grey_color;

extern Widget overlay_but;
extern Widget packet_button;


extern Widget mainwin, dshell1, dshell2;
extern Widget dshell3;
extern Pixel black_color, white_color, grey_color;
extern Widget legend_screen_1, legend_screen_2;
extern Widget legend_screen_3;

extern Widget img_size_opt1, img_size_opt2, img_size_menu1, img_size_menu2;
extern Widget img_size_opt3, img_size_menu3;

extern Widget img_sizebut1, img_sizebutnormal1, img_sizebut2, img_sizebutnormal2;
extern Widget img_sizebut3, img_sizebutnormal3;

extern int img_size;
/* CVG 9.0 */
extern int large_screen_flag, large_image_flag;

extern Widget radar_center1, radar_center2, radar_center3;
extern Widget mouse_center1, mouse_center2, mouse_center3;
extern Widget hide_ctr_loc_but1, hide_ctr_loc_but2, hide_ctr_loc_but3;
extern XmString hide_xmstr, show_xmstr;
extern int ctr_loc_icon_visible_flag;

extern screen_data *sd1, *sd2, *sd;
extern screen_data *sd3;

extern int verbose_flag;

extern Widget linked_toggle1, linked_toggle2;
extern Widget linked_toggle3;

extern int linked_flag;
extern int select_all_flag;

/* stuff for scrolling */

extern Widget vbar2, hbar2;
extern Widget vbar1, hbar1, vbar3, hbar3;

/* extern int scroll1_xpos, scroll1_ypos; */
extern int scroll2_xpos, scroll2_ypos;
extern int overlay_flag;

/* status of display attribute check boxes */

extern int transp_label_flag1, map_flag1, az_line_flag1, range_ring_flag1;
extern int transp_label_flag2, map_flag2, az_line_flag2, range_ring_flag2;
extern int transp_label_flag3, map_flag3, az_line_flag3, range_ring_flag3;

extern int linked_flag;  /* flag to determine if scrolling and data click are linked */

extern int prod_filter; /* determins which column product is filtered with */

/* overlay label display mode */
Boolean norm_format1, norm_format2, bkgd_format1, bkgd_format2;
Boolean norm_format3, bkgd_format3;


extern assoc_array_i *msg_type_list;
extern assoc_array_s *product_mnemonics;

extern float *resolution_number_list;

extern int number_of_resolutions;
extern Widget *optbut;


/* about box stuff */
Pixmap mts_logo_pix, nws_logo_pix, faa_logo_pix;
Widget mts_logo_label, nws_logo_label, faa_logo_label;

extern Widget compare_shell;
extern int compare_num;

/* cleanup of saved list */
extern XmString *xmstr_prodb_list;
extern int last_prod_list_size;
extern char *prod_list_msg; 

extern int orpg_build_i;



/* prototypes */
void expose_callback(Widget w,XtPointer client_data, XtPointer call_data);
void legend_expose_callback(Widget w,XtPointer client_data, XtPointer call_data);
void exit_callback(Widget w,XtPointer client_data, XtPointer call_data);

void orpg_build_callback(Widget w,XtPointer client_data,XtPointer call_data);

void screen1_radio_callback(Widget w,XtPointer client_data, XtPointer call_data);
void screen2_radio_callback(Widget w,XtPointer client_data, XtPointer call_data);
void screen3_radio_callback(Widget w,XtPointer client_data, XtPointer call_data);

void prodid_filter_callback(Widget w, XtPointer client_data, XtPointer call_data);
void type_filter_callback(Widget w, XtPointer client_data, XtPointer call_data);
void pcode_filter_callback(Widget w, XtPointer client_data, XtPointer call_data);

void select_all_graphic_callback(Widget w, XtPointer client_data, XtPointer call_data);
void overlayflag_callback(Widget w,XtPointer client_data,XtPointer call_data);
/* CVG 9.0 - added parameter */
void check_overlay(int overlay_non_geographic);

void vert_scroll_callback(Widget w, XtPointer client_data, XtPointer call_data);
void horiz_scroll_callback(Widget w, XtPointer client_data, XtPointer call_data);

void gab_page_up_callback(Widget w,XtPointer client_data, XtPointer call_data);

void display_tab_callback(Widget w, XtPointer client_data, XtPointer call_data);
void replot_image_callback(Widget w, XtPointer client_data, XtPointer call_data);

void sm_win_callback(Widget w, XtPointer client_data, XtPointer call_data);
void lg_win_callback(Widget w, XtPointer client_data, XtPointer call_data);
/* CVG 9.0 */
void calculate_small_screen();
void calculate_large_screen();


void zoom_callback(Widget w, XtPointer client_data, XtPointer call_data);

void draw_menu_callback(Widget w, XtPointer client_data, 
                                                  XtPointer call_data);
void img_center_menu_cb(Widget item, XtPointer client_data, XtPointer call_data);
void do_image_center(int mouse_xin, int mouse_yin, int but_num, int screen);
void center_image_callback(Widget w, XtPointer client_data, XtPointer call_data);



void disp_none_callback(Widget w, XtPointer client_data, XtPointer call_data);
void disp_r_callback(Widget w, XtPointer client_data, XtPointer call_data);
void disp_a_callback(Widget w, XtPointer client_data, XtPointer call_data);
void disp_ra_callback(Widget w, XtPointer client_data, XtPointer call_data);
void disp_m_callback(Widget w, XtPointer client_data, XtPointer call_data);
void disp_ma_callback(Widget w, XtPointer client_data, XtPointer call_data);
void disp_mr_callback(Widget w, XtPointer client_data, XtPointer call_data);
void disp_all_callback(Widget w, XtPointer client_data, XtPointer call_data);


void img_large_callback(Widget w, XtPointer client_data, XtPointer call_data);
void img_small_callback(Widget w, XtPointer client_data, XtPointer call_data);


void clear_screen_callback(Widget w,XtPointer client_data, XtPointer call_data);
void clear_screen_data(int screen, int plot_image);

void linked_callback(Widget w, XtPointer client_data, XtPointer call_data);

void show_pixmap(int screen_num);
void clear_pixmap(int screen_num);
void legend_show_pixmap(int screen_num);

void legend_copy_pixmap(int screen_num);
void legend_clear_pixmap(int screen_num);
void replot_image(int screen_num);

extern void add_or_page_gab(int screen);
extern void add_tab(int screen);

void norm_format_Callback(Widget w, XtPointer client_data, XtPointer call_data);
void bkgd_format_Callback(Widget w, XtPointer client_data, XtPointer call_data);


extern void write_orpg_build(int build);


extern int cvg_RPGP_product_free (void *prod);

extern void dispatch_packet_type(int packet, int offset); 
extern int transfer_packet_code(unsigned int val);
extern void clear_history(history_info **hist, int *hist_size);

extern void delete_layer_info(layer_info *linfo, int size);
extern void delete_packet_16();
extern void delete_packet_17();
extern void delete_raster();
extern void delete_radial_rle();
extern void delete_generic_radial();

extern void draw_az_lines(int screen);
extern void draw_range_rings(int screen);
extern void display_map(int screen);

extern void replay_history(history_info *hist, int hist_size, int screen_num);


extern void reset_time_series(int screen_num, int type_init);
extern void reset_elev_series(int screen_num, int type_init);
extern void reset_auto_update(int screen_num, int type_init);

extern void reset_screen_size(int screen_num);

extern void center_screen(Widget vertical_bar, Widget horizontal_bar);

extern void display_compare_product();


#endif
