/*
 * RCS info
 * $Author: jeffs $
 * $Locker:  $
 * $Date: 2014/03/19 18:02:32 $
 * $Id: cvt_dispatcher.h,v 1.10 2014/03/19 18:02:32 jeffs Exp $
 * $Revision: 1.10 $
 * $State: Exp $
 */
/* cvt_dispatcher.h */

#ifndef _DISPATCHER_H_
#define _DISPATCHER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <prod_gen_msg.h>
#include <product.h>



#include "misc_functions.h" 

/* CVT 4.4 */
#include "help_functions.h" 



#include "cvt.h"

#define FALSE 0
#define TRUE 1

int ignore_icd_check;
int skip_decompress;

/* CVT 4.4 - changed array size from 8 to 13 */
#define FLAG_ARRAY_SIZE 14

#define BADVAL -999




int dispatch_tasks(int argc,char *argv[]);
int check_args(int argc,char *argv[],char *param,int startval);
int check_for_msg_modifier(int argc,char *argv[]);
int print_product_components(int argc,char *argv[],char* buffer);
int test_bounds(int argc,int value);
void qc_flag_array(int *flag);
/* CVT 4.4 - added report non-digit as error field */
int get_int_value(int argc,char *argv[],int index, int non_digit_error);
void print_db_source( int alternate_db_used );

/* cvt 4.4 moved to cvt.h */
/* char cvt_prod_data_base[128]; */  /* the location of product database file  */
/** cvt 4.4 moved to cvt.h */
/*typedef struct {     */
/*  int message_length;*/ 
                        /* total length of message in bytes (from internal header)*/
  /* CVT 4.4 - added hdr_prod_id */
/*  int hdr_prod_id;   */  
                        /* product id from the internal header - future use with */
                        /* intermediate products                                 */
                        /* the following require an ICD product */
  /* CVT 4.4 - added prod_code and prod_id */
/*  short prod_code;   */
/*  short prod_id;     */   /* derived from the product code */
/*  int total_length;  */  /* length of linear buffer message*/
/*  int product_length;*/   /* length of ICD product */
/*  short n_blocks;    */   /* number of data blocks (2 plus optional) */
/*  short num_layers;  */   /* number of layers inside symbology block */
/*  int symb_offset;   */   /* offset (in HW) to beginning of symbology block */
/*  int gab_offset;    */   /* offset (in HW) to graphic alphanumeric block */
/*  int tab_offset;    */   /* offset (in HW) to tabular alphanumeric block */
/*} msg_data;          */




int initialize_msg_struct(char *buffer);

extern int print_ORPG_header(char* buffer);
extern int print_pdb_header(char* buffer);
extern int print_message_header(char* buffer);
extern void display_TAB(char *buffer,int *offset, int verbose);
extern void display_GAB(char *buffer,int *offset);
extern void display_SATAP(char *buffer);
extern void Extract_LB_Product(int, int, char *, char *, int, int, int);
extern int check_icd_format(char *bufptr, int error_flag);
extern int check_offset_length(char *bufptr, int verbose);


extern char* Load_ORPG_Product(int msg_id, int force);
extern char *load_product_file(char *filename, int header_type, int force);

extern int ORPG_ProductLB_inventory(char *);
extern int ORPG_Database_inventory(void);
extern int ORPG_Database_search(short lb_id);
extern int ORPG_requestLB_inventory(char *);

extern int display_summary_info(char *buffer, int force);
extern int print_symbology_block(char *buffer,int *flag);
extern int print_symbology_header(char *buffer);



#endif
