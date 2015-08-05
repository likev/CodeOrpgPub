/* prefs.h */

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/03/25 13:51:40 $
 * $Id: prefs.h,v 1.15 2014/03/25 13:51:40 steves Exp $
 * $Revision: 1.15 $
 * $State: Exp $
 */
 
 
#ifndef _PREFS_H_
#define _PREFS_H_

#include <Xm/Xm.h>
#include <Xm/MwmUtil.h>
#include <Xm/TextF.h>
#include <Xm/PushB.h>
#include <Xm/Label.h>
#include <Xm/Form.h>
#include <Xm/List.h>
#include <Xm/FileSB.h>
#include <Xm/ToggleB.h>
#include <Xm/MessageB.h>
#include <Xm/RowColumn.h>
#include <Xm/Separator.h>
#include <Xm/SelectioB.h>
#include <Xm/Frame.h>
#include <Xm/DrawingA.h>

#include <Xm/LabelG.h>
#include <Xm/Text.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <prod_gen_msg.h>
#include "palette.h"
#include "global.h"


extern int standalone_flag;

/*  pid of read_ddatabase child */
extern pid_t new_pid;


extern int maxProducts;

extern int sort_method;
extern int prev_sort_method;

extern int use_cvg_list_flag;
extern int prev_cvg_list_flag;

extern char config_dir[255];   /* the path in which the configuration files are kept */
extern char global_config_dir[255]; /* the location of the default configuration files */
extern char product_database_filename[256];
extern char prev_db_filename[256];
extern char map_filename[255]; /* the full file name of the map overlay file */


extern assoc_array_i *pid_list;

extern assoc_array_s *icao_list;

extern assoc_array_s *product_names;
extern assoc_array_s *short_prod_names;
extern assoc_array_s *product_mnemonics;


extern int road_d, rail_d, admin_d, co_d;
extern Widget screen_1, screen_2, screen_3;



extern Dimension       width,height;
extern Dimension       barwidth, sidebarwidth, sidebarheight;

extern Widget db_dialog;


Widget mapfile_text, verbose_toggle, rangering_toggle, azilines_toggle, mapback_toggle;
Widget databasefile_text;


Widget sort_method_opt, sort_method_menu;
Widget sortm_but1, sortm_but2;

/*  also set by product_names if lb not present */
Widget list1_but, list2_but, desc_list_radiobox;
/* CVG 9.0 */
Widget scr_small_radio, scr_large_radio, scr_size_radiobox;
Widget img_small_radio, img_large_radio, img_size_radiobox;

/* Product Info widgets */
Widget msgtype_opt, msgtype_menu, msgt_but_set;
Widget msgt_but0, msgt_but1, msgt_but2, msgt_but3;
Widget msgt_but4, msgt_but999, msgt_but1neg;

/* CVG 9.1 - added packet 1 coord override for geographic products */
Widget packet_1_opt, packet_1_menu, pkt1_but_set;
Widget pkt1_but0, pkt1_but1;

Widget resindex_opt, resindex_menu, resi_but_set;
Widget resi_but0, resi_but1, resi_but2, resi_but3;
Widget resi_but4, resi_but5, resi_but6, resi_but7;
Widget resi_but8, resi_but9, resi_but10, resi_but11;
Widget resi_but12, resi_but13, resi_but1neg;

Widget digflag_opt, digflag_menu, digf_but_set;
Widget digf_but0, digf_but1, digf_but2, digf_but3, digf_but1neg;

/* to be used for generic signed int ungigned int and float type */
Widget digf_but4, digf_but5, digf_but6; 

Widget assocpacket_opt, assocpacket_menu, assp_but_set;
Widget assp_but0,   assp_but4, assp_but6, assp_but8, assp_but9, assp_but10;
/*  possibly add 2 - Text Symb SYMBOL no value */
/*  possibly add 7 - Unlinked Vector no value */
Widget assp_but16, assp_but17, assp_but18, assp_but20; 
Widget assp_but41, assp_but42, assp_but43;

Widget assp_but51;

Widget assp_but53, assp_but54, assp_but55;


