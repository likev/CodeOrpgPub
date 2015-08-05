/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2011/04/13 22:53:05 $
 * $Id: gauge_radar_common.h,v 1.3 2011/04/13 22:53:05 ccalvert Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

#ifndef GAUGE_RADAR_COMMON_H
#define GAUGE_RADAR_COMMON_H

#include <time.h>
#include <string.h>
#include <limits.h>
#include <rpgc.h>
#include <rpgp.h>
#include <rpgdbm.h>
#include <orpg.h>
#include <orpgvst.h>
#include <orpgevt.h>
#include <dp_lt_accum_Consts.h>
#include <dp_lt_accum_types.h>
#include <dp_lib_func_prototypes.h>
#include "gauge_radar_types.h"

typedef struct {
   short divider;     /* block divider (always -1)                            */
   short block_id;    /* block ID (always 1)                                  */
   int block_length;  /* length of TAB block in bytes                         */
   } TAB_header_t;

typedef struct {
   short divider;    /* block divider (always -1)                             */
   short num_pages;  /* total number of pages in the TAB product              */
   } alpha_header_t;

typedef struct {
   short num_char;  /* number of characters per line                          */
   char data[80];   /* container for one line of TAB output                   */
   } alpha_data_t;

#endif
