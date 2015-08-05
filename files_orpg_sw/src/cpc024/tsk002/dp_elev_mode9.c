/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/03/03 18:48:36 $
 * $Id: dp_elev_mode9.c,v 1.2 2009/03/03 18:48:36 ccalvert Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

#include "dp_elev_func_prototypes.h"

/******************************************************************************
   Filename: dp_elev_mode9.c
   
   Description:
   ============
   mode_filter() applies a mode N filter to an array of characters.

   Inputs: char* input     - array of characters
           int num_bins    - number of bins
           int filter_size - the filter size (default 9)
   
   Outputs: input buffer filtered with mode N
   
   Change History
   ==============
   DATE          VERSION    PROGRAMMERS        NOTES
   ----------    -------    -----------------  ----------------------
   5 Oct 2007    0000       Ward               Initial implementation 

   Mode N Filtering Description
   ============================
      The input is a pointer to an (char) array of bins to filter, the number 
      of bins, and the filter_size. For mode 9 filtering, set the filter_size 
      is 9. We will work on the input in place. If the number of bins < the 
      filter size, there is nothing to do. If the filter size is even, increase
      it by one to make it a symmetric filter. We will perform a sliding filter
      and loop over all the bins, indexed by the center bin.  Some bins at the 
      beginning and end cannot be filtered because of the filter size, these
      bins will not be altered. 

      To compute a mode for a bin in the center of the slider:

      Several hydrometeor classes may be tied for the mode. To resolve ties, 
      pick the tied mode that is closest to the center bin, and if two tied 
      modes are equally close, pick the tied mode on the radar side of the 
      radial. We assume that the bins represent a radial with the lower numbered
      bin closer to the radar. 
      For two bins L and R at the same distance from the center bin this will 
      favor L over R.  We can resolve ties in one pass by examining the bins in
      tie resolution order (worst to best), building the histogram as we go 
      along, and always keeping the best mode as it appears. Let HF be half the
      filter size, rounded down. 
      
      The bins will be examined in this worst to best order:

      center+HF, center-HF, center+(HF-1), center-(HF-1), ...  center. 

      For example, in a mode 9 filter this order would be:

      center+4, center-4, center+3, center-3, center+2, center-2, center+1, 
      center-1, center.

      When we have a mode for the center bin, we can't place it back into input
      right away, because that would alter the mode calculation for the next 
      center+1 bin. This is also true for the modes of the previous bins:
      center-1, center-2, ...  center(HF - 1). 
      
      So we need an array of temporary storage:

      storage[0] will hold the mode of bins 0, HF, 2*HF, 3*HF ... 

      storage[HF - 1] will hold the mode of bins HF-1, 2*HF-1, 3*HF-1, ... 

      For example, for a mode 9 filter, the storage array would hold in turn:

      storage[0] will hold the mode 9 of bins 0, 4, 8, 12 ... 
      storage[1] will hold the mode 9 of bins 1, 5, 9, 13 ...
      storage[2] will hold the mode 9 of bins 2, 6, 10, 14 ...
      storage[3] will hold the mode 9 of bins 3, 7, 11, 15 ... 

      When a new bin has a mode calculated, write what's sitting in its storage
      place back to input, and then put the new mode into storage. This requires
      some bookkeeping. Loop over all the possible center bins. When you run out
      of center bins, flush any modes left in storage to input.

      Note: The mode filter doesn't consider edge effects from the geometry 
      of the radial. For example, the 0th bin of the 0th radial could consider 
      its leftmost bin to be the 0th bin of the 180th radial.
*******************************************************************************/

int print_mode_filter = 0;

