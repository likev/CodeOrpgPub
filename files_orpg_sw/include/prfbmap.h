/*
 * RCCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2007/03/05 22:44:27 $
 * $Id: prfbmap.h,v 1.5 2007/03/05 22:44:27 ryans Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

/************************************************************************
 *	prfbmap.h - This header file defines the constants and structes	*
 *	used for the PRF obscuration data product (PRFOVLY).  This	*
 *	product is used to identify, for different PRFs, areas of range	*
 *	folding (obscuration).  The obscuration data are contained in	*
 *	bitmaps.  The number of bitmaps correspond to the number of	*
 *	PRFs defined for a given product.				*
 ************************************************************************/

#include <basedata.h>


/*	Macro Definitions.  If changes made here, may need to change	*
  *	file a305prf.inc.						*/

#define MAX_ALWBPRFS   8        /* Maximum allowable PRFs.		*/      

#define MAX_RANGE      230      /* The maximum range of bit map, in km.	*/

#define MAX_BYTES      ( (MAX_RANGE/8) + 1 )
                                /* Number of bytes required for bit	*
                                 * map per radial.			*/



/*	Structure Definition For PRF Obscuration Bit Map Product Header.*/

typedef struct prfbmap_header {

   unsigned int   num_radials;  /* Number of radials in each bit map.	*/

   unsigned int   time;         /* Volume scan start time for product,	*
                                 * in milliseconds after midnight.	*/

   unsigned short date;         /* Volume scan start date for product,	*
                                 * modified Julian.			*/

   unsigned short num_prfs;     /* Number of PRF bit maps.  Currently 	*
                                 * limited to MAX_ALWBPRFS.		*/ 

   unsigned short prf_nums;     /* Bit map of PRF numbers. 		*
                                 *    PRF 1 - Bit 0			*
                                 *    PRF 2 - Bit 1			*
                                 *         etc				*/

   unsigned short elev_angle;   /* Elevation angle of product, in 	*
                                   degrees * 10.			*/

   unsigned short vcp_num;      /* Volume Coverage Pattern Number.	*/

   unsigned short vol_num;      /* Volume Scan Number (1 - 80)		*/

} Prfbmap_hdr_t;


/*	Structure definition for obscuration bit map for each radial.	*/

typedef struct radial_bitmap {

   unsigned short azm;          /* Azimuth angle of radial, in degrees	*
                                 * 10.					*/

   unsigned char  delta_azm;    /* Delta azimuth, in degrees * 10.	*/

   unsigned char  radbmap[MAX_BYTES];
                                /* Obscuration Bit Map for current 	*
                                 * radial.  Each bit is a range cell. 	*
                                 * Bit 0 is range cell 0.		*/ 

} Radbmap_t;


/*	Structure definition for obscuration bit map for elevation cut.*/

typedef struct prf_bitmap {

   Radbmap_t prfbmap[MAX_RADIALS]; 
                                /* Obscuration bit map for elevation	*
                                 * cut.					*/

} Prfbmap_t;


/*	Structure defintion for the PRF obscuration bitmap product.	*/

typedef struct bitmap {

   Prfbmap_hdr_t hdr;           
                                /* Product header.			*/
   Prfbmap_t    bmap[MAX_ALWBPRFS];
                                /* Obscuration bit maps.  One for each	*
                                 * allowable PRF.			*
				 * NOTE:  The first bitmap corresponds	*
				 * to the first allowable PRF, not PRF1.*
				 * For example, if the allowable PRFs	*
				 * are 4-8, then the first bitmap in the*
				 * list is for PRF 4.  The trailing 3	*
				 * bitmaps in this example would not	*
				 * be used.				*/

} Prfbmap_prod_t;
