/* anim_fp.h */

#ifndef _ANIM_FP_H_
#define _ANIM_FP_H_

#include <stdio.h>
#include <stdlib.h>
#include <lb.h>


#include "global.h"

extern int ANIM_DEBUG;

extern char *anim_prod_data;

/* the animation data structures for each screen, see global.h for typedef */
extern anim_data anim1, anim2, anim3;
/* anim_data *anim; */

/* /#define MAX_HIST_SZ 20 */


/*  CVG 6.5 - the following product lists are used for animation */
/*  defined in prod_select.h */
extern db_entry_string *product_list; 
extern short *msg_num_total_list;
extern unsigned int *sort_time_total_list;
extern int last_prod_list_size;
extern short *elev_num_total_list; /*  added CVG 7.4 */

extern int sort_method;


/* prototypes */
void find_last_prod(int lbfd,int num_products,LB_info *list,int buf_num,int elev_num);

int ts_next_base_prod(int buf_num, int elev_num, unsigned int vol_time, 
                                                          int screen_num);
int ts_prev_base_prod(int buf_num, int elev_num, unsigned int vol_time, 
                                                          int screen_num);
int ts_find_ovly_prod(int buf_num, int elev_num, int screen_num);


int es_find_next_prod(int buf_num, int hist_index, int screen_num);
int es_find_prev_prod(int buf_num, int hist_index, int screen_num);



/*  used by find_last_prod() */
/* extern int orpg_build_i; */
/* extern short get_elev_ind(char *bufptr, int orpg_build); */
extern time_t _88D_unix_time(short date, int time);



#endif          
