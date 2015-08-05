/* RCS info */
/* $Author: steves $ */
/* $Locker:  $ */
/* $Date: 2009/02/24 18:38:48 $ */
/* $Id: basrflct.h,v 1.3 2009/02/24 18:38:48 steves Exp $ */
/* $Revision: 1.3 $ */
/* $State: Exp $ */

#ifndef BASRFLCT_H
#define BASRFLCT_H

#include <rpgc.h>
#include <rpgcs.h>
#include <coldat.h>
#include <a309.h>
#include <packet_af1f.h>

#define NUMPRODS                6
#define ZPOINT1                 129
#define ZPOINT2                 0

/* Index values for Prod_info array. */
#define PROD16			0
#define PROD19			1
#define PROD17			2
#define PROD20			3
#define PROD18			4
#define PROD21			5

/* These are buffer sizes and must be defined in terms of # of bytes. */
#define BSIZ16                  92000
#define BSIZ17                  92000
#define BSIZ18                  46000
#define BSIZ19                  92000
#define BSIZ20                  92000
#define BSIZ21                  46000

/* Color Table indices for products. */
#define REFNC8                  2
#define REFNC16                 0

#define REFCL8                  2
#define REFCL16                 1

/* Global variables. */
Coldat_t Color_data;

typedef struct prod_info {

   char *pname;		/* Product name string. */

   int prod_id;		/* Product ID. */

   int psize;		/* Product output buffer size. */

   int pflag;		/* Product request flag. */

   int no_mem_flag;	/* No product buffer memory flag. */

   char *brefptrs;	/* Pointer to product buffer. */

   int maxbins;		/* Maximum number of bins in product. */

   int radstep;		/* Step size, in number of bins. */

   int numbins;		/* Number of bins in product. */

   int est_per_rad;	/* Estimated bytes of RLE per radial. */
 
   int clthtind;	/* Color table index. */

   int pbuffind;	/* Product buffer index. */

   int nrleb;		/* Number of run-length-encoded bytes. */

   int radcount;	/* Number of radials in product. */

} Prod_info_t;

    
#ifdef MAIN
Prod_info_t Prod_info[NUMPRODS] = { { "BREF16", 0, BSIZ16, 0, 0, NULL, 230, 1, 0, 252, 0, 0, 0, 0 },
                                    { "BREF19", 0, BSIZ19, 0, 0, NULL, 230, 1, 0, 252, 0, 0, 0, 0},
                                    { "BREF17", 0, BSIZ17, 0, 0, NULL, 230, 2, 0, 252, 0, 0, 0, 0 },
                                    { "BREF20", 0, BSIZ20, 0, 0, NULL, 230, 2, 0, 252, 0, 0, 0, 0 },
                                    { "BREF18", 0, BSIZ18, 0, 0, NULL, 115, 4, 0, 136, 0, 0, 0, 0 },
                                    { "BREF21", 0, BSIZ21, 0, 0, NULL, 115, 4, 0, 136, 0, 0, 0, 0 } };

#else
extern Prod_info_t Prod_info[NUMPRODS];
#endif

#define MAXB230			230
#define MAXB460			460

int Endelcut;
short Vc1;
short Vc2;
short Elmeas;
short Maxdl230;
short Maxdl460;
float Calib_const;

/* Function Prototypes */
void A30711_buffer_control();

#endif
