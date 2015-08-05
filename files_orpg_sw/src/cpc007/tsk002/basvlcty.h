/* RCS info */
/* $Author: steves $ */
/* $Locker:  $ */
/* $Date: 2009/02/24 18:28:11 $ */
/* $Id: basvlcty.h,v 1.5 2009/02/24 18:28:11 steves Exp $ */
/* $Revision: 1.5 $ */
/* $State: Exp $ */

#ifndef BASVLCTY_H
#define BASVLCTY_H

#include <rpgc.h>
#include <rpgcs.h>
#include <coldat.h>
#include <a309.h>
#include <packet_af1f.h>

#define NUMPRODS                6
#define ZPOINT1                 129
#define ZPOINT2                 0

/* These are buffer sizes and must be defined in terms of # of bytes. */
#define BSIZ22                  96000
#define BSIZ23                  92000
#define BSIZ24                  92000
#define BSIZ25                  96000
#define BSIZ26                  92000
#define BSIZ27                  92000

/* Color Table indices for products. */
#define VELCL81                 26
#define VELCL82                 28
#define VELCL161                25
#define VELCL162                27

#define VELNC81                 4
#define VELNC82                 6
#define VELNC161                3
#define VELNC162                5

/* Global variables. */
Coldat_t color_data;

typedef struct prod_info {

   char *pname;		/* Product name string. */

   int prod_id;		/* Product ID. */

   int psize;		/* Product output buffer size. */

   int pflag;		/* Product request flag. */

   int no_mem_flag;	/* No product buffer memory flag. */

   int *bvelptrs;	/* Pointer to product buffer. */

   int maxbins;		/* Maximum number of bins in product. */

   int radstep;		/* Radial bin step size. */

   int est_per_rad;	/* Estimated bytes of RLE per radial. */

   int radcount;	/* Radial counter. */

} Prod_info_t;

    
#ifdef MAIN
Prod_info_t Prod_info[NUMPRODS] = { { "BVEL22", 0, BSIZ22, 0, 0, NULL, 240, 1, 252, 0 },
                                    { "BVEL23", 0, BSIZ23, 0, 0, NULL, 230, 2, 252, 0 },
                                    { "BVEL24", 0, BSIZ24, 0, 0, NULL, 230, 4, 252, 0 },
                                    { "BVEL25", 0, BSIZ25, 0, 0, NULL, 240, 1, 252, 0 },
                                    { "BVEL26", 0, BSIZ26, 0, 0, NULL, 230, 2, 136, 0 },
                                    { "BVEL27", 0, BSIZ27, 0, 0, NULL, 230, 4, 136, 0 } };

#else
extern Prod_info_t Prod_info[NUMPRODS];
#endif

int Endelcut;
short Mxneg60;
short Mxneg115;
short Mxneg230;
short Vc1;
short Vc2;
short Elmeas;
short Mxpos60;
short Mxpos115;
short Mxpos230;

/* Function Prototypes */

int a30721_buffer_control();
int a30722_product_generation_control( void *bdataptr);
int a30723_product_header( void *bdataptr, int trshind, int pindx, short numbins);
int a30724_maxdl( char *dataptr, int frst_vel, int end_vel);
int a30725_end_of_product_processing( int nrleb, int pindx, int vres);
int a30726_rel_prodbuf(int relstat);
int a30727_get_colorix( Base_data_header *radhead, int *clthtind);


#endif
