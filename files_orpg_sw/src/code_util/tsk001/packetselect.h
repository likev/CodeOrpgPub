/* packetselect.h */

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:52:51 $
 * $Id: packetselect.h,v 1.12 2009/05/15 17:52:51 ccalvert Exp $
 * $Revision: 1.12 $
 * $State: Exp $
 */
#include <Xm/Xm.h>
#include <Xm/Form.h>
#include <Xm/List.h>
#include <Xm/Label.h>
#include <Xm/PanedW.h>
#include <Xm/RowColumn.h>
#include <Xm/PushB.h>
#include <Xm/SelectioB.h>
#include <Xm/BulletinB.h>
#include <Xm/MessageB.h>
#include <Xm/ToggleB.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lb.h>
#include <prod_gen_msg.h>

#include "global.h"
/*  This would include orpg_product.h (don't want to do this in CVG ) */
/*  A source of RPGP_print function prototypes which we also do not use. */
/* #include <rpgp.h> */
#include "cvg_orpg_product.h"

/* CVG 9.0 */
#include "packet_definitions.h"

static int dpa_info_flag = TRUE;

/* CVG 9.0 - changed name for clarity */
/* int num_display_packets; */
int list_index; /* index of items (stores) of the list of layers and packets */

Widget packetsel_dialog;
 
extern Widget shell;

extern Widget screen_1, screen_2;
extern Widget screen_3;

extern Widget dshell1, dshell2, dshell3;


extern Widget s1_radio, s2_radio, screen_radio_label;
extern Widget s3_radio;

extern Widget overlay_but;

/* selected product string */
extern char sel_prod_buf[150];

extern assoc_array_i *product_res;

extern assoc_array_i *msg_type_list;

extern int selected_screen;
extern int overlay_flag;

extern screen_data *sd1, *sd2, *sd;
extern screen_data *sd3;

extern int verbose_flag;
extern int select_all_flag;

/* prototypes */
void display_packetselect_callback(Widget w,XtPointer client_data, XtPointer call_data);
void packet_selection_menu(Widget w, XtPointer client_data, XtPointer call_data);
void packetselection_Callback(Widget w,XtPointer client_data, XtPointer call_data);
void select_layer_or_packet(Widget w,XtPointer client_data, XtPointer call_data);
void packetselectall_callback(Widget w,XtPointer client_data, XtPointer call_data);
void select_all_packets(Widget w,XtPointer client_data, XtPointer call_data);

void dpa_not_again_cb(Widget w,XtPointer client_data, XtPointer call_data);


extern void area_comp_opt_window_callback(Widget w, XtPointer client_data, XtPointer call_data);
extern void table_comp_opt_window_callback(Widget w, XtPointer client_data, XtPointer call_data);


int transfer_packet_code(unsigned int val);
void set_prod_resolution(int screen_num);

void get_component_subtype(char *desc, int comp_code, int index );

extern void overlayflag_callback(Widget w,XtPointer client_data,XtPointer call_data);

/* CVG 9.0 - added parameter */
extern void check_overlay(int overlay_non_geographic);

extern void help_window_callback(Widget w, XtPointer client_data, XtPointer call_data);
extern void plot_image(int screen_num, int add_history);
extern void open_display_screen(int screen_num);

extern void reset_time_series(int screen_num, int type_init);
extern void reset_elev_series(int screen_num, int type_init);
extern void reset_auto_update(int screen_num, int type_init);

