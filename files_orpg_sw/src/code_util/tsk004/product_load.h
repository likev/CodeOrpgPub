/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 18:19:11 $
 * $Id: product_load.h,v 1.1 2009/05/15 18:19:11 ccalvert Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

/* product_load.h */

#ifndef _PRODUCT_LOAD_H_
#define _PRODUCT_LOAD_H_


#include <prod_request.h>
#include <prod_gen_msg.h>
#include <product.h>

#include "cvt.h"

#include "misc_functions.h"


char* Load_ORPG_Product(int msg_id, int force);
char *load_product_file(char *filename, int header_type, int force);


#endif

