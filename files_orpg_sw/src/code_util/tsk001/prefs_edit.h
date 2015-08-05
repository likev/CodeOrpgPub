/* prefs_edit.h */

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/03/25 13:51:40 $
 * $Id: prefs_edit.h,v 1.7 2014/03/25 13:51:40 steves Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */
 
#ifndef _PREFS_EDIT_H_
#define _PREFS_EDIT_H_

#include <Xm/Xm.h>
#include <Xm/TextF.h>
#include <Xm/PushB.h>
#include <Xm/List.h>
#include <Xm/MessageB.h>
#include <Xm/SelectioB.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
/* /#include <prod_gen_msg.h> */
#include "palette.h"
#include "global.h"



extern char config_dir[255];   /* the path in which the configuration files are kept */


extern assoc_array_i *digital_legend_flag;
extern assoc_array_s *legend_units;
 
extern assoc_array_s *digital_legend_file;
extern assoc_array_s *dig_legend_file_2;
extern assoc_array_s *configured_palette;
extern assoc_array_s *config_palette_2;
extern assoc_array_i *associated_packet;
/* CVG 9.3 - added elevation flag 1=elevation-based product */
extern assoc_array_i *elev_flag;
 
extern assoc_array_i *msg_type_list;
/* CVG 9.1 - packet 1 coordinate override for geographic products */
extern assoc_array_i *packet_1_geo_coord_flag;
/* CVG 9.1 - added override of colors for non-2d array packets */
extern assoc_array_s *override_palette;
extern assoc_array_i *override_packet;

extern assoc_array_s *icao_list;
extern assoc_array_i *radar_type_list;
extern assoc_array_s *radar_names;




/* Product Info widgets */
extern Widget msgtype_opt, msgtype_menu, msgt_but_set;
extern Widget msgt_but0, msgt_but1, msgt_but2, msgt_but3;
extern Widget msgt_but4, msgt_but999, msgt_but1neg;

/* CVG 9.1 - added packet 1 coord override for geographic products */
extern Widget packet_1_opt, packet_1_menu, pkt1_but_set;
extern Widget pkt1_but0, pkt1_but1;

extern Widget resindex_opt, resindex_menu, resi_but_set;
extern Widget resi_but0, resi_but1, resi_but2, resi_but3;
extern Widget resi_but4, resi_but5, resi_but6, resi_but7;
extern Widget resi_but8, resi_but9, resi_but10, resi_but11;
extern Widget resi_but12, resi_but13, resi_but1neg;

extern Widget digflag_opt, digflag_menu, digf_but_set;
extern Widget digf_but0, digf_but1, digf_but2, digf_but3, digf_but1neg;
/*  CVG 8.4  to be used for generic signed int ungigned int and float type */
extern Widget digf_but4, digf_but5, digf_but6; 

extern Widget assocpacket_opt, assocpacket_menu, assp_but_set;
extern Widget assp_but0, assp_but4, assp_but6, assp_but8, assp_but9, assp_but10;
/*  possibly add 2 - Text Symb SYMBOL no value */
/*  possibly add 7 - Unlinked Vector no value */
extern Widget assp_but16, assp_but17, assp_but18, assp_but20; 
extern Widget assp_but41, assp_but42, assp_but43;
/*  contour packet IT'S BACK  CVG 8.4 */
extern Widget assp_but51; 

extern Widget assp_but53, assp_but54, assp_but55;


extern Widget pi_list, id_label, ledg_label;
extern Widget label_id, label_mt, label_pr, label_df, label_l1;
extern Widget label_l2, label_p1, label_p2, label_pt, label_um;
/* CVG 9.1 - added packet 1 coord override for geographic products */
extern Widget label_pk;
/* CVG 9.1 - added override of colors for non-2d array packets */
extern Widget overridepacket_opt, overridepacket_menu, over_but_set;

extern Widget over_but0, over_but4, over_but6, over_but8, over_but9, over_but10;
extern Widget over_but20, over_but43, over_but51;
extern Widget override_palette_text;
extern Widget label_o_pal, label_o_pkt;

