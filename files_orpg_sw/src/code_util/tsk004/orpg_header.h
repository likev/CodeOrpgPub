/*
 * RCS info
 * $Author: cheryls $
 * $Locker:  $
 * $Date: 2004/03/03 18:01:58 $
 * $Id: orpg_header.h,v 1.2 2004/03/03 18:01:58 cheryls Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/* orpg_header.h */

#ifndef _ORPG_HEADER_H_
#define _ORPG_HEADER_H_

#include <stdio.h>
#include <stdlib.h>
#include <prod_gen_msg.h>
#include "cvt_orpg_build_diffs.h"

#define FALSE 0
#define TRUE 1

extern int orpg_build_i;
extern short get_elev_ind(char *bufptr, int orpg_build);

int print_ORPG_header(char* buffer);


#endif


