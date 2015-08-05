/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:52:38 $
 * $Id: gui_display.h,v 1.5 2009/05/15 17:52:38 ccalvert Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */
/* gui_display.h */




#ifndef _GUI_CVG_H_
#define _GUI_CVG_H_

#include <X11/IntrinsicP.h>
#include <Xm/Xm.h>
#include <Xm/MwmUtil.h>
#include <Xm/Form.h>
#include <Xm/MainW.h>
#include <Xm/PushB.h>
#include <Xm/Label.h>
#include <Xm/Scale.h>
#include <Xm/RowColumn.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/ToggleB.h>
#include <Xm/ScrolledW.h>
#include <Xm/ScrollBar.h>
#include <Xm/DrawingA.h>
#include <Xm/CascadeB.h>
#include <Xm/BulletinB.h>
#include <Xm/PushBG.h>
#include <Xm/Separator.h>
#include <Xm/Frame.h>

/*merge error */
#include <Xm/MessageB.h>

#include <stdlib.h>

#include <stdio.h>

#include "global.h"

extern assoc_array_i *msg_type_list;


extern Widget shell;

extern Widget s1_radio, s2_radio;
extern Widget s3_radio;

Widget dshell1, dshell2, vbar2, hbar2; /*  used externally */
Widget dshell3;  /*  used externally */
Widget vbar1, hbar1;
Widget vbar3, hbar3;

/* for linked scrolling */
Widget port2; /*  used externally */
Widget port1;
Widget port3;

Widget zoom_opt, zoom_menu, zoombut, zoombutnormal, zsep;
Widget zoom_opt1, zoom_opt2, zoom_opt3;
Widget zoombutnormal1, zoombutnormal2, zoombutnormal3;


Widget win_size_opt, win_size_menu, sm_winbut, lg_winbut;

Widget radar_center1, radar_center2, radar_center3;
Widget mouse_center1, mouse_center2, mouse_center3;
Widget hide_ctr_loc_but1, hide_ctr_loc_but2, hide_ctr_loc_but3;
extern XmString hide_xmstr, show_xmstr;
extern int ctr_loc_icon_visible_flag;

/*Image Size Option */
Widget img_size_opt, img_size_menu;
Widget img_size_opt1, img_size_opt2, 
       img_size_menu1, img_size_menu2; /*  used externally */
Widget img_size_opt3, 
       img_size_menu3; /*  used externally */
       
Widget img_sizebut, img_sizebutnormal;
Widget img_sizebut1, img_sizebutnormal1, 
       img_sizebut2, img_sizebutnormal2; /*  used externally */
Widget img_sizebut3, img_sizebutnormal3; /*  used externally  */

extern int img_size;

/* CVG 9.0 */
extern int large_screen_flag, large_image_flag;


extern int screen1x, screen1y, screen2x, screen2y;
extern int screen3x, screen3y;

/* Screens Linked Toggle */
Widget linked_toggle; 
Widget linked_toggle1, linked_toggle2; /*  used externally */
Widget linked_toggle3;

extern int linked_flag;  /* flag to determine if scrolling and data click are linked */

Widget disp_opt, disp_menu, dispbut1, dispbut2, dispbut3, dsep;
Widget dispbut4, dispbut5, dispbut6, dispbut7, dispbut0;


Widget anim_opt, anim_menu, animbut1, animbut2, animbut3, animbut4, asep;

Widget ovly_label_opt, ovly_label_menu, label_clearbut, label_blackbut;

extern Boolean norm_format1, norm_format2, bkgd_format1, bkgd_format2;
extern Boolean norm_format3, bkgd_format3;

extern Widget screen_1, screen_2;
extern Widget screen_3;
extern Widget legend_screen_1, legend_screen_2;
extern Widget legend_screen_3;
extern Widget legend_frame_1, legend_frame_2; 
extern Widget legend_frame_3;

extern Display *display;
extern Pixel black_color, white_color, green_color, red_color, grey_color; 
extern Pixel blue_color, yellow_color;
extern GC gc;

extern int selected_screen;
int     is_zoomed = 0; /*  in click.h, not click.c */

int 	compare_num;

Widget compare_shell, compare_draw;
Widget compare_hbar, compare_vbar;
Widget compare_data_display, compare_data_display_2;
Widget compare_screennum;
Widget compare_base_info_2, compare_datetime;

Widget compare_prod_descript;

extern int scroll1_xpos, scroll1_ypos, scroll2_xpos, scroll2_ypos;
extern Dimension       width,height;
extern Dimension       bwidth,bheight;
extern Dimension       pwidth,pheight;
extern Dimension       barwidth, sidebarwidth, sidebarheight;

extern XtAppContext    app;

Dimension animb_height=20, animb_width=30;

extern screen_data *sd1, *sd2, *sd;
extern screen_data *sd3;
extern int verbose_flag;


extern int transp_label_flag1, map_flag1, az_line_flag1, range_ring_flag1;
extern int transp_label_flag2, map_flag2, az_line_flag2, range_ring_flag2;
extern int transp_label_flag3, map_flag3, az_line_flag3, range_ring_flag3;



/* prototypes */

void open_display_screen(int screen_num);

void screenkill_callback(Widget w, XtPointer client_data, XtPointer call_data);
void setup_default_screen_data_values(screen_data *the_sd);