/* CVG 9.3 - added elevation flag */
extern Widget elflag_opt, elflag_menu, label_el, elf_but_set;
extern Widget elf_but0, elf_but1;

extern Widget diglegfile_text,  confpalette_text, unit_text; 
extern Widget diglegfile2_text, confpalette2_text, el_text;

extern Widget pref_legend_draw;
extern Pixmap pref_legend_pix;

/* Site Info widgets */
extern Widget si_list, site_id_label, icao_text;

extern Widget rdrtype_opt, rdrtype_menu, rdrt_but_set;
extern Widget rdrt_but0, rdrt_but4; 
/*  possible future radar types */
extern Widget rdrt_but1, rdrt_but2, rdrt_but3;

extern int verbose_flag;

extern int area_label;
extern int area_symbol;
extern int area_line_type;
extern int include_points_flag;

extern assoc_array_i *product_res;

extern Widget shell;


/* prototypes */


void product_edit_select_callback(Widget w, XtPointer client_data, XtPointer call_data);
void product_edit_revert_callback(Widget w, XtPointer client_data, XtPointer call_data);
void product_edit_fill_fields();
void product_edit_commit_callback(Widget w, XtPointer client_data, XtPointer call_data);
void product_edit_save_callback(Widget w, XtPointer client_data, XtPointer call_data);
void product_edit_add_callback(Widget w, XtPointer client_data, XtPointer call_data);
void product_edit_add_cancel_callback(Widget w, XtPointer client_data, XtPointer call_data);
void product_edit_add_ok_callback(Widget w, XtPointer client_data, XtPointer call_data);

extern void pref_legend_show_pixmap();
extern void pref_legend_clear_pixmap();
extern void pref_legend_grey_pixmap();
/* CVG 9.0 - added "local_color" parameter */
extern void display_legend_blocks(Drawable canvas, int x, int in_y, int local_color, 
                                                               int frame_location);


void msgt_cb(Widget w, XtPointer client_data, XtPointer call_data);
/* CVG 9.1 - added packet 1 coord override for geographic products */
void pkt1_cb(Widget w, XtPointer client_data, XtPointer call_data);
/* CVG 9.1 - added override of colors for non-2d array packets */
void overp_cb(Widget w, XtPointer client_data, XtPointer call_data);
/* CVG 9.3 - added elevation flag */
void elflag_cb(Widget w, XtPointer client_data, XtPointer call_data);

void resi_cb(Widget w, XtPointer client_data, XtPointer call_data);
void digf_cb(Widget w, XtPointer client_data, XtPointer call_data);
void assp_cb(Widget w, XtPointer client_data, XtPointer call_data);

void set_sensitivity();
int check_filename(const char *type_file, char *buffer);


void site_edit_select_callback(Widget w, XtPointer client_data, XtPointer call_data);
void site_edit_revert_callback(Widget w, XtPointer client_data, XtPointer call_data);
void site_edit_fill_fields();
void site_edit_commit_callback(Widget w, XtPointer client_data, XtPointer call_data);
void site_edit_save_callback(Widget w, XtPointer client_data, XtPointer call_data);
void site_edit_add_callback(Widget w, XtPointer client_data, XtPointer call_data);
void site_edit_add_cancel_callback(Widget w, XtPointer client_data, XtPointer call_data);
void site_edit_add_ok_callback(Widget w, XtPointer client_data, XtPointer call_data);

void rdrt_cb(Widget w, XtPointer client_data, XtPointer call_data);



void area_ok_callback(Widget w, XtPointer client_data, XtPointer call_data);
void a_label_radio_callback(Widget w, XtPointer client_data, XtPointer call_data);
void a_symbol_radio_callback(Widget w, XtPointer client_data, XtPointer call_data);
void a_line_radio_callback(Widget w, XtPointer client_data, XtPointer call_data);
void a_line_points_cb(Widget w, XtPointer client_data, XtPointer call_data);
extern void area_prev_show_pixmap();
extern void area_prev_clear_pixmap();
extern void area_prev_grey_pixmap();
extern void display_area(int index, int location_flag);


void table_comp_opt_window_callback(Widget w, XtPointer client_data, XtPointer call_data);



#endif

