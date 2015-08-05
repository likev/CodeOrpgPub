/*
 */

#ifndef DP_PRECIP_8BIT_FUNC_PROTOTYPES_H
#define DP_PRECIP_8BIT_FUNC_PROTOTYPES_H

#include "dp_lt_accum_types.h"      /* LT_Accum_Buf_t      */
#include "DP_Precip_8bit_Const.h"   /* PRODID              */
#include "dp_lib_func_prototypes.h" /* time_to_julian_mins */

void build_DAA_product(LT_Accum_Buf_t* inbuf, int vol_num, char* prodname);

void build_DSA_product(LT_Accum_Buf_t* inbuf, int vol_num, char* prodname, 
                       int DSA_max);

void build_DOD_product(LT_Accum_Buf_t* inbuf, int vol_num, char* prodname);

void build_DSD_product(LT_Accum_Buf_t* inbuf, int vol_num, char* prodname);

unsigned int packet16_dig(short *prodbuf, unsigned char dataScaled[][MAX_BINS]);

int append_ascii_layer2 (LT_Accum_Buf_t *inbuf, char *outbuf,
                         unsigned int lyr1len, int vsnum);

void dp8bit_product_header(short* prodbuf, int vsnum, int prodcode,
                           int minval, int maxval, float scale, float offset,
                           LT_Accum_Buf_t* inbuf);

/* For Debug */

void print_layer2 (char* outbuf, unsigned int sym_block_len);

void print_product_header( char* outbuf );

#endif /* DP_PRECIP_8BIT_FUNC_PROTOTYPES */

