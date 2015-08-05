/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/08/19 14:48:41 $
 * $Id: decode_level.h,v 1.2 2009/08/19 14:48:41 ccalvert Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/* decode_level.h */

/* this file to be included only by decode_level.c */

#ifndef _DECODE_LEVEL_H_
#define _DECODE_LEVEL_H_

#include "misc_functions.h"



#define DEFAULT_DEC_PLACES 3

/* CVT 4.4 an array of decode params for existing products */
#define NUM_EXISTING_PARAMS 22  
/* CVT 4.4.2 added DP test products and additional DP products 170-175 */
/* FUTURE, can 156 EDR and 157 EDC be described via scale - offset? */
/*                              max         */
/*    ID     scale    offset    lvl   LF TF  */
decode_params_t  existing_params[NUM_EXISTING_PARAMS] = {
    {  57,      2.0,     66.0,   255,  2, 0},
    {  87,      2.0,    129.0,   255,  2, 0},
    {  94,      2.0,     66.0,   255,  2, 0},
    {  99,      2.0,    129.0,   255,  2, 0},  /* Scale is 1.0 for vel mode 2 */
    { 138,    100.0,      0.0,   255,  0, 0},
    { 153,      2.0,     66.0,   255,  2, 0},
    { 154,      2.0,    129.0,   255,  2, 0},  /* Scale is 1.0 for vel mode 2 */
    { 155,      2.0,    129.0,   255,  2, 0},
/* dual pol products */
    { 159,     16.0,    128.0,   255,  2, 0},
    { 161,    300.0,    -60.0,   255,  2, 0},
    { 163,     20.0,     43.0,   243,  2, 0},
/*    { 170, 0.623925, 0.937607,   255,  1, 0}, */ /*has dynamic scale-offset*/
/*    { 172, 0.623925, 0.937607,   255,  1, 0}, */ /*has dynamic scale-offset*/
/*    { 173, 0.623925, 0.937607,   255,  1, 0}, */ /*has dynamic scale-offset*/
/*    { 174, 0.311886,  128.000,   255,  1, 0}, */ /*has dynamic scale to center 0*/
/*    { 175, 0.311886,  128.000,   255,  1, 0}, */ /*has dynamic scale to center 0*/
    { 176,   1000.0,      0.0, 65535,  0, 0},
    { 177,      1.0,      0.0,   255,  0, 0}, /* special case, does this work? */
/* dual pol test products (raw dp) */
    { 600,      2.0,     66.0,   255,  2, 0},
    { 601,      2.0,    129.0,   255,  2, 0},  /* Scale is 1.0 for vel mode 2 */
    { 602,      2.0,    129.0,   255,  2, 0},
    { 603,     16.0,    128.0,   255,  2, 0},
    { 604, 0.702777,      2.0,   255,  2, 0}, /* Data 1024 levels, reduced to 256 */
    { 605,    300.0,    -60.0,   255,  2, 0},
/* sample algorithm products */
    {1990,      2.0,     66.0,   255,  2, 0},
    {1992,      2.0,     66.0,   255,  2, 0},
    {1995,      2.0,     66.0,   255,  2, 0}
};




float scale_parameter(short val,int flag);

/* CVT 4.4  */
int get_decode_params(int decode_flag, short *product, decode_params_t *params);
int read_params_from_file(char * params_filename);
void decode_level(unsigned int d_level, char *decode_val, decode_params_t *params,
                                                               int num_dec_spaces);



#endif