Pixmap pref_legend_pix;
Widget pref_legend_draw;


Widget pi_list, id_label, ledg_label;
Widget label_id, label_mt, label_pr, label_df, label_l1;
Widget label_l2, label_p1, label_p2, label_pt, label_um;
/* CVG 9.1 - packet 1 coord override geographic products */
Widget label_pk;

/* CVG 9.1 - added override of colors for non-2d array packets */
Widget overridepacket_opt, overridepacket_menu, over_but_set;
Widget over_but0, over_but4, over_but6, over_but8, over_but9, over_but10;
Widget over_but20, over_but43, over_but51;
Widget override_palette_text;
Widget label_o_pal, label_o_pkt;

/* CVG 9.3 - added elevation flag */
Widget elflag_opt, elflag_menu, label_el, elf_but_set;
Widget elf_but0, elf_but1;

Widget diglegfile_text, confpalette_text, unit_text; 
Widget diglegfile2_text, confpalette2_text;


/* Site Info widgets */
Widget si_list, site_id_label, icao_text;

Widget rdrtype_opt, rdrtype_menu, rdrt_but_set;
Widget rdrt_but0, rdrt_but4; 
/*  possible future radar types */
  Widget rdrt_but1, rdrt_but2, rdrt_but3; 



/* Area Comp sidgets */
Widget area_prev_draw;
Pixmap area_prev_pix;






/* default setting values for display attributes */
extern Boolean def_ring_val, def_map_val, def_line_val;

extern int verbose_flag;
/* CVG 9.0 */
extern int large_screen_flag, large_image_flag;

extern int area_label;
extern int area_symbol;
extern int area_line_type;
extern int include_points_flag;

extern Widget shell;
extern GC gc;
extern Pixel black_color, white_color, green_color, red_color, grey_color, blue_color, yellow_color;

/* prototypes */


void pref_window_callback(Widget w, XtPointer client_data, XtPointer call_data);

void choose_mapfile_callback(Widget w, XtPointer client_data, XtPointer call_data);
void mapfile_ok_callback(Widget w, XtPointer client_data, XtPointer call_data);
void mapfile_cancel_callback(Widget w, XtPointer client_data, XtPointer call_data);

void choose_pdlbfile_callback(Widget w, XtPointer client_data, XtPointer call_data);
void pdlbfile_ok_callback(Widget w, XtPointer client_data, XtPointer call_data);
void pdlbfile_cancel_callback(Widget w, XtPointer client_data, XtPointer call_data);


void list1_radio_callback(Widget w, XtPointer client_data, XtPointer call_data);
void list2_radio_callback(Widget w, XtPointer client_data, XtPointer call_data);
/* CVG 9.0 */
void small_scr_callback(Widget w, XtPointer client_data, XtPointer call_data);
void large_scr_callback(Widget w, XtPointer client_data, XtPointer call_data);
void small_img_callback(Widget w, XtPointer client_data, XtPointer call_data);
void large_img_callback(Widget w, XtPointer client_data, XtPointer call_data);

/* CVG 9.0 */
extern void create_db_list_label(XmString *lst_lbl_str);


/* extern void sortm_cb(Widget w, XtPointer client_data, XtPointer call_data); */
extern void write_sort_method(int method);
extern void write_descript_source(int use_cvg_list);

void pref_ok_callback(Widget w, XtPointer client_data, XtPointer call_data);
/* CVG 9.0 - added scr_sz and img_sz parameters */
extern void write_prefs_file(FILE *p_file, char *m_file, char *rr, char *az, 
                    char *map, char *vb, char *scr_sz, char *img_sz, char *db_file);

extern void load_program_prefs(int initial_read);
extern void load_product_names(int from_child, int initial_read);
void pref_cancel_callback(Widget w, XtPointer client_data, XtPointer call_data);

