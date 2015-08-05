/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/03/12 13:28:34 $
 * $Id: DP_Precip_4bit_Const.h,v 1.4 2012/03/12 13:28:34 ccalvert Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */

#ifndef DP_PRECIP_4BIT_CONST_H
#define DP_PRECIP_4BIT_CONST_H

/* 20090324 Ward It's hard to estimate the final product size,
 * because packet af1f is run length encoded, whose length varies
 * as the data varies. We need a minimum of:
 *
 * MAX_AZM (360) * EST_BYTES_PER_RAD (63) = 22680 bytes.
 *
 * Random 4 bit products were OHA: 35854 bytes, STA: 38474 bytes.
 *
 * so 40000 bytes was chosen. I ran all of Erin through,
 * and the buffers slowly increased from 9730 bytes (volume 2)
 * to 15226 bytes (volume 117). Since Erin is only 65 % fill,
 * with a x 2 margin of error:
 *
 * 15226 * (100 / 65) * 2 = 46649
 *
 * This is near a maximum buffer size of:
 *
 * MAX_AZM (360) * MAX_2KM_RESOLUTION (115) = 41400 bytes + a header.
 *
 * If we make a buffer too small, it produces incomplete products.
 * The way the code handles it now, it knows the final buffer size,
 * and keeps adding radials one at a time until all 360 have been added,
 * or the buffer is filled. */

#define SIZE_4BIT 40000

/* The TAB_SIZE (bytes) was calculated by
 * est_TAB_size in DP_Precip_4bit_STA_tab.c */

#define TAB_SIZE 2844

/* Product codes are used to switch symbology block parameters */

#define CODE169  169
#define CODE171  171

/* EST_BYTES_PER_RAD is the estimated average number of rle
 * bytes per radial. */

#define EST_BYTES_PER_RAD 63

/* The following constants are used in the af1f packet:
 * ICENTER and JCENTER are used to center the product on the PUP
 * display screen, For some reason, the af1f encoder starts
 * encoding at azm MX_DEG_IN_TENTHS. */

#define NUMBINS           MAX_2KM_RESOLUTION
#define ICENTER           256
#define JCENTER           280
#define MX_DEG_IN_TENTHS 3590 /* 359 * 10 */

#define LINES_PER_PAGE     17  /* cvg max TAB lines per page            */
#define LINE_WIDTH         80  /* width of each line of data in the TAB */

/* PPHRLYNC (22) and STMTOTNC (23) are defined in ~/include/a309.inc.
 *
 * Subtract 1 from each index to convert from FORTRAN to C. */

#define COLOR_INDEX_169    21
#define COLOR_INDEX_171    22

#define DP_PRECIP_4BIT_DEBUG FALSE

#endif /* DP_PRECIP_4BIT_CONST */
