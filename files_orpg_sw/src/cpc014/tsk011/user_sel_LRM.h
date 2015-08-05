/*
*  RCS info
*  $Author: ccalvert $
*  $Date: 2004/02/10 22:09:37 $
*  $Locker:  $
*  $Id: user_sel_LRM.h,v 1.4 2004/02/10 22:09:37 ccalvert Exp $
*  $Revision: 1.4 $
*  $State: Exp $
*  $Log: user_sel_LRM.h,v $
*  Revision 1.4  2004/02/10 22:09:37  ccalvert
*  code cleanup
*
*  Revision 1.3  2004/01/27 00:58:04  ccalvert
*  make site adapt dea format
*
*  Revision 1.2  2002/08/01 15:19:36  steves
*  issue 1-987
*
*  Revision 1.1  2002/02/28 19:05:03  nshen
*  Initial revision
*
*
*/ 


/*****************************************************
**
**  File:	user_sel_LRM.h
**  Author:	Ning Shen
**  Date:	Nov. 9, 2001
**
*/

#ifndef USER_SELECTABLE_LRM_H
#define USER_SELECTABLE_LRM_H

#define TRUE  1
#define FALSE 0 
#define OK    0 
#define ERROR -1 
#define CONTINUE    0         /* continue the control loop */
#define EXIT        1         /* exit control loop */

#define MAX_REQS    10        /* max # of requests per volume */     

#define PCODE     137         /* product code */ 
#define ULR       137         /* output linear buffer number */ 

#define PACKET_CODE 0xAF1F    /* code for ICD radial data packet(16 level) */ 

#define BEG_ELEV    0x00      /* beginning of the elevation */
#define END_ELEV    0x02      /* end of the elevation */
#define BEG_VOL     0x03      /* beginning of the volume */
#define END_VOL     0x04      /* end of the volume */

  /* define max and min dBz and color code */
#define THRESHOLD 2         /* -32 dBZ */
#define MAX_DBZ_INDEX 176   /* = 55 dBZ */
#define MIN_DBZ_INDEX 56    /* = -5 dBZ */
#define INDEX_DELTA   10    /* = delta 5 dBZ */
#define MAX_NUM_DBZ   256   /* number of encoded dBZ value, 0 ~ 255 */
#define MAX_NUM_CCODE 16    /* number of colors */
#define MISSING_CCODE 1     /* missing data color code */
#define BACKGROUND    0     /* background color code */
#define INIT_VALUE    -99   /* initial dBz value */ 

  /* define threshold values for product description block */
#define TH1   0x8401    /* < threshold */
#define TH2   0x8002    /* no data */
#define TH3   0x0505    /* < -5 dBZ */
#define TH4   0x0105    /* -5 dBZ */
#define TH5   0x0       /* 0 dBZ */
#define TH6   0x0205    /* 5 dBZ */
#define TH7   0x020A    /* 10 dBZ */
#define TH8   0x020F    /* 15 dBZ */
#define TH9   0x0214    /* 20 dBZ */
#define TH10  0x0219    /* 25 dBZ */
#define TH11  0x021E    /* 30 dBZ */
#define TH12  0x0223    /* 35 dBZ */
#define TH13  0x0228    /* 40 dBZ */
#define TH14  0x022D    /* 45 dBZ */
#define TH15  0x0232    /* 50 dBZ */
#define TH16  0x0237    /* 55 dBZ */

  /* define constants for slant range calculation */
#define IR  1.21
#define RE  6371        /* earth's radius in km */

#define AZIMUTH    360           /* number of radials per elevation */
#define RANGE1     230           /* 230 km */
#define RANGE2     460           /* 460 km */
 
#define THOUSAND   1000
#define KM_PER_NM  1.852         /* kilometers per nautical mile */  
#define NM_PER_KM  0.54          /* nautical miles per kilometer */ 
#define M_PER_KM   1000          /* meters per kilometer */ 
#define FT_PER_M   3.28          /* feet per meter */
#define KM_PER_KFT 0.3048        /* kilometers per kilo-foot */ 
#define M_PER_FT   0.3048        /* meters per foot */ 

#define SMALL_VALUE 0.02 
#define PI   3.1416
#define DEGREE_TO_RADIAN (PI/180)
#define TOP_LIMIT     70000      /* upper limit in feet */
#define BOTTOM_LIMIT  0
#define INTERVAL      1000       /* minimum depth for each layer in feet */
 
#define MSG_HDR         0        /* message header block entry offset in bytes */ 
#define SYM_BLK_ENTRY   120      /* symbology block entry offset in bytes */
#define PCKT_HDR_ENTRY  136      /* packet header entry offset in bytes */
#define PCKT_DATA_ENTRY 150      /* layer data entry offset in bytes */
#define PCKT_DATA_HDR   6        /* data packet header for each radial, 6 bytes */

#define BEAM_WIDTH 0.95          /* degree of radar beam width */  
#define HALF_BW    (BEAM_WIDTH/2) 

/************** type definitions **************/

typedef struct
{
  short start_bin; 
  short end_bin;
} layer_t;

typedef struct
{
  int bottom;
  int top;
} boundary_t;

  /* symbology block structure */
typedef struct 
{
  short block_divider;
  short block_ID;
  int   block_length;
  short num_layer;
  short layer_divider;
  int   layer_length;
} sym_block_t;

typedef struct
{
  short packet_code;
  short first_bin_index;
  short num_bins;
  short icenter;
  short jcenter;
  short scale_factor;
  short num_radials;
} packet_hdr_t;

typedef struct
{
  short num_2bytes;
  short start_angle;
  short angle_delta;
  unsigned char data[RANGE1];
} packet_data_t;

#endif

 