void mode_filter( char* input, int num_bins, int filter_size )
{
   int   i, center, half_filter;
   int   histogram[NUM_MODE_CLASSES];
   int   best_mode, new_mode, biggest_mode_count, storage_bin;
   char  txt_msg[100];  
   int   verbose = 1; /* can be 0, 1, or 2 */

   verbose = print_mode_filter;

   /* Print out before array */

   if ( verbose && DP_ELEV_PROD_DEBUG )
   {
      sprintf(txt_msg, "Before mode %d filter: ", filter_size);
      print_mode_array( input, num_bins, txt_msg );
   }

   /* Sanity checks */

   if ( filter_size <= 1 )
   {
     if ( DP_ELEV_PROD_DEBUG )
        fprintf(stderr, "filter_size %d <= 1, nothing to do\n", filter_size);
     return;
   }
   else if ( num_bins < filter_size )
   {
     if ( DP_ELEV_PROD_DEBUG )
        fprintf(stderr, "num_bins %d < filter_size %d, nothing to do\n", 
                         num_bins, filter_size);
     return;
   }
   else if ( num_bins > MAX_MODE_BINS ) /* the input is really bad */
   {
     if ( DP_ELEV_PROD_DEBUG )
        fprintf(stderr, "num_bins %d  > %d, not doing mode\n",
                         num_bins, MAX_MODE_BINS);
     return;
   }
   else if ( filter_size % 2 == 0 ) /* filter size is even */
   {
     if ( DP_ELEV_PROD_DEBUG )
        fprintf(stderr, "filter_size %d is even, rounding up.\n", filter_size);
     filter_size++;
   }

   /* half_filter is half the filter size, rounded down.
    * For filter_size = 9 (mode 9), half_filter is 4.
    */
   half_filter = (filter_size - 1) / 2;

   /* Init storage */
   char storage[half_filter];

   for ( i = 0; i < half_filter; i++ )
      storage[i] = input[i];

   /* Run algorithm. We start at the first filterable bin (half_filter), 
    * and end at the last filterable bin (num_bins - half_filter). Any
    * bin after the last filterable bin will not be touched.
    */
   for ( center = half_filter; center < (num_bins - half_filter); center++ )
   {
      /* Init histogram */
      memset( histogram, 0, NUM_MODE_CLASSES * sizeof(int) );
      best_mode = 0;
      biggest_mode_count = 0;

      /* Compute mode. We check the worst (farthest from center) 
       * to the best (closest from center) bins, hopping into 
       * the center. If two bins are equal distance from the
       * center, favor the left one, which is on the radial
       * closest to the radar.
       *
       * Use >= in comparisons to take the latest mode in 
       * case of a tie between modes.
       */
      for ( i = half_filter; i > 0; i-- )
      {
         /* Right hand side bin, away from radar */
         new_mode = input[center + i];
         if ( (new_mode >= 0) && (new_mode < NUM_MODE_CLASSES) )
         {
           histogram[new_mode]++;
           if ( histogram[new_mode] >= biggest_mode_count )
           {
              best_mode = new_mode;
              biggest_mode_count = histogram[new_mode];
           }
         }
         else if ( (verbose > 1) && DP_ELEV_PROD_DEBUG )
         {
           fprintf(stderr, "new_mode: %d = input[%d] is out of bounds, "
		           "ignoring.\n",  new_mode, center + i);
         }

         /* Left hand side bin, on radar side */
         new_mode = input[center - i];
         if ( (new_mode >= 0) && (new_mode < NUM_MODE_CLASSES) )
         {
            histogram[new_mode]++;
            if ( histogram[new_mode] >= biggest_mode_count )
            {
               best_mode = new_mode;
               biggest_mode_count = histogram[new_mode];
            }
         }
         else if ( (verbose > 1) && DP_ELEV_PROD_DEBUG )
         {
            fprintf(stderr, "new_mode: %d = input[%d] is out of bounds, "
		            "ignoring.\n", new_mode, center + i);
         }
      } /* end i loop */

      /* Center bin is best, with distance 0 from center */
      new_mode = input[center];
      if ( (new_mode >= 0) && (new_mode < NUM_MODE_CLASSES) )
      {
         histogram[new_mode]++;
         if ( histogram[new_mode] >= biggest_mode_count )
         {
            best_mode = new_mode;
            biggest_mode_count = histogram[new_mode];
         }
      }
      else if ( (verbose > 1) && DP_ELEV_PROD_DEBUG )
      {
         fprintf(stderr, "new_mode: %d = input[%d] is out of bounds, "
	                 "ignoring.\n", new_mode, center + i);
      }

      if ( (verbose > 1) && DP_ELEV_PROD_DEBUG )
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

      }

      /* Store the new mode, but first flush what's sitting in
       * its storage place back to input.
       */
      storage_bin                 = center % half_filter;
      input[center - half_filter] = storage[storage_bin];
      storage[storage_bin]        = best_mode;
   }

   /* Flush anything left in storage back to input. */
   for ( i = 1; i <= half_filter; i++ )
   {
      storage_bin       = (center - i) % half_filter;
      input[center - i] = storage[storage_bin];
   }    

   /* Print after array */
   if ( verbose && DP_ELEV_PROD_DEBUG )
   {
      sprintf(txt_msg, "After mode %d filter: ", filter_size);
      print_mode_array(input, num_bins, txt_msg);
   }
} /* mode_filter() ========================================================= */

