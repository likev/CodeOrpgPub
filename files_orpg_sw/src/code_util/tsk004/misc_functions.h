/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:37:54 $
 * $Id: misc_functions.h,v 1.10 2009/05/15 17:37:54 ccalvert Exp $
 * $Revision: 1.10 $
 * $State: Exp $
 */

/* misc_functions.h */

/* this file to be included by modules other than misc_functions.c */

#ifndef _MISC_FUNCTIONS_H_
#define _MISC_FUNCTIONS_H_

#include <stdio.h>
#include <stdlib.h>
#include <orpgpat.h>
#include <product.h>
#include <bzlib.h>
#include <strings.h>

#include <prod_gen_msg.h>
#include <misc.h>


#include "cvt.h"
#include "cvt_orpg_build_diffs.h"

#define FALSE 0
#define TRUE 1


#define BZIP2_NOT_VERBOSE 0
#define BZIP2_NOT_SMALL   0
/* CVG 4.4 moved to cvt.h */
/*#define MAX_LAYERS 30 */


/* CVT 4.4 */
#define THRESHOLD_OFFSET  (60) 
#define THR_01_OFFSET (60)
#define THR_02_OFFSET (62)
#define THR_03_OFFSET (64)
#define THR_04_OFFSET (66)
#define THR_05_OFFSET (68)
#define THR_06_OFFSET (70)
#define THR_07_OFFSET (72)
#define THR_08_OFFSET (74)
#define THR_09_OFFSET (76)
#define THR_10_OFFSET (78)
#define THR_11_OFFSET (80)
#define THR_12_OFFSET (82)
#define THR_13_OFFSET (84)
#define THR_14_OFFSET (86)
#define THR_15_OFFSET (88)
#define THR_16_OFFSET (90)









/* swaps the bytes in a short */
#define SHORT_BSWAP(a) ((((a) & 0xff) << 8) | (((a) >> 8) & 0xff))

/* swaps all 4 bytes in an integer */
#define INT_BSWAP(a) ((((a) & 0xff) << 24) | (((a) & 0xff00) << 8) | \
		            (((a) >> 8) & 0xff00) | (((a) >> 24) & 0xff))

/* swaps just the two shorts in an integer - can be used after swapping */
/* the bytes in the individual shorts of an integer                     */
#define INT_SSWAP(a) ((((a) & 0xffff) << 16) | (((a) >> 16) & 0xffff))

/* not used */
#define SHORT_SSWAP(a) {short z; z = a[0]; a[0] = a[1]; a[1] = z;}






/* CVT 4.4 decode parameters structure */
typedef struct{
    int            pcode;
    float          Scale;
    float          Offset;
    unsigned int   max_level;
    unsigned short n_l_flags;
    unsigned short n_t_flags;
} decode_params_t;

/* CVT 4.4 the global decode parameters used by the decode function */
decode_params_t s_o_params;





int read_word(char *buffer,int *offset);
short read_half(char *buffer,int *offset);
unsigned char read_byte(char *buffer,int *offset);
void advance_offset(char *buffer, int *offset,short num_bytes); 


/* CVT 4.4  */
int prod_code_to_id(int prod_code);
void read_to_eol(FILE *list_file, char *buf);

int write_orpg_product_int( void *loc, void *value );
int read_orpg_product_int( void *loc, void *value );
int write_orpg_product_float( void *loc, void *value );
int read_orpg_product_float( void *loc, void *value );


/* CVT 4.4 - to help_functions.h */
/*void packet_definitions(void); */
/*void quick_help(void);         */
/*void help_screen(void);        */
/*void version_information(void);*/

/* CVT 4.4 - to product_load.h */
/*char *load_product_file(char *filename, int header_type, int force);*/

void product_decompress(char **buffer);
int check_icd_format(char *bufptr, int error_flag);
int check_offset_length(char *bufptr, int verbose);

short get_elev_ind(char *bufptr, int orpg_build);

/* Type of file to be loaded */
#define HEADER_NONE    0
#define HEADER_WMO     1
#define HEADER_PRE_ICD 2


#endif

