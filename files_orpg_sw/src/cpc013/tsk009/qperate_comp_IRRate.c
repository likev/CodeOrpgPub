/*
 * RCS info
 * $Author: dberkowitz $
 * $Locker:  $
 * $Date: 2014/09/02 20:02:10 $
 * $Id: qperate_comp_IRRate.c,v 1.11 2014/09/02 20:02:10 dberkowitz Exp $
 * $Revision: 1.11 $
 * $State: Exp $
 */

/*** Local Include Files  ***/

#include "qperate_func_prototypes.h"

/******************************************************************************
    Filename: qperate_comp_IRRate.c

    Description:
    ============
       compute_IRRate() computes instantaneous rainfall rate.

    NOTE: It is based on version 2 of NSSL's dual-Polarimetric QPE algorithm
          coded by John Krause, NSSL, January 2003.

    Inputs:
       int         azm                      - radial number
       int         rng                      - range index
       int         blocked_percent          - percent the beam is blocked
       Rate_Buf_t* rate_out                 - rate output buffer
       Moments_t*  bin_moments              - the moments for this bin
       short       beam_edge_top[MAX_AZM]   - top edge of the melting layer
       float*      RateZ_table              - table of R(Z) rates

    Return:
       The Instantaneous Rainfall Rate, R(combined), in mm/hr.

    Change History
    ==============
    DATE        VERSION   PROGRAMMER  NOTES
    ----        -------   ----------  -----
    01/20/07    0000      C. Pham     Initial implementation for
                          D.Stein     dual-polarization project
                                      (ORPG Build 11).

    08/13/07    0001      Jihong Liu  HCA categories determine
                                      the correct selection of the rate
                                      calculation.  So some logic are
                                      added to demonstrate those change.
                                      And those methods are based on
                                      the QPE V2

    14 Nov 08   0002      Dan Stein   Added a check for Kdp != NODATA.
 	                              We just learned that the
 	                              preprocessor will set PhiDP to
 	                              NODATA ifRhoHV is less than 0.90.
 	                              If that's the case, we don't want
 	                              to try to compute R(Kdp).

    20090331    0003     James Ward   Added RateZ_table.

    20100910    0004     James Ward   Made changes for partial beam
                                      blockage method 2.

    20101221    0005     James Ward   Added attenuation checks before
                                      hydroclass processing and fail
                                      safe after hydroclass processing.

    20111031    0006     James Ward   For CCR NA11-00372:
                                      PBB method 1 not to be used.
                                      Replaced MIN_BLOCKED_PERCENT_TO_USE_KDP
                                      (20) with Kdp_min_beam_blk
                                      Replaced MAX_BLOCKED_PERCENT_TO_USE_KDP
                                      (70) with Kdp_max_beam_blk
                                      Added Min_blockage
                                      Replaced RhoHV_min_rate with art_corr

    RhoHV_min_rate and art_corr were matched at CCR NA11-00290, John Krause
    suggests they should always be equal, so now they are.

    20131205    0007    Dan Berkowitz Replaced BEAM_CENTER_TOP with 
                                      BEAM_EDGE_TOP.

    20140812    0008    Berkowitz     CCR NA13-00260 and 00261;
                        Murnan        Make Heavy Rain Class above Z
                                      Threshold use R(Kdp).  Also, added
                                      multiplier for dry snow below top of
                                      melting layer.  

******************************************************************************/

