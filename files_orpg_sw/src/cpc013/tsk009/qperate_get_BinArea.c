/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/01/10 21:00:00 $
 * $Id: qperate_get_BinArea.c,v 1.7 2012/01/10 21:00:00 ccalvert Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */

/* __USE_GNU directs the preprocessor to use the long ('l') value for pi
 * (double precision) inside math.h.
 */

#define __USE_GNU
#include <math.h>
#include <stdio.h>

/*** Local Include Files  ***/

#include "qperate_func_prototypes.h"

#define BIN_RES   250.000000000L   /* The length of one range bin in meters   */
#define BEAM_WIDTH  1.00000000000L /* Width of radar beam in degrees          */

/* 20111207 Ward CCR NA11-00376 Notes:
 *
 * 1. The base data originally comes in at a 0.9 degree beamwidth
 * (PPS hydromet_prep.alg beam_width = 0.9) but in a preprocessing step,
 * they are re-combined into exactly 1.0 degree-wide beams, with the
 * beam centers all at the half-degree azimuth marks.
 *
 * 2. Derivation from AEL 3.2.2 E formula:
 *
 * AREA_SV = PI * ((RNG + (RAD_RES/2))^2 - (RNG - (RAD_RES/2))^2) * (AZ_RES)/360
 *
 * Because RNG is the bin center, and BIN_RES = RAD_RES = 250 m,
 * RNG + (RAD_RES/2) = bin leading  edge = radius_two,
 * RNG - (RAD_RES/2) = bin trailing edge = radius_one
 *
 * AREA_SV = PI * ((radius_two)^2 - (radius_one)^2) * (BEAM_WIDTH/360)
 *
 * 3. The formula could be further simplified into a 1-liner:
 *
 * BEAM_WIDTH = 1.0
 *
 * AREA_SV = PI * ((RNG + (RAD_RES/2))^2 - (RNG - (RAD_RES/2))^2) * (1.0/360)
 * AREA_SV = ((RNG + (RAD_RES/2))^2 - (RNG - (RAD_RES/2))^2) * PI/360
 *
 * Using (A+B)^2 - (A-B)^2 = 4*A*B; with A = RNG, B = RAD_RES/2
 *
 * AREA_SV = (4 * RNG * RAD_RES/2) * PI/360
 * AREA_SV = (RNG * RAD_RES) * PI/180
 *
 * Because RNG is the bin center, then
 * RNG = (i * RAD_RES) + RAD_RES/2, i = 0, 1, ... 919
 * RNG = (i + 1/2) * RAD_RES, i = 0, 1, ... 919
 * AREA_SV = (i + 1/2) * RAD_RES * RAD_RES * PI/180 (m^2)
 * AREA_SV = (i + 1/2) * RAD_RES^2 * PI/180 (m^2)
 * AREA_SV = (i + 1/2) * 62500 * PI/180 (m^2)
 * AREA_SV = (i + 1/2) * 3125 * PI/9 (m^2)
 * AREA_SV = (i + 1/2) * 3125 * PI/9 * 0.000001 (km^2)
 * AREA_SV = (i + 1/2) * 0.0010908307825 (km^2)
 */

/******************************************************************************
    Filename: qperate_get_BinArea.c

    Description:
    ============
       get_BinArea() returns the area, in square kilometers, of the sample
    volume specified by rng.  The area for all sample volumes are
    calculated and stored the first time this function is called.

    Input:
       int rng - range bin

    Return:
       The bin area in square kilometers.

    Change History
    ==============
    DATE        VERSION    PROGRAMMER        NOTES
    ----        -------    ----------        --------------------------
    27 Feb 07    1.0       Daniel Stein      Initial implementation for
                                             dual-polarization project
                                             (RPG Build 11).
    12 Apr 07    2.0       Dan Stein         Corrected an error in the
                                             computations; accuracy was
                                             improved to 100%.
    07 Dec 11    3.0       James Ward        CCR NA11-00376. Changed BEAM_WIDTH
                                             to 1.0 to match AEL 3.2.2 E
******************************************************************************/

long double get_BinArea ( int rng )
{
   static int areas_computed = FALSE; /* Tells us whether the areas for the
                                       * range bins have already been computed.
                                       * This should only be done once per
                                       * execution. */

   static long double area_array[MAX_BINS]; /* Area (in km^2) covered by
                                             * each range bin. */

   int i; /* loop counter */

   #ifdef QPERATE_DEBUG
      fprintf(stderr, "Beginning get_sector_area() .........\n\n");
   #endif

   if ( !areas_computed ) /* If we haven't already done this once */
   {
      /* The following variables are declared only inside this block in order
       * to make the operation of this function more efficient.  These
       * variables are only used once, the first time the function is called.
       * Since the function will be called thousands of times per hour, we
       * decided to declare and initialize these only once */

      /* Radius of the first (smaller, inner) circle */

      long double radius_one = 0.0000000000L;

      /* Radius of the second (larger, outer) circle */

      long double radius_two = (long double) BIN_RES;

      areas_computed = TRUE;

      for ( i = 0; i < MAX_BINS; ++i )
      {
         /* Area of 0.5 degrees from the outer ring */
         area_array[i] = ((radius_two * radius_two * M_PIl) -
                          (radius_one * radius_one * M_PIl)) /
                          (long double) (360.000L / BEAM_WIDTH);
         radius_one += BIN_RES;
         radius_two += BIN_RES;
      }  /* for i */

   }  /* if (!areas_computed) */

   return ( area_array[rng] * 0.000001 );

} /* end get_BinArea() =========================== */
