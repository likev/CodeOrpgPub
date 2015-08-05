/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:52:55 $
 * $Id: res.h,v 1.6 2009/05/15 17:52:55 ccalvert Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */
/* res.h */

#ifndef _RES_H_
#define _RES_H_

#include <stdio.h>
#include "global.h"

/* the resolution of each product, indexed by product ID */
assoc_array_i *product_res;

extern char config_dir[255];  /* the path in which the configuration files are kept */
/* extern screen_data *sd1, *sd2, *sd; */
extern int verbose_flag;

extern float *resolution_number_list;
extern int number_of_resolutions;


/* prototypes */
float res_index_to_res(int res);
int calc_prod_range(int prod_id, int num_bins);
float calc_range(int prod_id, int bin);


int temporary_set_screen_res_index(float res);


#endif
