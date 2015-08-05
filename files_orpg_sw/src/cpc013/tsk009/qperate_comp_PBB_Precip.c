/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/03/12 13:43:24 $
 * $Id: qperate_comp_PBB_Precip.c,v 1.8 2012/03/12 13:43:24 ccalvert Exp $
 * $Revision: 1.8 $
 * $State: Exp $
 */

/*** Local Include Files  ***/

/*
#include "qperate_func_prototypes.h"
*/

/******************************************************************************
    Function name: compute_Precip_PBB_method1()

    Description:
    ============
       It computes instantaneous rainfall rate in the presence of PBB using
       Method 1, which was agreed upon during a meeting with Dr. Ryzkhov on
       6 Nov 06, where we discussed and agreed upon an algorithm and a set
       of rules to compute precip in the presence of partial beam blockage.
       The summary of this meeting is in a MS Word document titled
       "Dr. Ryzkhov Discussion Summary.doc" with key words: Dr. Ryzkhov
       discussion nssl roc dual polarization partial beam blockage and
       Kdp usage r(Kdp).

    NOTE: It is based on version 2 of NSSL's dual-Polarimetric QPE algorithm
          coded by John Krause, NSSL, January 2003.

    Inputs:
       int         azm             - radial number
       int         rng             - range index
       int         blocked_percent - percentage of beam blocked
       Rate_Buf_t* rate_out        - rate output buffer
       Moments_t*  bin_moments     - the moments for this bin

    Return:
       The Instantaneous Rainfall Rate, R(combined), in mm/hr.

    Change History
    ==============
    DATE        VERSION   PROGRAMMER          NOTES
    ----        -------   ----------          -----
    01/20/07    0000      C. Pham & D. Stein  Initial implementation for
                                              dual-polarization project
                                              (ORPG Build 11).
    08/13/07    0001      Jihong Liu          HCA categories determine
                                              the correct selection of the rate
                                              calculation.  So some logic are
                                              added to demonstrate those change.
                                              And those methods are based on
                                              the QPE V2
    11/05/08    0002      Zhan Zhang          Wrap the previously implemented
                                              IRRate calculation in the
                                              presence of PBB into a function.
    03/10/10    0003      James Ward          Replaced RhoHV_min_Kdp_rate with
                                              corr_thresh.
    10/31/11    0004     James Ward           For CCR NA11-00372:
                                              PBB method 1 not to be used.
******************************************************************************/

