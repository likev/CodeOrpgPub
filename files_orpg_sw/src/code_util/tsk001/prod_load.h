/* prod_load.h */

/*
 * RCS info
 * $Author: 
 * $Date: 2008/03/13 22:48:38 $
 * $Id: prod_load.h,v 1.3 2008/03/13 22:48:38 ccalvert Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */


#ifndef _PROD_LOAD_H_
#define _PROD_LOAD_H_

#include <Xm/MessageB.h>
#include <Xm/ToggleB.h>

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <orpg.h>
#include <prod_gen_msg.h>
#include "global.h"
#include <bzlib.h>

/* #define MAX_PRODUCTS 16000 */
#define FALSE 0
#define TRUE 1

#define BZIP2_NOT_VERBOSE   0
#define BZIP2_NOT_SMALL     0

extern int maxProducts;

extern int verbose_flag;

extern Widget shell;

/* CVG 8.2 */
extern Widget prod_disk_sel;
extern int header_type;
extern Widget diskfile_radio, diskfile_icd_but, diskfile_icdwmo_but, diskfile_preicdheaders_but;
extern Widget diskfile_raw_but;



extern int orpg_build_i;
extern short get_elev_ind(char *bufptr, int orpg_build);


/* prototypes */
/*  THIS FUNCTION IS NOT CURRENTLY USED */
char* get_default_data_directory(char *);

extern int check_for_directory_existence(char *);
char* Load_ORPG_Product(int msg_id, char *lb_filename);

char *load_icd_product_disk(FILE *data_file, int filesize);
char *load_cvg_raw_disk(FILE *data_file, int filesize);
char *load_cvg_raw_lb(char *filename, int position);
/* CVG 8.2 */
void accept_icd_error_cb(Widget w,XtPointer client_data, XtPointer call_data);


void product_decompress(char **buffer);

extern int test_for_icd(short div, short ele, short vol, int silent);

extern int read_word(char *buffer,int *offset);
extern short read_half(char *buffer,int *offset);
extern unsigned char read_byte(char *buffer,int *offset);

#endif