/*THIS NAME LIST CORRESPONDS TO AN ENUMERATED TYPE IN PACKET_DEFINITIONS.H*/
/*     The relative position in both lists must be maintained.            */
/*                                                                        */
/*     If the position of the packets with Hex values (CONTOUR_VECTOR_,   */
/*     RADIAL_DATA_16, RASTER_DATA_, etc.) or the generic component data  */
/*     is changed:                                                        */
/*          A. the function transfer_packet_code() in packetselect.c      */
/*             must be modified.                                          */
/*          B. CVG configuration files prod_config and palette_list       */
/*             must be modified.                                          */
/*                                                                        */
/*     Support for the Stand-Alone Tabular Alpha Product is being         */
/*     reworked.  Previously was treated like a TAB but needs special     */
/*     treatment.  New function for display_SATAP needed.                 */
/*                                                                        */
/*     Currently no products use packet type DIGITAL RASTER DATA, which   */
/*     is not defined.                                                    */


/*** NEW PACKET ***/
char *packet_name[]={
             "NO VAL                                     ",   /*  0 */
             "Text & Special Symbol TEXT - NO VALUE      ",
             "Text & Special Symbol SYMBOL - NO VALUE    ",
             "MESOCYCLONE DATA                           ",
             "WIND BARB DATA                             ",
             "VECTOR ARROW DATA                          ",   /*  5 */
             "LINKED VECTOR - NO VALUE                   ",
             "UNLINKED VECTOR - NO VALUE                 ",
             "Text & Special Symbol TEXT - UNIFORM VALUE ",
             "LINKED VECTOR - UNIFORM VALUE              ",
             "UNLINKED VECTOR - UNIFORM VALUE            ",   /* 10 */
             "CORRELATED SHEAR MESO                      ",
             "TVS DATA                                   ",
             "POSITIVE HAIL DATA                         ",
             "PROBABLE HAIL DATA                         ",
             "STORM ID DATA                              ",   /* 15 */
             "DIGITAL RADIAL DATA ARRAY                  ",
             "DIGITAL PRECIP DATA ARRAY                  ",
             "PRECIP RATE DATA ARRAY                     ",
             "HDA HAIL DATA                              ",
             "POINT FEATURE DATA                         ",   /* 20 */
             "CELL TREND DATA                            ",
             "CELL TREND VOLUME SCAN TIME                ",
             "SCIT PAST POSITION DATA                    ",
             "SCIT FORECAST POSITION DATA                ",
             "STI CIRCLE DATA                            ",   /* 25 */
             "ETVS DATA                                  ",
             "SUPEROB WIND DATA                          ",
             "GENERIC PRODUCT DATA                       ",
             "UNDEFINED B                                ",
             "UNDEFINED C                                ",   /* 30 */
             "UNDEFINED D                                ",
             "UNDEFINED E                                ",
             "UNDEFINED F                                ",
             "UNDEFINED G                                ",
             "UNDEFINED H                                ",   /* 35 */
             "UNDEFINED I                                ",
             "UNDEFINED J                                ",
             "UNDEFINED K                                ",
             "UNDEFINED L                                ",
             "UNDEFINED M                                ",   /* 40 */
             "RADIAL DATA - ", /* component type radial */
             "GRID DATA - ",  /* component type grid */
             "AREA DATA - ",  /* component type area */
             "TEXT DATA - ",
             "TABLE DATA - ",  /* component type table */     /* 45 */
             "EVENT DATA - ",
             "UNDEFINED T                                ",
             "UNDEFINED U                                ",
             "UNDEFINED V                                ",          
             "CONTOUR VECTOR - COLOR                     ",   /* 50 */
             "CONTOUR VECTOR - LINKED                    ",
             "CONTOUR VECTOR - UNLINKED                  ",
             "RADIAL DATA (4/8/16 LEVEL)                 ",
             "RASTER DATA                                ",
             "RASTER DATA                                ",   /* 55 */
             "TABULAR ALPHA BLOCK                        ",
             "GRAPHIC ALPHA BLOCK                        ",
             "STAND-ALONE TAB ALPHA PROD                 ",
             "DIGITAL RASTER DATA                        ",
             "BACKGROUND MAP DATA                        ",   /* 60 */
             "AZIMUTH LINE RANGE RING                    ",
             "LEGEND TEXT DATA                           "
};
   