float compute_IRRate(int azm, int rng, int blocked_percent,
                     Rate_Buf_t* rate_out, Moments_t* bin_moments,
                     short beam_edge_top[MAX_AZM],
                     float* RateZ_table)
{
   float fshield = 0.0;
   int   z_index = 0;
   short hc_type = 0;

   /* The AEL calls precip_rate "R(Combined)". */

   float precip_rate = QPE_NODATA;
   float Rate_Z      = QPE_NODATA;

   /* Copy hydro class data to local variable hc_type */

   hc_type = bin_moments->HydroClass;

   /* If the hydrometeor class is Biota or No Echo (AEL 3.1.2.1),
    * this bin is valid, so mark it filled, and don't go to a higher
    * elevation. Return 0.0 = no precipitation, for this bin.
    *
    * Note: Check RhoHV after BI/NE, because the HCA has already considered
    *       RhoHV in its fuzzy logic classification to determine the
    *       hydrometeor type. The RhoHV check should only be applied to
    *       a precip. generating hydrometeors (not GC/UK/BI/NE).
    *       Ward 24 Mar 08.
    *
    * 20081211 Ward - Mark Fresch suggested changing No Echo (NE)" to
    * "No Data (ND)" to match the HCA AEL. Brian Klein pointed out that
    * ND means no data above the threshold, as there are always some echoes.
    * Mike Istok pointed out that AWIPS uses ND, and ND has meant No Data for
    * over 20 years. It was agreed to go with what AWIPS displays (ND)
    * -> hca.h NE may change to ND in the future. */

   if((hc_type == BI) || (hc_type == NE))
   {
       return(0.0); /* AEL 3.1.2.1 */
   }

   /* The rest of the hydrometeors are precip. generating,
    * so for non-attenuated radials, check RhoHV next.
    * Attenuated radials don't do any check.
    *
    * The QPE AEL calls art_corr - "THRESHOLD(Minimum RhoHV for Precip)"
    *
    * Default art_corr: 0.80 */

   if((bin_moments->attenuated == FALSE) &&
      (bin_moments->CorrelCoef < rate_out->qpe_adapt.dpprep_adapt.art_corr))
   {
      /* go to the next elevation */
      return(QPE_NODATA); /* AEL 3.1.2.1 */
   }

   /* If beam is partially blocked - AEL 3.2.2 A
    *
    * In John Krause's RadialRainfallRates.cpp, the PBB2 correction
    * for HR/RA/RH is done after the hydroclass switch statement.
    * This would cause HR/RA/RH to have a rate computed twice, so
    * we augment Z before looking at hydroclasses.
    *
    * A 'blocked_percent >= Min_blockage' check must be applied instead
    * of always augmenting Z because even at 0 % blockage the
    * fshield formula adds a correction:
    *
    * (0.5*tanh(0.0277*(50.0-0.0)))+ 0.5 = 0.94
    *
    * z_unblocked = (1 / fshield) * z_blocked = 1.06 * z_blocked */

   if(blocked_percent >= rate_out->qpe_adapt.dp_adapt.Min_blockage /* 5 */)
   {
      /* 20111031 Ward PBB1 method not to be used.
       * if(rate_out->qpe_adapt.dp_adapt.Use_pbb1 == TRUE)
       * {
       *    return(compute_Precip_PBB_method1(azm, rng, blocked_percent,
       *           rate_out, bin_moments, stats));
       * }
       */

      if(blocked_percent >= rate_out->qpe_adapt.dp_adapt.Kdp_max_beam_blk /* 70 */)
      {
         /* Go to the next elevation. This should already have been checked
          * in the calling routine, Add_bin_to_RR_Polar_Grid(). */

         return(QPE_NODATA);
      }
      else if(bin_moments->Z == QPE_NODATA) /* we can't correct Z */
      {
         return(QPE_NODATA);
      }
      else /* apply the fshield correction to dBZ */
      {
         fshield = (0.5*tanhf(0.0277*(50.0-blocked_percent)))+ 0.5;

         /* Fshield is a number between 0.59 (100% blocked) and 0.94 (0% blocked).
          *
          * z_blocked = z_unblocked * fshield
          *
          * dbz_blocked = 10 * log10(z_unblocked * fshield)
          *
          * dbz_blocked = 10 * (log10(z_unblocked) + log10(fshield))
          *
          * dbz_blocked = 10 * log10(z_unblocked) + 10 * log10(fshield)
          *
          * dbz_blocked = dbz_unblocked + 10 * log10(fshield)
          *
          * dbz_blocked - 10 * log10(fshield) = dbz_unblocked
          *
          * Augment Z in place, we don't need a Zprime. */

         bin_moments->Z -= (10.0 * log10f(fshield));
      }

   } /* end if blocked_percent >= Min_blockage */

   /* Most hydroclasses need a base R(Z) */

   if(bin_moments->Z == QPE_NODATA)
   {
      Rate_Z = QPE_NODATA;
   }
   else if(bin_moments->Z <= rate_out->qpe_adapt.dp_adapt.Refl_min)
   {
      /* Use the minimum in the rate table */

      Rate_Z = RateZ_table[0];
   }
   else if(bin_moments->Z >= rate_out->qpe_adapt.dp_adapt.Refl_max)
   {
      /* Use the maximum in the rate table
       *
       * index   0   1     2 ... 64 ... 168  169  170
       *
       *  refl -32 -31.5 -31 ...  0 ...  52  52.5  53
       *
       * Note: Refl_max and Refl_min are floats with an accuracy
       *       of 0.1 so we typecast to get an int index. */

      z_index = (int) 2 * (rate_out->qpe_adapt.dp_adapt.Refl_max -
                           rate_out->qpe_adapt.dp_adapt.Refl_min);

      Rate_Z = RateZ_table[z_index];
   }
   else /* Refl_min <= bin_moments->Z <= Refl_max */
   {
      if(blocked_percent >= rate_out->qpe_adapt.dp_adapt.Min_blockage /* 5 */)
      {
         /* dBZ has been augmented by fshield, so is not on a
          * 0.5 dBZ boundary, so we must compute R(Z) */

         Rate_Z = compute_RateZ(bin_moments->Z, rate_out);
      }
      else /* dBZ is on a 0.5 boundary, so we can use a rate table lookup */
      {
         /* Note: Refl_min is a float with an accuracy
          *       of 0.1, so we typecast to get an int index. */

         z_index = (int) 2 * (bin_moments->Z -
                              rate_out->qpe_adapt.dp_adapt.Refl_min);

         Rate_Z = RateZ_table[z_index];
      }
   }

   /* Calculate a hydroclass based rate. */

   switch(hc_type)
   {
      /* BD and RA always use R(Z, Zdr) */

      case BD:
      case RA:
         precip_rate = compute_RateZ_Zdr(bin_moments->Z, bin_moments->Zdr,
                                         rate_out);
         break;

      /* GR, IC, and WS are multiples of Rate_Z.
       *
       * 20080117 Ryzhkov says the IC rate should be 2.8 * R(z) for
       * all instances of Crystals, independent of the melting layer */

      case GR:
         precip_rate = Rate_Z * rate_out->qpe_adapt.dp_adapt.Gr_mult; /* 0.8 */
         break;
      case IC:
         precip_rate = Rate_Z * rate_out->qpe_adapt.dp_adapt.Ic_mult; /* 2.8 */
         break;
      case WS:
         precip_rate = Rate_Z * rate_out->qpe_adapt.dp_adapt.Ws_mult; /* 0.6 */
         break;

      /* The HR formula changes if you're blocked or not. */

      case HR:
         if((blocked_percent >= rate_out->qpe_adapt.dp_adapt.Kdp_min_beam_blk) || /* 20 */
            (bin_moments->Z > rate_out->qpe_adapt.dp_adapt.Hr_HighZThresh)) /* 45 */
         {
            precip_rate = compute_RateKdp(bin_moments, Rate_Z, rate_out);
         }
         else /* use R(Z, Zdr) */
         {
            precip_rate = compute_RateZ_Zdr(bin_moments->Z, bin_moments->Zdr,
                                            rate_out);
         }
         break;

      /* DS and RH require a melting layer, except for RH blocked */

      case DS:
        /* DS is more likely to be above the beam_edge_top.
         *
         * The AEL has the same multiplicative coefficient for DS
         * and IC, but we broke them out separately here. */

        if(beam_edge_top[azm] == QPE_NODATA)
        {
            precip_rate = QPE_NODATA;
        }
        else if(rng > beam_edge_top[azm])
        {
            precip_rate = Rate_Z * rate_out->qpe_adapt.dp_adapt.Ds_mult; /* 2.8 */
        }
        else /* rng <= beam_edge_top[azm] */
        {
            precip_rate = Rate_Z * rate_out->qpe_adapt.dp_adapt.Ds_BelowMLTop_mult;
        }
        break;

      case RH:
        /* RH is more likely to be below the beam_edge_top */

        if(blocked_percent >= rate_out->qpe_adapt.dp_adapt.Min_blockage /* 5 */)
        {
            precip_rate = compute_RateKdp(bin_moments, Rate_Z, rate_out);
        }
        else if(beam_edge_top[azm] == QPE_NODATA)
        {
            precip_rate = QPE_NODATA;
        }
        else if(rng <= beam_edge_top[azm])
        {
            precip_rate = compute_RateKdp(bin_moments, Rate_Z, rate_out);
        }
        else /* rng > beam_edge_top[azm] */
        {
            precip_rate = Rate_Z * rate_out->qpe_adapt.dp_adapt.Rh_mult; /* 0.8 */
        }
        break;

      default: /* unknown hydroclass - this should never happen */
        return(QPE_NODATA);
        break;

   } /* end switch on hydroclass */

   /* Fail-safe for an attenuated radial. AEL 3.1.2.4 */

   if(bin_moments->attenuated == TRUE)
   {
      if(precip_rate == QPE_NODATA) /* 1. Try R(Z, Zdr) */
      {
         precip_rate = compute_RateZ_Zdr(bin_moments->Z, bin_moments->Zdr,
                                         rate_out);
         if(precip_rate == QPE_NODATA) /* 2. Try R(Z) */
         {
            precip_rate = compute_RateZ(bin_moments->Z, rate_out);

            if(precip_rate == QPE_NODATA) /* 3. Use R(Kdp) */
            {
               precip_rate = compute_RateKdp(bin_moments, Rate_Z, rate_out);
            }
         }
     }

   } /* end attenuated fail safe */

   return(precip_rate);

} /* end compute_IRRate() ================================== */
