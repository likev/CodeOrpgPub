/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2009/02/18 18:47:13 $
 * $Id: bref8bit.h,v 1.2 2009/02/18 18:47:13 steves Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

#ifndef BREF8BIT_H
#define BREF8BIT_H

#include <rpgc.h>
#include <rpgcs.h>
#include <coldat.h>
#include <a309.h>
#include <packet_16.h>
#include <siteadp.h>

/* These are buffer sizes and must be defined in terms of # of bytes. */
#define OBUF_SIZE       188000
#define EST_PER_RAD	466
#define MAXBINS		460
#define LASTBIN		460

/* Global variables. */
Siteadp_adpt_t Siteadp;		/* Site adaptation data. */

int Pcode;			/* Product code for the 8-bit product. */

int DR_prod_id;			/* Product ID for the DR product. */

int Endelcut;			/* End of elevation cut flag. */

int Proc_rad;			/* Process radial flag. */

int Mxpos;			/* Maximum data level. */

int Radcount;			/* Radial counter. */

int Elmeas;			/* Elevation angle of product, in degree*10.0. */

int Vc;				/* Scaling constant for packet 16. */

/* Function Prototypes. */
int BREF8bit_buffer_control();
int BREF8bit_product_generation_control( char *ibrptr, char *bdataptr );
int BREF8bit_product_header( char *ibrptr, int volnumber );
int BREF8bit_maxdl( char *ibrptr, int first_bin, int last_bin );
int BREF8bit_end_of_product_processing( int ndpb, int numbins, char *outbuff );

# endif
