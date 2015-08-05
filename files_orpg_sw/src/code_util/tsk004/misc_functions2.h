/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:27:01 $
 * $Id: misc_functions2.h,v 1.1 2009/05/15 17:27:01 ccalvert Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

/* misc_functions2.h */

/* this file to be included only by misc_functions.c */

#ifndef _MISC_FUNCTIONS2_H_
#define _MISC_FUNCTIONS2_H_

#include "misc_functions.h"

int code_to_id[] = 
     {    0,                                              /*  0 NOT USED */
 /*        1    2    3    4    5    6    7    8    9   10 */
  /*----------------------------------------------------------------------*/
          0,   0,   0,   0,   0,   0,   0,   0,   1,   0, /*    1 -  10 */
/* 11*/   0,   0,   0,   0,   0,   3,   5,   7,   2,   4, /*   11 -  20 */
/* 21*/   6,  12,  14,  16,  11,  13,  15,   8,   9,  10, /*   21 -  30 */
/* 31*/  56,  57,  58, 119,  24,  26,  23,  25,  18,  19, /*   31 -  40 */
/* 41*/  29,  28,  60,  62,  61,  59,  63,  35,   0,  84, /*   41 -  50 */
          
 /*       51   52   53   54   55   56   57   58   59   60           */
  /*----------------------------------------------------------------------*/
         83,   0,   0,   0,  69,  68,  85,  51,  33,  42, /*   51 -  60 */
/* 61*/  80,  49,  36,  37,  38,  39,  44,   0,   0,   0, /*   61 -  70 */
/* 71*/   0,   0,  43, 129,  32,   0,   0, 105, 106, 107, /*   71 -  80 */
/* 81*/ 108, 109,   0, 110, 111, 112, 114, 115, 116, 117, /*   81 -  90 */
/* 91*/   0,   0,  87,  94, 124, 126, 123, 125,  99,   0, /*   91 - 100 */

 /*      101  102  103  104  105  106  107  108  109  110 */
  /*----------------------------------------------------------------------*/
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0, /*  101 - 110 */
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0, /*  111 - 120 */
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0  /*  121 - 130 */
     };
/*  In addition, in order to support display of Legacy contour         */
/*  products which are no longer configured in the product_attr_table, */
/*  the following have been added to the table                         */
/*     CODE   ID                                     */
/*      42    28  Echo Tops Contour 2.2 NM           */
/*      88   115  Combined Shear Contour             */
/*      39    18  Composit Reflectivity Contour 0.54 */
/*      40    19  Composit Reflectivity Contour 2.2  */




#endif