void product_info_edit_window_callback(Widget w, XtPointer client_data, XtPointer call_data);
extern void product_edit_select_callback(Widget w, XtPointer client_data, XtPointer call_data);
extern void product_edit_revert_callback(Widget w, XtPointer client_data, XtPointer call_data);
extern void product_edit_fill_fields();
extern void product_edit_commit_callback(Widget w, XtPointer client_data, XtPointer call_data);
extern void product_edit_save_callback(Widget w, XtPointer client_data, XtPointer call_data);
extern void product_edit_add_callback(Widget w, XtPointer client_data, XtPointer call_data);
extern void product_edit_add_cancel_callback(Widget w, XtPointer client_data, XtPointer call_data);
extern void product_edit_add_ok_callback(Widget w, XtPointer client_data, XtPointer call_data);
void pref_legend_expose_cb(Widget w, XtPointer client_data, XtPointer call_data);
void prod_edit_kill_cb(Widget w, XtPointer client_data, XtPointer call_data);
void pref_legend_show_pixmap();
void pref_legend_clear_pixmap();
void pref_legend_grey_pixmap();

extern void msgt_cb(Widget w, XtPointer client_data, XtPointer call_data);
/* CVG 9.1 - added packet 1 coord override for geographic products */
extern void pkt1_cb(Widget w, XtPointer client_data, XtPointer call_data);
/* CVG 9.1 - added override of colors for non-2d array packets */
extern void overp_cb(Widget w, XtPointer client_data, XtPointer call_data);

extern void resi_cb(Widget w, XtPointer client_data, XtPointer call_data);
extern void digf_cb(Widget w, XtPointer client_data, XtPointer call_data);
extern void assp_cb(Widget w, XtPointer client_data, XtPointer call_data);
/* CVG 9.3 - added elevation flag */
extern void elflag_cb(Widget w, XtPointer client_data, XtPointer call_data);

void site_info_edit_window_callback(Widget w, XtPointer client_data, XtPointer call_data);
extern void site_edit_select_callback(Widget w, XtPointer client_data, XtPointer call_data);
extern void site_edit_revert_callback(Widget w, XtPointer client_data, XtPointer call_data);
extern void site_edit_fill_fields();
extern void site_edit_commit_callback(Widget w, XtPointer client_data, XtPointer call_data);
extern void site_edit_save_callback(Widget w, XtPointer client_data, XtPointer call_data);
extern void site_edit_add_callback(Widget w, XtPointer client_data, XtPointer call_data);
extern void site_edit_add_cancel_callback(Widget w, XtPointer client_data, XtPointer call_data);
extern void site_edit_add_ok_callback(Widget w, XtPointer client_data, XtPointer call_data);



extern void rdrt_cb(Widget w, XtPointer client_data, XtPointer call_data);


void area_comp_opt_window_callback(Widget w, XtPointer client_data, XtPointer call_data);
extern void area_ok_callback(Widget w, XtPointer client_data, XtPointer call_data);
extern void a_label_radio_callback(Widget w, XtPointer client_data, XtPointer call_data);
extern void a_symbol_radio_callback(Widget w, XtPointer client_data, XtPointer call_data);
extern void a_line_radio_callback(Widget w, XtPointer client_data, XtPointer call_data);
extern void a_line_points_cb(Widget w, XtPointer client_data, XtPointer call_data);
void area_prev_expose_cb(Widget w, XtPointer client_data, XtPointer call_data);
void area_opt_kill_cb(Widget w, XtPointer client_data, XtPointer call_data);
void area_prev_show_pixmap();
void area_prev_clear_pixmap();
void area_prev_grey_pixmap();

extern void display_area(int index, int location_flag);

void table_comp_opt_window_callback(Widget w, XtPointer client_data, XtPointer call_data);


extern void replot_image(int screen_num);
void map_opt_window_callback(Widget w, XtPointer client_data, XtPointer call_data);
void map_ok_callback(Widget w, XtPointer client_data, XtPointer call_data);

void road_radio_callback(Widget w, XtPointer client_data, XtPointer call_data);
void rail_radio_callback(Widget w, XtPointer client_data, XtPointer call_data);

void county_radio_callback(Widget w, XtPointer client_data, XtPointer call_data);


extern void help_window_callback(Widget w, XtPointer client_data, XtPointer call_data);


extern void build_msg_list(int err);

#endif

