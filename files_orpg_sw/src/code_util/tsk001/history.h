/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2008/03/13 22:46:13 $
 * $Id: history.h,v 1.7 2008/03/13 22:46:13 ccalvert Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */
/* history.h */

#ifndef _HISTORY_H_
#define _HISTORY_H_

#include <stdlib.h>
#include <stdio.h>
#include "global.h"

/*  from orpg file packet_28.h */
typedef struct
{
   short  code;         /* Packet code                                   */
   short  align;
   int    num_bytes;    /* Byte length not including self or packet code */
} packet_28_t;




/* extern Dimension pwidth; */

extern screen_data *sd1, *sd2, *sd;
extern screen_data *sd3;

extern int verbose_flag;
extern int overlay_flag;

/* prototypes */
void save_current_state_to_history(history_info **hist, int *hist_size);
void replay_history(history_info *hist, int hist_size, int screen_num);
void clear_history(history_info **hist, int *hist_size);

extern void plot_image(int screen_num, int add_history);

extern void delete_layer_info(layer_info *linfo, int size);
extern layer_info *copy_layer_info(layer_info *linfo, int size);

extern int cvg_RPGP_product_deserialize (char *serial_data,
                                       int size, void **prod);
extern int cvg_RPGP_product_free (void *prod);

#endif
