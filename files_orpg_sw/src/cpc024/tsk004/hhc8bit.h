/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/03/03 18:48:38 $
 * $Id: hhc8bit.h,v 1.2 2009/03/03 18:48:38 ccalvert Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

#ifndef HHC8BIT_H
#define HHC8BIT_H

#include <rpgc.h>
#include <rpgcs.h>
#include <a309.h>
#include <packet_16.h>
#include <siteadp.h>

/* These are buffer sizes and must be defined in terms of # of bytes. */
#define OBUF_OVERHEAD  (sizeof(Graphic_product)+sizeof(Packet_16_hdr_t)+sizeof(Packet_16_data_t))
#define OBUF_SIZE   ((BASEDATA_DOP_SIZE*400)+OBUF_OVERHEAD)
#define EST_PER_RAD	     504
#define MAXBINS		920
#define RF_FLAG        1

/* Global variables. */
Siteadp_adpt_t Siteadp;		/* Site adaptation data. */

char INDATA_NAME[65];
char OUTDATA_NAME[65];

int Pcode;			/* Product code for the 8-bit product. */

int Prod_id;		     /* Product ID for the product. */

int Proc_rad;			/* Process radial flag. */

int Radcount;			/* Radial counter. */

int Vc;				/* Scaling constant for packet 16. */

/* Function Prototypes. */
int Hhc8bit_buffer_control();
int Hhc8bit_product_generation_control( char *ibrptr, char *bdataptr );
int Hhc8bit_product_header( char *ibrptr, int volnumber );
int Hhc8bit_end_of_product_processing( int ndpb, int numbins, char *outbuff );

# endif