/* float compute_Precip_PBB_method1 (int azm, int rng, int blocked_percent,
 *                                   Rate_Buf_t* rate_out, Moments_t* bin_moments,
 *                                   Stats_t* stats)
 * {
 *    float zprime      = 0.0;
 *    float rate_zprime = 0.0;
 *    float rate_Kdp    = 0.0;
 *    short hydro_class = 0;
 *    float precipRate  = QPE_NODATA;
 *
 *    -* Ward - To force QPE_NODATA for testing, uncomment this:
 *     *
 *     *  if(azm == 300)
 *     *    return (QPE_NODATA);                                 *-
 *
 *    -* Copy hydro class data to local variable hydro_class. *-
 *
 *    hydro_class = bin_moments->HydroClass;
 *
 *    -* If function is mis-called *-
 *
 *    if (blocked_percent == 0)
 *    {
 *       RPGC_log_msg(GL_INFO, "blockage equals zero,function is miscalled\n");
 *       return (QPE_NODATA);
 *    }
 *    else -* if beam is partially blocked *-
 *    {
 *       #ifdef QPERATE_DEBUG
 *       {
 *          fprintf (stderr, "azm: %d; rng: %d; blk: %d\n",
 *                   azm, rng, blocked_percent);
 *       }
 *       #endif
 *
 *       -* If blockage percentage is greater than the maximum usability
 *        * blockage, then skip to the next sample volume. Also, if Z
 *        * contains QPE_NODATA, then we can't use it; return QPE_NODATA
 *        * and skip this sample bin
 *        *
 *        * Default Z_max_beam_blk: 50 % *-
 *
 *       if ((blocked_percent > rate_out->qpe_adapt.dp_adapt.Z_max_beam_blk) ||
 *           (bin_moments->Z == QPE_NODATA))
 *       {
 *          precipRate = QPE_NODATA;
 *
 *          return (precipRate);
 *       }
 *
 *       -* The blockage is greater than zero, we add the missing power
 *        * back to Z(processed) to create an updated value called Z' (Z prime).
 *        *
 *        * The PPS blockage formula:
 *        *
 *        * z_unblocked = z_blocked * 100.0 / (100.0 - blocked_percent)
 *        *
 *        * z_unblocked = z_blocked * 1.0 / (1.0 - (blocked_percent * 0.01))
 *        *
 *        * dbz_unblocked = 10.0 * log10(z_blocked * 1.0 / (1.0 - (blocked_percent * 0.01)))
 *        *
 *        * dbz_unblocked = 10.0 * (log10(z_blocked) + log10(1.0 / (1.0 - (blocked_percent * 0.01))))
 *        *
 *        * dbz_unblocked = 10.0 * (log10(z_blocked)) + 10.0 * log10(1.0 / (1.0 - (blocked_percent * 0.01)))
 *        *
 *        * dbz_unblocked = dbz_blocked + 10.0 * log10(1.0 / (1.0 - (blocked_percent * 0.01)))
 *        *
 *        * = old: zprime = bin_moments->Z + (10.0 * log10(1.0 / (1.0 - (blocked_percent * 0.01))));
 *        *
 *        * This is the PPS formula. You can get rid of a multiplication and a division by:
 *        *
 *        * dbz_unblocked = dbz_blocked + (10.0 * log10(100.0 / (100.0 - (blocked_percent * 0.01 * 100.0))))
 *        *
 *        * dbz_unblocked = dbz_blocked + (10.0 * log10(100.0 / (100.0 - blocked_percent)))
 *        *
 *        * dbz_unblocked = dbz_blocked + (10.0 * (log10(100.0) - log10(100.0 - blocked_percent))
 *        *
 *        * dbz_unblocked = dbz_blocked + (10.0 * (2.0 - log10(100.0 - blocked_percent))
 *        *
 *        * dbz_unblocked = dbz_blocked + 20.0 - 10.0 * log10(100.0 - blocked_percent)
 *        *
 *        * The PPS also rounded zprime to the nearest 0.5 dBZ:
 *        *
 *        * zprime = (double)(int)(10. * (zprime+0.05))/10.;
 *        *
 *        * We don't do this, as our compute_RateZ can handle dBZs not on 0.5 boundaries *-
 *
 *       zprime = bin_moments->Z - (10.0 * log10(100.0 - blocked_percent)) + 20.0;
 *
 *       -* Compute Rainfall Rate(Z) based on Z'. Note: we can't use the lookup
 *        * table because zprime in not in 0.5 dB increments *-
 *
 *       rate_zprime = compute_RateZ(zprime, rate_out);
 *
 *       if (rate_zprime == QPE_NODATA)
 *       {
 *          precipRate = QPE_NODATA;
 *
 *          return (precipRate);
 *       }
 *
 *       -* If the rainfall rate based on the power corrected refl. is above a
 *        * minimum Kdp rate threshold, then use rainfall rate based on Kdp.
 *        * Rate(Kdp) is only reliable in "heavy" precip (~ 10mm/hr).
 *        * If rate_zprime is greater than this threshold and HydroClass is
 *        * Heavy Rain or Rain/Hail, we can use R(Kdp). The QPE AEL calls
 *        * corr_thresh - "THRESHOLD(Minimum RhoHV for KDP Usage).
 *        *
 *        * Default Kdp_min_usage_rate: 10.0 mm/hr
 *        * Default        corr_thresh: 0.9 *-
 *
 *       if ((rate_zprime > rate_out->qpe_adapt.dp_adapt.Kdp_min_usage_rate) &&
 *           ((hydro_class == HR) || (hydro_class == RH)) &&
 *           (bin_moments->CorrelCoef >= rate_out->qpe_adapt.dpprep_adapt.corr_thresh))
 *       {
 *          -* Compute Rainfall Rate Kdp based on Kdp *-
 *
 *          rate_Kdp = compute_RateKdp(bin_moments, rate_zprime, rate_out);
 *
 *          -* Sometimes R(Kdp) yields a negative rate (invalid); eliminate
 *           * that possibility here. *-
 *
 *          if (rate_Kdp > 0.0)
 *          {
 *             precipRate = rate_Kdp;
 *          }
 *          else -* rate_Kdp <= 0.0 *-
 *          {
 *             precipRate = rate_zprime;
 *          }
 *
 *          return (precipRate);
 *       }
 *       else -* return rainfall rate R(Z') *-
 *       {
 *          -* NOTE: this mimics the legacy PPS; better than using next elev. *-
 *
 *          precipRate = rate_zprime;
 *
 *          return (precipRate);
 *       }
 *
 *    } -* end if beam is partially blocked *-
 *
 * } -* end compute_Precip_PBB_method1() ================================== *-
 */

/******************************************************************************
    Function name: compute_Precip_PBB_method2()

    Description:
    ============
       It computes instantaneous rainfall rate in the presence of PBB using
       Method 2, which is presented in a paper titled "Classification and
       rainfall measurements in the presence of partial beam blockage".
       The paper is stored at
       S:/OHD-11/NEXRAD/Dual Polarization/Partial Beam Blockage - 1.doc

    NOTE:

    Inputs:
       int         azm             - radial number
       int         rng             - range index
       int         blocked_percent - percentage of beam blocked
       Rate_Buf_t* rate_out        - rate output buffer
       Moments_t*  bin_moments     - the moments for this bin

    Return:
       The Instantaneous Rainfall Rate, in mm/hr.

    Change History
    ==============
    DATE        VERSION   PROGRAMMER          NOTES
    ----        -------   ----------          -----
    11/05/08    0000      Zhan Zhang          Initial implementation
    03/10/10    0001      James Ward          Replaced RhoHV_min_Kdp_rate with
                                              corr_thresh.
    10/31/11    0002     James Ward           PBB method 2 is in the code.
******************************************************************************/

