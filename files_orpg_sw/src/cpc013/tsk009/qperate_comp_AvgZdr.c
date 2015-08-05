/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/04/03 19:52:21 $
 * $Id: qperate_comp_AvgZdr.c,v 1.3 2009/04/03 19:52:21 ccalvert Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

/*** Local Include Files  ***/

#include "qperate_func_prototypes.h"

/* Macros for finding min and max value */

/* 20080812 Ward - Commented out, no longer used
#define min(x1,x2) ((x1) > (x2))? (x2):(x1)
#define max(x1,x2) ((x1) > (x2))? (x1):(x2) */

/******************************************************************************
    Filename: qperate_comp_AvgZdr.c

    Description:
    ============
      get_Zdr_Accumulations() adds Zdr_linear to zdrsum if the data is valid
      and increments the count.   This function sums all Zdr values in a window
      bounded by hi and low az and hi and low rng.

    Input:
       low_az            - Azimuth low boundary value
       hi_az             - Azimuth high boundary value
       low_rng           - Range low boundary value
       hi_rng            - Range high boundary value
       Zdr_array[][]     - Zdr (processed) in dB
       hydro_classes[][] - Hydrometeor Classification data.

    Output:
       zdrsum   - Sum Zdr linear for compute average.
       zdrcount - Number of items for average.

    20080812 Ward - Commented out, no longer used

    Change History
    ==============
    DATE        VERSION    PROGRAMMER         NOTES
    ----        -------    ----------         -----
    01/09/07    1.0        Daniel Stein       Initial implementation for
                                              dual-polarization project
                                              (ORPG Build 11).

******************************************************************************/

/* void get_Zdr_Accumulations ( int low_az, int hi_az, int low_rng, int hi_rng,
 *                              float *zdrsum, int *zdrcount,
 *                              float Zdr_array[][MAX_BINS],
 * 			     short hydro_classes[][MAX_BINS] )
 * {
 *    int k, m;                       -* Loop counters              *-
 *    float Zdr_linear;               -* contains Zdr linear value  *-
 *    short hc_type;                  -* contains hydro class value *-
 *
 *    -* NOTE: We're using less than OR EQUAL operators in these loops *-
 *    for ( k = low_az; k <= hi_az; ++k )
 *    {
 *       for ( m = low_rng; m <= hi_rng; ++m )
 *       {
 * 	 -* If this sample volume isn't NODATA and if it's a precip type, then
 * 	  * we can use it in our average Zdr computations.  Otherwise, we just
 * 	  * ignore it.
 * 	  *-
 *        hc_type = hydro_classes[k][m];
 *
 *          if ( ( Zdr_array[k][m] != QPE_NODATA ) && (
 *               ( hc_type == DS) ||
 *               ( hc_type == WS) ||
 *               ( hc_type == IC) ||
 *               ( hc_type == GR) ||
 *               ( hc_type == BD) ||
 *               ( hc_type == RA) ||
 *               ( hc_type == HR) ||
 *               ( hc_type == RH) )
 *             )
 *          {
 * 	    #ifdef QPERATE_DEBUG
 *            {
 *               fprintf(stderr,
 *                       "Adding Zdr_array[%d][%d], value %f, to zdrsum\n",
 *                       k, m, Zdr_array[k][m]);
 *             }
 *             -* Convert Zdr (processed) to Zdr linear *-
 *             Zdr_linear = powf ( 10., ( Zdr_array[k][m] * 0.1 ) );  
 *
 *             -* Adds to the sum if the data is valid, increments the count *-
 *             *zdrsum += Zdr_linear;
 *             ++(*zdrcount);
 *
 *          } -* if precip *-
 *       }  -* for m *-
 *    }  -* for k *-
 *
 *    #ifdef QPERATE_DEBUG
 *       fprintf ( stderr, "Exit get_Zdr_Accumulation() ..........\n" );
 *
 * } /* end get_Zdr_Accumulation()============================== */

