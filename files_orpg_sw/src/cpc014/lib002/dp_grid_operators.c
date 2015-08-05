/*
 */

#include <limits.h>                 /* INT_MAX        */
#include "dp_lib_func_prototypes.h" /* print_min_max  */
#include "dp_lt_accum_Consts.h"     /* INTERP_FORWARD */
#include "qperate_Consts.h"         /* QPE_NODATA     */

/* Notes:
 *
 * 1. It was a design decision to break out the grid operators into multiple
 *    functions that add/subtract different combinations of biased/unbiased
 *    int/short grids instead of having a single function with many switches.
 *    The default grid type is int, an accumulation grid.
 *
 * 2. After doing a grid operation, it is possible for the result to land
 *    on QPE_NODATA. To distinguish a good result from QPE_NODATA, the result
 *    can be bumped up or down by 1. This is not a big change since accum
 *    grids are in 1000ths of an inch. Since QPE_NODATA is SHRT_MIN, we always
 *    add 1 to avoid an overflow if the grid is a short.
 *
 * 3. Accumulation grids are ints, not unsigned ints, because difference grids
 *    can be negative. The final check of a good value against QPE_NODATA
 *    should be done when either adding or subtracting grid2[], because you
 *    don't know the sign of the grid2[] value.
 *
 * 4. For adding a bias corrected value to a grid:
 *
 *       grid1[i][j] += (int) RPGC_NINT (temp_val);
 *
 *    is used instead of:
 *
 *       grid1[i][j] += grid2[i][j] * bias;
 *
 *    becase bias is a float and the RPGC_NINT() will still have to be done
 *    to round instead of doing the default truncation.
 */

/******************************************************************************
    Filename: dp_grid_operators.c

    Description:
    ============
      add_biased_grids() will add two accum grids,
      grid1 + (grid2 * bias), with the result stored in grid1.

    Inputs:
       grid1    - bias corrected accumulation grid. The result of the
                  addition will be stored here. This is an array of ints.
       grid2    - not bias corrected accumulation grid. This is an array
                  of ints.
       bias     - multiplicative mean-field bias value.
       max_grid - the maximum value allowed for grid1.

    Outputs:
       Sum of grid1 and grid2 stored in grid1. grid2 has bias applied before
    being added in.

    Returns: FUNCTION_SUCCEEDED (0), NEGATIVE_BIAS (3)

    Currently uncalled.

    Change History
    ==============
    DATE         VERSION    PROGRAMMERS        NOTES
    ---------    -------    ---------------    ------------------
    09 Oct 07    0000       Daniel Stein       Initial implementation
    15 Oct 07    0001       Daniel Stein       Added a biased version to add
                                               and subtract functions
    22 Jan 09    0002       James Ward         Added checks for overflow
******************************************************************************/

int add_biased_grids ( int grid1[][MAX_BINS], int grid2[][MAX_BINS],
                       float bias, int max_grid )
{
   int   i, j;     /* indices */
   float temp_val; /* to guard against overflow */
   char  msg[200]; /* stderr message */

   sprintf(msg, "WARNING! ERROR! Biased functions should NOT be called!!!\n");
   RPGC_log_msg(GL_INFO, msg);
   if(DP_LIB002_DEBUG)
      fprintf(stderr, msg);

   sprintf(msg, "Dual-pol bias not functional.\n");
   RPGC_log_msg(GL_INFO, msg);
   if(DP_LIB002_DEBUG)
      fprintf(stderr, msg);

   sprintf(msg, "Invalid call to add_biased_grids.\n");
   RPGC_log_msg(GL_INFO, msg);
   if(DP_LIB002_DEBUG)
      fprintf(stderr, msg);

   /* Check for negative bias */

   if(bias_is_negative(bias, "add_biased_grids"))
      return(NEGATIVE_BIAS);

   for (i = 0; i < MAX_AZM; ++i)
   {
      /* Note: QPE_NODATA handling logic applies only to accum grids,
       * not to rate grids. For rate grids, if either is QPE_NODATA,
       * they cannot be added. */

      for (j = 0; j < MAX_BINS; ++j)
      {
         if(grid2[i][j] == QPE_NODATA)
         {
            /* No change to grid1[i][j] */
            continue;
         }
         else if(grid1[i][j] == QPE_NODATA)
         {
            /* Change grid1[i][j] to 0 so can add */
            grid1[i][j] = 0;
         }

         temp_val = (float) grid2[i][j] * bias;

         if ( (grid1[i][j] + temp_val) > max_grid )
         {
            grid1[i][j] = max_grid;

            if ( DP_LIB002_DEBUG )
            {
               fprintf(stderr, "%s temp_val: %f, grid1[%d][%d] capped at "
          	              "max_grid (%d)\n",
                              "add_biased_grids:", temp_val, i, j, max_grid);
            }
         }
         else if ( (grid1[i][j] + temp_val) > INT_MAX )
         {
            grid1[i][j] = INT_MAX;

            if ( DP_LIB002_DEBUG )
            {
               fprintf(stderr, "%s temp_val: %f, grid1[%d][%d] capped at "
	   	               "INT_MAX (%d)\n",
                               "add_biased_grids:", temp_val, i, j, INT_MAX);
            }
         }
         else if ( (grid1[i][j] + temp_val) < INT_MIN )
         {
            grid1[i][j] = INT_MIN;

            if ( DP_LIB002_DEBUG )
            {
               fprintf(stderr, "%s temp_val: %f, grid1[%d][%d] capped at "
	 	               "INT_MIN (%d)\n",
                               "add_biased_grids:", temp_val, i, j, INT_MIN);
            }
         }
         else /* a good value */
         {
            grid1[i][j] += (int) RPGC_NINT (temp_val);
            if(grid1[i][j] == QPE_NODATA) /* accidentally landed */
               grid1[i][j] += 1;          /* move up 1 */
         }

      } /* end loop over all bins */

   } /* end loop over all radials */

   return(FUNCTION_SUCCEEDED);

}/* end add_biased_grids() =========================================== */

/******************************************************************************
    Filename: dp_grid_operators.c

    Description:
    ============
      add_biased_short_to_int() will add two accum grids,
      grid1 + (grid2 * bias), with the result stored in grid1.

    Inputs:
       grid1    - bias corrected accumulation grid. The result of the addition
                  will be stored here. This is an array of ints.
       grid2    - not bias corrected accumulation grid. This is an array of
                  shorts.
       bias     - multiplicative mean-field bias value.
       max_grid - the maximum value allowed for grid1.

    Outputs:
       Sum of grid1 and grid2 stored in grid1. grid2 has bias applied before
    being added in.

    Returns: FUNCTION_SUCCEEDED (0), NEGATIVE_BIAS (3)

    Called by: compute_ST_grid(), compute_Hourly_grids(), CQ_Trim_To_Hour().

    Change History
    ==============
    DATE         VERSION    PROGRAMMERS        NOTES
    ---------    -------    ---------------    ------------------
    09 Oct 07    0000       Daniel Stein       Initial implementation
    15 Oct 07    0001       Daniel Stein       Added a biased version to add
                                               and subtract functions
    22 Jan 09    0002       James Ward         Added checks for overflow
******************************************************************************/

int add_biased_short_to_int ( int grid1[][MAX_BINS], short grid2[][MAX_BINS],
                              float bias, int max_grid )
{
   int   i, j;     /* indices */
   float temp_val; /* to guard against overflow */
   char  msg[200]; /* stderr message */

   sprintf(msg, "WARNING! ERROR! Biased functions should NOT be called!!!\n");
   RPGC_log_msg(GL_INFO, msg);
   if(DP_LIB002_DEBUG)
      fprintf(stderr, msg);

   sprintf(msg, "Dual-pol bias not functional.\n");
   RPGC_log_msg(GL_INFO, msg);
   if(DP_LIB002_DEBUG)
      fprintf(stderr, msg);

   sprintf(msg, "Invalid call to add_biased_short_to_int.\n");
   RPGC_log_msg(GL_INFO, msg);
   if(DP_LIB002_DEBUG)
      fprintf(stderr, msg);

   /* Check for negative bias */

   if(bias_is_negative(bias, "add_biased_short_to_int"))
      return(NEGATIVE_BIAS);

   for (i = 0; i < MAX_AZM; ++i)
   {
      for (j = 0; j < MAX_BINS; ++j)
      {
         /* Note: QPE_NODATA handling logic applies only to accum grids,
          * not to rate grids. For rate grids, if either is QPE_NODATA,
          * they cannot be added. */

         if(grid2[i][j] == QPE_NODATA)
         {
            /* No change to grid1[i][j] */
            continue;
         }
         else if(grid1[i][j] == QPE_NODATA)
         {
            /* Change grid1[i][j] to 0 so can add */
            grid1[i][j] = 0;
         }

         temp_val = (float) grid2[i][j] * bias;

         if ( (grid1[i][j] + temp_val) > max_grid )
         {
            grid1[i][j] = max_grid;

            if ( DP_LIB002_DEBUG )
            {
               fprintf(stderr, "%s temp_val: %f, grid1[%d][%d] capped at "
		               "max_grid (%d)\n",
                     "add_biased_short_to_int:", temp_val, i, j, max_grid);
            }
         }
         else if ( (grid1[i][j] + temp_val) > INT_MAX )
         {
            grid1[i][j] = INT_MAX;

            if ( DP_LIB002_DEBUG )
            {
               fprintf(stderr, "%s temp_val: %f, grid1[%d][%d] capped at "
	  	               "INT_MAX (%d)\n",
                     "add_biased_short_to_int:", temp_val, i, j, INT_MAX);
            }
         }
         else if ( (grid1[i][j] + temp_val) < INT_MIN )
         {
            grid1[i][j] = INT_MIN;

            if ( DP_LIB002_DEBUG )
            {
               fprintf(stderr, "%s temp_val: %f, grid1[%d][%d] capped at "
		               "INT_MIN (%d)\n",
                     "add_biased_short_to_int:", temp_val, i, j, INT_MIN);
            }
         }
         else /* a good value */
         {
            grid1[i][j] += (int) RPGC_NINT (temp_val);
            if(grid1[i][j] == QPE_NODATA) /* accidentally landed */
               grid1[i][j] += 1;          /* move up 1 */
         }

      } /* end loop over all bins */

   } /* end loop over all radials */

   return(FUNCTION_SUCCEEDED);

} /* end add_biased_short_to_int() ==================================== */

