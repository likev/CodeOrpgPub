/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2011/03/08 17:21:26 $
 * $Id: qperate_comp_RateKdp.c,v 1.6 2011/03/08 17:21:26 ccalvert Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */

/*** Local Include Files  ***/

#include "qperate_func_prototypes.h"

/******************************************************************************
    Filename: qperate_comp_RateKdp.c

    Description:
    ============
       compute_RateKdp() calculates the Rainfall Rate R(Kdp) for each sample
    volume based on the specific differential phase Kdp(processed).

       For calculation of R(Kdp), the following applies for each sample volume:

       1. Kdp(processed) equal to QPE_NODATA results in a R(Kdp) equal to
          QPE_NODATA, and
       2. R(Kdp) calculation reflects the same mathematical sign (+/-) as
          Kdp(processed) value for the sample volume.

    NOTE: compute_RateKdp() is based on version 2 of NSSL's dual-Polarimetric
          QPE algorithm coded by John Krause, NSSL, January 2003.

    AEL 3.2.2 E.

    Inputs:
       float       Kdp_processed - Processed specific differential phase value,
                                   in deg/km.
       Rate_Buf_t* rate_out      - rate output buffer

    Return:
       The rainfall Rate R(Kdp), in mm/h.

    Change History
    ==============
    DATE        VERSION    PROGRAMMER         NOTES
    ----        -------    ----------         -----
    12/09/06    0000       Cham Pham          Initial implementation for
                                              dual-polarization project
                                              (ORPG Build 11).
    27 Nov 07   0001       Stein/Pham         Removed correlation coeff,
                                              updating this computation for
				              Version 2 of the QPE algorithm.
    08/09/2010  0002       James Ward         Moved Kdp rate checks inside
                                              function, so they are always done.
    12/21/2010  0003       James Ward         Removed checks if attenuated
******************************************************************************/

/* ISIGN is a macro for finding the sign of a variable */

#define ISIGN(a) ( (a) < 0.0 ? -1 : 1 )

float compute_RateKdp (Moments_t* bin_moments, float rate_z,
                       Rate_Buf_t* rate_out)
{
   float kdp_rate = 0.0;

   /* Compute rainfall rate R(Kdp) for each sample volume based on the
    * specific Differential Phase Kdp(processed) value. ISIGN can make
    * RateKdp negative.
    *
    * Non-attenuated Kdp checks are:
    *
    * Rate(Kdp) is not valid for CC < 0.90.
    * Rate(Kdp) is only reliable in "heavy" precip > 10 mm/hr.
    *
    * Note: rate_out is the rate output buffer structure, and a pointer
    * to it is passed as convenient way to get 4 parameters:
    *
    * dpprep_adapt.corr_thresh
    * dp_adapt.Kdp_min_usage_rate
    * dp_adapt.Kdp_mult
    * dp_adapt.Kdp_power */

   if(bin_moments->Kdp == QPE_NODATA) /* no Kdp to compute on */
      return(rate_z);

   if(bin_moments->attenuated == FALSE)
   {
      if(bin_moments->CorrelCoef < rate_out->qpe_adapt.dpprep_adapt.corr_thresh)
         return(rate_z);

      if(rate_z < rate_out->qpe_adapt.dp_adapt.Kdp_min_usage_rate)
         return(rate_z);
   }

   /* Default  Kdp_mult: 44.0
    * Default Kdp_power:  0.822 */

   kdp_rate = rate_out->qpe_adapt.dp_adapt.Kdp_mult *
              powf(fabsf(bin_moments->Kdp),
              rate_out->qpe_adapt.dp_adapt.Kdp_power) *
              ISIGN (bin_moments->Kdp);

   if(kdp_rate < 0.0)
   {
      return(rate_z);
   }

   return(kdp_rate);

} /* end compute_RateKdp() ================================== */
