/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/11/25 16:20:07 $
 * $Id: dualpol8bit.h,v 1.7 2013/11/25 16:20:07 steves Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */

#ifndef DUALPOL8BIT_H
#define DUALPOL8BIT_H

#include <rpgc.h>
#include <rpgcs.h>
#include <a309.h>
#include <packet_16.h>
#include <siteadp.h>

/* These are buffer sizes and must be defined in terms of # of bytes. */
#define OBUF_OVERHEAD  (sizeof(Graphic_product)+sizeof(Packet_16_hdr_t)+sizeof(Packet_16_data_t))
#define OBUF_SIZE   ((BASEDATA_DOP_SIZE*400)+OBUF_OVERHEAD)
#define MAXBINS        1200
#define EST_PER_RAD	   1250

#define RF_FLAG        1
#define N8BIT_PRODS    4

#define ZDR8BIT      159
#define   ZDR_MINMAX_PRECISION 0.1f
#define   ZDR_ICD_SCALE 16
#define   ZDR_ICD_OFFSET 128
#define CC8BIT       161
#define   CC_MINMAX_PRECISION 0.00333f
#define   CC_ICD_SCALE 300
#define   CC_ICD_OFFSET -60.5
#define KDP8BIT      163
#define   KDP_MINMAX_PRECISION 0.05f
#define   KDP_ICD_SCALE 20
#define   KDP_ICD_OFFSET 43
#define HC8BIT       165
#define   DHC_SCALE    1.f  
#define   DHC_OFFSET   0.f
#define PHI8BIT      340
/*#define   PHI_ICD_SCALE 2.8361f*/
#define   PHI_ICD_SCALE 0.70277f /* 8-bits with a range of 0 to 360 deg */
#define   PHI_ICD_OFFSET 2
#define SNR8BIT      341
#define SMZ8BIT      342
#define SDZ8BIT      344
/* Global variables. */
Siteadp_adpt_t Siteadp;		/* Site adaptation data. */

char INDATA_NAME[65];
char OUTDATA_NAME[N8BIT_PRODS][65];

int Prod_id[N8BIT_PRODS]; /* Product ID for the product. */

int Totalbytes[N8BIT_PRODS];/* Total byte count for each output product */

int Endelcut;			/* End of elevation cut flag. */

int Proc_rad[N8BIT_PRODS];/* Process radial flag. */

int Radcount[N8BIT_PRODS];/* Radial counter. */

int Elmeas;			/* Elevation angle of product, in degree*10.0. */

int Vc;				/* Scaling constant for packet 16. */

/* Function Prototypes. */
int Dualpol8bit_buffer_control();
int Dualpol8bit_product_generation_control( char *ibrptr, char *bdataptr, int p );
int Dualpol8bit_product_header( char *ibrptr, int volnumber, int p );
int Dualpol8bit_end_of_product_processing( int ndpb, int elev_index, int numbins,
                                           int p, char *outbuff );

# endif
