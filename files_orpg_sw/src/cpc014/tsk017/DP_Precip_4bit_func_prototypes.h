/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/03/11 21:35:15 $
 * $Id: DP_Precip_4bit_func_prototypes.h,v 1.6 2014/03/11 21:35:15 steves Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */

#ifndef DP_PRECIP_4BIT_FUNC_PROTOTYPES_H
#define DP_PRECIP_4BIT_FUNC_PROTOTYPES_H

#include <dp_lt_accum_types.h>      /* LT_Accum_Buf_t      */
#include "DP_Precip_4bit_Const.h"   /* PRODID              */
#include "DP_Precip_4bit_types.h"   /* TAB_header_t        */
#include <dp_lib_func_prototypes.h> /* time_to_julian_mins */
#include <limits.h>                 /* INT_MIN             */
#include <coldat.h>                 /* Coldat_t            */

int Build_OHA_product(LT_Accum_Buf_t* inbuf, int vol_num, char* prodname,
                      Coldat_t* Color_data);

int Build_STA_product(LT_Accum_Buf_t* inbuf, int vol_num,
                      char* prodname, Coldat_t* Color_data,
                      Siteadp_adpt_t* Siteadp);

int packetAF1F_rle(unsigned int *prod_length, short *outbuf,
                   short accumGrid[][MAX_2KM_RESOLUTION],
                   int prod_id, Coldat_t* Color_data);

int dp4bit_product_header(short* prodbuf, int vsnum, int prodcode,
                          Coldat_t* Color_data, int maxval,
                          LT_Accum_Buf_t *inbuf);

void convert_Resolution(int hres_grid[][MAX_BINS],
                        int lres_grid[][MAX_2KM_RESOLUTION]);

int STA_tab(char* outbuf, int sym_block_len, int* tab_len,
            int* tab_offset, int vsnum, LT_Accum_Buf_t* inbuf,
            Siteadp_adpt_t* Siteadp, int* offset);

int msecs_to_string (int time, char* str);

int add_tab_str(int   num_leading_spaces,
                char* str,
                char* outbuf,
                int*  offset);

int add_tab_page1_float(int   num_leading_spaces,
                        char* label,
                        char* format,
                        float value,
                        char* outbuf,
                        int*  offset);

int add_tab_page1_str(int   num_leading_spaces,
                      char* label,
                      char* format,
                      char* str,
                      char* outbuf,
                      int*  offset);

int add_tab_page2_floats(char* label1,
                         char* format1,
                         float value1,
                         char* units1,
                         char* label2,
                         char* format2,
                         float value2,
                         char* units2,
                         char* outbuf,
                         int*  offset);

int add_tab_page3_float(char* label,
                        char* format,
                        float value,
                        char* units,
                        char* outbuf,
                        int*  offset);

/* int add_tab_page2_str_float(char* label1, *
 *                            char* format1, *
 *                            char* str1,    *
 *                            char* label2,  *
 *                            char* format2, *
 *                            float value2,  *
 *                            char* units2,  *
 *                            char* outbuf,  *
 *                            int*  offset); *
 *                                           *
 *                                           *
 * int add_tab_page3_str(char* label,        *
 *                       char* format,       *
 *                       char* str,          *
 *                       char* outbuf,       *
 *                       int*  offset);      */

int add_tab_zone(int zone, char* outbuf, int* offset);

int tab_page_end(int page_num, char* outbuf, int* offset);

#endif /* DP_PRECIP_4BIT_FUNC_PROTOTYPES */