void reset_screen_size(int screen_num);

void center_screen(Widget vertical_bar, Widget horizontal_bar);

void compare_screen_callback(Widget w,XtPointer client_data, XtPointer call_data);
void toggle_compare_callback(Widget w,XtPointer client_data, XtPointer call_data);
void expose_compare_callback(Widget w, XtPointer client_data, XtPointer call_data);
void comparekill_callback(Widget w, XtPointer client_data, XtPointer call_data);
void display_compare_product();

extern void draw_menu_callback(Widget w, XtPointer client_data, 
                                                  XtPointer call_data);
extern void img_center_menu_cb(Widget item, XtPointer client_data, 
                                                  XtPointer call_data);


static void click_info(Widget w, XButtonEvent *event, String *params, 
                                                           Cardinal *num_params);
void update_compare_click_info();


/*#ifdef SUNOS*/
static void compare_click_info(Widget w, XButtonEvent *event, String *params, 
                                                           Cardinal *num_params);
/*#endif*/


extern void do_click_info(int screen_num, int inx, int iny);

extern void clear_history(history_info **hist, int *hist_size);

extern int cvg_RPGP_product_free (void *prod);

extern void clear_pixmap(int screen_num);
extern void legend_clear_pixmap(int screen_num);
extern void expose_callback(Widget w, XtPointer client_data, XtPointer call_data);
extern void legend_expose_callback(Widget w, XtPointer client_data, XtPointer call_data);

extern void vert_scroll_callback(Widget w, XtPointer client_data, XtPointer call_data);
extern void horiz_scroll_callback(Widget w, XtPointer client_data, XtPointer call_data);

extern void gab_page_up_callback(Widget w, XtPointer client_data, XtPointer call_data);
extern void display_tab_callback(Widget w, XtPointer client_data, XtPointer call_data);
extern void replot_image_callback(Widget w, XtPointer client_data, XtPointer call_data);

extern void center_image_callback(Widget w, XtPointer client_data, XtPointer call_data);

extern void sm_win_callback(Widget w, XtPointer client_data, XtPointer call_data);
extern void lg_win_callback(Widget w, XtPointer client_data, XtPointer call_data);
/* CVG 9.0 */
extern void calculate_small_screen();
extern void calculate_large_screen();

extern void zoom_callback(Widget w, XtPointer client_data, XtPointer call_data);

extern void img_large_callback(Widget w, XtPointer client_data, XtPointer call_data);
extern void img_small_callback(Widget w, XtPointer client_data, XtPointer call_data);

extern void linked_callback(Widget w, XtPointer client_data, XtPointer call_data);

extern void disp_none_callback(Widget w, XtPointer client_data, XtPointer call_data);
extern void disp_r_callback(Widget w, XtPointer client_data, XtPointer call_data);
extern void disp_a_callback(Widget w, XtPointer client_data, XtPointer call_data);
extern void disp_ra_callback(Widget w, XtPointer client_data, XtPointer call_data);
extern void disp_m_callback(Widget w, XtPointer client_data, XtPointer call_data);
extern void disp_ma_callback(Widget w, XtPointer client_data, XtPointer call_data);
extern void disp_mr_callback(Widget w, XtPointer client_data, XtPointer call_data);
extern void disp_all_callback(Widget w, XtPointer client_data, XtPointer call_data);

extern void norm_format_Callback(Widget w, XtPointer client_data, XtPointer call_data);
extern void bkgd_format_Callback(Widget w, XtPointer client_data, XtPointer call_data);

extern void clear_screen_callback(Widget w,XtPointer client_data, XtPointer call_data);

extern void gif_output_file_select_callback(Widget w, XtPointer client_data, 
                                                                  XtPointer call_data);
extern void png_output_file_select_callback(Widget w, XtPointer client_data, 
                                                                  XtPointer call_data);

/* animation callbacks and functions */
extern void back_one_callback(Widget w,XtPointer client_data, XtPointer call_data);
extern void fwd_one_callback(Widget w,XtPointer client_data, XtPointer call_data);
extern void fwd_play_callback(Widget w,XtPointer client_data, XtPointer call_data);
extern void stop_anim_callback(Widget w,XtPointer client_data, XtPointer call_data);
extern void back_play_callback(Widget w,XtPointer client_data, XtPointer call_data);

extern void select_time_callback(Widget w,XtPointer client_data, XtPointer call_data);
extern void select_elev_callback(Widget w,XtPointer client_data, XtPointer call_data);
extern void select_update_callback(Widget w,XtPointer client_data, XtPointer call_data);
extern void select_file_callback(Widget w,XtPointer client_data, XtPointer call_data);

extern void reset_time_series(int screen_num, int type_init);
extern void reset_elev_series(int screen_num, int type_init);
extern void reset_auto_update(int screen_num, int type_init);

extern void time_option_callback(Widget w, XtPointer client_data, XtPointer call_data);
extern void file_option_callback(Widget w, XtPointer client_data, XtPointer call_data);

extern void delete_layer_info(layer_info *linfo, int size);
extern void delete_packet_16();
extern void delete_packet_17();
extern void delete_raster();
extern void delete_radial_rle();

extern void delete_generic_radial();

extern  char *_88D_secs_to_string(int time);
extern void calendar_date (short date,int *dd, int *dm,	int *dy );

#endif
