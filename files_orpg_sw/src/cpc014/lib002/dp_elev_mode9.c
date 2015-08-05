/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/10/27 22:49:36 $
 * $Id: dp_elev_mode9.c,v 1.3 2009/10/27 22:49:36 ccalvert Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

#include "dp_lib_func_prototypes.h"

/******************************************************************************
   Filename: dp_elev_mode9.c

   Description:
   ============
   mode_filter() applies a mode N filter to an array of characters.

   Inputs: char* input     - array of characters
           int num_bins    - number of bins
           int filter_size - the filter size (default 9)

   Outputs: input buffer filtered with mode N (filter size)

   Change History
   ==============
   DATE          VERSION    PROGRAMMERS        NOTES
   ----------    -------    -----------------  ----------------------
   5 Oct 2007    0000       Ward               Initial implementation

   Mode N Filtering Description
   ============================
   The problem is that hydroclasses along a radial don't make smooth
   transitions, for example a radial might look like:

      radar ... GR RH RH GR HR GR RH HR GR
                            |
                          center

   To smooth, we want to replace the center bin with the mode of the 8
   surrounding bins, including the center. There are 4 GR, 3 RH and 2 HR,
   so the mode is GR, and we'll replace the center HR with GR:

      radar ... GR RH RH GR GR GR RH HR GR
                            |
                          center

   The inputs are a pointer to an array of bins to filter, the number
   of bins, and the filter_size. We are filtering generic moment hydrometeors,
   which are stored as chars (currently there are 12 hydrometeors). For mode 9
   filtering, set the filter_size to 9, which uses 4 bins on either side of
   the center bin to filter. We will filter on the input in place.

   If the number of bins < the filter size, there is nothing to do. If the
   filter size is even, increase it by one to make it a symmetric filter.

   We perform a one-pass sliding filter and loop over all the bins, indexed
   by the center bin. Some bins at the beginning/end cannot be filtered
   because of the filter size - these bins will not be filtered and will
   remain their input values.

   To compute a mode for a bin in the center of the slider:

   Find the mode (most frequent value) among the 9 bins. Several hydrometeor
   classes may be tied for the mode, so to resolve ties, pick the tied mode
   that is closest to the center bin (this could be the center bin with
   a distance of 0). If two tied modes are equally close to the center,
   pick the tied mode on the radar side of the radial. We assume that the
   bins represent a radial with the lower numbered bin closer to the radar.

   For two bins L and R at the same distance from the center bin this will
   favor L over R.  We can resolve ties in one pass by examining the bins in
   tie resolution order (worst to best), building a histogram as we go
   along, and always keeping track of the best mode as it appears. Let HF
   be half the filter size, rounded down (default has HF = 4).

   The bins will be examined in this worst to best order:

      center+HF, center-HF, center+(HF-1), center-(HF-1), ...  center.

   For example, in a mode 9 filter this order is:

      center+4, center-4, center+3, center-3, center+2, center-2, center+1,
      center-1, center.

   After examining all 9 bins, we can't place the mode back into the input
   buffer right away, because that would alter the mode calculation for
   the next slider, the center+1 bin. This is also true for the modes of
   the previous bins:

      center-1, center-2, ...  center(HF - 1).

   So, we need an array of temporary storage:

      storage[0] will hold the mode of bins 0, HF, 2*HF, 3*HF ...
      ...
      storage[HF - 1] will hold the mode of bins HF-1, 2*HF-1, 3*HF-1, ...

   For example, for a mode 9 filter, the storage array would hold in turn:

      storage[0] will hold the mode 9 of bins 0, 4, 8, 12 ...
      storage[1] will hold the mode 9 of bins 1, 5, 9, 13 ...
      storage[2] will hold the mode 9 of bins 2, 6, 10, 14 ...
      storage[3] will hold the mode 9 of bins 3, 7, 11, 15 ...

   When a new bin has its mode calculated, write what's sitting in its storage
   place back to input, and then put the new mode into storage. This requires
   a little bookkeeping. Loop over all the possible center bins. When you run
   out of center bins, flush any modes left in storage to input.

   Note1: The mode filter does not consider edge effects from the radial
   geometry. For example, the 0th bin of the 0th radial could have considered
   its leftmost bin to be the 0th bin of the 180th radial. We do not do this.

   Note2: The mode filter works on the input array. If the input array is
   a pointer to an RPG input buffer, it will fail because the RPG doesn't let
   you write to an input buffer (some lock). Copy the input buffer to
   your output, and then apply the mode filter.
*******************************************************************************/

/* #define PRINT_FILTER_STATS 1 */

