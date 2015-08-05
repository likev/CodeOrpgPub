/* prod_select.h */

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2008/03/13 22:47:28 $
 * $Id: prod_select.h,v 1.4 2008/03/13 22:47:28 ccalvert Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */

#ifndef _PROD_SELECT_H_
#define _PROD_SELECT_H_

#include <Xm/Xm.h>
#include <Xm/List.h>
#include <Xm/Form.h>
#include <Xm/Separator.h>
#include <Xm/Label.h>
#include <Xm/RowColumn.h>
#include <Xm/ToggleB.h>
#include <Xm/FileSB.h>
#include <Xm/SelectioB.h>
#include <Xm/MessageB.h>


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>

#include <sys/stat.h>
#include <sys/types.h>


#include <lb.h>
#include <prod_gen_msg.h>

#include "global.h"


/* #define SECS_PER_DAY       86400 */
/* #define HEADER_DATE_OFFSET (96+2) */
/* #define HEADER_TIME_OFFSET (96+4) */
/* #define HEADER_ID_OFFSET   (96+12) */


/*globals defined in global2.h */
/* typedef char db_entry_string[112]; */



/*  Needed to prevent animation from using data */
/*  derived from the previous product list */
#define FILE_SERIES 4

/*  CVG 8.2 */
#define AUTO_MODE 1
#define MANUAL_MODE 2


/* the animation data structures for each screen, see global.h for typedef */
extern anim_data anim1, anim2, anim3;
/* anim_data *anim; */


extern void reset_time_series(int screen_num, int type_init);
extern void reset_elev_series(int screen_num, int type_init);
extern void reset_auto_update(int screen_num, int type_init);



#define NUM_LINES 5
/* should be similar to initstr in gui_main.h */
static char *init_dialog[NUM_LINES] = {
                    "                                                                           ",
                    "                  Change in Preferences Resets Database List              \n",
                    "       Press the 'Update List & Filter' button to get a new product list.  ",
                    "                                                                           ",
                    "       If no list appears, wait 5 seconds and try again.                   " };

/*  read from a preference file, default is 16000 */
extern int maxProducts;

/*  CVG 8.2 */
Widget prod_disk_sel;
int header_type;

Widget diskfile_radio, diskfile_icd_but, diskfile_icdwmo_but, diskfile_preicdheaders_but;
Widget diskfile_raw_but;


/* selected product string */
char sel_prod_buf[150];

/* reusable product list */
XmString *xmstr_prodb_list=NULL;  /*  used in exit_callback in callbacks.c */

/*  ------------------------------------------ */
/*  new list message */
int list_msg_size;
int msg_num_off, sort_time_off, descript_off;
/*  CVG 7.4 */
int elev_num_off;

char *prod_list_msg=NULL;  /*  used in exit_callback in callbacks.c */

typedef struct{
  
  int max_list_size;
  int prod_list_size;
  
} Prod_list_hdr;

/*  CVG 6.5 - the following lists are used for product animation */
 db_entry_string *product_list=NULL; 
 short *msg_num_total_list=NULL;
/*  CVG 6.5 added: */
 unsigned int *sort_time_total_list=NULL;
/*  CVG 7.4 added: */
 short *elev_num_total_list=NULL;

/*  ------------------------------------------ */




extern char config_dir[255];

int last_prod_list_size=0; /*  used in exit_callback in callbacks.c */
                           /*  and by anim.c */


/* int reuse_list_flag; */

extern XtAppContext app;

extern Widget shell;

extern int num_products;  /*  used in update_prod_list */

extern int prod_filter; /* determins which column product is filtered with */

/* extern Dimension bwidth,bheight; */

extern int last_plotted;
extern assoc_array_s *product_names;
extern assoc_array_i *product_res;
extern assoc_array_i *msg_type_list;
extern assoc_array_i *digital_legend_flag;
extern assoc_array_s *digital_legend_file;
extern assoc_array_s *dig_legend_file_2;
extern assoc_array_s *legend_units;
/*  for cvg 6 plus */
extern assoc_array_s *product_mnemonics;


extern Widget res_opt; 

extern int selected_screen;

char prod_disk_last_filename[300];
extern char product_database_filename[256];

extern screen_data *sd1, *sd2, *sd;
extern screen_data *sd3;
extern int verbose_flag;


/* some db widgets on the main screen */
extern Widget db_dialog; 
extern Widget prodid_text, vol_text;
extern Widget num_prod_label, list_size_label;

extern Widget prod_info_label;

/* child of db_dialog */
extern Widget db_list;


/*  CVG 8.2 */
extern int code_to_id[];

/* prototypes */

void init_db_prod_list();

void diskfile_select_callback(Widget w,XtPointer client_data, XtPointer call_data);
/*  CVG 8.2 */
void auto_detect_callback(Widget w, XtPointer client_data, XtPointer call_data);

void diskfile_ok_callback(Widget w,XtPointer client_data, XtPointer call_data);
void diskfile_cancel_callback(Widget w,XtPointer client_data, XtPointer call_data);
void product_specify_ok_callback(Widget w, XtPointer client_data, XtPointer call_data);
void product_specify_cancel_callback(Widget w, XtPointer client_data, XtPointer call_data);
void ilb_file_ok_callback(Widget w, XtPointer client_data, XtPointer call_data);
void ilb_file_cancel_callback(Widget w, XtPointer client_data, XtPointer call_data);
void ilb_product_selection_callback(Widget w, XtPointer client_data, XtPointer call_data);
void do_fnf_error_box();
void do_fnl_error_box();
int product_post_load_setup();
int handle_load_error(int load_error);


/* used by main db select dialog */
void build_list_Callback(Widget w,XtPointer client_data,XtPointer call_data);
void filter_list_Callback(Widget w,XtPointer client_data,XtPointer call_data);
void listselection_Callback(Widget w,XtPointer client_data, XtPointer call_data);
void browse_select_Callback(Widget w,XtPointer client_data,XtPointer call_data);

void build_list();
int new_update_prod_list();

void filter_prod_list();
void build_msg_list(int err);


extern int orpg_build_i;
extern short get_elev_ind(char *bufptr, int orpg_build);

extern int cvg_RPGP_product_free (void *prod);

/*  possible use if product_post_load_setup() fails */
extern void clear_screen_data(int screen, int plot_image);
extern void replot_image(int screen_num);

extern void packet_selection_menu();

extern char *load_icd_product_disk(FILE *data_file, int filesize);
extern char *load_cvg_raw_disk(FILE *data_file, int filesize);
extern char *load_cvg_raw_lb(char *filename, int position);

extern void help_window_callback(Widget w, XtPointer client_data, XtPointer call_data);
extern char* Load_ORPG_Product(int msg_id, char *lb_filename);
extern int parse_packet_numbers(char *buffer);

extern int test_for_icd(short div, short ele, short vol, int silent);


#endif



