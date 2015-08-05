/*
 * RCS info
 * $Author: jeffs $
 * $Locker:  $
 * $Date: 2014/03/19 18:02:32 $
 * $Id: summary_info.h,v 1.8 2014/03/19 18:02:32 jeffs Exp $
 * $Revision: 1.8 $
 * $State: Exp $
 */

/* summary_info.h */

#ifndef _SUMMARY_INFO_H_
#define _SUMMARY_INFO_H_

#include <stdio.h>
#include <prod_gen_msg.h>
#include <product.h>

#include "cvt.h"
#include "misc_functions.h"

#define FALSE 0
#define TRUE 1


int display_summary_info(char *buffer, int force);
extern int check_icd_format(char *bufptr, int error_flag);

extern int orpg_build_i;
extern short get_elev_ind(char *bufptr, int orpg_build);


extern void calendar_date (short,int*,int*,int*);
extern char *_88D_secs_to_string(int time);


#endif