/******************************************************************************
    Filename: qperate_comp_AvgZdr.c

    Description:
    ============
    compute_Avg_Zdr() computes a windowed average centered on the SAMPLE VOLUME
    specified at [az][rng].  This function assumes that win_size is an odd
    value.  If the  [az][rng] coordinate is near an az boundary in the array
    (near 0 degrees or near 360 degrees), this algorithm will wrap around the
    other "side" to get all the relevant values.  This algorithm  will NOT wrap
    values near the boundaries in rng, but that is how it should work.

    Input:
       Zdr_array[][] - Zdr (processed) in dB
       hydrClass[][] - hydrometeor classification
       az            - azimuth index
       rng           - range index

    Return:
       The average Zdr value.

    20080812 Ward - Commented out, no longer used

    Change History
    ==============
    DATE        VERSION    PROGRAMMER         NOTES
    ----        -------    ----------         -----
    01/09/07    1.0        Daniel Stein       Initial implementation for
                                              dual-polarization project
                                              (ORPG Build 11).
******************************************************************************/

/* float compute_Avg_Zdr ( float Zdr_array[][MAX_BINS],
 *                        short hydrClass[][MAX_BINS],
 *                        int az, int rng )
 *{
 *   int   low_az, hi_az,           -* Azimuth boundary values         *-
 *         low_rng, hi_rng;         -* Range boundary values           *-
 *   int   zdrcount = 0;            -* Number of items for average     *-
 *   float zdrsum = 0.0;            -* Sum value for computing average *-
 *  
 *   -* int win_size = qperate_adapt.Avg_Zdr_WinSize; removed from structure;*-
 *
 *   int win_size = 1;
 *
 *   int   width;
 *
 *   if ((win_size % 2) == 1) -* If this is an odd number, it's valid *-
 *   {
 *      width = ( win_size - 1 ) / 2; -* The dist checked on either side *-
 *   }
 *   else  -* win_size is an even, non-valid number *-
 *   {
 *      width = win_size / 2;  -* The dist checked on either side *-
 *   }
 *
 *   #ifdef QPERATE_DEBUG
 *   {
 *      fprintf ( stderr, "Inside Compute_Avg_Zdr, " );
 *      fprintf ( stderr, "winsize = %d, az = %d, rng = %d, width = %d.\n",
 *                         win_size, az, rng, width );
 *   }
 *
 *   low_az = max ( az - width, 0 );
 *   hi_az = min ( az + width, MAX_AZM - 1 );
 *   low_rng = max (rng - width, 0);
 *   hi_rng = min ( rng + width, MAX_BINS - 1 );
 *
 *   #ifdef QPERATE_DEBUG
 *   {
 *      fprintf (stderr, 
 *               "low_az = %d, hi_az = %d, low_rng = %d, hi_rng = %d\n",
 *                         low_az,  hi_az, low_rng, hi_rng );
 *   }
 *
 *   get_Zdr_Accumulations ( low_az, hi_az, low_rng, hi_rng,
 *                           &zdrsum, &zdrcount, Zdr_array, hydrClass );
 *
 *   -* Now we handle the special cases where az values are near the edge.
 *    * Reset the min & max values and iterate again.
 *    *-
 *
 *   -* If bin is near the 0 radial, wrap around to the other side *-
 *   if ( ( az - width ) < 0 )
 *   {
 *      low_az = MAX_AZM - ( width - az );
 *      hi_az = MAX_AZM - 1;
 *      get_Zdr_Accumulations ( low_az, hi_az, low_rng, hi_rng,
 *                              &zdrsum, &zdrcount, Zdr_array, hydrClass );
 *   }
 *   -* Else if bin is near the last radial, wrap around to the 0 radial *-
 *   else if ( ( width + az ) > ( MAX_AZM - 1 ) )
 *   {
 *      hi_az = ( width + az ) - MAX_AZM ;
 *      low_az = 0;
 *      get_Zdr_Accumulations ( low_az, hi_az, low_rng, hi_rng,
 *                              &zdrsum, &zdrcount, Zdr_array, hydrClass );
 *   }
 *
 *   #ifdef QPERATE_DEBUG
 *      fprintf ( stderr, "About to leave compute_Avg_Zdr() .....\n" );
 *
 *   -* If there is at least 1 value used in the average computation, return it.
 *    * Otherwise, return QPE_NODATA to indicate that an average value could not
 *    * be computed.
 *    *-
 *   if ( zdrcount > 0 )
 *      return ( zdrsum / (float) zdrcount );
 *   else
 *      return ( QPE_NODATA );
 *
 *} /- end compute_Avg_Zdr() =================================== */
