/*
 * RCS info
 * $Author: christie $
 * $Locker:  $
 * $Date: 2003/07/17 15:08:10 $
 * $Id: mpda_range_unf_data.h,v 1.2 2003/07/17 15:08:10 christie Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

/*******************************************************************

   range_unf_data.h

   PURPOSE:

   This include file contains the structure that holds the
   reflectivity data from the base scan for range dealiasing
   the velocity data.

   DEFINITIONS:

   struct sur_data

   float  dBZ           - lookup table for converting ref in UIF to float
   float  log10_rng     - lookup table for performing range power adjustment
   int    num_gates     - array containing the number of gates per radial
   int    tot_ref_rads  - total number of radials of sur scan
   float  pwr           - array containing power values of each gate (db)
   int    az            - array of azimuth values for each radial (deg*100)
   int    sv_dp_intrvl  - sur scan gate spacing (m) 

********************************************************************/

struct sur_data
  {
  float         pwr[MAX_RADS][MAX_REF_GATES];
  float         log10_rng[MAX_REF_GATES];
  int           num_gates[MAX_RADS];
  int           az[MAX_RADS];
  float         dBZ[MAX_NUM_UIF];
  int           tot_ref_rads;
  int           sv_dp_intrvl;
  }base_scan;

float      atm_atten_fac;   /* attenuation factor for range dealiasing  */