/******************************************************************************
    Filename: dp_grid_operators.c

    Description:
    ============
      add_unbiased_grids() will add two accum grids,
      grid1 + grid2, with the result stored in grid1.

    Inputs:
       grid1    - not bias corrected accumulation grid. The result of the
                  addition will be stored here. This is an array of ints.
       grid2    - not bias corrected accumulation grid. This is an array
                  of ints.
       max_grid - the maximum value allowed for grid1.

    Outputs:
       Sum of grid1 and grid2 stored in grid1.

    Currently uncalled.

    Change History
    ==============
    DATE         VERSION    PROGRAMMERS        NOTES
    ---------    -------    ---------------    ------------------
    09 Oct 07    0000       Daniel Stein       Initial implementation
    15 Oct 07    0001       Daniel Stein       Added a biased version to add
                                               and subtract functions
    22 Jan 09    0002       James Ward         Added checks for overflow
******************************************************************************/

void add_unbiased_grids ( int grid1[][MAX_BINS], int grid2[][MAX_BINS],
                          int max_grid )
{
   int   i, j;     /* indices */
   float temp_val; /* to guard against overflow */

   for (i = 0; i < MAX_AZM; ++i)
   {
      for (j = 0; j < MAX_BINS; ++j)
      {
         /* Note: QPE_NODATA handling logic applies only to accum grids,
          * not to rate grids. For rate grids, if either is QPE_NODATA,
          * they cannot be added. */

         if(grid2[i][j] == QPE_NODATA)
         {
            /* No change to grid1[i][j] */
            continue;
         }
         else if(grid1[i][j] == QPE_NODATA)
         {
            /* Change grid1[i][j] to 0 so can add */
            grid1[i][j] = 0;
         }

         temp_val = grid2[i][j];

         if ( (grid1[i][j] + temp_val) > max_grid )
         {
            grid1[i][j] = max_grid;

            if ( DP_LIB002_DEBUG )
            {
               fprintf(stderr, "%s temp_val: %f, grid1[%d][%d] capped at "
		               "max_grid (%d)\n",
                     "add_unbiased_grids:", temp_val, i, j, max_grid);
            }
         }
         else if ( (grid1[i][j] + temp_val) > INT_MAX )
         {
           grid1[i][j] = INT_MAX;

           if ( DP_LIB002_DEBUG )
           {
              fprintf(stderr, "%s temp_val: %f, grid1[%d][%d] capped at "
		              "INT_MAX (%d)\n",
                     "add_unbiased_grids:", temp_val, i, j, INT_MAX);
           }
         }
         else if ( (grid1[i][j] + temp_val) < INT_MIN )
         {
            grid1[i][j] = INT_MIN;

            if ( DP_LIB002_DEBUG )
            {
               fprintf(stderr, "%s temp_val: %f, grid1[%d][%d] capped at "
 		               "INT_MIN (%d)\n",
                     "add_unbiased_grids:", temp_val, i, j, INT_MIN);
            }
         }
         else /* a good value */
         {
            grid1[i][j] += grid2[i][j];
            if(grid1[i][j] == QPE_NODATA) /* accidentally landed */
               grid1[i][j] += 1;          /* move up 1 */
         }

      } /* end loop over all bins */

   } /* end loop over all radials */

} /* end add_unbiased_grids() ======================================== */

/******************************************************************************
    Filename: dp_grid_operators.c

    Description:
    ============
      add_unbiased_short_to_int() will add two accum grids,
      grid1 + grid2, with the result stored in grid1.

    Inputs:
       grid1    - not bias corrected accumulation grid. The result of the
                  addition will be stored here. This is an array of ints.
       grid2    - not bias corrected accumulation grid. This is an array
                  of shorts.
       max_grid - the maximum value allowed for grid1

    Outputs:
       Sum of grid1 and grid2 stored in grid1.

    Called by: compute_ST_grid(), compute_Hourly_grids(), CQ_Trim_To_Hour().

    Change History
    ==============
    DATE         VERSION    PROGRAMMERS        NOTES
    ---------    -------    ---------------    ------------------
    09 Oct 07    0000       Daniel Stein       Initial implementation
    15 Oct 07    0001       Daniel Stein       Added a biased version to add
                                               and subtract functions
    22 Jan 09    0002       James Ward         Added checks for overflow
******************************************************************************/

void add_unbiased_short_to_int ( int grid1[][MAX_BINS], short grid2[][MAX_BINS],
                                 int max_grid )
{
   int   i, j;     /* indices */
   float temp_val; /* to guard against overflow */

   for (i = 0; i < MAX_AZM; ++i)
   {
      for (j = 0; j < MAX_BINS; ++j)
      {
         /* Note: QPE_NODATA handling logic applies only to accum grids,
          * not to rate grids. For rate grids, if either is QPE_NODATA,
          * they cannot be added. */

         if(grid2[i][j] == QPE_NODATA)
         {
            /* No change to grid1[i][j] */
            continue;
         }
         else if(grid1[i][j] == QPE_NODATA)
         {
            /* Change grid1[i][j] to 0 so can add */
            grid1[i][j] = 0;
         }

         temp_val = grid2[i][j];

         if ( (grid1[i][j] + temp_val) > max_grid )
         {
            grid1[i][j] = max_grid;

            if ( DP_LIB002_DEBUG )
            {
               fprintf(stderr, "%s temp_val: %f, grid1[%d][%d] capped at "
	  	               "max_grid (%d)\n",
                       "add_unbiased_short_to_int:", temp_val, i, j, max_grid);
            }
         }
         else if ( (grid1[i][j] + temp_val) > INT_MAX )
         {
            grid1[i][j] = INT_MAX;

            if ( DP_LIB002_DEBUG )
            {
               fprintf(stderr, "%s temp_val: %f, grid1[%d][%d] capped at"
 		               " INT_MAX (%d)\n",
                      "add_unbiased_short_to_int:", temp_val, i, j, INT_MAX);
            }
         }
         else if ( (grid1[i][j] + temp_val) < INT_MIN )
         {
            grid1[i][j] = INT_MIN;

            if ( DP_LIB002_DEBUG )
            {
               fprintf(stderr, "%s temp_val: %f, grid1[%d][%d] capped at"
	 	               " INT_MIN (%d)\n",
                      "add_unbiased_short_to_int:", temp_val, i, j, INT_MIN);
            }
         }
         else /* a good value */
         {
            grid1[i][j] += grid2[i][j];
            if(grid1[i][j] == QPE_NODATA) /* accidentally landed */
               grid1[i][j] += 1;          /* move up 1 */
         }

      } /* end loop over all bins */

   } /* end loop over all radials */

} /* end add_unbiased_short_to_int() ================================== */

/******************************************************************************
    Filename: dp_grid_operators.c

    Description:
    ============
      subtract_biased_grids() will subtract two accum grids,
      grid1 - (grid2 * bias), with the result stored in grid1.

    Inputs:
       grid1 - biased corrected accumulation grid. The result of the
               subtraction will be stored here. This is an array of ints.
       grid2 - not bias corrected accumulation grid. This is an array of ints.
       bias  - multiplicative mean-field bias value.

    Outputs:
       Difference of grid1 and grid2 stored in grid1.

    Returns: FUNCTION_SUCCEEDED (0), NEGATIVE_BIAS (3)

    Currently uncalled.

    Change History
    ==============
    DATE         VERSION    PROGRAMMERS        NOTES
    ---------    -------    ---------------    ------------------
    09 Oct 07    0000       Stein              Initial implementation
    15 Oct 07    0001       Stein              Added a biased version to add
                                               and subtract functions
    22 Jan 09    0002       James Ward         Added checks for overflow
******************************************************************************/

int subtract_biased_grids ( int grid1[][MAX_BINS], int grid2[][MAX_BINS],
                            float bias )
{
   int   i, j;     /* indices */
   float temp_val; /* to guard against overflow */
   char  msg[200]; /* stderr message */

   sprintf(msg, "WARNING! ERROR! Biased functions should NOT be called!!!\n");
   RPGC_log_msg(GL_INFO, msg);
   if(DP_LIB002_DEBUG)
      fprintf(stderr, msg);

   sprintf(msg, "Dual-pol bias not functional.\n");
   RPGC_log_msg(GL_INFO, msg);
   if(DP_LIB002_DEBUG)
      fprintf(stderr, msg);

   sprintf(msg, "Invalid call to subtract_biased_grids.\n");
   RPGC_log_msg(GL_INFO, msg);
   if(DP_LIB002_DEBUG)
      fprintf(stderr, msg);

   /* Check for negative bias */

   if(bias_is_negative(bias, "subtract_biased_grids"))
      return(NEGATIVE_BIAS);

   for (i = 0; i < MAX_AZM; ++i)
   {
      for (j = 0; j < MAX_BINS; ++j)
      {
         /* Note: QPE_NODATA handling logic applies only to accum grids,
          * not to rate grids. For rate grids, if either is QPE_NODATA,
          * they cannot be subtracted. */

         if(grid2[i][j] == QPE_NODATA)
         {
            /* No change to grid1[i][j] */
            continue;
         }
         else if(grid1[i][j] == QPE_NODATA)
         {
            /* Change grid1[i][j] to 0 so can subtract */
            grid1[i][j] = 0;
         }

         temp_val = (float) grid2[i][j] * bias;

         if ( (grid1[i][j] - temp_val) > INT_MAX )
         {
            grid1[i][j] = INT_MAX;

            if ( DP_LIB002_DEBUG )
            {
               fprintf(stderr, "%s temp_val: %f, grid1[%d][%d] capped at"
		               " INT_MAX (%d)\n",
                     "subtract_biased_grids:", temp_val, i, j, INT_MAX);
            }
         }
         else if ( (grid1[i][j] - temp_val) < INT_MIN )
         {
            grid1[i][j] = INT_MIN;

            if ( DP_LIB002_DEBUG )
            {
               fprintf(stderr, "%s temp_val: %f, grid1[%d][%d] capped at "
		               "INT_MIN (%d)\n",
                     "subtract_biased_grids:", temp_val, i, j, INT_MIN);
            }
         }
         else /* a good value */
         {
            grid1[i][j] -= (int) RPGC_NINT(temp_val);
            if(grid1[i][j] == QPE_NODATA) /* accidentally landed */
               grid1[i][j] += 1;          /* move up 1 */
         }

      } /* end loop over all bins */

   } /* end loop over all radials */

   return(FUNCTION_SUCCEEDED);

} /* end subtract_biased_grids() ===================================== */