/* float compute_Precip_PBB_method2 (int azm, int rng, int blocked_percent,
 *                                   Rate_Buf_t* rate_out, Moments_t* bin_moments,
 *                                   Stats_t* stats)
 * {
 *    float zprime;
 *    float fshield; -* used to facilitate the calculation of zprime *-
 *    short hydro_class;
 *    float precipRate = QPE_NODATA;
 *    float rate_Kdp;
 *
 *    hydro_class = bin_moments->HydroClass;
 *
 *    if (blocked_percent > 0 && blocked_percent <= HIGH_BLOCKED_PERCENTAGE_THRESHOLD)
 *    {  -* HIGH_BLOCKED_PERCENTAGE_THRESHOLD is 70 *-
 *       -* We add the condition of EQUAL TO *-
 *
 *       #ifdef QPERATE_DEBUG
 *       {
 *          fprintf (stderr, "azm: %d; rng: %d; blk: %d\n",
 *                   azm, rng, blocked_percent);
 *       }
 *       #endif
 *
 *       if (bin_moments->Z == QPE_NODATA)
 *       {
 *          precipRate = QPE_NODATA;
 *
 *          return (precipRate);
 *       }
 *
 *       -* The blockage is greater than zero, we add the missing power          *
 *        * back to Z(processed) to create an updated value called Z' (Z prime). *
 *        * The formula is fshield = 0.5tanh[0.0277(50-blocked_percent)]+0.5     *
 *        * Note that function tanh(x) = {exp(2x)-1}/{exp(2x)+1}                 *-
 *
 *       fshield = 0.5*(exp(FSHIELD_FORMULA_COEFFICIENT_1*(50-blocked_percent)*2)-1)/
 *                     (exp(FSHIELD_FORMULA_COEFFICIENT_1*(50-blocked_percent)*2)+1)
 *                    + 0.5; -* Note that FSHIELD_FORMULA_COEFFICIENT_1 = 0.0277 *-
 *
 *       zprime = bin_moments->Z + 10.0 * log10(fshield);
 *
 *       -* compute precip rate based on zprime. Note: we can't use the lookup
 *        * table because zprime in not in 0.5 dB increments *-
 *
 *       precipRate = compute_RateZ(zprime, rate_out);
 *
 *       -* Brian suggests to add this conditional code snippet *-
 *
 *       if (bin_moments->CorrelCoef < rate_out->qpe_adapt.dpprep_adapt.corr_thresh)
 *       {  -* corr_thresh = 0.900 *-
 *          return (precipRate);
 *       }
 *
 *       -* Compute Rainfall Rate if blockage percentage is in 0%--20% *-
 *
 *       if (blocked_percent < MIDDLE_BLOCKED_PERCENTAGE_THRESHOLD )
 *       {  -* MIDDLE_BLOCKED_PERCENTAGE_THRESHOLD is 20 *-
 *
 *          -* use the R(Z,ZDR) relation if the echo behind obstruction is
 *           * classified as rain (light and moderate) or heavy rain (RA or HR);
 *           * use the R(KDP) relation if the echo is classified as rain/hail (RH).
 *           *-
 *
 *          if ((hydro_class == RA) || (hydro_class == HR))
 *          {
 *             precipRate = compute_RateZ_Zdr(zprime, bin_moments->Zdr, rate_out);
 *          }
 *          else if (hydro_class == RH)
 *          {
 *             rate_Kdp = compute_RateKdp(bin_moments, precipRate, rate_out);
 *             if (rate_Kdp > 0.0)
 *             {
 *                 precipRate = rate_Kdp;
 *             }
 *          }
 *       }
 *       else -* Compute Rainfall Rate if blockage percentage is in 20%--70% *-
 *       {
 *          -* use the R(Z,ZDR) relation if the echo behind obstruction is
 *           * classified as rain (RA) and the R(KDP) relation if the echo is
 *           * classified as heavy rain or rain / hail (HR or RH).
 *           *-
 *
 *          if (hydro_class == RA)
 *          {
 *             precipRate = compute_RateZ_Zdr(zprime, bin_moments->Zdr, rate_out);
 *          }
 *          else if ((hydro_class == HR) || (hydro_class == RH))
 *          {
 *             rate_Kdp = compute_RateKdp(bin_moments, precipRate, rate_out);
 *             if (rate_Kdp > 0.0)
 *             {
 *                 precipRate = rate_Kdp;
 *             }
 *          }
 *       }
 *    } -* end of if (blocked_percent > 0 && blocked_percent < = 70) *-
 *    else if (blocked_percent > HIGH_BLOCKED_PERCENTAGE_THRESHOLD)
 *    {  -* no need to calculate *-
 *       precipRate = QPE_NODATA;
 *    }
 *    else -* blocked_percent == 0, the function is miscalled *-
 *    {
 *       -* following Ning's suggestion to use PRGC_log *-
 *
 *       RPGC_log_msg(GL_INFO, "blockage equals zero,function is miscalled\n");
 *       return (QPE_NODATA);
 *    }
 *
 *    return (precipRate);
 *
 * } -* end compute_Precip_PBB_method2() ================================== *-
 */
