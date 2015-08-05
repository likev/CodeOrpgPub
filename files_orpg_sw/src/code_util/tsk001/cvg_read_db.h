/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2008/03/13 22:45:32 $
 * $Id: cvg_read_db.h,v 1.3 2008/03/13 22:45:32 ccalvert Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */
/* cvg_read_db.h */
#ifndef _CVG_READ_DB_H_
#define _CVG_READ_DB_H_



#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
/*  CVG 8.4 */
#include <signal.h>
#include <errno.h>

#include <lb.h>
#include <product.h>
#include <prod_gen_msg.h>
#include <prod_distri_info.h>


#define TRUE 1
#define FALSE 0

/* /////////////////////////////////////////////////////// */

#include "assoc_array_s.h"

#include "global2.h"
/* following are defined in global2.h */
/* typedef char db_entry_string[112]; */
/* #define DEFAULT_DB_SIZE 16000 */
/* #define PROD_ATTR_MSG_ID 3 */

/*  also defined in prefs_load.h */
char config_dir[255]; 
int maxProducts;  /*  requires cvg (and orpg) restart */

/*  also defined in cvg.h */
int verbose_flag;
int use_cvg_list_flag, prev_cvg_list_flag;



/*  define the INTERNAL PRODUCT  SORT INDEX LIST */

int *db_sort_list;  /*  lowest message number is 1, first list index is 0 */



/****** The Internal Product List definition  ******/

/*  VERSION 3 */
typedef struct{
  /*  sorting members: */
  unsigned int sort_time; /*  calculated volume date/time when sort_method == 1 */
                          /*  the volume sequence number when sort_method == 2 */
  unsigned int hdr_vol_time; /*  for intermediate prods, time string with  */
                             /*  sort_method == 2 */

  short        elev_index; /*  the one used to sort and visible in the list */
  short        prod_id;
  /*  data members:   */
  short           divider;
  short           vol_num;
  short           prod_code;
  unsigned short  vol_date;
  unsigned short  vol_time_ms;
  unsigned short  vol_time_ls;

  short           elev;  /*  used for animation, all volume products = 0; */
} Prod_data_element_t;

/*  the internal list contains MaxProducts+1 Prod_data_element_t items */
/*  some of the entries will not contain valid data (expired products, etc) */

/*  the internal list pointer */
Prod_data_element_t *data_element_list;  /*  first index is 1 */

/******  end INTERNAL PRODUCT LIST definition  ******/


/*  define the PRODUCT LIST MESSAGE structure written into the buffer */

typedef struct{
  
  int max_list_size;
  int prod_list_size;
  
} Prod_list_hdr;
  
/*  Beginning with CVG 7.4: */
  /* ///// the complete product list message contains: */
  /*  a header (Prod_list_hdr *list_header): */
  /*           1        four-byte intetger for MaxProducts+1  */
  /*           1        four-byte intetger for p_list_size */
  /*  the list (static char   *prod_list_message): */
  /*    maxProducts+1   db_entry_string (112 bytes each) */
  /*    maxProducts+1   4-byte integers for the sort time, either  */
  /*                    vol date/time (method 1) or vol seqence (method 2) */
  /*    maxProducts+1   2-byte integers for internal msg number */
  /*    maxProducts+1   2-byte integers for elevation number (used by animation) */
  /*  */



Prod_list_hdr *list_header;

/*  the product list message pointer */
static char *prod_list_message;
  
 
/*  CVG 7.4 */
  /*  default message size is (MaxProducts = 16000): */
  /*  8 + ((16000+1)*112) + ((16000+1)*4)+ ((16000+1)*2) +  */
  /*                     ((16000+1)*2) = 1,920,128 bytes (1875KB or 1.8MB)  */
  
  
unsigned int list_message_size;
unsigned int msg_num_offset, sort_time_offset, descript_offset;

unsigned int elev_num_offset;

/*  variables to access entry strings, volume times,  and message numbers in the list */
static db_entry_string  *product_listP;

static unsigned int *sort_time_total_listP;

static short *elev_num_total_listP;

static short *msg_num_total_listP;

/*  end PRODUCT LIST MESSAGE structure definition */



/*  defined in product_names.h */
extern assoc_array_s *product_names;
extern assoc_array_s *short_prod_names;
extern assoc_array_s *product_mnemonics;

/* ////////////////////////////////////////////////////////////// */

char list_filename[256];
int list_lbfd=-1;
char prod_db_filename[256];
int verbose_f=TRUE;


/* /////////////////////////////////////////////// */
/* // SIGNAL HANDLING FLAG VALUES */
#define CHILD_P_CONT 0
#define CHILD_P_USR1 1  /*  SIGUSR1: read_db_child re-reads prefs (db filename) */
#define CHILD_P_USR2 2  /*  SIGUSR2: read_db_child finishes current list */
#define CHILD_P_TERM 3  /*  SIGTERM: read_db_child finishes current list */
volatile int readFlag = CHILD_P_CONT;
volatile int continueFlag = CHILD_P_CONT;
/*  SIGNAL HANDLERS */
void read_db_SIGUSR1();
void read_db_SIGUSR2();
void read_db_SIGTERM();

#define PANIC (panic(__FILE__,__LINE__))
/*  not currently used */
/* extern void panic(); */
/* ///////////////////////////////////////////////// */


/* //////////////////////////////////////////////// */
/* // PROTOTYPES FOR CHILD */
int read_db();
void build_database_list();
void process_list_itemP(int msg_num, int lbfiledes, LB_info *lblist);
void process_list_item(int msg_num);
void get_db_filename();
int read_orpg_build();
int read_sort_method();
int read_descript_source();
void create_list_file();
void write_empty_message(int num_prod);
void blank_prod_list();
void blank_sort_list();
void blank_data_list();


/* cvg8.0 */
void init_read_db_prefs();
void read_db_size();
/*  in prefs_load.c moved to helpers.c */
extern void read_to_eol(FILE *list_file, char *buf);
/*  in prodduct_names.c */
extern void load_product_names(int from_child, int initial_read);


/*  must be defined locally for future exec'd executable */
/*  both in prod_load.c  moved to helpers.c */
extern short get_elev_ind(char *bufptr, int orpg_build);
extern int check_for_directory_existence(char *dirname);


/* ///////////////////////////////////////////////////// */

/*  in helpers.c */
extern int test_for_icd(short div, short ele, short vol, int silent);

/*  in helpers.c */
extern time_t _88D_unix_time(short date, int time);


/* //////////////////////////////////////////////////////////// */
/*  heapsort */
void heapsort_vol(int a[], int array_size);
void downHeap_vol(int a[], int root, int bottom);

void heapsort_ele(int a[], int array_size);
void downHeap_ele(int a[], int root, int bottom);

void heapsort_pid(int a[], int array_size);
void downHeap_pid(int a[], int root, int bottom);

void heapsort_pcd(int a[], int array_size);
void downHeap_pcd(int a[], int root, int bottom);

/* /////////////////////////////////////////////////////// */
/*  test functions */

void print_new_list_item(Prod_data_element_t data_item, int msg_num, int index);

#endif



