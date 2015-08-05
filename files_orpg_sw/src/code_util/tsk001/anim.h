/*
 * RCS info
 * $Author:
 * $Date: 2008/03/13 22:44:57 $
 * $Id: anim.h,v 1.11 2008/03/13 22:44:57 ccalvert Exp $
 * $Revision: 1.11 $
 * $State: Exp $
 */

/* anim.h */

#ifndef _ANIM_H_
#define _ANIM_H_

#include <Xm/Xm.h>
#include <Xm/Scale.h>
#include <Xm/MessageB.h>
#include <stdio.h>
#include <stdlib.h>
#include <lb.h>
#include "global.h"
 
/* #define MAX_PRODUCTS 16000 */ /* maximum size of product database for CVG */
#define MIN_PAUSE_TIME 100 /* the minimum amount of time to pause between animation
			    * frames to give the rest of the program some time */

#define TIME_SERIES 1
#define ELEV_SERIES 2
#define AUTO_UPDATE 3
#define FILE_SERIES 4

/*  CVG 6.5   */
#define FORWARD 1
#define REVERSE -1

/*  defined in global.h */
/* animation initialization type */
/* #define ANIM_NO_INIT 0   */
/* #define ANIM_FULL_INIT 1 // displaying product, clearing screen, and  */
/*                          // if ANIM_NO_INIT when setting time series */
/* #define ANIM_CHANGE_MODE_INIT 2 // set when changing animation mode */
/* #define ANIM_CHANGE_MODE_NEW_TS_INIT 3 // set if CVG user sets new new */
/*                               // base vol an loop after change mode */
/* #define ANIM_UPDATE_LIST_INIT 4 // set when updating sorted product list */


/* the animation data structures for each screen, see global.h for typedef */
extern anim_data anim1, anim2, anim3;
/* anim_data *anim; */



/* FILE SCOPE VARIABLES */
char *anim_prod_data;

/* which button is pressed (i.e., which mode is selected */
Boolean time_series_flag, elev_series_flag, auto_update_flag, file_series_flag;


/* ----------------------------------------------------------------- */
/*  CVG 6.5 alpha - only base product volume number needs to be recorded */
/*  CVG 6.5 use volume date-time to locate volume change rather than volume number */
/*  the following are not needed since the base prod vol_num and time are read directly */
/*  from the screen history data rather than the animation history, see get_history() */
/* ------------------------------------------------------------------- */


extern int linked_flag;

/* extern const int maxProducts; */
extern int maxProducts;

/* screen specific data */
extern screen_data *sd1, *sd2, *sd;
extern screen_data *sd3;

/* extern Widget shell; */
extern Widget dshell1, dshell2;
extern Widget dshell3;

extern int selected_screen, overlay_flag;
extern XtAppContext app;

/* extern Dimension pwidth; */

/* we need to keep track of the ids of the timed out functions
 * so that we can remove them to stop the auto looping */
/* XtIntervalId timeout_id1, timeout_id2; */



extern anim_data anim1, anim2, anim3;



/*  keep these ---------------------------------- */
extern int user_reset_time_s1, user_reset_time_s2;
extern int user_reset_time_s3;
/*  --------------------------------------------- */


extern int sort_method;
extern int verbose_flag;


extern int orpg_build_i;
extern short get_elev_ind(char *bufptr, int orpg_build);


/*  CVG 6.5 - the following product lists are used for animation */
/*  defined in prod_select.h */
extern db_entry_string *product_list; 
extern short *msg_num_total_list;
extern unsigned int *sort_time_total_list;
extern int last_prod_list_size;
extern short *elev_num_total_list; /*  added CVG 7.4 */


/* prototypes */

extern void open_time_opt_dialog(int screen_N);

extern int cvg_RPGP_product_free (void *prod);

void select_time_callback(Widget w,XtPointer client_data, XtPointer call_data);
void select_elev_callback(Widget w,XtPointer client_data, XtPointer call_data);
void select_update_callback(Widget w,XtPointer client_data, XtPointer call_data);
void select_file_callback(Widget w,XtPointer client_data, XtPointer call_data);

void back_one_callback(Widget w,XtPointer client_data, XtPointer call_data);
void back_play_callback(Widget w,XtPointer client_data, XtPointer call_data);
void back_play_cont(XtPointer client_data, XtIntervalId *id);

void stop_anim_callback(Widget w,XtPointer client_data, XtPointer call_data);

void fwd_one_callback(Widget w,XtPointer client_data, XtPointer call_data);
void fwd_play_callback(Widget w,XtPointer client_data, XtPointer call_data);
void fwd_play_cont(XtPointer client_data, XtIntervalId *id);

/*  CVG 6.5 */
void animate_prod_database(int screen_num, int direction); 
void animate_prod_file(int screen_num, int direction);

int get_history(int screen);
void display_animation_product(int hist_lyr, int screen_num);

int init_time_series(int screen_num, unsigned int vol_t, short vol_n);
void reset_time_series(int screen_num, int type_init);

int init_elev_series(int screen_num, unsigned int vol_t, short vol_n);
void reset_elev_series(int screen_num, int type_init);

int init_auto_update(int screen_num);
void reset_auto_update(int screen_num, int type_init);

extern void find_last_prod(int lbfd,int num_products,LB_info *list,int buf_num,
                                                                  int elev_num);

extern int ts_next_base_prod(int buf_num, int elev_num, unsigned int vol_time, 
                                                                int screen_num);
extern int ts_prev_base_prod(int buf_num, int elev_num, unsigned int vol_time, 
                                                                int screen_num);
extern int ts_find_ovly_prod(int buf_num, int elev_num, int screen_num);


extern int es_find_next_prod(int buf_num,int hist_index, int screen_num);
extern int es_find_prev_prod(int buf_num, int hist_index, int screen_num);

extern void product_decompress(char **buffer);
extern void plot_image(int screen_num, int add_history);
extern int parse_packet_numbers(char *buffer);

/*  used by get_history() */
extern int orpg_build_i;
extern short get_elev_ind(char *bufptr, int orpg_build);

extern time_t _88D_unix_time(short date, int time);

/* Flag for debugging */
int ANIM_DEBUG = FALSE;                    

#endif
