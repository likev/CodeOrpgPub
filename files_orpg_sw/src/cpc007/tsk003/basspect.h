/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2006/06/09 21:19:13 $
 * $Id: basspect.h,v 1.1 2006/06/09 21:19:13 steves Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

#ifndef BASSPECT_H
#define BASSPECT_H

#include <rpgc.h>
#include <rpgcs.h>
#include <coldat.h>
#include <a309.h>
#include <packet_af1f.h>

#define NUMPRODS	3
#define SCALED_ZERO	129

/* These are buffer sizes and must be defined in terms of # of bytes. */
#define BSIZ28          96000
#define BSIZ29          92000
#define BSIZ30          92000

/* Colar Table indices for products. */
#define SPNC8           7
#define SPCL8           7

/* Global variables. */
Coldat_t color_data;

int Pflag[3];		/* Product generation flags. */

int Endelcut;		/* End of elevation cut flag. */

short Radcount; 	/* Count of radials in this product. */

short Maxdl60;  	/* Maximum data level for 60 km product. */  

short Maxdl115; 	/* Maximum data level for 115 km product. */

short Maxdl230; 	/* Maximum data level 230 km product. */

short Vc1; 		/* Scaling parameter. */

short Vc2; 		/* Scaling parameter. */

short Elmeas;		/* Elevation measurement. */ 

/* Function Prototypes. */
int a30731_buffer_control();
int a30732_product_generation_control( int *bspcptrs[], void *bdataptr);
int a30733_product_header( short *outbuff, void *bdataptr, int trshind,
                           short pcode, short numbins );
int a30734_maxdl( char *bdataptr, int frst_spw, int end_spw );
int a30735_end_of_product_processing( short *outbuff, int nrleb, int pcode );
int a30736_rel_prodbuf( int *bspcptrs[], int relstat );

# endif
