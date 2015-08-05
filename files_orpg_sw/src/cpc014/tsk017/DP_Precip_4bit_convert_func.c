/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/10/27 22:54:27 $
 * $Id: DP_Precip_4bit_convert_func.c,v 1.4 2009/10/27 22:54:27 ccalvert Exp $
 * $Revision: 1.4 $
 * $State: Exp $
 */

/******************************************************************************
   Filename: DP_Precip_4bit_convert_func.c

   Description
   ===========
      convert_Resolution() converts accumulation grids in high res (250 m) to
   low res (2km) by averaging 8 bin input to produce low-res.

   Note: This averaging will cause the Max Accum (halfword 47, parameter 4)
         for the 4 bit products, 169/171, to be lower than that of the 8 bit
         products, 170/172. For example, on tropical storm ERIN, volume 2,
         the 4 bit max accum is .278 inches, while the 8 bit is .638 inches.

   Inputs:
      int hres_grid - accum grid in high resolution (250 m by 1 deg).

   Output:
      int lres_grid - accum grid in low resolution (2 km by 1 deg).

   Returns:
     N/A

   Change History
   ==============
   DATE        VERSION    PROGRAMMER         NOTES
   ----        -------    ----------         -----
   10/07       0000       Cham Pham          Initial Implementation
******************************************************************************/

#include "DP_Precip_4bit_func_prototypes.h"

#define NODATA_4BIT_THRESHOLD 1 /* 1 good bin and we can compute an average */

void convert_Resolution(int hres_grid[][MAX_BINS],
                        int lres_grid[][MAX_2KM_RESOLUTION])
{
   int   rad, bin, avgbin;
   float sum_8bin = 0.0;
   int   num_gooddata;

   static unsigned int lres_size = sizeof(lres_grid);

   if(DP_PRECIP_4BIT_DEBUG)
      fprintf(stderr,"\nBeginning convert_Resolution()\n");

   /* Initialize lres_grid to zero */

   memset(lres_grid, 0, lres_size);

   for (rad = 0; rad < MAX_AZM; ++rad)
   {
     avgbin       = 0;
     sum_8bin     = 0.0;
     num_gooddata = 0;

     for(bin = avgbin; bin < MAX_BINS ; ++bin)
     {
        if(hres_grid[rad][bin] < 0) /* a bad value */
        {
           if(DP_PRECIP_4BIT_DEBUG)
           {
              fprintf(stderr, "hres_grid[%d][%d] = %d\n", rad, bin,
                      hres_grid[rad][bin]);
           }
        }
        else if(hres_grid[rad][bin] != QPE_NODATA)
        {
           /* Only add to the sum if the data is valid */

           sum_8bin += (float) hres_grid[rad][bin];
           num_gooddata++;
        }

        if((bin % 8) == 7) /* last bin in the set of 8 = (2 Km / 250 m) */
        {
           /* When inspecting the 8 bins, if:
            *
            * num_gooddata >= NODATA_4BIT_THRESHOLD,
            *
            * then the output bin is set to the average.
            * NODATA_VALUE_4BIT is the flag value for final product
            * 4 bit no data. */

           if(num_gooddata >= NODATA_4BIT_THRESHOLD)
           {
              lres_grid[rad][bin/8] = (int) RPGC_NINT(sum_8bin / num_gooddata);

              /* Don't scale away small values */

              if((lres_grid[rad][bin/8] == 0) && (sum_8bin > 0.0))
                 lres_grid[rad][bin/8] = 1;
           }
           else
              lres_grid[rad][bin/8] = NODATA_VALUE_4BIT;

           avgbin      += 8;
           sum_8bin     = 0.0;
           num_gooddata = 0;
           continue;
        }
     } /* end bin loop */
   } /* end rad loop */

   if(DP_PRECIP_4BIT_DEBUG)
   {
      for (rad = 120; rad < 121; rad++ )
      {
         fprintf(stderr,"RAD(%d)\n",rad);

         for (bin = 0; bin < MAX_2KM_RESOLUTION; bin++ )
            fprintf( stderr,"%d ", lres_grid[rad][bin] );

         fprintf( stderr,"\n" );
      }

      fprintf(stderr,"\nEnd convert_Resolution()\n\n");
   }
} /* end convert_Resolution() ============================== */