void mode_filter(char* input, int num_bins, int filter_size)
{
   int  i, center, half_filter;
   int  histogram[NUM_MODE_CLASSES];
   int  new_value, best_mode, best_mode_count, storage_index;
   char txt_msg[100];
   #ifdef PRINT_FILTER_STATS
      int num_changed = 0;
   #endif

   if (DP_LIB002_DEBUG) /* print input before filter */
   {
      sprintf(txt_msg, "Before mode %d filter: ", filter_size);
      print_mode_array( input, num_bins, txt_msg );
   }

   /* Sanity checks */

   if ( input == NULL )
   {
     if (DP_LIB002_DEBUG)
     {
        fprintf(stderr, "input == NULL, nothing to do\n");
     }
     #ifdef PRINT_FILTER_STATS
        print_filter_stats(num_changed, num_bins);
     #endif 
     return; /* input is unchanged */
   }
   else if ( filter_size <= 1 ) /* filter too small, do nothing */
   {
     if (DP_LIB002_DEBUG)
     {
        fprintf(stderr, "filter_size %d <= 1, nothing to do\n",
                         filter_size);
     }
     #ifdef PRINT_FILTER_STATS
        print_filter_stats(num_changed, num_bins);
     #endif 
     return; /* input is unchanged */
   }
   else if ( num_bins < filter_size ) /* filter too big, do nothing */
   {
     if (DP_LIB002_DEBUG)
     {
        fprintf(stderr, "num_bins %d < filter_size %d, nothing to do\n",
                         num_bins, filter_size);
     }
     #ifdef PRINT_FILTER_STATS
        print_filter_stats(num_changed, num_bins);
     #endif 
     return; /* input is unchanged */
   }
   else if ((filter_size % 2) == 0 ) /* filter is even, not symmetric */
   {
     if (DP_LIB002_DEBUG )
     {
        fprintf(stderr, "filter_size %d is even, rounding up.\n",
                         filter_size);
     }
     filter_size++;
   }

   /* half_filter is half the filter size, rounded down.
    * For filter_size = 9 (mode 9), half_filter is 4. */

   half_filter = (filter_size - 1) / 2;

   /* Init temporary storage. We do this here because now we know
    * the value of half_filter. The storage keeps track of the filtered
    * modes of the 4 trailing bins to the left of the center bin.
    * The storage does not keep track of the entire filtered array. */

   char storage[half_filter];

   for(i = 0; i < half_filter; i++)
      storage[i] = input[i];

   /* Run algorithm. Start at the first filterable bin (half_filter),
    * and end at the last filterable bin (num_bins - half_filter - 1).
    *
    * Any bin before the first filterable, or after the last filterable,
    * will be untouched. */

   for(center = half_filter; center < (num_bins - half_filter); center++ )
   {
      /* Begin loop over all filterable bins. Init histogram. */

      memset(histogram, 0, NUM_MODE_CLASSES * sizeof(int));

      /* The best mode is the best mode we've seen so far, in case two
       * or more hydroclasses tie with the same mode count. You could
       * have something like:
       *
       *   radar ... GR RH RH GR HR RH GR RH GR
       *                         |
       *                       center
       *
       * with 4 GR, 4 RH and 1 HR, so the best mode would be GR, because
       * that's the mode closest to the radar. The filtered center would
       * look like:
       *
       *   radar ... GR RH RH GR GR RH GR RH GR
       *                         |
       *                       center
       *
       * By the order we look at the hydroclass bins, if there is a tie,
       * the best mode will favor the left (radar) side of the radial. */

      best_mode       = 0; /* best mode we've seen so far          */
      best_mode_count = 0; /* number of times we've seen best mode */

      /* Compute mode. Check the worst (farthest from center) to the best
       * (closest from center) bins, in tie resolution order, hopping from
       * right to left into the center. If two bins are equal distance from
       * the center, favor the left one, which lies on the radial closest
       * to the radar.
       *
       * Use >= in mode comparisons to take the latest mode in case of a tie
       * between right/left hand side modes. */

      for ( i = half_filter; i > 0; i-- )
      {
         /* Take 4 hops between right and left sides.
          * Check right hand side bin first, away from the radar */

         new_value = input[center + i];

         if ((new_value >= 0) && (new_value < NUM_MODE_CLASSES))
         {
           histogram[new_value]++;

           if ( histogram[new_value] >= best_mode_count )
           {
              best_mode       = new_value;
              best_mode_count = histogram[new_value];
           }
         }
         else if (DP_LIB002_DEBUG) /* new_value not a hydrometeor */
         {
           fprintf(stderr, "new_value: %d = input[%d] is out of bounds, "
		           "ignoring.\n",  new_value, center + i);
         }

         /* Check left hand side bin second, on the radar side. By
          * checking it second, the left bin takes priority over the
          * equidistant right bin in the best mode count. */

         new_value = input[center - i];

         if ((new_value >= 0) && (new_value < NUM_MODE_CLASSES))
         {
            histogram[new_value]++;

            if ( histogram[new_value] >= best_mode_count )
            {
               best_mode       = new_value;
               best_mode_count = histogram[new_value];
            }
         }
         else if(DP_LIB002_DEBUG) /* new_value not a hydrometeor */
         {
            fprintf(stderr, "new_value: %d = input[%d] is out of bounds, "
		            "ignoring.\n", new_value, center + i);
         }
      } /* end 4 hops between right and left */

      /* Check center bin last, at distance 0 from center. By checking it last
       * it has priority over all left/right bins in the best mode count. */

      new_value = input[center];

      if ((new_value >= 0) && (new_value < NUM_MODE_CLASSES))
      {
         histogram[new_value]++;

         if ( histogram[new_value] >= best_mode_count )
         {
            best_mode       = new_value;
            best_mode_count = histogram[new_value];
         }
      }
      else if (DP_LIB002_DEBUG) /* new_value not a hydrometeor */
      {
         fprintf(stderr, "new_value: %d = input[%d] is out of bounds, "
	                 "ignoring.\n", new_value, center + i);
      }

      if (DP_LIB002_DEBUG) /* print out the current state of the filter */
      {
         fprintf(stderr, "----------------------------------------------\n");
         fprintf(stderr, "[");
         for ( i = 0; i < num_bins; i++ )
         {
            if ( i > 0 )
               fprintf(stderr, " ");
            fprintf(stderr, "%2d", input[i]);
         }
         fprintf(stderr, "]\n");

         /* Print slider */

         for ( i = 0; i < center; i++ )
         {
            fprintf(stderr, "   ");
         }
         fprintf(stderr, "  |\n");
	
         for ( i = 0; i < (center - half_filter); i++ )
            fprintf(stderr, "   ");
	
         for ( i = center-half_filter; i <= center+half_filter; i++ )
            fprintf(stderr, "%3d", input[i]);
	
         fprintf(stderr, "\n\ncenter: %d best_mode = %d best_mode_count = "
	                 "%d\n", center, best_mode, best_mode_count);
      }

      /* Save the new mode, after sending what's sitting in its storage bin
       * back to input, so the storage isn't lost. Here's an example.
       * Let I1 = input value 1, M1 = filtered mode of input value 1, etc.
       *
       * Center  Storage           Filtered input
       *  Bin     0   1   2   3    0  1  2  3  4  5  6  7  8 ...
       * ------  ---------------  ------------------------------
       *  init   I0  I1  I2  I3
       *    4    M4  I1  I2  I3   I0
       *    5    M4  M5  I2  I3   I0 I1
       *    6    M4  M5  M6  I3   I0 I1 I2
       *    7    M4  M5  M6  M7   I0 I1 I2 I3
       *    8    M8  M5  M6  M7   I0 I1 I2 I3 M4
       *    9    M8  M9  M6  M7   I0 I1 I2 I3 M4 M5
       *   10    M8  M9  M10 M7   I0 I1 I2 I3 M4 M5 M6
       *   11    M8  M9  M10 M11  I0 I1 I2 I3 M4 M5 M6 M7
       *   12    M12 M9  M10 M11  I0 I1 I2 I3 M4 M5 M6 M7 M8
       *  ... */

      storage_index = center % half_filter;

      #ifdef PRINT_FILTER_STATS
         if(input[center - half_filter] != storage[storage_index])
            num_changed++;
      #endif

      input[center - half_filter] = storage[storage_index];
      storage[storage_index]      = best_mode;

   } /* end loop over all filterable bins */

   /* We're done filtering. Put anything left in storage back to input.
    * There should be exactly half_filter (4) modes sitting in storage. */

   for (i = 1; i <= half_filter; i++)
   {
      storage_index = (center - i) % half_filter;

      #ifdef PRINT_FILTER_STATS
         if(input[center - i] != storage[storage_index])
            num_changed++;
      #endif

      input[center - i] = storage[storage_index];
   }

   if (DP_LIB002_DEBUG) /* print input after filter */
   {
      sprintf(txt_msg, "After mode %d filter: ", filter_size);
      print_mode_array(input, num_bins, txt_msg);
   }

   #ifdef PRINT_FILTER_STATS
      print_filter_stats(num_changed, num_bins);
   #endif 

} /* mode_filter() ========================================================= */

void print_filter_stats(int num_changed, int num_bins)
{
   if(num_bins > 0)
   {
      fprintf(stderr, "%5.2f %% = %d / %d bins were changed\n",
              (100.0 * num_changed) / num_bins, num_changed, num_bins);
   }
   else /* can't divide by 0 */
   {
      fprintf(stderr, "num_bins = 0\n");
   }

} /* print_filter_stats() ================================================ */