/******************************************************************************
    Filename: dp_grid_operators.c

    Description:
    ============
      subtract_biased_short_from_int() will subtract two accum grids,
      grid1 - (grid2 * bias), with the result stored in grid1.

    Inputs:
       grid1 - bias corrected accumulation grid. The result of the subtraction
               will be stored here. This is an array of ints.
       grid2 - not bias corrected accumulation grid.
               This is an array of shorts.
       bias  - multiplicative mean-field bias value.

    Outputs:
       Difference of grid1 and grid2 stored in grid1.

    Returns: FUNCTION_SUCCEEDED (0), NEGATIVE_BIAS (3)

    Called by: CQ_Trim_To_Hour().

    Change History
    ==============
    DATE         VERSION    PROGRAMMERS        NOTES
    ---------    -------    ---------------    ------------------
    09 Oct 07    0000       Daniel Stein       Initial implementation
    15 Oct 07    0001       Daniel Stein       Added a biased version to add
                                               and subtract functions
    22 Jan 09    0002       James Ward         Added checks for overflow
******************************************************************************/

int subtract_biased_short_from_int ( int grid1[][MAX_BINS],
                                     short grid2[][MAX_BINS], float bias )
{
   int   i, j;     /* indices */
   float temp_val; /* to guard against overflow */
   char  msg[200]; /* stderr message */

   sprintf(msg, "WARNING! ERROR! Biased functions should NOT be called!!!\n");
   RPGC_log_msg(GL_INFO, msg);
   if(DP_LIB002_DEBUG)
      fprintf(stderr, msg);

   sprintf(msg, "Dual-pol bias not functional.\n");
   RPGC_log_msg(GL_INFO, msg);
   if(DP_LIB002_DEBUG)
      fprintf(stderr, msg);

   sprintf(msg, "Invalid call to subtract_biased_short_from_int.\n");
   RPGC_log_msg(GL_INFO, msg);
   if(DP_LIB002_DEBUG)
      fprintf(stderr, msg);

   /* Check for negative bias */

   if(bias_is_negative(bias, "subtract_biased_short_from_int"))
      return(NEGATIVE_BIAS);

   for (i = 0; i < MAX_AZM; ++i)
   {
      for (j = 0; j < MAX_BINS; ++j)
      {
        /* Note: QPE_NODATA handling logic applies only to accum grids,
         * not to rate grids. For rate grids, if either is QPE_NODATA,
         * they cannot be subtracted. */

        if(grid2[i][j] == QPE_NODATA)
         {
            /* No change to grid1[i][j] */
            continue;
         }
         else if(grid1[i][j] == QPE_NODATA)
         {
            /* Change grid1[i][j] to 0 so can subtract */
            grid1[i][j] = 0;
         }

         temp_val = (float) grid2[i][j] * bias;

         if ( (grid1[i][j] - temp_val) > INT_MAX )
         {
            grid1[i][j] = INT_MAX;

            if ( DP_LIB002_DEBUG )
            {
               fprintf(stderr, "%s temp_val: %f, grid1[%d][%d] capped at "
		               "INT_MAX (%d)\n",
                               "subtract_biased_short_from_int:",
			       temp_val, i, j, INT_MAX);
            }
         }
         else if ( (grid1[i][j] - temp_val) < INT_MIN )
         {
            grid1[i][j] = INT_MIN;

            if ( DP_LIB002_DEBUG )
            {
               fprintf(stderr, "%s temp_val: %f, grid1[%d][%d] capped at "
		               "INT_MIN (%d)\n",
			       "subtract_biased_short_from_int:",
			       temp_val, i, j, INT_MIN);
            }
         }
         else /* a good value */
         {
            grid1[i][j] -= (int) RPGC_NINT(temp_val);
            if(grid1[i][j] == QPE_NODATA) /* accidentally landed */
               grid1[i][j] += 1;          /* move up 1 */
         }

      } /* end loop over all bins */

   } /* end loop over all radials */

   return(FUNCTION_SUCCEEDED);

} /* end subtract_biased_short_from_int() =========================== */

/******************************************************************************
    Filename: dp_grid_operators.c

    Description:
    ============
      subtract_unbiased_short_from_int() will subtract accum two grids,
      grid1 - grid2, with the result stored in grid1.

    Inputs:
       grid1 - not bias corrected accumulation grid. The result of the
               subtraction will be stored here. This is an array of ints.
       grid2 - not bias corrected accumulation grid.
               This is an array of shorts.

    Outputs:
       Difference of grid1 and grid2 stored in grid1.

    Called by: CQ_Trim_To_Hour().

    Change History
    ==============
    DATE         VERSION    PROGRAMMERS        NOTES
    ---------    -------    ---------------    ------------------
    09 Oct 07    0000       Daniel Stein       Initial implementation
    22 Jan 09    0001       James Ward         Added checks for overflow
******************************************************************************/

void subtract_unbiased_short_from_int ( int grid1[][MAX_BINS],
                                        short grid2[][MAX_BINS] )
{
   int   i, j;     /* indices */
   float temp_val; /* to guard against overflow */

   for (i = 0; i < MAX_AZM; ++i)
   {
      for (j = 0; j < MAX_BINS; ++j)
      {
        /* Note: QPE_NODATA handling logic applies only to accum grids,
         * not to rate grids. For rate grids, if either is QPE_NODATA,
         * they cannot be subtracted. */

        if(grid2[i][j] == QPE_NODATA)
         {
            /* No change to grid1[i][j] */
            continue;
         }
         else if(grid1[i][j] == QPE_NODATA)
         {
            /* Change grid1[i][j] to 0 so can subtract */
            grid1[i][j] = 0;
         }

         temp_val = (float) grid2[i][j];

         if ( (grid1[i][j] - temp_val) > INT_MAX )
         {
            grid1[i][j] = INT_MAX;

            if ( DP_LIB002_DEBUG )
            {
               fprintf(stderr, "%s temp_val: %f, grid1[%d][%d] capped at "
		               "INT_MAX (%d)\n",
                               "subtract_unbiased_short_from_int:",
		               temp_val, i, j, INT_MAX);
            }
         }
         else if ( (grid1[i][j] - temp_val) < INT_MIN )
         {
            grid1[i][j] = INT_MIN;

            if ( DP_LIB002_DEBUG )
            {
               fprintf(stderr, "%s temp_val: %f, grid1[%d][%d] capped at "
		               "INT_MIN (%d)\n",
                               "subtract_unbiased_short_from_int:",
			       temp_val, i, j, INT_MIN);
            }
         }
         else /* a good value */
         {
            grid1[i][j] -= (int) grid2[i][j];
            if(grid1[i][j] == QPE_NODATA) /* accidentally landed */
               grid1[i][j] += 1;          /* move up 1 */
         }

      } /* end loop over all bins */

   } /* end loop over all radials */

} /* end subtract_unbiased_short_from_int() ======================== */

/******************************************************************************
    Filename: dp_grid_operators.c

    Description:
    ============
      subtract_unbiased_short_grids() will subtract two accum grids,
      grid1 - grid2, with the result stored in grid1.

    Inputs:
       grid1 - not bias corrected accumulation grid. The result of the
               subtraction will be stored here. This is an array of shorts.
       grid2 - not bias corrected accumulation grid.
               This is an array of shorts.

    Outputs:
       Difference of grid1 and grid2 stored in grid1.

    Called by: compute_Diff_grids().

    Change History
    ==============
    DATE         VERSION    PROGRAMMERS        NOTES
    ---------    -------    ---------------    ------------------
    09 Oct 07    0000       Daniel Stein       Initial implementation
    22 Jan 09    0001       James Ward         Added checks for overflow
******************************************************************************/

void subtract_unbiased_short_grids ( short grid1[][MAX_BINS],
                                     short grid2[][MAX_BINS] )
{
   int   i, j;     /* indices */
   float temp_val; /* to guard against overflow */

   for (i = 0; i < MAX_AZM; ++i)
   {
      for (j = 0; j < MAX_BINS; ++j)
      {
         /* Note: QPE_NODATA handling logic applies only to accum grids,
          * not to rate grids. For rate grids, if either is QPE_NODATA,
          * they cannot be subtracted. */

         if(grid2[i][j] == QPE_NODATA)
         {
            /* 20080708 Ward - Noticed during CCR NA08-16301 testing.
             * When a radial is missing, the legacy buffer will code
             * the accum values as 0 and the legacy product will display
             * a blank wedge. The DP difference products do NOT show a wedge,
             * for it treats an accum value of 0 as a 0 accum,
             * and DP - 0 = DP. The problem is that there is no difference
             * in the legacy buffer between a 0 accum and no data. To treat
             * the 0 as a no data, and show a wedge in the DP, add the
             * following line:
             *
             * grid1[i][j] = QPE_NODATA; */

            /* No change to grid1[i][j] */
            continue;
         }
         else if(grid1[i][j] == QPE_NODATA)
         {
            /* Change grid1[i][j] to 0 so can subtract */
            grid1[i][j] = 0;
         }

         temp_val = (float) grid2[i][j];

         if ((grid1[i][j] - temp_val) > SHRT_MAX)
         {
            grid1[i][j] = SHRT_MAX;

            if ( DP_LIB002_DEBUG )
            {
               fprintf(stderr, "%s temp_val: %f, grid1[%d][%d] capped at "
		               "SHRT_MAX (%d)\n",
                    "subtract_unbiased_short_grids:", temp_val, i, j, SHRT_MAX);
            }
         }
         else if ((grid1[i][j] - temp_val) <= SHRT_MIN)
         {
            /* Add 1 since QPE_NODATA is SHRT_MIN */

            grid1[i][j] = SHRT_MIN + 1;

            if ( DP_LIB002_DEBUG )
            {
               fprintf(stderr, "%s temp_val: %f, grid1[%d][%d] capped at "
		               "SHRT_MIN + 1 (%d)\n",
                       "subtract_unbiased_short_grids:",
                       temp_val, i, j, SHRT_MIN + 1);
            }
         }
         else /* a good value */
         {
            grid1[i][j] -= grid2[i][j];
            if(grid1[i][j] == QPE_NODATA) /* accidentally landed */
               grid1[i][j] += 1;          /* move up 1 */
         }

      } /* end loop over all bins */

   } /* end loop over all radials */

} /* end subtract_unbiased_short_grids() ============================== */

