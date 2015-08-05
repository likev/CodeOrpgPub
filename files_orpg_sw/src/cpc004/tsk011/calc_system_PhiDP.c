/* RCS info */
/* $Author: steves $ */
/* $Locker:  $ */
/* $Date: 2014/11/10 15:05:43 $ */
/* $Id: calc_system_PhiDP.c,v 1.3 2014/11/10 15:05:43 steves Exp $ */
/* $Revision:  */
/* $State: */

/* ===Project: DualPol ===== Author: John M. Krause =====Feb 2010 =============
 *
 * Module Name: calc_system_PhiDP.c
 *
 * Module Version: 3.0
 *
 * Module Language: c
 *
 * Change History:
 *
 * Date    Version    Programmer  Notes
 * -------------------------------------------------------------
 * 03/26/10  1.0    John Krause  Creation
 * 04/16/12  2.0    Brian Klein  RPG prototype implementation
 * 06/06/13  3.0    Brian Klein  RPG Build 15 modifications
 *
 * Calling Sequence:
 *
 *
 * Description:
 *
 * (c) Copyright 2012, Board of Regents of the University of Oklahoma.
 * All rights reserved. Not to be provided or used in any format without
 * the express written permission of the University of Oklahoma.
 * Software provided as is no warranties expressed or implied.
 * ======================================================================== */

#include <rpgc.h> /* RPGC_NINT() */
#include <dpprep.h>

/* Macros */
#define MIN_ECHO_SIZE     11 /* should be odd */
#define MAX_QUE_SIZE     200
#define THRESH_CROSSOVER 200 /* threshold for determining if the sorted que *
                              * crossed 360 degrees                         */
#define THRESH_ADJUST    270 /* threshold for determining if the que value  *
                              * should be adjusted to to eliminate the 360  *
                              * degree crossover                            */

/* Due to data conversion, the min/max phis are not 0/360: *
 *                                                         *
 * word_size: 16                                           *
 *     scale: 0.016480                                     *
 *    offset: -2.000427                                    *
 *                                                         *
 *        out[54] = (p[54] + offset   ) * scale            *
 *      -0.000007 = (2     + -2.000427) * 0.016480         *
 *                                                         *
 * word_size: 16                                           *
 *     scale: 0.352597                                     *
 *    offset: -2.000000                                    *
 *                                                         *
 *        out[30] = (p[30] + offset   ) * scale            *
 *     360.001373 = (1023  + -2.000000) * 0.352597         */

#define MAX_PHI 360.0014   /* max seen 360.001373 */
#define MIN_PHI  -0.000008 /* min seen  -0.000007 */

/* Global Variables */
extern float Current_system_phidp;
extern short Init_index; /* 0 -> init the queue index */
extern short Too_close_count;

/* Infrastructure */
static unsigned int max_que_count = -1; /* how many radials have been processed? */
static unsigned int que_index;          /* which slot in the que are we at?      */
static float PhiDP_que[MAX_QUE_SIZE];   /* the que which is not a que            */

/* Function Prototypes */
static void check_phi(float* phi);
static unsigned int check_index(unsigned int c_index);
static int float_compare(const void * a, const void * b);
static void qsort_360(float phi_array[], int num);
static float compute_system_PhiDP_single_radial(const int    num_bins, const float  no_data,
                                                const float* RhoHV,    const float* PhiDP,
                                                const float* Z);


/******************************************************************************
 *    Function name: check_index()
 *
 *    Description:
 *    ============
 *       check_index() ensures 0 <= c_index < MAX_QUE_SIZE (200).
 *       This keeps the queue from over-running.
 *
 *    Inputs:
 *       unsigned int c_index - the queue index
 *
 *    Return:
 *       The queue_index, updated
 *
 *    Change History
 *    ==============
 *     DATE        VERSION   PROGRAMMER   NOTES
 *    ----------   -------   -----------  ----------------
 *    2012/03/26    00000    John Krause  Creation
 ******************************************************************************/

static unsigned int check_index(unsigned int c_index)
{
  if(Init_index != 1)
  {
     c_index       = 0;
     max_que_count = 0;
     Init_index    = 1;
     Current_system_phidp = -99;
  }

  if(c_index >= MAX_QUE_SIZE)
  {
     c_index = 0;
  }

  if(c_index < 0)
  {
     c_index = 0;
  }

  return c_index;

} /* end check_index() */

/******************************************************************************
 *    Function name: float_compare()
 *
 *    Description:
 *    ============
 *       float_compare() is the float comparison function used by qsort()
 *
 *    Inputs:
 *       const void*  a - the first  float
 *       const void*  b - the second float
 *
 *    Return:
 *       1 if a > b
 *       0 if a = b
 *      -1 if a < b
 *
 *    Change History
 *    ==============
 *     DATE        VERSION   PROGRAMMER   NOTES
 *    ----------   -------   -----------  ----------------
 *    2012/03/26    00000    John Krause  Creation
 ******************************************************************************/

