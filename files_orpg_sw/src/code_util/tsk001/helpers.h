/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/05/15 17:52:39 $
 * $Id: helpers.h,v 1.9 2009/05/15 17:52:39 ccalvert Exp $
 * $Revision: 1.9 $
 * $State: Exp $
 */


#ifndef _88D_HELPERS_H
#define _88D_HELPERS_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>

/* CVG 9.0 */
#include "global.h"

#include "prod_gen_msg.h"


#ifndef TRUE
#define TRUE    1
#endif

#ifndef FALSE
#define FALSE   0
#endif



/* CVG 9.0 */
#define PUP_WIDTH  512
#define PUP_HEIGHT 512
#define RESCALE_FACTOR (0.25)  /* 1/4 km */



/* /////////////////////////////////////////////////////////////////////// */
/* BUILD 5 AND EARLIER INTERNAL PRODUCT HEADER */

typedef struct {
    short prod_id;      /* Product id (buffer number) */
    short input_stream;     /* Replay or realtime product */
    LB_id_t id;         /* corresponding product LB message ID */
    time_t gen_t;       /* local clock (Unix) generation time */
    int vol_t;          /* volscan start time (secs since 01JAN1970) */
    int len;            /* Total product length (bytes).  If
                   this value is negative, indicates
                   an abort message.  The macro definitions
                   for abort reasons are define above. */
    int req_num;        /* Product request number */
    unsigned int vol_num;   /* Volume scan sequence number */
    short req_params[PGM_NUM_PARAMS];
                                /* Product-dependent parameters of product
                                   request. Refer to figure 3-5 and table V
                                   of RPG/APUP ICD */
    short resp_params[PGM_NUM_PARAMS];
                                /* Product-dependent parameters of product
                                   response. Refer to figure 3-5 and table V 
                                   of RPG/APUP ICD */
} Prod_gen_msg_b5;


/* header for all products; The original RPG product and intermediate 
   product will follow this header. */
typedef struct {    /* total size - 96 bytes */

    Prod_gen_msg_b5 g;  /* product generation info */

    int elev_t;     /* elevation time; This is also set for non-elevation 
               based product; elev_t = vol_t indicates that this 
               is the first elevation in a volume */
    short elev_cnt; /* generation sequence number (start with 1) of this 
               elevation based product within a volume; 
               elev_cnt = 1 indicates that this is the first 
               product generated in the volume which may not 
               correspond to the first elevation in the volume */
    short elev_ind; /* elevation index */
    short archIII_flg;  /* This flag indicates if product being read from the
               database is a live or archived product. Two possible
               values for this field are PGM_IS_LIVE and 
               PGM_IS_ARCHIVED */
    short bd_status;    /* status field in the base data header of the last 
               radial that generated this product (it contains 
               useful end of volume info) */
    int spot_blank_bitmap;
            /* the spot blank bitmap from the RPG basedata header */

    short wx_mode;  /* weather mode; 0 = maintenance; 
               1 = clear air; 2 = precipitation */
    short vcp_num;  /* VCP number */

    int compr_method;   /* Compression method used. */
    int orig_size;  /* Original size of the compressed product (does not include 
               product header size). */
    int reserved [4];

} Prod_header_b5;



/* /////////////////////////////////////////////////////////////////////// */
/* BUILD 6 AND LATER INTERNAL PRODUCT HEADER */

typedef struct {
    short prod_id;      /* Product id (buffer number) */
    short input_stream;     /* Replay or realtime product */
    LB_id_t id;         /* corresponding product LB message ID */
    time_t gen_t;       /* local clock (Unix) generation time */
    int vol_t;          /* volscan start time (secs since 01JAN1970) */
    int len;            /* Total product length (bytes).  If
                   this value is negative, indicates
                   an abort message.  The macro definitions
                   for abort reasons are define above. */
    unsigned short req_num; /* Product request number */
    short elev_ind;         /* elevation index */
    unsigned int vol_num;   /* Volume scan sequence number */
    short req_params[PGM_NUM_PARAMS];
                                /* Product-dependent parameters of product
                                   request. Refer to figure 3-5 and table V
                                   of RPG/APUP ICD */
    short resp_params[PGM_NUM_PARAMS];
                                /* Product-dependent parameters of product
                                   response. Refer to figure 3-5 and table V 
                                   of RPG/APUP ICD */
} Prod_gen_msg_b6;


/* header for all products; The original RPG product and intermediate 
   product will follow this header. */
