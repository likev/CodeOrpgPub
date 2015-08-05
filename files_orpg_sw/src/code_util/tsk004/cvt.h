/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2010/02/03 18:00:53 $
 * $Id: cvt.h,v 1.15 2010/02/03 18:00:53 ccalvert Exp $
 * $Revision: 1.15 $
 * $State: Exp $
 */

/* cvt.h */

#ifndef _CODEVIEW_H_ 
#define _CODEVIEW_H_

#include <stdio.h>
#include <sys/stat.h>
#include <string.h>

#include <stdlib.h>


char graphic_dir[255];   /* the default path for the exported files */

#define CV "CODEview Text (CVT) Version 4.4.3"
#define DATE "January, 2010"

/* value set by variable CV_ORPG_BUILD to permit use 
   of Linear Buffers generated by prior ORPG versions */
int orpg_build_i;

/* CVG 4.4 - moved from cvt.c */
char *home_dir;
  
  
#define FALSE 0 
#define TRUE 1 



/* enumerated type to capture the presence of command line arguments 
 */
/* the following is entered in flag[0] to capture which part of the product
 * is to be displayed based on the presence of one of these arguments:
 * "radial", "row", "gab", "tab", "tabv", "satap"  */
/* Used in cvt_dispatcher.c, packet_AF1F.c, packet_BA07.c, and sym_block.c */
  enum{NOPART,RADIAL,ROW,GAB,TAB,TABV,SATAP}; /* product type enumerations */

/* the following is entered in flag[5] to capture the format output of the
 * the traditional packets af1f and ba07 based on the presence of arguments:
 * "rle", "bscan"  */
/* Used in bscan_format.c, cvt_dispatcher.c, cvt_packet_16.c, packet_AF1F.c,
 *         packet_BA07.c, and sym_block.c */
  enum{NOMOD,RLE,BSCAN}; /* product modifier enumerations */

/* the following value is used to capture the extraction command
 * based on the presence of the arguments "extract" and "hexdump"  */
/* Used in cvt_dispatcher.c and inventory_functions.c */
  enum {NONE,EXTRACT,HEXDUMP,BOTH};
  
/* to replace hard coded values in cvt_dispatch.c */
/* entered in flag[7] to represent the presence of the arguments
   scaler, scalev1, scalev2, and scalesw */
#define NOSCALE 0
#define REFL 1
#define VEL1 2
#define VEL2 3
#define SW 4
/* CVT 4.4 - to uses scale offset parameters in file or product */
#define FDECODE 5
#define PDECODE 6


/* CVT 4.4 */
/* used in cvt_dispatcher.c, cvt_packet_28.c, and symblock.c to 
 * determine which components should be listed
 */
  enum {LIST_NONE,LIST_ALL,LIST_AREA,LIST_RAD,LIST_TEXT,
        LIST_TABLE,LIST_GRID,LIST_EVENT};
/* used in cvt_dispatcher.c, cvt_packet_28.c, and symblock.c to 
 * determine which components should be printed
 */
  enum {PRINT_NONE,PRINT_ALL,PRINT_AREA,PRINT_RAD,PRINT_TEXT,
        PRINT_TABLE,PRINT_GRID,PRINT_EVENT};
/* NOTE: AS Of BUILD 11, only text, area, and radial components have been used */
/*                       in products.                                          */
/*                       The concept of the grid component is not complete.    */

    

/* CVT 4.4 - moved here, had multiple definitions */
/* Type of file to be loaded */
#define HEADER_NONE    0
#define HEADER_WMO     1
#define HEADER_PRE_ICD 2

/* CVT 4.4 - moved here, uses in several modules */
#define MAX_PRODUCTS 16000
char cvt_prod_data_base[128];    /* the location of product database file  */
#define MAX_LAYERS 30


/* for length, the value of "message_length" is used while the product is in the */
/*             inear buffer,  The value of "total_length" and product_length"    */
/*             can be used after distribution (i.e, received via nbtcp tool      */
typedef struct {
  short *ptr_to_product;
  /* "message_length" and "hdr_prod_id" require a pre-icd header */
  unsigned int message_length;  /* total length of message in bytes (from internal header) */
  /* CVT 4.4 - added hdr_prod_id */
  int hdr_prod_id;     /* product id from the internal header - future use with   */
                       /* intermediate products                                   */
  /* the following require an ICD product */
  /* CVT 4.4 - added prod_code and prod_id */
  short prod_code;
  short prod_id;               /* derived from the product code */

  int total_length;   /* length of linear buffer message */ 
  int product_length; /* length of ICD product */
  
  short n_blocks;              /* number of data blocks (2 plus optional) */
  short num_layers;            /* number of layers inside symbology block */
  int symb_offset;    /* offset (in HW) to beginning of symbology block */
  int gab_offset;     /* offset (in HW) to graphic alphanumeric block */
  int tab_offset;     /* offset (in HW) to tabular alphanumeric block */
} msg_data;


msg_data md; /* declare global structure to hold needed msg components */


typedef struct{
  short divider;
  short blockID;
  int block_length;
  short n_layers;
  }Sym_hdr;






/* prototypes */
void check_libraries(int install_type);

extern int dispatch_tasks(int argc,char *argv[]);

#endif