/******************************************************************************
    Filename: dp_grid_operators.c

    Description:
    ============
      multiply_grid_by_C() will multiply every bin in an accum grid by
      a constant multiplier C, storing the result in grid.

    Inputs:
       grid1      - accumulation grid. The result of the multiplication
                    will be stored here.
       multiplier - a constant value multiplier.

    Outputs:
       grid with all bins multiplied by "multiplier".

    Currently uncalled.

    Change History
    ==============
    DATE         VERSION    PROGRAMMERS        NOTES
    ---------    -------    ---------------    ------------------
    09 Oct 07    0000       Daniel Stein       Initial implementation
    22 Jan 09    0001       James Ward         Added checks for overflow
******************************************************************************/

void multiply_grid_by_C ( short grid1[][MAX_BINS], float multiplier )
{
   int   i, j;     /* indices */
   float temp_val; /* to guard against overflow */

   for (i = 0; i < MAX_AZM; ++i)
   {
      for (j = 0; j < MAX_BINS; ++j)
      {
         if (grid1[i][j] == QPE_NODATA)
         {
            continue;
         }

         temp_val = (float) grid1[i][j] * multiplier;

         if (temp_val > SHRT_MAX)
         {
            grid1[i][j] = SHRT_MAX;

            if ( DP_LIB002_DEBUG )
            {
               fprintf(stderr, "%s temp_val: %f, grid1[%d][%d] capped at "
		               "SHRT_MAX (%d)\n",
                               "multiply_grid_by_C:",
			       temp_val, i, j, SHRT_MAX);
            }
         }
         else if (temp_val <= SHRT_MIN)
         {
            /* Add 1 since QPE_NODATA is SHRT_MIN */

            grid1[i][j] = SHRT_MIN + 1;

            if ( DP_LIB002_DEBUG )
            {
               fprintf(stderr, "%s temp_val: %f, grid1[%d][%d] capped at "
		               "SHRT_MIN + 1(%d)\n",
                               "multiply_grid_by_C:",
			       temp_val, i, j, SHRT_MIN + 1);
            }
         }
         else /* a good value */
         {
            grid1[i][j] = (short) RPGC_NINT(temp_val);
            if(grid1[i][j] == QPE_NODATA) /* accidentally landed */
               grid1[i][j] += 1;          /* move up 1 */
         }

      } /* end loop over all bins */

   } /* end loop over all radials */

} /* end multiply_grid_by_C() ======================================= */

/******************************************************************************
    Filename: dp_grid_operators.c

    Description:
    ============
      convert_grid_to_DP_res() converts a legacy accumulation grid,
    360 X 115 in 10ths of millimeters to a DP accumulation grid,
    360 X 920 in 1000ths of inches.

    NOTE - Stores the result in DP_grid.

    Inputs:
       Legacy_grid - 360 X 115 accumulation grid in 10ths of millimeters.
       20071108    - Cham verified that HYADJSCN Scan-to-Scan and Hourly
                     Accumulation are in 0.1 mm (360x115).
    Outputs:
       DP_grid     - 360 X 920 accumulation grid in 1000ths of inches

    Called by: compute_Diff_grids(). Accumulation grids are non-negative.

    Change History
    ==============
    DATE          VERSION    PROGRAMMERS        NOTES
    -----------   -------    ---------------    ------------------
    09 Oct 2007   0000       Cham Pham          Initial implementation
    13 Nov 2007   0001       James Ward         Changed from mm -> in to
                                                10ths mm -> 1000 in.
    22 Jan 2009   0002       James Ward         Added checks for overflow
******************************************************************************/

void convert_grid_to_DP_res ( short Legacy_grid[][MAX_2KM_RESOLUTION],
                              short DP_grid[][MAX_BINS] )
{
   int   i, j;     /* indices */
   float temp_val; /* to guard against overflow */
   static int ratio = MAX_BINS / MAX_2KM_RESOLUTION; /* = 920 / 115 = 8 */

   /* Iterate over the Dual Pol grid, plucking the right legacy
    * value to fill it. */

   for ( i = 0; i < MAX_AZM; ++i )
   {
      for ( j = 0; j < MAX_BINS; ++j )
      {
         /* INIT_VALUE is the legacy no data value, defined in a313h.h
          * 20071204 Confirmed by Cham Pham:
          * The "no data" value is INIT_VALUE (0) or IINIT(0) so you can
          * replace it with QPE_NODATA. */

         if (Legacy_grid[i][j / ratio] == INIT_VALUE)
         {
            DP_grid[i][j] = QPE_NODATA;
            continue;
         }

         /* Legacy_grid is in 10ths   of mm
          * DP_grid     is in 1000ths of inches
          * so mult factor is 1000/10 = 100.0
          */

         temp_val = (float) (Legacy_grid[i][j / ratio] * MM_TO_IN * 100.0);

         if (temp_val > SHRT_MAX)
         {
            DP_grid[i][j] = SHRT_MAX;

            if ( DP_LIB002_DEBUG )
            {
               fprintf(stderr, "%s temp_val: %f, DP_grid[%d][%d] capped at "
		               "SHRT_MAX (%d)\n",
                               "convert_grid_to_DP_res:",
			       temp_val, i, j, SHRT_MAX);
            }
         }
         else if (temp_val <= SHRT_MIN)
         {
            /* Add 1 since QPE_NODATA is SHRT_MIN */

            DP_grid[i][j] = SHRT_MIN + 1;

            if (DP_LIB002_DEBUG)
            {
               fprintf(stderr, "%s temp_val: %f, DP_grid[%d][%d] capped at "
		               "SHRT_MIN + 1 (%d)\n",
                               "convert_grid_to_DP_res:",
			       temp_val, i, j, SHRT_MIN + 1);
            }
         }
         else /* a good value */
         {
            DP_grid[i][j] = (short) RPGC_NINT(temp_val);
            if(DP_grid[i][j] == QPE_NODATA) /* accidentally landed */
               DP_grid[i][j] += 1;          /* move up 1 */
            else if((DP_grid[i][j] == 0) && (Legacy_grid[i][j / ratio] != 0))
               DP_grid[i][j] = 1;           /* don't lose small values */
         }

      } /* end loop over all bins */

   } /* end loop over all radials */

} /* end convert_grid_to_DP_res() ==================================== */

/******************************************************************************
    Filename: dp_grid_operators.c

    Description:
    ============
       convert_DSA_int_to_256_char() takes a 2-dimensional array of ints and
    converts them to an array of unsigned chars with 256 data levels.

    Inputs: int           int_grid     - 2-dim array of ints.
            int           DSA_max      - a value equal to 2 times the user-
                                         defined maximum value for the
                                         4-bit storm-total products
                                         (STA and STP). DSA_max will act as a
                                         cap on the maximum valued encoded in
                                         the final DSA product.

    Output: int*          min_val      - int_grid min before conversion
            int*          max_val      - int_grid max before conversion
            float*        scale        - scale used to convert back
                                         to the data levels
            float*        offset       - offset used to convert back
                                         to the data levels
            int           product_code - product code used to set scale/offset
            unsigned char UC_grid      - 2-dim array of unsigned chars of
                                         int_grid converted to 256 data levels.

    Returns: FUNCTION_SUCCEEDED (0), NULL_POINTER (2)

    Note: MIN and min_val mean "min non-zero accum". There might be some
    zero accum, but scale/offset is calculated from the min non-zero accum,
    because zero accum always codes in the output product to 0. The accum
    line looks like:

    0 ---- min non-zero accum ---- max accum.

    and if you scale/offset from 0 to max accum, the range between 0 and
    min non-zero accum is wasted because no values ever get coded there.
    However, the min non-zero accum always seems to be 1, so it's probably
    a useless economy.

    Called by: DSA_product() only.

    Change History
    ==============
    DATE         VERSION    PROGRAMMER         NOTES
    -----------  -------    ----------         -----------------------------
    23 Oct 2007   0000      Dan Stein          Initial implementation
    05 May 2008   0001      James Ward         Changed to scale and offset
    21 Oct 2008   0002      James Ward         Changed to fixed scale/offset
                                               until cvg 8.8 comes out
    22 Jan 2009   0003      James Ward         Added checks for overflow

NOTE - this function was copied from convert_int_to_256_char.

    16 Aug 2010   0001      Dan Stein          Copied old function, added
                                               DSA_max parameter.
******************************************************************************/

