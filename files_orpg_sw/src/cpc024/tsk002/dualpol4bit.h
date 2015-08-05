/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/03/03 18:48:36 $
 * $Id: dualpol4bit.h,v 1.3 2009/03/03 18:48:36 ccalvert Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

#ifndef DUALPOL4BIT_H
#define DUALPOL4BIT_H

#include <rpgc.h>
#include <rpgcs.h>
#include <a309.h>
#include <packet_af1f.h>
#include <siteadp.h>

/* These are buffer sizes and must be defined in terms of # of bytes. */
#define OBUF_OVERHEAD  (sizeof(Graphic_product)+sizeof(packet_af1f_hdr_t)+sizeof(packet_af1f_radial_data_t))
#define EST_PER_RAD	     504
#define OBUF_SIZE   ((EST_PER_RAD*400)+OBUF_OVERHEAD)
#define OUTBINS          230
#define MAXBINS		BASEDATA_DOP_SIZE
#define LASTBIN		BASEDATA_DOP_SIZE
#define RF_FLAG          1
#define N4BIT_PRODS      4

#define ZDR4BIT      158
#define   ZDR_MINMAX_PRECISION 0.1
#define CC4BIT       160
#define   CC_MINMAX_PRECISION 0.00333f
#define KDP4BIT      162
#define   KDP_MINMAX_PRECISION 0.05f
#define   MAX_KDP_DISPLAY      10.0f
#define HC4BIT       164

/* Global variables. */
Siteadp_adpt_t Siteadp;		/* Site adaptation data. */

char INDATA_NAME[65];
char OUTDATA_NAME[N4BIT_PRODS][65];

int Prod_id[N4BIT_PRODS];/* Product ID for the each output product. */

int Totalbytes[N4BIT_PRODS]; /* Total byte count for each output product */

int Endelcut;            /* End of elevation cut flag. */

int Proc_rad[N4BIT_PRODS];/* Process radial flag. */

int Mxpos[N4BIT_PRODS];  /* Maximum data level. */

int Mnpos[N4BIT_PRODS];  /* Minimum data level. */

int Radcount[N4BIT_PRODS];/* Radial counter. */

int Elmeas;			/* Elevation angle of product, in degree*10.0. */

int Vc;				/* Scaling constant for packet 16. */


/* Function Prototypes. */
int Dualpol4bit_buffer_control();
int Dualpol4bit_product_generation_control( char *ibrptr, char *bdataptr, int p );
int Dualpol4bit_product_header( char *ibrptr, int volnumber, int p );
int Dualpol4bit_end_of_product_processing( int ndpb, int elev_ind, int p,
                                           int shortbins, char *outbuff );

# endif