typedef struct {    /* total size - 96 bytes */

    Prod_gen_msg_b6 g;  /* product generation info */

    int elev_t;     /* elevation time; This is also set for non-elevation 
               based product; elev_t = vol_t indicates that this 
               is the first elevation in a volume */
    short elev_cnt; /* generation sequence number (start with 1) of this 
               elevation based product within a volume; 
               elev_cnt = 1 indicates that this is the first 
               product generated in the volume which may not 
               correspond to the first elevation in the volume */
    short archIII_flg;  /* This flag indicates if product being read from the
                       database is a live or archived product. Two possible
               values for this field are PGM_IS_LIVE and 
               PGM_IS_ARCHIVED */
    short bd_status;    /* status field in the base data header of the last 
               radial that generated this product (it contains 
               useful end of volume info) */
    short spare;
    int spot_blank_bitmap;
            /* the spot blank bitmap from the RPG basedata header */

    short wx_mode;  /* weather mode; 0 = maintenance; 
               1 = clear air; 2 = precipitation */
    short vcp_num;  /* VCP number */

    int compr_method;   /* Compression method used. */
    int orig_size;  /* Original size of the compressed product (does not include 
               product header size). */
    int reserved [4];

} Prod_header_b6;
















/* /////////////////////////////////////////////////////////////////////// */
/* ORIGINAL LOOKUP TABLE FOR THE LEGACY PRODUCTS (ID < 130) */

/*  these assignments are determined by the contents of the product_attr_table */
/*  configuration file.   */

/*  CVG 8.4 - In addition, in order to support display of Legacy contour */
/*  products which are no longer configured in the product_attr_table, */
/*  the following have been added to the table */
/*     CODE   ID */
/*      42    28  Echo Tops Contour 2.2 NM */
/*      88   115  Combined Shear Contour */
/*      39    18  Composit Reflectivity Contour 0.54 */
/*      40    19  Composit Reflectivity Contour 2.2 */


/*  a return ID value of 0 is used for a product code that is not  */
/*  assigned to a final product */
int code_to_id[] = 
     {    0,                                              /*  0 NOT USED */
     
 /*        1    2    3    4    5    6    7    8    9   10 */
  /*----------------------------------------------------------------------*/
          0,   0,   0,   0,   0,   0,   0,   0,   1,   0, /*    1 -  10 */
/* 11*/   0,   0,   0,   0,   0,   3,   5,   7,   2,   4, /*   11 -  20 */
/* 21*/   6,  12,  14,  16,  11,  13,  15,   8,   9,  10, /*   21 -  30 */
/* 31*/  56,  57,  58, 119,  24,  26,  23,  25,  18,  19, /*   31 -  40 */
/* 41*/  29,  28,  60,  62,  61,  59,  63,  35,   0,  84, /*   41 -  50 */
          
 /*       51   52   53   54   55   56   57   58   59   60           */
  /*----------------------------------------------------------------------*/
         83,   0,   0,   0,  69,  68,  85,  51,  33,  42, /*   51 -  60 */
/* 61*/  80,  49,  36,  37,  38,  39,  44,   0,   0,   0, /*   61 -  70 */
/* 71*/   0,   0,  43, 129,  32,   0,   0, 105, 106, 107, /*   71 -  80 */
/* 81*/ 108, 109,   0, 110, 111, 112, 114, 115, 116, 117, /*   81 -  90 */
/* 91*/   0,   0,  87,  94, 124, 126, 123, 125,  99,   0, /*   91 - 100 */

 /*      101  102  103  104  105  106  107  108  109  110 */
  /*----------------------------------------------------------------------*/
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0, /*  101 - 110 */
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0, /*  111 - 120 */
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0  /*  121 - 130 */

     };



/********************************************************************************
Helper functions
*********************************************************************************/

short       _88D_orpg_date(time_t seconds);
short       _88D_orpg_time(time_t seconds);
time_t      _88D_unix_time(short date, int time);
char        *_88D_LatLon_to_DDDMMSS(int LatLon);
char        *_88D_LatLon_to_DdotD(int LatLon);
char        *_88D_msecs_to_string (int time);
char            *_88D_secs_to_string(int time);
char        *_88D_date_to_string (short date);

char        *_88D_errno_to_string (int local_errno);
void        _88D_elev_print_status(FILE *fp, int status);
void        _88D_rad_print_status(FILE *fp, int status);

int _88D_Round ( float r ); 


short get_elev_ind(char *bufptr, int orpg_build);

void calendar_date (short date, int *dd, int *dm, int *dy );

void julian_date( int year, int month, int day, int *julian_date );


/* CVG 8.9 */
void non_geo_scale_and_center(float screen_width, float screen_height,
                                 float *x_scale_adjust, float *y_scale_adjust,
                                 int *center_x_adjust, int *center_y_adjust);
void geo_scale_and_center(int screen_width, int screen_height, 
                    float screen_zoom, int screen_x_center, int screen_y_center, 
                          float base_resolution, float product_resolution,
                                 float *x_scale_adjust, float *y_scale_adjust,
                                 int *center_x_adjust, int *center_y_adjust);


int test_for_icd(short div, short ele, short vol, int silent);

void read_to_eol(FILE *list_file, char *buf);

int check_for_directory_existence(char *);




#endif  /* _88D_HELPERS_H */
