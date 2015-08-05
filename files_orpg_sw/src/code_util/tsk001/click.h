/* click.h */

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:52:33 $
 * $Id: click.h,v 1.10 2009/05/15 17:52:33 ccalvert Exp $
 * $Revision: 1.10 $
 * $State: Exp $
 */
 
#ifndef _CLICK_H_
#define _CLICK_H_

#include <Xm/Xm.h>

#include <stdio.h>
#include <math.h>
#include <prod_gen_msg.h>

#include "global.h"

/* renamed to CVG_RAD_TO_DEG and moved to global.h */
#define	RADDEG	        (180.0/3.14159265)
/* defined in global.h */
/* #define CVG_KM_TO_NM 0.539957  */

extern Widget main_screen, screen_1, screen_2;
extern Widget screen_3;

extern Widget dshell1, dshell2;
extern Widget dshell3;

float pixel_int, scanl_int;
extern int selected_screen;
extern Dimension pwidth, pheight;

extern Pixel white_color;
extern Display *display;
extern GC gc;
extern int is_zoomed;

extern unsigned int *packet_code;
extern assoc_array_i *product_res;
extern int packet_selection; /* holds code from the packet selection window */

extern assoc_array_i *msg_type_list;

extern screen_data *sd1, *sd2, *sd;
extern screen_data *sd3;

extern int verbose_flag;
extern int linked_flag;

    
extern int p17_orig_x, p17_orig_y;
extern int p17_g_size;

/* prototypes */

void do_click_info(int screen_num, int inx, int iny);

void click_decode_level(unsigned int d_level, char *decode_val);

extern float calc_range(int prod_id, int bin);
extern float res_index_to_res(int res);


extern int _88D_azranelev_to_xy( float range, float azm, float elev,
                           float *x, float *y ); 
extern int _88D_xy_to_latlon( float x, float y, float *lat, float *lon );

extern void replot_image(int screen_num);
extern void show_pixmap(int screen_num);


#endif
