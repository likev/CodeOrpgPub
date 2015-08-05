/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/03/03 18:48:39 $
 * $Id: mlprod.h,v 1.2 2009/03/03 18:48:39 ccalvert Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

#ifndef MLPROD_H
#define MLPROD_H

#include <rpgc.h>
#include <rpgcs.h>
#include <a309.h>
#include <packet_af1f.h>
#include <siteadp.h>

/* These are buffer sizes and must be defined in terms of # of bytes. */
#define BYTES_PER_VECT   4
#define SHORTS_PER_VECT  2
#define ML_TOP_EDGE      1
#define ML_TOP_CENTER    2
#define ML_BOT_CENTER    3
#define ML_BOT_EDGE      4
#define OBUF_OVERHEAD  (sizeof(Graphic_product)+sizeof(Symbology_block)+sizeof(packet_0e03_hdr_t))
#define OBUF_SIZE   ((730*BYTES_PER_VECT*4)+OBUF_OVERHEAD)
#define M_TO_QKM         0.004f
/* LABEL_DIS_THRESH is in units of meters squared 1e08 = 10km */
#define LABEL_DIST_THRESH 1e08
#define EST_PER_RAD	     504
#define MAXBINS		BASEDATA_DOP_SIZE

/* Global variables. */
Siteadp_adpt_t Siteadp;		/* Site adaptation data. */

char INDATA_NAME[65];

char OUTDATA_NAME[65];

int Pcode;			/* Product code for the 4-bit product. */

int Prod_id;		     /* Product ID for the product. */

int Endelcut;			/* End of elevation cut flag. */

int Proc_rad;			/* Process radial flag. */

int Mxpos;			/* Maximum data level. */

int Mnpos;               /* Minimum data level. */

int Radcount;			/* Radial counter. */

int Elmeas;			/* Elevation angle of product, in degree*10.0. */

int Vc;				/* Scaling constant for packet 16. */


/* Function Prototypes. */
int mlprod_buffer_control();
int mlprod_generation_control( char *ibrptr, char *bdataptr );
int mlprod_header( char *ibrptr, int volnumber );
int mlprod_end_of_product_processing( int elev_ind, float elev, char *outbuff );
void add_label( int range, short i, short j, char *label); 
# endif