int convert_DSA_int_to_256_char (int int_grid [][MAX_BINS],
                                 int*   min_val,
                                 int*   max_val,
                                 float* scale,
                                 float* offset,
                                 int    product_code,
                                 unsigned char UC_grid[][MAX_BINS],
                                 int DSA_max)
{
   int i, j;                /* loop counters             */
   int mini = 0;            /* i index of min value      */
   int minj = 0;            /* j index of min value      */
   int maxi = 0;            /* i index of max value      */
   int maxj = 0;            /* j index of max value      */
   int temp_val = 0;        /* temporary value holder    */
   int fixed_scale = FALSE;  /* TRUE -> use a fixed scale */

   /* 17 Aug 2010 - Dan Stein - addded the following variables to be used in
    * the new DSA encoding scheme.
    */
   int scaling_max = 0;
   int scaling_factor = 0;

   /* Check for NULL pointers */

   if(pointer_is_NULL(min_val, "convert_DSA_int_to_256_char", "min_val"))
      return(NULL_POINTER);

   if(pointer_is_NULL(max_val, "convert_DSA_int_to_256_char", "max_val"))
      return(NULL_POINTER);

   if(pointer_is_NULL(scale, "convert_DSA_int_to_256_char", "scale"))
      return(NULL_POINTER);

   if(pointer_is_NULL(offset, "convert_DSA_int_to_256_char", "offset"))
      return(NULL_POINTER);

   /* Init values */

   *min_val = INT_MAX;
   *max_val = INT_MIN;

   /* Loop through grid once to determine min/max values
    * (1000ths of an inch). */

   for (i = 0; i < MAX_AZM; ++i)
   {
      for (j = 0; j < MAX_BINS; ++j)
      {
         /* Ignore QPE_NODATA when looking for min/max.
          * Since convert_DSA_int_to_256_char() is only called for
          * non-negative DSA, also ignore negative values.
          */

	 if ((int_grid[i][j] == QPE_NODATA) || (int_grid[i][j] <= 0))
           continue;

         if (int_grid[i][j] > *max_val)
         {
           maxi = i;
           maxj = j;
           *max_val = int_grid[i][j];
         }

         if (int_grid[i][j] < *min_val)
         {
           mini = i;
           minj = j;
           *min_val = int_grid[i][j];
         }

      } /* J - end loop over all bins */

   } /* I - end loop over all radials */

   if(fixed_scale == TRUE)
   {
      /* The maximum STA value is 25.4 inches. */
      *scale  = 0.1; /* FACTOR_8BIT / (25400 = 25.4 in 1000ths of inches) */
      *offset = 1.0;
   }
   else /* use a variable scale and offset */
   {
      /* Find the scale and offset so the min accum_value goes to 1
       * and the max accum_value goes to 255 under the formulas:
       *
       * encoded_uchar = (accum_value * SCALE) + OFFSET
       *
       * accum_value = (encoded_uchar - OFFSET) / SCALE
       *
       * A 10.0 converts 1000ths of inches to (output) 100ths of inches.
       *
       * So:
       *
       * 255 = ((max / 10) * scale) + offset
       *
       *   1 = ((min / 10) * scale) + offset
       *
       * Subtracting equations:
       *
       * 254 = (((max - min) / 10) * scale)
       *
       * Use the min equation to get the offset.
       */

      /* 16 Aug 2010 - Dan Stein - DSA will be encoded like the legacy DSP for
       * values less than 2 times the 4-bit storm-total maximum (using a
       * multiplicative scaling factor). For values greater than this, use
       * 2 times the 4-bit storm-total maximum as a cap. So the scaling_max
       * is the lesser of max_val and DSA_max.
       */

      if (*max_val < DSA_max)
         scaling_max = *max_val;
      else
         scaling_max = DSA_max;

      /* Determine the scaling_factor. This value starts at 1 and increments
       * every time the max value goes over a multiple of 255.  This process
       * will continue until the max is more than 2 times the 4-bit storm-
       * total max; then it will be capped.
       *
       * encoded_uchar = (accum_value * SCALE) + OFFSET
       *
       * encoded level = (accum in hundredths in * SCALE) + OFFSET
       *
       * SCALE = change in 1 level / change in accum in hundredths inches
       *
       * scaling_factor level 1 (in) level 2 (in) ... level 255 (in)
       * -------------- ------------ ------------     --------------
       *       1           0.01         0.02             2.55
       *       2           0.02         0.04             5.10 
       *       3           0.03         0.06             7.65
       *       4           0.04         0.08            10.20 
       *
       * SCALE = 1 / scaling_factor
       *
       * OFFSET = encoded level when accum is 0. This is always 0.
       *
       * The scaling factor = the scaling_max in hundredths inches / 255 levels
       *
       *  scaling_factor = (scaling_max / 10) / 255
       *                 = scaling_max / 2550
       */

      if(scaling_max % 2550 == 0)
         scaling_factor = scaling_max / 2550;
      else
         scaling_factor = (scaling_max / 2550) + 1;

      if(*max_val != *min_val) /* if there is a range of data */
      {
         *scale  = 1.0 / scaling_factor;
         *offset = 0.0;
      }
      else /* can't use a variable scale, use the fixed */
      {
         *scale  = 0.1;
         *offset = 1.0;
      }
   }

   print_min_max(mini, minj, *min_val,
                 maxi, maxj, *max_val,
                 *scale, *offset, product_code);

   /* Determine the data level for each bin. */

   /* For debugging individual values:
    *
    * fprintf(stderr, "---------- %d ----------\n", product_code);
    */

   for (i = 0; i < MAX_AZM; ++i)
   {
       for (j = 0; j < MAX_BINS; ++j)
       {
          /* Since convert_DSA_int_to_256_char() is only called for
           * non-negative DSA, negative values are bad.
           */

          if((int_grid[i][j] == QPE_NODATA) || (int_grid[i][j] <= 0))
          {
	     UC_grid[i][j] = (unsigned char) NODATA_VALUE_8BIT;
          }
          else /* a good value */
	  {
             /* 10.0 converts 1000ths of inches to (output) 100ths of inches. */

             temp_val = (int) RPGC_NINT(((int_grid[i][j] / 10.0) * *scale) +
                                          *offset);
             /* For debugging
              *
              * if(i == 248)
              * {
              *    fprintf(stderr, "int_grid[%d][%d] %d, temp_val %d",
              *            i, j, int_grid[i][j], temp_val);
              * }
              */

             /* Make sure we don't scale away small values */

             if((temp_val == 0) && (int_grid[i][j] != 0))
		temp_val = 1;

             /* Store the value in the UC_grid, with boundary checks.
              *
              * In previous scale encodings the maximum value was encoded
              * to 255, so we didn't have to worry about encoding a
              * temp_val > 255. Now the max_value could be larger than the
              * (default) encoded cap value of 30.0 inches (unlikely),
              * so we have to worry about encoding > 255. */

	     if(temp_val > 255)
             {
                UC_grid[i][j] = (unsigned char) 255;

                if(DP_LIB002_DEBUG)
                   fprintf(stderr, "%s temp_val: %d, UC_grid[%d][%d] capped at"
                                    " 255 (%d)\n",
                                    "convert_DSA_int_to_256_char:",
                                     temp_val, i, j, 255);

             }
             else if (temp_val <= NODATA_VALUE_8BIT)
             {
                /* 20081222 Ward The lowest good value is 1,
                 *               as NODATA_VALUE_8BIT (0) is
                 *               reserved for the no-data flag */

                UC_grid[i][j] = (unsigned char) NODATA_VALUE_8BIT + 1;

                if(DP_LIB002_DEBUG)
                   fprintf(stderr, "%s temp_val: %d, UC_grid[%d][%d] capped "
                                   "at 1\n",
                                   "convert_DSA_int_to_256_char:", temp_val, i, j);
             }
             else /* a good value */
	        UC_grid[i][j] = (unsigned char) temp_val;

             /* For debugging
              *
              * if(i == 248)
              * {
              *    fprintf(stderr, ", UC_grid %d\n", UC_grid[i][j]);
              * }
              */

	  } /* end bin value is good */

      } /* J - end loop over all bins */

   } /* I - end loop over all radials */

   return(FUNCTION_SUCCEEDED);

} /* end convert_DSA_int_to_256_char() =================================== */

/******************************************************************************
    Filename: dp_grid_operators.c

    Description:
    ============
       convert_int_to_256_char() takes a 2-dimensional array of ints and
    converts them to an array of unsigned chars with 256 data levels.

    Inputs: int           int_grid     - 2-dim array of ints.

    Output: int*          min_val      - int_grid min before conversion
            int*          max_val      - int_grid max before conversion
            float*        scale        - scale used to convert back
                                         to the data levels
            float*        offset       - offset used to convert back
                                         to the data levels
            int           product_code - product code used to set scale/offset
            unsigned char UC_grid      - 2-dim array of unsigned chars of
                                         int_grid converted to 256 data levels.

    Returns: FUNCTION_SUCCEEDED (0), NULL_POINTER (2)

    Note: MIN and min_val mean "min non-zero accum". There might be some
    zero accum, but scale/offset is calculated from the min non-zero accum,
    because zero accum always codes in the output product to 0. The accum
    line looks like:

    0 ---- min non-zero accum ---- max accum.

    and if you scale/offset from 0 to max accum, the range between 0 and
    min non-zero accum is wasted because no values ever get coded there.
    However, the min non-zero accum always seems to be 1, so it's probably
    a useless economy.

    Called by: DAA_product(), DSA_product(), DUA write_to_output_product().

    Change History
    ==============
    DATE         VERSION    PROGRAMMER         NOTES
    -----------  -------    ----------         -----------------------------
    23 Oct 2007   0000      Dan Stein          Initial implementation
    05 May 2008   0001      James Ward         Changed to scale and offset
    21 Oct 2008   0002      James Ward         Changed to fixed scale/offset
                                               until cvg 8.8 comes out
    22 Jan 2009   0003      James Ward         Added checks for overflow
******************************************************************************/

