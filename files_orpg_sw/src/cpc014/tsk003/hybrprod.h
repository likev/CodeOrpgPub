/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2007/06/18 23:19:22 $
 * $Id: hybrprod.h,v 1.2 2007/06/18 23:19:22 steves Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

#ifndef HYBRPROD_H
#define HYBRPROD_H

#include <rpgc.h>
#include <rpgcs.h>
#include <itc.h>
#include <siteadp.h>
#include <coldat.h>
#include <alg_adapt.h>
#include <hydromet.h>
#include <a313hparm.h>
#include <packet_16.h>
#include <packet_af1f.h>
#include <packet_1.h>

/* Macro definitions. */
#define SIZE_P32        85872
#define SIZE_P33        81920

/* Global variables. */
A3136C3_t A3136c3;
Siteadp_adpt_t Siteadp;
Coldat_t Color_table;
hydromet_adj_t Hyd_adj;
int HSR_color_index;
int Code_DHR;
int Code_HSR;

/* Function prototypes. */
void A31431_buffer_controller();
int  A31432_determ_maxval( void *input );
unsigned int  A31433_dig_radial( short *input, short *output );
void A31434_append_ascii( int *hydrmesg, float *hydradap, int *supl_pre, 
                          unsigned int lyr1en, unsigned short *pbuff );
void A31436_hr_product_hdr( short *prodbuf, int vsnum, int prodcode,
                            int maxval, int *hydrsupl );
void A3143a_hsr_rle( short *inbuf, short *outbuf );

 
#endif