static int float_compare(const void * a, const void * b)
{
  if (*(float*) a > *(float*) b)
  {
    return 1;
  }

  if(*(float*) a < *(float*) b)
  {
    return -1;
  }

  return 0;

} /* end float_compare() */

/******************************************************************************
 *    Function name: check_phi()
 *
 *    Description:
 *    ============
 *       check_phi() ensures that MIN_PHI < phi < MAX_PHI
 *
 *    Inputs:
 *       float*  phi - the phi value
 *
 *    Return:
 *       None.
 *
 *    Change History
 *    ==============
 *     DATE        VERSION   PROGRAMMER   NOTES
 *    ----------   -------   -----------  ----------------
 *    2012/03/26    00000    James Ward   Creation
 ******************************************************************************/

static void check_phi(float* phi)
{
   if(*phi < 0.0)
      *phi += 360.0;

   if(*phi >= 360.0)
      *phi -= 360.0;

   if((*phi <= MIN_PHI) || (*phi >= MAX_PHI))
   {
      /* This should not happen */

      fprintf(stdout, "ERROR - check_phi %f\n", *phi);

      RPGC_log_msg(GL_INFO, "ERROR - check_phi %f", *phi);
   }

} /* end check_phi() */

/******************************************************************************
 *    Function name: qsort_360()
 *
 *    Description:
 *    ============
 *       qsort_360() does a qsort() on a phi array adjusted for the 360 degree
 *       boundary. We want the median of {1, 358, 359} = {358, 359, 361}
 *       to be 359 instead of 358.
 *
 *    Inputs:
 *       float phi_array[] - the phi array to be qsorted
 *       int   num         - the size of the array
 *
 *    Return:
 *       phi_array[], qsorted.
 *
 *    Change History
 *    ==============
 *     DATE        VERSION   PROGRAMMER   NOTES
 *    ----------   -------   -----------  ----------------
 *    2012/03/26    00000    John Krause  Creation
 *                           Brian Klein
 ******************************************************************************/

static void qsort_360(float phi_array[], int num)
{
   int i;

   /* Do the normal qsort() */

   qsort(phi_array, num, sizeof(float), float_compare);

   /* See if the sorted data crosses the 360 degree boundary */

   if((phi_array[num-1] - phi_array[0]) > THRESH_CROSSOVER)
   {
      /* The sorted data crosses 360 degrees. Eliminate the crossing */

      for(i=0; i<num; i++)
      {
         if(phi_array[i] < THRESH_ADJUST)
            phi_array[i] += 360.0;
      }

      /* Sort again now that we've eliminated the 360 degree crossover */

      qsort(phi_array, num, sizeof(float), float_compare);
   }

} /* end qsort_360() */

/******************************************************************************
 *    Function name: compute_system_PhiDP_single_radial()
 *
 *    Description:
 *    ============
 *       compute_system_PhiDP_single_radial() computes a radial phi based
 *       upon a single radial. It is called by calc_system_PhiDP() on a
 *       radial by radial basis.
 *
 *    Inputs:
 *       const int    num_bins - number of bins in the radial
 *       const float  no_data  - the no data value
 *       const float* RhoHV    - RhoHV for the radial
 *       const float* PhiDP    - PhiDP for the radial
 *       const float* Z        - Z     for the radial
 *       const DPP_d_t rda_value - RDA's value for initial System PhiDP
 *
 *    Return:
 *       the radial phi
 *
 *    Change History
 *    ==============
 *     DATE        VERSION   PROGRAMMER   NOTES
 *    ----------   -------   -----------  ----------------
 *    2012/03/26    00000    John Krause  Creation
 *    2012/04/16    00001    Brian Klein  Added check for nearby weather and
 *                                        tuned for bio elimination and echo
 *                                        too close to radar.
 ******************************************************************************/

static float compute_system_PhiDP_single_radial(const int    num_bins, const float  no_data,
                                                const float* RhoHV,    const float* PhiDP,
                                                const float* Z)
{
  float Z_reject  = 40.0;
  float Z_min     = 0.0;     /* added to eliminate very weak echo */
  float min_rho   = 0.986;   /* (was .95) min Rho if echo is weather */
  int   count     = 0;
  int   too_close = 100;     /* weather is too close to radar (25 km) */

  float single_radial_PhiDP_que[MIN_ECHO_SIZE];
  float radial_phi = 0.0;
  int   i, j;

  for(i=0; i<num_bins; i++)
  {
    if((PhiDP[i] != no_data) && (RhoHV[i] >= min_rho) && Z[i] >= Z_min)
    {
       /* It is a weather echo */

       single_radial_PhiDP_que[count] = PhiDP[i];

       count++;

       if(count == MIN_ECHO_SIZE) /* got 11 good bins in a row */
       {
          /* Check the range of the bin closest to the radar *
           *                                                 *
           * The 11 bins range from (i - MIN_ECHO_SIZE + 1)  *
           * to i.                                           */

          if((i - MIN_ECHO_SIZE + 1) < too_close)
          {
             /* The weather is too close to the radar, we can't get
              * an accurate radial phi */
             Too_close_count++;
             return(no_data); /* DPP_NO_DATA */
          }

          /* Consider rejecting data if Z or Rho are odd. */

          for(j=0; j<MIN_ECHO_SIZE; j++)
          {
              if((Z[i-j] >= Z_reject) || (RhoHV[i-j] > 1.0))
              {
                 /* These conditions suggest that the whole radial is problematic */
                 return(no_data); /* DPP_NO_DATA */
              }
          }

          /* Sort and return the median value */

          qsort_360(single_radial_PhiDP_que, MIN_ECHO_SIZE);

          radial_phi = single_radial_PhiDP_que[MIN_ECHO_SIZE / 2];

          check_phi(&radial_phi);

          return(radial_phi);
       }
    }
    else /* restart the count */
    {
        count = 0;
    }

  } /* end loop over all bins */

  /* If we got here, we couldn't find 11 good bins in a row */

  return(no_data); /* DPP_NO_DATA */

} /* end compute_system_PhiDP_single_radial() */