int convert_int_to_256_char (int int_grid [][MAX_BINS],
                             int*   min_val,
                             int*   max_val,
                             float* scale,
                             float* offset,
                             int    product_code,
                             unsigned char UC_grid[][MAX_BINS])
{
   int i, j;                /* loop counters             */
   int mini = 0;            /* i index of min value      */
   int minj = 0;            /* j index of min value      */
   int maxi = 0;            /* i index of max value      */
   int maxj = 0;            /* j index of max value      */
   int temp_val = 0;        /* temporary value holder    */
   int fixed_scale = FALSE;  /* TRUE -> use a fixed scale */

   /* Check for NULL pointers */

   if(pointer_is_NULL(min_val, "convert_int_to_256_char", "min_val"))
      return(NULL_POINTER);

   if(pointer_is_NULL(max_val, "convert_int_to_256_char", "max_val"))
      return(NULL_POINTER);

   if(pointer_is_NULL(scale, "convert_int_to_256_char", "scale"))
      return(NULL_POINTER);

   if(pointer_is_NULL(offset, "convert_int_to_256_char", "offset"))
      return(NULL_POINTER);

   /* Init values */

   *min_val = INT_MAX;
   *max_val = INT_MIN;
   *scale   = 0.2;     /* default scale  for OHA */
   *offset  = 1.0;     /* default offset for OHA */

   /* Loop through grid once to determine min/max values
    * (1000ths of an inch). */

   for (i = 0; i < MAX_AZM; ++i)
   {
      for (j = 0; j < MAX_BINS; ++j)
      {
         /* Ignore QPE_NODATA when looking for min/max.
          * Since convert_int_to_256_char() is only called for
          * non-negative DAA, DSA, and DUA, also ignore negative values.
          */

	 if ((int_grid[i][j] == QPE_NODATA) || (int_grid[i][j] <= 0))
           continue;

         if (int_grid[i][j] > *max_val)
         {
           maxi = i;
           maxj = j;
           *max_val = int_grid[i][j];
         }

         if (int_grid[i][j] < *min_val)
         {
           mini = i;
           minj = j;
           *min_val = int_grid[i][j];
         }

      } /* end loop over all bins */

   } /* end loop over all radials */

   if(fixed_scale == TRUE)
   {
      /* The maximum OHA value is 12.7 inches,
       * the maximum STA value is 25.4 inches. */

      if(product_code == 170) /* DAA */
      {
         *scale  = 0.2; /* FACTOR_8BIT / (12700 = 12.7 in 1000ths of inches) */
         *offset = 1.0;
      }
      else /* DSA and DUA */
      {
         *scale  = 0.1; /* FACTOR_8BIT / (25400 = 25.4 in 1000ths of inches) */
         *offset = 1.0;
      }
   }
   else /* use a variable scale and offset */
   {
      /* Find the scale and offset so the min accum_value goes to 1
       * and the max accum_value goes to 255 under the formulas:
       *
       * encoded_uchar = (accum_value * SCALE) + OFFSET
       *
       * accum_value = (encoded_uchar - OFFSET) / SCALE
       *
       * A 10.0 converts 1000ths of inches to (output) 100ths of inches.
       *
       * So:
       *
       * 255 = ((max / 10) * scale) + offset
       *
       *   1 = ((min / 10) * scale) + offset
       *
       * Subtracting equations:
       *
       * 254 = (((max - min) / 10) * scale)
       *
       * Use the min equation to get the offset.
       */

      if(*max_val != *min_val) /* there is a range of data */
      {
         *scale  = FACTOR_8BIT / (*max_val - *min_val);
         *offset = 1.0 - ((*min_val * *scale) / 10.0);
      }
      else /* can't use a variable scale, use the fixed */
      {
         if(product_code == 170) /* DAA */
         {
            *scale  = 0.2;
            *offset = 1.0;
         }
         else /* DSA and DUA */
         {
            *scale  = 0.1;
            *offset = 1.0;
         }
      }
   }

   print_min_max(mini, minj, *min_val,
                 maxi, maxj, *max_val,
                 *scale, *offset, product_code);

   /* Determine the data level for each bin. */

   /* For debugging individual values:
    *
    * fprintf(stderr, "---------- %d ----------\n", product_code);
    */

   for (i = 0; i < MAX_AZM; ++i)
   {
       for (j = 0; j < MAX_BINS; ++j)
       {
          /* Since convert_int_to_256_char() is only called for
           * non-negative DAA, DSA, and DUA, negative values are bad.
           */

          if((int_grid[i][j] == QPE_NODATA) || (int_grid[i][j] <= 0))
          {
	     UC_grid[i][j] = (unsigned char) NODATA_VALUE_8BIT;
          }
          else /* a good value */
	  {
             /* 10.0 converts 1000ths of inches to (output) 100ths of inches. */

             temp_val = (int) RPGC_NINT(((int_grid[i][j] / 10.0) * *scale) +
                                          *offset);

             /* For debugging individual values:
              *
              * if(((i ==   0) && (j ==  16)) ||
              *    ((i ==  42) && (j == 242)) ||
              *    ((i == 224) && (j ==  15)) ||
              *    ((i == 245) && (j ==  29)))
              * {
              *    fprintf(stderr, "int_grid[%d][%d] %d, temp_val %d\n",
              *            i, j, int_grid[i][j], temp_val);
              * }
              */

             /* Make sure we don't scale away small values */

             if ((temp_val == 0) && (int_grid[i][j] != 0))
		temp_val = 1;

             /* Store the value in the UC_grid, with sanity checks. */

	     if(temp_val > UCHAR_MAX)
             {
                UC_grid[i][j] = UCHAR_MAX;

                if(DP_LIB002_DEBUG)
                   fprintf(stderr, "%s temp_val: %d, UC_grid[%d][%d] capped at"
                                    " UCHAR_MAX (%d)\n",
                                    "convert_int_to_256_char:",
                                     temp_val, i, j, UCHAR_MAX );

             }
             else if (temp_val <= NODATA_VALUE_8BIT)
             {
                /* 20081222 Ward The lowest good value is 1,
                 *               as NODATA_VALUE_8BIT (0) is
                 *               reserved for the no-data flag */

                UC_grid[i][j] = (unsigned char) NODATA_VALUE_8BIT + 1;

                if(DP_LIB002_DEBUG)
                   fprintf(stderr, "%s temp_val: %d, UC_grid[%d][%d] capped "
                                   "at 1\n",
                                   "convert_int_to_256_char:", temp_val, i, j);
             }
             else /* a good value */
	        UC_grid[i][j] = (unsigned char) temp_val;

	  } /* end bin value is good */

      } /* end loop over all bins */

   } /* end loop over all radials */

   return(FUNCTION_SUCCEEDED);

} /* end convert_int_to_256_char() =================================== */

/******************************************************************************
    Filename: dp_grid_operators.c

    Description:
    ============
       compute_datalevel_diffprod() takes a 2-dimensional array of ints and
    converts them to an array of unsigned chars with 256 data levels.

    Inputs: int           int_grid     - 2-dim array of ints.

    Output: int*          min_val      - int_grid min before conversion
            int*          max_val      - int_grid max before conversion
            float*        scale        - scale used to convert back
                                         to the data levels
            float*        offset       - offset used to convert back
                                         to the data levels
            int           product_code - product code used to set scale/offset
            unsigned char UC_grid      - 2-dim array of unsigned chars of
                                         int_grid converted to 256 data levels.

    Returns: FUNCTION_SUCCEEDED (0), NULL_POINTER (2)

    Called by: DOD_product(), DSD_product().

    Change History
    ==============
    DATE        VERSION    PROGRAMMER         NOTES
    ----------- -------    ----------         ---------------------------
    O1 Oct 2007   0000     Cham Pham          Initial implementation
    05 May 2008   0001     James Ward         Changed to scale and offset
    21 Oct 2008   0002     James Ward         Changed to fixed scale/offset
                                              until cvg 8.8 comes out
    22 Jan 2009   0003     James Ward         Added checks for overflow
******************************************************************************/

int compute_datalevel_diffprod (int int_grid [][MAX_BINS],
                                int*   min_val,
                                int*   max_val,
                                float* scale,
                                float* offset,
                                int    product_code,
                                unsigned char UC_grid[][MAX_BINS])
{
   int i, j;                /* loop counters             */
   int mini = 0;            /* i index of min value      */
   int minj = 0;            /* j index of min value      */
   int maxi = 0;            /* i index of max value      */
   int maxj = 0;            /* j index of max value      */
   int temp_val = 0;        /* temporary value holder    */
   int furthest = 0;        /* furthest bin from 0       */
   int fixed_scale = FALSE;  /* TRUE -> use a fixed scale */

   /* Check for NULL pointers */

   if(pointer_is_NULL(min_val, "compute_datalevel_diffprod", "min_val"))
      return(NULL_POINTER);

   if(pointer_is_NULL(max_val, "compute_datalevel_diffprod", "max_val"))
      return(NULL_POINTER);

   if(pointer_is_NULL(scale, "compute_datalevel_diffprod", "scale"))
      return(NULL_POINTER);

   if(pointer_is_NULL(offset, "compute_datalevel_diffprod", "offset"))
      return(NULL_POINTER);

   /* Init values */

   *min_val = INT_MAX;
   *max_val = INT_MIN;
   *scale   = 1.0;     /* default scale for OHA       */
   *offset  = 128.0;   /* calculates to be a constant */

   /* Loop through grid once to determine the maximum magnitude.
    * max_val and min_val will point to the max and min
    * in 1000ths of an inch. */

   for ( i = 0; i < MAX_AZM; ++i )
   {
      for ( j = 0; j < MAX_BINS; ++j )
      {
         /* Ignore QPE_NODATA when looking for min/max. */

         if (int_grid[i][j] == QPE_NODATA)
           continue;

         if (int_grid[i][j] > *max_val)
         {
            maxi = i;
            maxj = j;
            *max_val = int_grid[i][j];
         }

         if (int_grid[i][j] < *min_val)
         {
            mini = i;
            minj = j;
            *min_val = int_grid[i][j];
         }

      } /* end loop over all bins */

   } /* end loop over all radials */

   if(fixed_scale == TRUE)
   {
      /* The maximum OHA value is 12.7 inches,
       * the maximum STA value is 25.4 inches.
       *
       * We are going to assume that the maximum difference will
       * only be 1/10 of the maximum values. */

      if(product_code == 174) /* DOD */
      {
         *scale  = 1.0; /* FACTOR_8BIT / (2 * 1270 = 1.27, 1000ths of inches) */
      }
      else /* DSD */
      {
         *scale  = 0.5; /* FACTOR_8BIT / (2 * 2540 = 2.54, 1000ths of inches) */
      }
   }
   else /* use a variable scale */
   {
     /* We want a difference product to have accum_value 128 represent
      * no difference, so set find the distance of the furthest bin
      * from 0, and then encode the negative of that as 1 and the
      * positive of that as 255, under the formulas:
      *
      * encoded_uchar = (accum_value * SCALE) + OFFSET
      *
      * accum_value = (encoded_uchar - OFFSET) / SCALE
      *
      * A 10 converts 1000ths of inches to (output) 100ths of inches.
      *
      * So:
      *
      * 255 = ((furthest / 10) * scale) + offset
      *
      *   1 = ((-furthest / 10) * scale) + offset
      *
      * Subtracting equations:
      *
      * 254 = (((2 * furthest) / 10) * scale)
      *
      * Use the second equation to get the offset.
      *
      * Note: abs() is used for ints
      */

     if(abs(*max_val) > abs(*min_val))
        furthest = abs(*max_val);
     else /* min is furthest */
        furthest = abs(*min_val);

     if(furthest != 0) /* there is a range of data */
        *scale  = FACTOR_8BIT / (2.0 * furthest);
     else /* can't use a variable scale, use the fixed */
     {
        if(product_code == 174) /* DOD */
           *scale  = 1.0;
        else /* DSD */
           *scale  = 0.5;
     }
   }

   /* The offset always calculates to be a constant:
    *
    * offset = 1.0 + ((furthest / 10.0) * scale);
    *
    * offset = 1.0 + ((furthest / 10.0) * FACTOR_8BIT / (2.0 * furthest)
    *
    * offset = 1.0 + (FACTOR_8BIT / 20.0)
    *
    * offset = 1.0 + (2540.0 / 20.0) = 128.0 */

   print_min_max(mini, minj, *min_val,
                 maxi, maxj, *max_val,
                 *scale, *offset, product_code);

   /* Determine the data level for each bin. */

   /* For debugging individual values:
    *
    * fprintf(stderr, "---------- %d ----------\n", product_code);
    */

   for ( i = 0; i < MAX_AZM; ++i )
   {
      for ( j = 0; j < MAX_BINS; ++j )
      {
         if (int_grid[i][j] == QPE_NODATA)
         {
            UC_grid[i][j] = NODATA_VALUE_8BIT;
         }
         else /* a good value */
         {
            /* 10.0 converts 1000ths of inches to (output) 100ths of inches. */

            temp_val = (int) RPGC_NINT(((int_grid[i][j] / 10.0) * *scale) +
                                         *offset);

            /* For debugging individual values:
             *
             * if(((i ==   0) && (j ==  16)) ||
             *    ((i ==  42) && (j == 242)) ||
             *    ((i == 224) && (j ==  15)) ||
             *    ((i == 245) && (j ==  29)))
             * {
             *    fprintf(stderr, "int_grid[%d][%d] %d, temp_val %d\n",
             *            i, j, int_grid[i][j], temp_val);
             * }
             */

            /* Make sure we don't scale away small values.
             * Note: scale/offset automatically applies a 128 bump. */

            if ((temp_val == 128) && (int_grid[i][j] != 0))
            {
               if (int_grid[i][j] < 0)
                  temp_val = 127;
               else /* int_grid[i][j] > 0 */
                  temp_val = 129;
            }

            /* Store the value in the UC_grid, with sanity checks. */

            if(temp_val > UCHAR_MAX)
            {
               UC_grid[i][j] = UCHAR_MAX;

               if(DP_LIB002_DEBUG)
                  fprintf(stderr, "%s temp_val: %d, UC_grid[%d][%d] capped "
                                  "at UCHAR_MAX (%d)\n",
                                  "compute_datalevel_diffprod:",
                                  temp_val, i, j, UCHAR_MAX );
            }
            else if (temp_val <= NODATA_VALUE_8BIT)
            {
               /* 20081222 Ward The lowest good value is 1, as
                *               NODATA_VALUE_8BIT (0) is reserved
                *               for the no-data flag */

               UC_grid[i][j] = (unsigned char) NODATA_VALUE_8BIT + 1;

               if(DP_LIB002_DEBUG)
                  fprintf(stderr, "%s temp_val: %d, UC_grid[%d][%d] capped "
                                  "at 1\n",
                                  "compute_datalevel_diffprod:",
                                  temp_val, i, j);
            }
            else /* a good value */
               UC_grid[i][j] = (unsigned char) temp_val;

         } /* end bin value is good */

      } /* end loop over all bins */

   } /* end loop over all radials */

   return(FUNCTION_SUCCEEDED);

} /* end compute_datalevel_diffprod() ================================  */