/******************************************************************************
   Filename: dp_elev_mode9.c
   
   Description:
   ============
   test_filter() test a mode filter.

   Inputs: none
   
   Outputs: none
   
   Change History
   ==============
   DATE          VERSION    PROGRAMMERS        NOTES
   ----------    -------    -----------------  ----------------------
   5 Oct 2007    0000       Jame Ward          Initial implementation 
*****************************************************************************/

void test_mode_filter( void )
{
   float val[MAX_MODE_BINS];
   char  val_c[MAX_MODE_BINS];
   int   i, no_of_gates = 20;

   val[0]  = 5.0;
   val[1]  = 3.0;
   val[2]  = 3.0;
   val[3]  = 3.0;
   val[4]  = 1.0;
   val[5]  = 2.0;
   val[6]  = 1.0;
   val[7]  = 2.0;
   val[8]  = 3.0;
   val[9]  = 2.0;
   val[10] = 1.0;
   val[11] = 2.0;
   val[12] = 1.0;
   val[13] = 2.0;
   val[14] = 1.0;
   val[15] = 2.0;
   val[16] = 4.0;
   val[17] = 4.0;
   val[18] = 4.0;
   val[19] = 7.0;

   for ( i = 0; i < no_of_gates; i++ )
      val_c[i] = (char) RPGC_NINT( val[i] );

   mode_filter( val_c, 20, 9 );

   for ( i = 0; i < no_of_gates; i++ )
      val[i] = (float) val_c[i];

   for ( i = 0; i < no_of_gates; i++ )
   {
      if ( DP_ELEV_PROD_DEBUG )
         fprintf(stderr, "val[%d]: %f\n", i, val[i]);
   }
} /* test_mode_filter() ==================================================== */

/******************************************************************************
   Filename: dp_elev_mode9.c
   
   Description:
   ============
   print_mode_array() prints out a mode array in a  tabular format for quick 
   inspection. It assumes that the number of bins < 1000.

   Inputs: char* input    - the input arry
           int   num_bins - the number of bins
           char* text     - some extra text you can print
   
   Outputs: none
   
   Change History
   ==============
   DATE          VERSION    PROGRAMMERS        NOTES
   ----------    -------    -----------------  ----------------------
   5 Oct 2007    0000       Jame Ward          Initial implementation 
*****************************************************************************/

void print_mode_array( char* input, int num_bins, char* text )
{
   int  i; 
   int  different;
   int  num_too_big, num_too_small;
   int  num_singletons;

   if ( DP_ELEV_PROD_DEBUG == FALSE )
      return;

   fprintf(stderr, "------------------------------------------------------\n");
   fprintf(stderr, "%s", text);

   if ( num_bins > MAX_MODE_BINS )
   {
      fprintf(stderr, " num_bins %d > %d, can't print\n", 
                      num_bins, MAX_MODE_BINS);
      return;
   }

   /* Do a quick check to see if it's all the same value. */
   different = 0;
   for ( i = 1; i < num_bins; i++ )
   {
      if ( input[i] != input[0] ) 
      {
        different = 1;
        break;
      }
   }

   if ( different )
   {
      fprintf(stderr, "input[%d] %d != input[0] %d\n", 
                       i, input[i], input[0]);
   }
   else
   {
      fprintf(stderr, "All %d bins are: %d\n", num_bins, input[0]);
      return;
   }

   num_too_big    = 0;
   num_too_small  = 0;
   num_singletons = 0;

   for ( i = 0; i < num_bins; i++ )
   {
      if ( input[i] > NUM_MODE_CLASSES )
      {
        num_too_big++;
      }
      else if ( input[i] < 0 )
      {
        num_too_small++;
      }
      if ( (i > 0)                  && 
           (i < (num_bins-1))       &&  
           (input[i-1] != input[i]) &&
           (input[i+1] != input[i]) )
      {
         num_singletons++;
         fprintf(stderr, "%2d) %3d [%2d]   %3d [%2d]   %3d [%2d]\n",
                          num_singletons, i-1, input[i-1], i, input[i], 
			  i+1, input[i+1]);
      }
   }

   fprintf(stderr, "num_singletons: ");

   if ( num_singletons )
      fprintf(stderr, "%d.\n", num_singletons);
   else
   {
      fprintf(stderr, "no singletons.\n");
      return;
   }

   fprintf(stderr, "Too big: %d, too small: %d, just right: %d\n", 
                   num_too_big, num_too_small, 
		   num_bins - (num_too_big + num_too_small));

/**   print_table( input, num_bins );**/

} /* print_mode_array */
