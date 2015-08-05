/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/03/03 18:19:27 $
 * $Id: DP_Precip_4bit_types.h,v 1.2 2009/03/03 18:19:27 ccalvert Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

#ifndef DP_PRECIP_4BIT_TYPES_H
#define DP_PRECIP_4BIT_TYPES_H

#include "DP_Precip_4bit_Const.h" /* LINE_WIDTH */

typedef struct {
   short divider;      /* block divider (always -1)    */
   short block_id;     /* block ID (always 1)          */
   int   block_length; /* length of TAB block in bytes */
} TAB_header_t;

typedef struct {
   short divider;   /* block divider (always -1)                */
   short num_pages; /* total number of pages in the TAB product */
} alpha_header_t;

/* Note: data[] is not a string and does not contain a trailing NULL */

typedef struct {
   short num_char;         /* number of characters per line        */
   char  data[LINE_WIDTH]; /* container for one line of TAB output */
} alpha_data_t;

#endif /* DP_PRECIP_4BIT_TYPES_H */