/******************************************************************************
   Filename: dp_grid_operators.c

   Description
   ===========
      convert_precip_4bit() takes a 2-dimensional array of ints and
   converts them to an array of shorts from 0 to 255.

   Note: Even though short_grid is an array of shorts, it is used as an
         input into RPGC_run_length_encode(), which is expecting a
         short grid with 256 level data.

   Inputs: int    int_grid     - 2-dim array of ints.

   Output: int*   min_val      - int_grid min before conversion
           int*   max_val      - int_grid max before conversion
           int    product_code - product code used to set scale/offset
           short  short_grid   - 2-dim array of int_grid converted to
                                 256 levels, stored as shorts.

   Returns: FUNCTION_SUCCEEDED (0), NULL_POINTER (2)

   Called by: OHA_product(), STA_product().

   Change History
   ==============
   DATE        VERSION    PROGRAMMER         NOTES
   --------    -------    ----------         --------------------------------
   20071007    0000       Cham Pham          Initial implementation
   20080618    0001       James Ward         Added printing of min/max values
                                             for debugging.
   20090122    0002       James Ward         Added checks for overflow
   20090416    0003       James Ward         Changed max value from SHRT_MAX
                                             to UCHAR_MAX (255).
******************************************************************************/

int convert_precip_4bit(int int_grid[][MAX_2KM_RESOLUTION],
                        int* min_val,
                        int* max_val,
                        int product_code,
                        short short_grid[][MAX_2KM_RESOLUTION])
{
  int   i, j;         /* loop counters          */
  int   mini = 0;     /* i index of min value   */
  int   minj = 0;     /* j index of min value   */
  int   maxi = 0;     /* i index of max value   */
  int   maxj = 0;     /* j index of max value   */
  int   temp_val = 0; /* temporary value holder */
  float scale = 0.0;  /* fixed scale            */

  /* Check for NULL pointers */

  if(pointer_is_NULL(min_val, "convert_precip_4bit", "min_val"))
     return(NULL_POINTER);

  if(pointer_is_NULL(max_val, "convert_precip_4bit", "max_val"))
     return(NULL_POINTER);

  /* Init values */

  *min_val = INT_MAX;
  *max_val = INT_MIN;

  /* Loop through grid once to determine min/max values (1000ths of an inch).
   * MAX_2KM_RESOLUTION is 115. */

  for(i = 0; i < MAX_AZM; i++)
  {
     for(j = 0; j < MAX_2KM_RESOLUTION; j++)
     {
        /* Ignore QPE_NODATA when looking for min/max.
         * Since convert_precip_4bit() is only called for
         * non-negative OHA and STA, also ignore negative values.
         */

        if((int_grid[i][j] == QPE_NODATA) || (int_grid[i][j] <= 0))
          continue;

        if(int_grid[i][j] > *max_val)
        {
          maxi = i;
          maxj = j;
          *max_val = int_grid[i][j];
        }

        if(int_grid[i][j] < *min_val)
        {
          mini = i;
          minj = j;
          *min_val = int_grid[i][j];
        }
     }
  }

  /* Find the scale factor. Scale is fixed and does not depend on the
   * max value. The offset is 0.0. */

  if(product_code == 169) /* OHA */
  {
     /* Scale factor for converting 1000th of inches to 0.05 inch
      * product data categories. 1 inch of short_grid = 20 data levels.
      * X in a bin = (X/1000) in = (X/1000) * 20 data levels
      *                          = (X/50) data levels
      */

     scale = 0.02;
  }
  else /* product_code == 171, STA */
  {
     /* Scale factor for converting 1000th of inches to 0.1 inch
      * product data categories. 1 inch of short_grid = 10 data levels.
      * Y in a bin = (Y/1000) in = (Y/1000) * 10 data levels
      *                          = (Y/100) data levels
      */

     scale = 0.01;
  }

  print_min_max(mini, minj, *min_val,
                maxi, maxj, *max_val,
                scale, 0.0, product_code);

  /* Determine the data level for each bin. */

  /* For debugging individual values:
   *
   * fprintf(stderr, "---------- %d ----------\n", product_code);
   */

  for (i = 0; i < MAX_AZM; i++)
  {
     for (j = 0; j < MAX_2KM_RESOLUTION; j++)
     {
         /* Since convert_precip_4bit() is only called for
          * non-negative OHA, STA, negative values are bad.
          */

         if((int_grid[i][j] == (int) QPE_NODATA) || (int_grid[i][j] <= 0))
         {
            short_grid[i][j] = (short) NODATA_VALUE_4BIT;
         }
         else /* a good value */
         {
            temp_val = (int) RPGC_NINT (int_grid[i][j] * scale);

            /* Make sure we don't scale away small values */

            if ((temp_val == 0) && (int_grid[i][j] != 0))
               temp_val = 1;

            /* Store the value in the short_grid, with sanity checks.
             * Note the max value is UCHAR_MAX (255) not SHRT_MAX (32767). */

            if (temp_val > UCHAR_MAX)
            {
               short_grid[i][j] = UCHAR_MAX;

               if(DP_LIB002_DEBUG)
                  fprintf(stderr, "%s temp_val: %d, %s[%d][%d] %s (%d)\n",
                          "convert_precip_4bit:",
                          temp_val,
                          "short_grid",
                          i, j,
                          "capped at UCHAR_MAX",
                          UCHAR_MAX);

            }
            else if (temp_val <= NODATA_VALUE_4BIT)
            {
               /* 20081222 Ward The lowest good value is 1,
                *               as NODATA_VALUE_4BIT (0)
                *               is reserved for the no-data flag */

               short_grid[i][j] = (short) NODATA_VALUE_4BIT + 1;

               if(DP_LIB002_DEBUG)
                  fprintf(stderr, "%s temp_val: %d, short_grid[%d][%d] capped "
                          "at 1\n",
                          "convert_precip_4bit:", temp_val, i, j);
             }
             else /* a good value */
               short_grid[i][j] = (short) temp_val;

         } /* end bin value is good */

      } /* end loop over all bins */

   } /* end loop over all radials */

   return(FUNCTION_SUCCEEDED);

} /* end convert_precip_4bit() ================================== */

/******************************************************************************
    Filename: dp_grid_operators.c

    Description:
    ============
    interpolate_grid() adjusts an S2S_Accum_Buf_t to a fraction of its
    time span. It is used by DUA, and to trim an hourly product back to
    an hour.

    If INTERP_FORWARD is set,

       accum->accum_grid[][] will be replaced by a grid that interpolates
       FORWARD from interp_time to accum->supl.end_time.

       accum->supl.begin_time will be replaced with interp_time.

    If INTERP_BACKWARD is set,

       accum->accum_grid[][] will be replaced by a grid that interpolates
       BACKWARD from interp_time to accum->supl.begin_time.

       accum->supl.end_time will be replaced with interp_time.

    Inputs: S2S_Accum_Buf_t* accum       - buffer you want to interpolate
            time_t           interp_time - interpolation time
            int              direction   - INTERP_FORWARD or INTERP_BACKWARD

    Outputs: S2S_Accum_Buf_t* accum modified

    Returns: FUNCTION_SUCCEEDED (0), FUNCTION_FAILED (1), NULL_POINTER (2)

    Called by: CQ_Trim_To_Hour(), compute_dua_accum_grid().

    Change History
    ==============
    DATE          VERSION    PROGRAMMERS        NOTES
    -----------   -------    ---------------    ------------------
    30 Oct 2007   0001       James Ward         Initial implementation
    22 Jan 2009   0002       James Ward         Added checks for overflow
******************************************************************************/