/******************************************************************************
 *    Function name: calc_system_PhiDP()
 *
 *    Description:
 *    ============
 *       calc_system_PhiDP() computes a system phi based upon a queue of radial
 *       phis. It is called on a radial by radial basis.
 *
 *    Inputs:
 *       const int    num_bins - number of bins in the radial
 *       const float  no_data  - the no data value
 *       const float* RhoHV    - RhoHV for the radial
 *       const float* PhiDP    - PhiDP for the radial
 *       const float* Z        - Z     for the radial
 *
 *    Return:
 *       the system phi
 *
 *    Change History
 *    ==============
 *     DATE        VERSION   PROGRAMMER   NOTES
 *    ----------   -------   -----------  ----------------
 *    2012/03/26    00000    John Krause  Creation
 *    2013/09/12    00001    Brian Klein  Tuning for bio elimination and
 *                                        use of RPG indexed elevation numbers.
 *    2014/10/07    00002    Brian Klein  Additional tuning for bio elimination
 *                                        under CCR NA14-00325
 ******************************************************************************/

float DPPC_calc_system_PhiDP(const int    num_bins, const float  no_data,
                             const int    elev_num, const float* RhoHV,
                             const float* PhiDP,    const float* Z)
{

   int max_elev_num        = 4;   /* Was 6. Lowered to account for use of RPG indexing */
   int min_num_samples     = 40; /* Was 20. Raised due to double counting split cuts */
   float radial_system_Phi = 0.0;;

   float copy_PhiDP_que[MAX_QUE_SIZE]; /* the que which is not a que */

   unsigned int que_limit;
   unsigned int return_index;

   int i;

   que_index = check_index(que_index);

   /* Check to see that we are working on an elevation that we understand
    * to have actual weather data. Running all the time is pure CPU waste */

   if(elev_num >= max_elev_num)
   {
      /* Just return the last value of min_system_Phi rather than recalc */

      return(Current_system_phidp);
   }

   radial_system_Phi = compute_system_PhiDP_single_radial(num_bins, no_data, RhoHV, PhiDP, Z);

   if(radial_system_Phi <= no_data)
   {
      /* Ignore this radial */
   }
   else if((radial_system_Phi < 0.0) || (radial_system_Phi >= 360.0))
   {
      /* This should not happen */

      fprintf(stdout, "radial_system_Phi %f - ERROR\n", radial_system_Phi);
   }
   else /* valid radial_system_Phi */
   {
      /* radial_system_Phi for 1 radial is valid */

      que_index            = check_index(que_index);
      PhiDP_que[que_index] = radial_system_Phi;
      que_index++;
      if(que_index > max_que_count)
      {
        max_que_count = que_index;
      }
   }

   /* We need at least min_num_samples radials to find min_system_PhiDP */

   if(max_que_count < min_num_samples)
   {
      /* not enough data to know */

      return(no_data);
   }

   if(max_que_count >= MAX_QUE_SIZE)
   {
      que_limit = MAX_QUE_SIZE;
   }
   else
   {
      que_limit = max_que_count;
   }

   /* Copy the array of data */

   for(i=0; i<que_limit; i++)
   {
       copy_PhiDP_que[i] = PhiDP_que[i];
   }

   /* Work on the copy. Sort the data for median determination below */

   qsort_360(copy_PhiDP_que, que_limit);

   /* NSSL used the lower 25% of the median value (divided que_limit by 4)  */
   /* with the orginal RhoHV threshold of 0.95.  When RhoHV threshold was   */
   /* raised to help eliminate biota contamination, it was necessary to     */
   /* lower the median value to 5% (divide que_limit by 20) to compensate   */
   /* assuming the selected bins using a higher RhoHV are slighty farther   */
   /* into the precipitation than with the lower RhoHV threshold.           */

   return_index = RPGC_NINT(que_limit/20.);

   /* Paranoia */

   return_index = check_index(return_index);

   Current_system_phidp = copy_PhiDP_que[return_index];

   check_phi(&Current_system_phidp);

   return(Current_system_phidp);

} /* end DPPC_calc_system_PhiDP() */
