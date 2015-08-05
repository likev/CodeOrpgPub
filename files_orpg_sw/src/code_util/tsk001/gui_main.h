/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:52:38 $
 * $Id: gui_main.h,v 1.4 2009/05/15 17:52:38 ccalvert Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */
/* gui_main.h */



#ifndef _GUI_CVG_H_
#define _GUI_CVG_H_

#include <X11/IntrinsicP.h>
#include <Xm/Xm.h>
#include <Xm/MwmUtil.h>
#include <Xm/Form.h>
#include <Xm/MainW.h>
#include <Xm/PushB.h>
#include <Xm/Label.h>
#include <Xm/LabelG.h>

#include <Xm/RowColumn.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/ToggleB.h>
#include <Xm/ScrolledW.h>
#include <Xm/ScrollBar.h>

#include <Xm/CascadeB.h>
#include <Xm/BulletinB.h>
#include <Xm/PushBG.h>
#include <Xm/Separator.h>
#include <Xm/Frame.h>
#include <Xm/SelectioB.h>
#include <Xm/List.h>

#include <Xm/MessageB.h>

#include <stdlib.h>

#include "global.h"

#define NUM_VISIBLE 12 

extern Widget shell;
Widget form;
Widget mainwin; /*  used in callbacks.c  */
Widget menubar;


extern Dimension       width,height;
extern Dimension       bwidth,bheight;



Widget packet_button;

Widget screen_radiobox, s1_radio, s2_radio, screen_radio_label;

Widget s3_radio;
Widget prod_filter_radiobox, prodid_radio, type_radio, pcode_radio;
Widget overlay_but;

/* db widgets on the main screen referenced in prod_select.c */
Widget db_dialog; 
Widget prodid_text, vol_text;
Widget num_prod_label, list_size_label;
Widget db_list;

Widget prod_info_label;
/* should be identical to message in prod_select.c */
/* same size as error messages in prod_select.c */
char *initstr[5] = {" \n                           Initializing Product List \n                         ",
                    "                     Initial Read of the Product Database...  \n      ",
                    "           Press the 'Update List & Filter' button for future updates.",
                    "                                                                      ",
                    "                                                                      " };

extern int orpg_build_i;

extern int sort_method;

extern int select_all_flag;


Colormap cmap;
Display *display;
Window window;
Pixel black_color, white_color, green_color, red_color, grey_color, blue_color, yellow_color; 
Pixel orange_color, cyan_color, magenta_color, brown_color, light_grey_color;
Pixel snow_color, indian_red_color, ghost_white_color; 
GC gc;
XGCValues gcv;



/* prototypes */

void setup_gui_display(void);
void setup_gui_colors(void);

/* CVG 9.0 */
void create_db_list_label(XmString *lst_lbl_str);

extern void orpg_build_callback(Widget w,XtPointer client_data,XtPointer call_data);

extern void build_list_Callback(Widget w,XtPointer client_data,XtPointer call_data);
extern void listselection_Callback(Widget w,XtPointer client_data, XtPointer call_data);
extern void filter_list_Callback(Widget w,XtPointer client_data,XtPointer call_data);
extern void browse_select_Callback(Widget w,XtPointer client_data,XtPointer call_data);
extern void read_database_cont( XtPointer client_data, XtIntervalId *id );

extern void about_callback(Widget w, XtPointer client_data, XtPointer call_data);
extern void exit_callback(Widget w, XtPointer client_data, XtPointer call_data);

extern void select_all_graphic_callback(Widget w, XtPointer client_data, XtPointer call_data);
extern void overlayflag_callback(Widget w,XtPointer client_data,XtPointer call_data);
extern void display_packetselect_callback(Widget w, XtPointer client_data, XtPointer call_data);

extern void screen1_radio_callback(Widget w, XtPointer client_data, XtPointer call_data);
extern void screen2_radio_callback(Widget w, XtPointer client_data, XtPointer call_data);
extern void screen3_radio_callback(Widget w, XtPointer client_data, XtPointer call_data);

extern void prodid_filter_callback(Widget w, XtPointer client_data, XtPointer call_data);
extern void type_filter_callback(Widget w, XtPointer client_data, XtPointer call_data);
extern void pcode_filter_callback(Widget w, XtPointer client_data, XtPointer call_data);

extern void pref_window_callback(Widget w, XtPointer client_data, XtPointer call_data);
extern void help_window_callback(Widget w, XtPointer client_data, XtPointer call_data);

extern void diskfile_select_callback(Widget w,XtPointer client_data, XtPointer call_data);
extern void ilb_file_select_callback(Widget w,XtPointer client_data, XtPointer call_data);

extern void init_db_prod_list();
extern void update_prod_list();

#endif