int interpolate_grid(S2S_Accum_Buf_t *accum, time_t interp_time, int direction)
{
   int    i, j; /* indices */
   int    accum_secs      = 0;
   time_t interp_secs     = 0L;
   float  interp_fraction = 0.0;
   float  temp_val        = 0.0;

   /* Check for NULL pointer */

   if(pointer_is_NULL(accum, "interpolate_grid", "accum"))
      return(NULL_POINTER);

   /* Don't interpolate over a missing period */

   if (accum->supl.missing_period_flg == TRUE)
      return (FUNCTION_FAILED);

   /* Do we have a time span to interpolate over? */

   accum_secs = accum->supl.end_time - accum->supl.begin_time;

   if (accum_secs == 0) /* no time span */
      return (FUNCTION_FAILED);

   /* How long to interpolate? */

   switch (direction)
   {
      case INTERP_FORWARD:  /* interp_time to end_time */
           interp_secs = accum->supl.end_time - interp_time ;
           break;
      case INTERP_BACKWARD: /* begin_time to interp_time */
           interp_secs = interp_time - accum->supl.begin_time;
           break;
      default: /* unknown direction */
           return (FUNCTION_FAILED);
           break;
   }

   /* Is interp_secs is in range? Default max_interp_time: 30 mins */

   if((interp_secs <= 0) ||
      (interp_secs >
       (accum->qpe_adapt.accum_adapt.max_interp_time * SECS_PER_MIN)))
   {
      return (FUNCTION_FAILED);
   }

   /* Interpolation should be less than the entire grid. */

   interp_fraction = ((float) interp_secs) / accum_secs;

   if(interp_fraction >= 1.0)
      return (FUNCTION_FAILED);

   /* Do the interpolation */

   for (i = 0; i < MAX_AZM; ++i)
   {
      for (j = 0; j < MAX_BINS; ++j)
      {
         /* Don't interpolate a QPE_NODATA */

         if (accum->accum_grid[i][j] == QPE_NODATA)
            continue;

         temp_val = (float) accum->accum_grid[i][j] * interp_fraction;

         if (temp_val > SHRT_MAX)
         {
            accum->accum_grid[i][j] = SHRT_MAX;

            if(DP_LIB002_DEBUG)
            {
               fprintf(stderr, "%s temp_val: %f, accum_grid[%d][%d] capped at "
		               "SHRT_MAX (%d)\n",
                               "interpolate_grid:", temp_val, i, j, SHRT_MAX);
            }
         }
         else if (temp_val <= SHRT_MIN)
         {
            /* Add 1 since QPE_NODATA is SHRT_MIN */

            accum->accum_grid[i][j] = SHRT_MIN + 1;

            if (DP_LIB002_DEBUG)
            {
               fprintf(stderr, "%s temp_val: %f, accum_grid[%d][%d] capped at "
		               "SHRT_MIN + 1(%d)\n",
                               "interpolate_grid:",
			       temp_val, i, j, SHRT_MIN + 1);
            }
         }
         else /* a good value */
         {
            accum->accum_grid[i][j] = (short) RPGC_NINT (temp_val);
            if(accum->accum_grid[i][j] == QPE_NODATA) /* accidentally landed */
               accum->accum_grid[i][j] += 1;          /* move up 1 */
         }

      } /* end loop over all bins */

   } /* end loop over all radials */

   /* Adjust accum buffer starting/ending time */

   if ( direction == INTERP_FORWARD )
      accum->supl.begin_time = interp_time;
   else /* direction == INTERP_BACKWARD */
      accum->supl.end_time = interp_time;

   return (FUNCTION_SUCCEEDED);

} /* end interpolate_gird() =================================== */

/******************************************************************************
    Filename: dp_grid_operators.c

    Description:
    ============
      compare_grids() compares two int grids, grid1, grid2.

    Inputs:
       grid1      - first grid
       grid2      - second grid
       grid1_name - name of first grid, for display purposes
       grid2_name - name of second grid, for display purposes
       int        - print differences (TRUE -> print each difference)

    It assumes that grid2 was made after grid1, and thus should be
    increasing (as an accum).

    Outputs: the number of differences
             the differences, printed, if requested

    Returns: FUNCTION_SUCCEEDED (0), FUNCTION_FAILED (1), NULL_POINTER (2)

    Currently uncalled, used for debugging.

    Change History
    ==============
    DATE         VERSION    PROGRAMMERS        NOTES
    ---------    -------    ---------------    ----------------------
    27 Dec 07    0001       James Ward         Initial implementation
******************************************************************************/

int compare_grids (int grid1[][MAX_BINS], int grid2[][MAX_BINS],
                   char* grid1_name, char* grid2_name, int print_differences)
{
   int  i, j; /* indices */
   long num_equal     = 0;
   long num_got_data  = 0;
   long num_lost_data = 0;
   long num_increased = 0;
   long num_decreased = 0;
   long num_counted   = 0;

   if(DP_LIB002_DEBUG == FALSE) /* safety check */
      return(FUNCTION_FAILED);

   /* Check for NULL pointers */

   if(pointer_is_NULL(grid1_name, "compare_grids", "grid1_name"))
      return(NULL_POINTER);

   if(pointer_is_NULL(grid2_name, "compare_grids", "grid2_name"))
      return(NULL_POINTER);

   /* Tote up stats */

   for (i = 0; i < MAX_AZM; i++)
   {
     for (j = 0; j < MAX_BINS; j++)
     {
        if (grid1[i][j] != grid2[i][j])
        {
           if(print_differences)
           {
              fprintf(stderr, "%s[%d][%d] %d != %s[%d][%d] %d\n",
                      grid1_name, i, j, grid1[i][j],
                      grid2_name, i, j, grid2[i][j]);
           }

           if(grid1[i][j] == QPE_NODATA)
              num_got_data++;
           else if(grid2[i][j] == QPE_NODATA)
              num_lost_data++;
           else if(grid1[i][j] < grid2[i][j])
              num_increased++;
           else /* must've decreased */
              num_decreased++;

        } /* end values are different */
        else /* values are the same */
           num_equal++;

     } /* end loop over all bins */

   } /* end loop over all radials */

  num_counted = num_equal + num_got_data + num_lost_data +
                num_increased + num_decreased;

  if(num_counted != MAX_AZM_BINS) /* MAX_AZM_BINS = 360 * 920 */
  {
     fprintf(stderr, "num_counted %ld != MAX_AZM_BINS %d\n",
             num_counted, MAX_AZM_BINS);
  }
  else if(num_equal == MAX_AZM_BINS)
  {
     fprintf(stderr, "%s and %s are identical\n",
             grid1_name, grid2_name);
  }
  else if(num_lost_data > 0)
  {
     fprintf(stderr, "%s lost data going to %s, num_lost_data: %ld\n",
             grid1_name, grid2_name, num_lost_data);
  }
  else if(num_decreased > 0)
  {
     fprintf(stderr, "%s decreased going to %s, num_decreased: %ld\n",
             grid1_name, grid2_name, num_decreased);
  }

  return(FUNCTION_SUCCEEDED);

} /* end compare_grids() ========================================== */

/******************************************************************************
    Filename: dp_grid_operators.c

    Description:
    ============
      print_min_max() prints the minimum and maximum values

    Inputs: int   mini         - i index for minimum value
            int   minj         - j index for minimum value
            int   min_val      - min value in the product
            int   max          - i index for maximum value
            int   maxj         - j index for maximum value
            int   max_val      - max value in the product
            float scale        - scale  used to code product
            float offset       - offset used to code product
            int   product_code - product code

    Outputs: Printed min/max string

    Called by: convert_int_to_256_char(), compute_datalevel_diffprod(),
               convert_precip_4bit()

    Change History
    ==============
    DATE          VERSION    PROGRAMMERS        NOTES
    -----------   -------    ---------------    ----------------------
    06 May 2008     0000     James Ward         Initial version
    06 Oct 2008     0001     James Ward         Removed suggested cvg usage as
                                                cvg 8.7 handles scale/offset.
******************************************************************************/

void print_min_max (int mini, int minj, int min_val,
                    int maxi, int maxj, int max_val,
                    float scale, float offset, int product_code)
{
   char msg[200];  /* 1st stderr message */
   char msg2[200]; /* 2nd stderr message */

   switch (product_code)
   {
      case 169: sprintf(msg, "OHA ");
                break;
      case 170: sprintf(msg, "DAA ");
                break;
      case 171: sprintf(msg, "STA ");
                break;
      case 172: sprintf(msg, "DSA ");
                break;
      case 173: sprintf(msg, "DUA ");
                break;
      case 174: sprintf(msg, "DOD ");
                break;
      default:  sprintf(msg, "DSD "); /* 175 */
                break;
   }

   sprintf(msg2, "MIN[%d][%d] %d %s MAX[%d][%d] %d %s %s %lf %s %lf\n",
                  mini, minj, min_val, "(1000ths in)",
                  maxi, maxj, max_val, "(1000ths in)",
                  "scale",  scale,
                  "offset", offset);

   strcat(msg, msg2);

   RPGC_log_msg(GL_INFO, msg);
   if (DP_LIB002_DEBUG)
      fprintf(stderr, msg);

} /* end print_min_max() ========================================== */

/******************************************************************************
    Filename: dp_grid_operators.c

    Description:
    ============
      pointer_is_NULL() returns TRUE if a pointer is NULL, FALSE otherwise

    Inputs: void* ptr       - pointer to test
            char* func_name - function which called this
            char* ptr_name  - name of pointer

    Outputs: TRUE if ptr is NULL, FALSE otherwise

    Change History
    ==============
    DATE          VERSION    PROGRAMMERS        NOTES
    -----------   -------    ---------------    ----------------------
    15 Jan 2009     0001     James Ward         Initial version
******************************************************************************/

int pointer_is_NULL (void* ptr, char* func_name, char* ptr_name)
{
   char msg[200]; /* stderr message */

   if(ptr == NULL)
   {
      if((func_name != NULL) && (ptr_name != NULL))
      {
         sprintf(msg, "%s: %s is a NULL pointer\n",
                 func_name,
                 ptr_name);

         RPGC_log_msg(GL_INFO, msg);
         if(DP_LIB002_DEBUG)
            fprintf(stderr, msg);
      }

      return(TRUE);
   }

   return(FALSE);

} /* end pointer_is_NULL() ========================================== */

/******************************************************************************
    Filename: dp_grid_operators.c

    Description:
    ============
      bias_is_negative() returns TRUE if a bias is negative, FALSE otherwise

    Inputs: float bias      - bias to test
            char* func_name - function which called this

    Outputs: TRUE if bias is negative, FALSE otherwise

    Change History
    ==============
    DATE          VERSION    PROGRAMMERS        NOTES
    -----------   -------    ---------------    ----------------------
    15 Jan 2009   0001       James Ward         Initial version
******************************************************************************/

int bias_is_negative (float bias, char* func_name)
{
   char msg[200]; /* stderr message */

   if(bias < 0.0)
   {
      if(func_name != NULL)
      {
         sprintf(msg, "%s: bias %f < 0.0\n",
                 func_name,
                 bias);

         RPGC_log_msg(GL_INFO, msg);
         if(DP_LIB002_DEBUG)
            fprintf(stderr, msg);
      }

      return(TRUE);
   }

   return(FALSE);

} /* end bias_is_negative() ========================================== */
