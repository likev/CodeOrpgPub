/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2009/02/23 22:55:35 $
 * $Id: bvel8bit.h,v 1.4 2009/02/23 22:55:35 steves Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */

#ifndef BVEL8BIT_H
#define BVEL8BIT_H

#include <rpgc.h>
#include <rpgcs.h>
#include <coldat.h>
#include <a309.h>
#include <packet_16.h>
#include <siteadp.h>

/* These are buffer sizes and must be defined in terms of # of bytes. */
#define OBUF_SIZE       483000
#define EST_PER_RAD	1206
#define MAXBINS		1200
#define LASTBIN		1200

/* Global variables. */
Siteadp_adpt_t Siteadp;		/* Site adaptation data. */

int Pcode;			/* Product code for the 8-bit product. */

int DV_prod_id;			/* Product ID for the DV product. */

int Endelcut;			/* End of elevation cut flag. */

int Proc_rad;			/* Process radial flag. */

int Mxpos;			/* Maximum data level. */

int Mxneg;			/* Maximum data level. */

int Radcount;			/* Radial counter. */

int Elmeas;			/* Elevation angle of product, in degree*10.0. */

int Vc;				/* Scaling constant for packet 16. */

int Zero_velocity;		/* ICD values corresponding to 0.0 m/s. */

/* Function Prototypes. */
int BVEL8bit_buffer_control();
int BVEL8bit_product_generation_control( char *ibrptr, char *bdataptr );
int BVEL8bit_product_header( char *ibrptr, int volnumber, int velreso );
int BVEL8bit_maxdl( char *ibrptr, int first_bin, int last_bin );
int BVEL8bit_end_of_product_processing( int ndpb, int numbins, char *outbuff );

# endif
