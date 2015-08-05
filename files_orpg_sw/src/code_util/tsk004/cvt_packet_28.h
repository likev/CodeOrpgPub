/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:37:52 $
 * $Id: cvt_packet_28.h,v 1.4 2009/05/15 17:37:52 ccalvert Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */

/* cvt_packet_28.h */

#ifndef _PACKET_28_H_
#define _PACKET_28_H_

#include <stdio.h>
#include <stdlib.h>
/* CVT 4.4 */
#include <string.h>

/* CVT 4.4 */
#include "cvt.h"
#include "misc_functions.h" 

#define FALSE 0
#define TRUE 1

/* CVT 4.4 - added the flag parameter */
void packet_28(char *buffer,int *offset,int *flag);


extern int read_word(char *buffer,int *offset);
extern short read_half(char *buffer,int *offset);


/*  This would include orpg_product.h (don't want to do this in CVT ) */
/*  A source of RPGP_print function prototypes which we also do not use. */
/* #include <rpgp.h> */
#include "cvt_orpg_product.h"

/* #include "cvt_xdr_infra.h" */
/* provides the following functions, among others */

/***************************************************************
    MODIFIED NAMES OF PUBLIC FUNCTIONS IN orpg_products.h TO AVOID COLLISIONS
****************************************************************/
extern int cvt_RPGP_product_deserialize (char *serial_data,
                        int size, void **prod);
extern int cvt_RPGP_product_free (void *prod);



/* CVT 4.4 */
/* extern void Print_prod (RPGP_product_t *prod); */

/*************************************************************/
/* CVT 4.4 - new print prod functions passing flag parameter */
/*         - only radial component has scale offset decoding */
/* The following functions are modified versions of print    */
/* functions in xdr_test.c (cpc101/lib003)                   */
/* (last verified with Build 11 rel 1.2)                     */
/*************************************************************/
void cvt_Print_prod (void *prod, int *flag);
void cvt_Print_params (int n_params, RPGP_parameter_t *params);
void cvt_Print_components (int n_comps, char **comps, int *flag);

void cvt_Print_radial (RPGP_radial_t *radial, int index, int *flag);
void cvt_Print_a_radial (RPGP_radial_data_t *rad, int *flag, 
                                              int rad_num, int start, int end);
void cvt_Print_binary_data (RPGP_data_t *data, int cnt, int *flag);

void cvt_Print_area (RPGP_area_t *area, int index, int *flag);
void cvt_Print_points (int n_points, RPGP_location_t *points);
void cvt_Print_xy_points (int n_points, RPGP_xy_location_t *points);
void cvt_Print_azran_points (int n_points, RPGP_azran_location_t *points);

void cvt_Print_text (RPGP_text_t *text, int index, int *flag);

void cvt_Print_table (RPGP_table_t *table, int index, int *flag);

void cvt_Print_grid (RPGP_grid_t *grid, int index, int *flag);

void cvt_Print_event (RPGP_event_t *event, int index, int *flag);

int cvt_Get_data_type (char *attrs, char *buf, int buf_size);
char *cvt_Get_token (char *text, char *buf, int buf_size);


/* CVT 4.4 */
extern int get_decode_params(int decode_flag, short *product, 
                                            decode_params_t *params);
extern void decode_level(unsigned int d_level, char *decode_val, 
                         decode_params_t *params, int num_dec_spaces);

/* CVT 4.4 */
void decode_prod_time(unsigned int in_time, char **date_str, 
                                                     char **time_str);
extern char *date_to_string (short date);
extern char *msecs_to_string (int time);

#endif



