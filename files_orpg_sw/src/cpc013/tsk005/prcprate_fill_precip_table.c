/*
 * RCS info
 * $Author: ryans $
 * $Locker:  $
 * $Date: 2006/02/09 18:19:58 $
 * $Id: prcprate_fill_precip_table.c,v 1.2 2006/02/09 18:19:58 ryans Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */
/****************************************************************************
   Filename: prcprate_fill_precip_table.c

   Description
   ===========
      This function initializes look-up table for the conversion of biased 
   dbz to precipitation rate in 10ths of mm/hr.

   Change History
   =============
   08/25/95      0000      Dennis Miller        CCR NA94-08459
   12/23/96      0001      Toolset              CCR NA95-11807
   03/16/99      0002      Toolset              CCR NA98-23803
   01/13/05      0003      D. Miller; J. Liu    CCR NA04-33201 
   02/20/05      0004      Cham Pham            CCR NA05-01303
   10/26/05      0005      Cham Pham            CCR NA05-21401
****************************************************************************/
/* Global include files */
#include <rpgc.h>
#include <a313hbuf.h>
#include <a313h.h>

/* Local include file */
#include "prcprtac_Constants.h"

#define DB               10.	/* Base 10 exponential power                  */
#define INVRT             1.0	/* Parameter used to invert a number          */
#define MIN_NONZERO_RATE  0.05	/* Minimum non-zero precip rate               */
#define MAX_RATE_POS   3200.0	/* Maximum allowable precip rate in conversion
                                   table.                                     */
#define ZERO_RATE         0	/* Parameter for zero precip rate             */

void fill_precip_table( double mindbz, double maxdbz, 
                        double zrmlt_coeff, double zrpwr_coeff )
{     
int   i,             /* Loop counter                                          */
      min_brefl,     /* Minimum biased dBZ index for conversion to precip rate*/
      max_brefl;     /* Maximum biased dBZ index for conversion to precip rate*/
  
double dbz,          /* Reflectivity value in dBZ                             */
       ze,           /* Equivalent Reflectivity                               */
       power_coeff;  /* Inverse of zrpwr_coeff                                */
      
   if (DEBUG) {fprintf(stderr,"========= FILL PRECIP TABLE A3134J =======\n");}

/* Inverse of zrpwr_coeff */
   power_coeff = INVRT/zrpwr_coeff;

/* Convert the minimum resolvable rate to dbz*/
   dbz = DB*log10(zrmlt_coeff*pow(MIN_NONZERO_RATE,zrpwr_coeff));

/* Use the maximum of the lowest resolvable reflectivity and the
   minimum dbz from the adaptation parameter
 */
   if (dbz < mindbz) 
   {
     dbz = mindbz;
   }

/* Compute the minimum biased dbz indx for which precip rate must be computed*/
/* Note: Changed for LINUX - Used RPGC_NINT library function instead of adding
         0.5 for rounding to the nearest integer.
 */

   min_brefl = RPGC_NINT((bdbz_cftr*(dbz+bdbz_const_offset))+bdbz_indx_offset);

/* Check to make sure adaptation max reflectivity doesn't correspond
   to a rate greater than what an i*2 word can handle 
 */
   dbz = DB*log10( zrmlt_coeff * pow(MAX_RATE_POS, zrpwr_coeff) );
   if (dbz > maxdbz) 
   {
     dbz = maxdbz;
   }

/* Compute the maximum biased dbz index for which precip rate must be computed*/
/* Note: Changed for LINUX - Used RPGC_NINT library function instead of adding
         0.5 for rounding to the nearest integer.
 */

   max_brefl = RPGC_NINT((bdbz_cftr*(dbz+bdbz_const_offset))+bdbz_indx_offset);

/* Set all table values = 0 for dbz < min_brefl*/
   for ( i=0; i<min_brefl; i++ ) 
   {
      a3134ca.rate_table[i] = ZERO_RATE;
   }

/* Do for all dbz values that need a rate computed */
   for ( i=min_brefl; i<=max_brefl; i++ ) 
   {

/* Convert biased reflectivity to dbz */
      dbz = (double)(i-bdbz_indx_offset)/bdbz_cftr-bdbz_const_offset;

/* Convert dbz to equivalent reflectivity (ze) */
      ze = pow(DB,(dbz/DB));

/* Compute precip rate from ze and scale by the scaling factor */
/* Note: Changed for LINUX - Used RPGC_NINT library function instead of adding
         0.5 for rounding to the nearest integer.
 */

      a3134ca.rate_table[i] =(short)RPGC_NINT(pow((ze/zrmlt_coeff),power_coeff)*
                                       RATE_SCALING);
   }/* End max_brefl looping */

/* Do for all dbz greater than the maximum biased dbz and set
   equal to the maximum precip rate at the maximum reflectivity 
 */
   for ( i=max_brefl+INCR; i<MAX_BDBZ; i++ ) 
   {
      a3134ca.rate_table[i] = a3134ca.rate_table[max_brefl];
   }

/* Set missing value*/
   a3134ca.rate_table[MAX_BDBZ] = FLG_MISSNG;

   if (DEBUG) {fprintf(stderr,"========= End A3134J =============\n");}
}
