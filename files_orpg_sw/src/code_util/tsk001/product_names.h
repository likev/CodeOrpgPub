/* product_names.h */

/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2008/03/13 22:47:30 $
 * $Id: product_names.h,v 1.8 2008/03/13 22:47:30 ccalvert Exp $
 * $Revision: 1.8 $
 * $State: Exp $
 */
 
 
#ifndef _PRODUCT_NAMES_H_
#define _PRODUCT_NAMES_H_

#include <prod_gen_msg.h>
#include <prod_distri_info.h>
/*  ORPG Build 9 change */
#include <orpgpat.h>

#include "assoc_array_s.h"
#include "global.h"

/*  ORPG Build 8 and earlier: */
/*$ORPGDIR/pdist/prod_info.lb is used for product info*/
/* #define PROD_ATTR_MSG_ID 3 */

/* ORPG Build 9 and later: */
/*$ORPGDIR/mngrpg/pat.lbis used for product info*/
/*PROD_ATTR_MSG_ID  is defined in orpgpat.h*/

#define FALSE 0


extern int use_cvg_list_flag, prev_cvg_list_flag;

assoc_array_s *product_names;
assoc_array_s *short_prod_names;
/*  for cvg 6 plus */
assoc_array_s *product_mnemonics;

/* extern screen_data *sd1, *sd2, *sd; */
extern int verbose_flag;

extern char config_dir[255];

/* prototypes */
void load_product_names(int from_child, int initial_read);

void write_descript_source(int use_cvg_list);
extern void read_to_eol(FILE *list_file, char *buf);
extern int check_for_directory_existence(char *);

#endif




