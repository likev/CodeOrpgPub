/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:37:53 $
 * $Id: inventory_functions.h,v 1.8 2009/05/15 17:37:53 ccalvert Exp $
 * $Revision: 1.8 $
 * $State: Exp $
 */

/* inventory_functions.h */


#ifndef _INVENTORY_FUNCTIONS_H_
#define _INVENTORY_FUNCTIONS_H_

#include <prod_request.h>
#include <prod_gen_msg.h>
#include <product.h>

#include "misc_functions.h" /* why necessary? */

#include "cvt.h"

#define FALSE 0
#define TRUE 1


int ORPG_ProductLB_inventory(char *);
int ORPG_Database_inventory(void);
char* get_default_data_directory(char *);
int check_for_directory_existence(char *);

/* CVT 4.4 - to product_load.h */
/*char* Load_ORPG_Product(int msg_id, int force);*/

int ORPG_Database_search(short lb_id);
int ORPG_requestLB_inventory(char *);
int get_LB_status(int);
void Extract_LB_Product(int, int, char *, char *, int, int, int);

extern int orpg_build_i;

extern short get_elev_ind(char *bufptr, int orpg_build);
extern void product_decompress(char **buffer);
extern int check_icd_format(char *bufptr, int error_flag);

/* defined in cvt.h */
/* #define MAX_PRODUCTS 16000 */




#endif





