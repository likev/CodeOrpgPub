/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/06/11 16:53:14 $
 * $Id: prod_cmpr.h,v 1.6 2014/06/11 16:53:14 steves Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */  

#ifndef PROD_CMPR_
#define PROD_CMPR_

#include <product.h>
#include <legacy_prod.h>
#include <rpgcs_time_funcs.h>

/* Function return values */
#define PROD_NOT_MATCH -5
#define ELANGLE_MISMATCH -4
#define VOLTIME_MISMATCH -3
#define PRODCODE_MISMATCH -2
#define FILESIZE_MISMATCH -1
#define PROD_MATCH_FOUND 0

/* Miscellaneous macros */
#define MAX_PROD_ID  2000
#define MAX_MNEMONIC_LEN 5
#define DESC_BLK_HW_OFFSET 9
#define DATA_BLK_HW_OFFSET 60


/* Necessary function prototypes */
int Init_attr_tbl( char *file_name, int clear_table );
void* Decompress_product( void *bufptr, int *size );

/* This structure has all the information needed to create a product data file
 * */
typedef struct 
{
  char*                filename;
  size_t               file_sz;
  Prod_msg_header_icd* msg_hdr_ptr;
  Prod_desc_blk_st*    prod_desc_blk_ptr;
  short*               rest_of_data_ptr;
} Prod_File_st;


/* This structure is used to house the product comparison summary data */
typedef struct 
{
  int               total_num; /* total number in product comparison */
  int               num_miscompared; /* total files that miscompared for this
                                        product */
} Prod_summary_st;

typedef struct wmo_header {

   char form_type[2];           /* Should always be "S" - surface data followed by "D" - radar reports (i.e., SD) */
   char geo_area[2];            /* Country of original 2 letter designatorr... normally "US" */
   char distribution[2];        /* Not sure what value to give .... setiting to "00" */
   char space1;
   char originator[4];          /* Use 4 character ICAO. */
   char space2;
   char date_time[6];           /* In YYGGgg format, where YY is day of month, GGgg is hours and minutes, UTC. */
   char crcrlf[3];

} WMO_header_t;

typedef struct awips_header {

   char category[3];
   char product[3];             /* Use ICAO without the leading "K" or "P", i.e., last three characters */
   char crcrlf[3];

} AWIPS_header_t;


typedef struct WMO_AWIPS_hdr {

   WMO_header_t wmo;
   AWIPS_header_t awips;

} WMO_AWIPS_hdr_t;


#endif
