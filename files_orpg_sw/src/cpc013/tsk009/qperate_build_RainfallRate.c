/*
 * RCS info
 * $Author: dberkowitz $
 * $Locker:  $
 * $Date: 2013/12/19 17:41:22 $
 * $Id: qperate_build_RainfallRate.c,v 1.12 2013/12/19 17:41:22 dberkowitz Exp $
 * $Revision: 1.12 $
 * $State: Exp $
 */

/*** Local Include Files  ***/

#include "qperate_func_prototypes.h"
#include "dp_lib_func_prototypes.h"
#include "qperate_list_coord.h"

static float slant_rng_m[MAX_BINS] = {0.0}; /* Slant range in meters */

static int FirstTime = TRUE;                /* Flag to compute slant range
                                             * which is executed only the
                                             * first time the algorithm is
                                             * processed. */

/******************************************************************************
    Filename: qperate_build_RainfallRate.c

    Description:
    ============
      Add_bin_to_RR_Polar_Grid() determines whether the current sample bin
    is unblocked, partially blocked, or if the blockage percentage is greater
    than the blockage threshold.  If it is, we'll need to get this bin's
    information from the next higher elevation.

    Inputs:
      Compact_dp_basedata_elev* inbuf - input buffer
      int    azm                      - radial index
      int    rng                      - range index
      float  elev_angle_deg           - elevation angle in degrees
      char   Beam_Blockage[][MX_RNG]  - beam blockage percentage.
      float  exzone[][EXZONE_FIELDS]  - exclusion zones
      short  beam_edge_top[MAX_AZM]   - absolute top edge of the melting layer
      float* RateZ_table              - table of R(Z) rates

    Outputs:
      int*        num_bins_filled  - The number of sample bins that have
                                     either had precip. rates calculated
                                     or have been ignored because they
                                     are empty or because they contain
                                     biological returns.  This value is
                                     used to determine whether we have
                                     enough bins filled to exceed
                                     the Min_Bins_Thresh.
      Rate_Buf_t* rate_out         - rate output buffer
      HHC_Buf_t*  hhc_out          - hhc output buffer
      unsigned short* max_no_of_gates  - maximum number of gates in a moment

    Change History
    ==============
    DATE        VERSION  PROGRAMMER         NOTES
    ----------  -------  ---------------    ------------------
    01/09/07    0000     Daniel Stein       Initial implementation for
                                            dual-polarization project
                                            (ORPG Build 11).
    04/18/07    0001     Jihong Liu         Add Precip. Accum. Initiation
                                            Function
    06/20/07    0002     Jihong Liu         Accumulation beginning time and
                                            latest rain time are based on
                                            average date/time for the
                                            current volume instead of the
                                            current elevation when the rain
                                            is detected.
    20081117    0003     James Ward         Added logging of flag transitions
    20090331    0004     James Ward         Added RateZ_table
    20131205    0005     Dan Berkowitz      Replaced beam_center_top with
                                            beam_edge_top. 

******************************************************************************/

void Add_bin_to_RR_Polar_Grid (Compact_dp_basedata_elev* inbuf,
                               int azm,     int rng,
                               int*         num_bins_filled,
                               float        elev_angle_deg,
                               char         Beam_Blockage[BLOCK_RAD][MX_RNG],
                               Rate_Buf_t*  rate_out,
                               HHC_Buf_t*   hhc_out,
                               float    exzone[MAX_NUM_ZONES][EXZONE_FIELDS],
                               unsigned short* max_no_of_gates,
                               struct listitem_t* *hybrid_list,
                               short         beam_edge_top[MAX_AZM],
                               float*        RateZ_table)
{
   int       blockage_azm, blockage_rng;
   int       blocked_percent;
   short     hc_type;
   float     azm_angle;
   Moments_t bin_moments;          /* the moments for this [azm][rng] */
   float     IRRate = QPE_NODATA;

   #ifdef QPERATE_DEBUG
      fprintf ( stderr, "Beginning Add_bin_to_RR_Polar_Grid() ...........\n" );
   #endif

   /* Checking if radial is not good, add coordinates to list then
    * skip this sample bin. */

   azm_angle = (float) inbuf->radial[azm].bdh.azimuth;

   if((inbuf->radial_is_good[azm] == FALSE) ||
      (azm_angle < 0.)                      ||
      (azm_angle > 360.))
   {
      #ifdef QPERATE_DEBUG
         fprintf(stderr,"Missing radial - ( %d, %d ) = %d\n",azm, rng,
                         inbuf->radial_is_good[azm]);
      #endif

      add_coord_to_list(hybrid_list, azm, rng);

      return;
   }

   /* The blockage azimuth angle (azm_angle) is in tenths of a degree.
    * The Blockage Algorithm computes beam blockage for every 1 km
    * range bin and for every 0.1 deg.
    *
    * NOTE: blockage_azm is deliberately rounded to the nearest integer
    * to ensure that the range of values falls within the array
    * limits of 0 to 3599. */

   blockage_azm = (int) RPGC_NINT (azm_angle * 10.0);

   /* Make sure array index not over boundary */

   if(blockage_azm > 3599)
      blockage_azm = 3599;

   /* Convert range bin index from 250m to 1km */

   blockage_rng = (int) (rng / 4);

   #ifdef QPERATE_DEBUG
      fprintf(stderr,
             "AZMRNG(%d, %d) azm_angle: %6.2f; %s %d; blockage_rng: %d\n",
              azm, rng/4, azm_angle,
              "blockage_azm:",
              blockage_azm, blockage_rng);
   #endif

   /* Obtain the beam blockage value for each bin, convert from char to int.
    * NOTE: blocked_percent is a percentage (i.e 4%). */

   blocked_percent = (int) Beam_Blockage[blockage_azm][blockage_rng];

   /* Fetch the bin_moments */

   read_Moments(inbuf, azm, rng, &bin_moments, max_no_of_gates);

   hc_type = bin_moments.HydroClass;

   /* (AEL 1.1.2.4) If the hydrometeor is:
    *
    * Ground Clutter or Unknown, or
    *
    * if the blocked percentage is too high, or
    *
    * if it's in an exclusion zone,
    *
    * we need to go to a higher elevation and try again.
    *
    * 20080317 Based on a conversation with Brian Klein,
    *          we check RhoHV inside compute_IRRate().
    *
    * 20080609 You can get the default hydroclass of QPE_NODATA
    *          when the rng (919) > number of gates (592).
    *
    * Default Kdp_max_beam_blk: 70% */

   if((hc_type != GC)                                                   &&
      (hc_type != UK)                                                   &&
      (hc_type != QPE_NODATA)                                           &&
      (blocked_percent < rate_out->qpe_adapt.dp_adapt.Kdp_max_beam_blk) &&
      (is_Excluded (azm_angle, slant_rng_m[rng], elev_angle_deg,
         rate_out, exzone) == FALSE))
   {
      /* Compute instantaneous rainfall rate */

      IRRate = compute_IRRate(azm, rng, blocked_percent,
                              rate_out, &bin_moments,
                              beam_edge_top, RateZ_table);

      /* Due to a flaw in the R(KDP) algorithm, it is possible to compute
       * negative rainfall rates. This condition will catch that error.
       * Also, if something goes wrong with the other R() computations,
       * such as a too high or low Z, then compute_IRRate () function
       * will return QPE_NODATA, indicating to try at a higher elevation. */

      if((IRRate < 0.0) || (IRRate == QPE_NODATA)) /* no rate calculated */
      {
         add_coord_to_list(hybrid_list, azm, rng);
      }
      else /* a rate was calculated, AEL 3.1.2.5 */
      {
         /* Is IRRate enough to include in the rain area calculation?
          * Precip. could be 0.0 depending on the hydroclass - we don't
          * want to trigger precip. detection from tiny or 0 rates.
          *
          * The minimum a rain gauge can detect is 0.01 in.
          *
          * The default hydromet_prep.alg rain_dbz_thresh = 20.0 dBZ,
          * which under the standard R(Z) = 0.456 mm/hr.
          *
          * Default Paif_rate: 0.5 mm/hr */

         if(IRRate > rate_out->qpe_adapt.dp_adapt.Paif_rate)
         {
            /* Increment area sum, AEL 3.1.3 */

            rate_out->rate_supl.sum_area += get_BinArea(rng);
         }

         /* Check to see if the rate > a maximum rate. AEL 3.1.2.5 */

         if(IRRate > rate_out->qpe_adapt.dp_adapt.Max_precip_rate)
         {
            IRRate = rate_out->qpe_adapt.dp_adapt.Max_precip_rate;
         }

         /* Save the rate (in) and hydroclass in their output buffers */

         rate_out->RateComb[azm][rng] = IRRate * MM_TO_IN;

         hhc_out->HybridHCA[azm][rng] = hc_type;

         ++(*num_bins_filled);

      } /* end a rate was calculated */

   } /* end if the bin can calculate a rate */
   else /* we CANNOT use this sample bin, add it into list. AEL 3.1.2.3 */
   {
      add_coord_to_list(hybrid_list, azm, rng);
   } /* end else the bin can't calculate a rate */

   #ifdef QPERATE_DEBUG
      fprintf (stderr , "Exit Add_bin_to_RR_Polar_Grid() ..........\n");
   #endif

} /* end Add_bin_to_RR_Polar_Grid() ========================= */

/******************************************************************************
    Filename: qperate_build_RainfallRate.c

    Description:
    ============
      build_RR_Polar_Grid() builds the instantaneous precipitation
    rainfall rate data.  It stores rainfall rates into a 720 X 920 grid, with
    each element representing a volume 0.5 degrees X 250m.

    hybrid_list is a linked list of az/rng coordinates where each item on
    the list represents a bin that couldn't be used at a "low" elevation
    and has to be obtained from a higher elevation.

    Inputs:
      Compact_dp_basedata_elev* inbuf          - input buffer
      int*             new_vol                 - indicates starting new volume
      int              elev_ind                - elevation number
      int              elev_angle_tenths       - elevation angle in
                                                 tenths of a degree
      int              max_ntilts              - maximum elevation number
                                                 within VCP
      int              surv_bin_size           - reflectivity data gate size
                                                 (meters)
      float            exzone[][EXZONE_FIELDS] - exclusion zones
      float*           RateZ_table             - table of R(Z) rates

    Outputs:
      int*             num_bins_filled         - The number of sample bins that
                                                 have either had precip. rates
                                                 calculated or have been
                                                 ignored because they are empty
                                                 or because they contain
                                                 biological returns. This value
                                                 is used to determine whether
                                                 we have enough bins filled to
                                                 exceed the Min_Bins_Thresh.
      Rate_Buf_t*      rate_out                - rate output buffer
      HHC_Buf_t*       hhc_out                 - hhc output buffer
      int*             get_next_elev           - flag to keep going
      unsigned short*  max_no_of_gates         - max gates in a moment

    Change History
    ==============
    DATE        VERSION   PROGRAMMER          NOTES
    ----        -------   ----------          -----
    01/10/07    0000      C. Pham & D. Stein  Initial implementation for
                                              dual-polarization project
                                              (ORPG Build 11).
    04/18/07    0001      Jihong Liu          Add Precip. Acuum. Initiation
                                              Function
    06/20/07    0002      Jihong Liu          Accumulation beginning time and
                                              latest rain time are based on
                                              average date/time for the current
                                              volume instead of the current
                                              elevation when the rain is
                                              detected
    20090331    0004      James Ward          Added RateZ_table.

    20110823    0005      James Ward          Deleted unused check_area_exceeded
    20131205    0006      Dan Berkowitz       Replaced beam_center_top with
                                              beam_edge_top

******************************************************************************/

void build_RR_Polar_Grid(Compact_dp_basedata_elev* inbuf,
                         int*   new_vol,
                         int    elev_ind,        int  elev_angle_tenths,
                         int    max_ntilts,      int* num_bins_filled,
                         int    surv_bin_size,
                         Rate_Buf_t*        rate_out,
                         HHC_Buf_t*         hhc_out,
                         int*               get_next_elev,
                         float           exzone[MAX_NUM_ZONES][EXZONE_FIELDS],
                         unsigned short*    max_no_of_gates,
                         Regression_t*      rates,
                         float*             RateZ_table)
{
   char  Beam_Blockage[BLOCK_RAD][MX_RNG]; /* blockage uses legacy MX_RNG */
   float elev_angle_deg      = 0.0;
   int   azm                 = 0;
   int   rng                 = 0;
   int   Min_Bins_Thresh     = 0;

   static struct listitem_t* hybrid_list    = NULL;  /* hybrid llist      */
   static struct listitem_t* hybrid_current = NULL;  /* hybrid current    */

   short             beam_edge_top[MAX_AZM];   /* to collect beam absolute top */
   Generic_moment_t* gm  = NULL;               /* generic moment pointer     */
   float             val = QPE_NODATA;         /* value of beam absolute top   */

   /* Load blockage data into the Beam_Blockage array */

   read_Blockage(elev_angle_tenths, Beam_Blockage);

   /* Convert the (int) elevation angle in tenths of a degree
    * to a (float) in degrees.                                */

   elev_angle_deg = (float) elev_angle_tenths / 10.0;

   for(rng = 0; rng < MAX_BINS; ++rng)
   {
      /* NOTE: slant range (in meters) is computed only the first time
       *       execution of the algorithm processed.
       *
       * surv_bin_size is the reflectivity data gate size in METERS
       *
       * Like Beam_Blockage, there is no reason to compute
       * slant_range_m if there are no exclusion zones. A flag
       * could be set to FALSE and passed around. Check dp_adapt.Num_zones
       * at the start of every volume and malloc slant_rng_m
       * on demand. Maybe revisit this if PBB is redone. */

      if(FirstTime)
      {
         slant_rng_m[rng] = (float)( (rng + 1 ) * surv_bin_size);

         #ifdef QPERATE_DEBUG
         {
            fprintf(stderr,"slant_range_m(%d) = %.2f \n",
                    rng, slant_rng_m[rng]);
         }
          #endif

         /* Reset FirstTime flag to FALSE for the rest
          * of the algorithm processed. */

         if(rng == (MAX_BINS - 1))
            FirstTime = FALSE;
      }
   }

   /* Get the melting layer beam edge top for all 360 azimuths.
    * The beam_edge_top is the intersection of the beam bottom with the top
    * of the melting layer. Bins above the beam_edge_top are usually frozen.
    * Big Drops, which can occur above the beam_center_top, are not defined
    * above the beam_edge_top.
    *
    * The melting layer has 4 levels, 2 top and 2 bottom, but we only use
    * the beam edge top, at index BEAM_EDGE_TOP (0). The other levels
    * are: BEAM_CENTER_TOP (1), BEAM_CENTER_BOTTOM (2),  BEAM_EDGE_BOTTOM (3).
    *
    * The code was borrowed from read_Moments().
    *
    * Note: 0 and 1 are valid levels and do not indicate QPE_NODATA,
    * as they do for the other moments. */

   for(azm = 0; azm < MAX_AZM; azm++)
   {
       gm = (Generic_moment_t*) inbuf->radial[azm].ml_moment;

       if(gm->no_of_gates <= BEAM_EDGE_TOP)
          beam_edge_top[azm] = QPE_NODATA;
       else if(gm->gate.u_s[BEAM_EDGE_TOP] == 0)
          beam_edge_top[azm] = 0;
       else if(gm->gate.u_s[BEAM_EDGE_TOP] == 1)
          beam_edge_top[azm] = 1;
       else
       {
          val =
           ((float) gm->gate.u_s[BEAM_EDGE_TOP] - gm->offset) / gm->scale;
          beam_edge_top[azm] = (short) RPGC_NINT(val);
       }
   }

   /* Init if is a new volume or the first elevation in a volume */

   if((*new_vol) || (elev_ind == 1))
   {
      /* Set new_vol flag to FALSE for the rest of this volume scan */

      *new_vol = FALSE;

      /* Initialize the current position in the hybrid list and destroy
       * the hybrid list to empty it for this new volume scan. */

      hybrid_current = NULL;

      if(hybrid_list != NULL)
      {
         destroy_list(&hybrid_list);

         #ifdef QPERATE_DEBUG
            fprintf(stderr, "DESTROY hybrid_list ............\n" );
         #endif
      }

      /* Since this is the first elevation, check all the grid, */

      for(azm = 0; azm < MAX_AZM; ++azm) /* AEL 3.1.2 */
      {
         for(rng = 0; rng < MAX_BINS; ++rng)
         {
            /* Add bin to IRRate Grid */

            Add_bin_to_RR_Polar_Grid(inbuf, azm, rng, num_bins_filled,
                                     elev_angle_deg, Beam_Blockage,
                                     rate_out, hhc_out,
                                     exzone, max_no_of_gates,
                                     &hybrid_list,
                                     beam_edge_top,
                                     RateZ_table);
         } /* End rng loop */

      } /* End azm loop */

      /* Save the current hybrid list position. */

      hybrid_current = hybrid_list;

   } /* end start of a new volume */
   else /* inside, but not at the start of, a volume */
   {
      /* While the list of skipped coords is not empty */

      while(get_coord_from_list(&hybrid_list, &hybrid_current,
                                &azm, &rng) == TRUE)
      {
         /* Add bin to IRRate Grid */

         Add_bin_to_RR_Polar_Grid(inbuf, azm, rng, num_bins_filled,
                                  elev_angle_deg, Beam_Blockage,
                                  rate_out, hhc_out,
                                  exzone, max_no_of_gates,
                                  &hybrid_list,
                                  beam_edge_top,
                                  RateZ_table);

      } /* end while get_coord_from_list */

   } /* end not the start of a new volume */

   /* Check to see if this should be the last elevation processed.
    *
    * If num_bins_filled exceeds Min_Bins_Thresh         or
    * you're on the next to last elevation,              or
    * the input buffer tells you it's the last elevation then
    * set get_next_elev to FALSE                         and
    * destroy the list coordinates.
    *
    * Note: We compare on the number of bins, not the % bins filled
    *
    * 26 MAR 08 - When Automated Volume Scan Early Termination (AVSET)
    * is implemented, we'll need to modify the following
    * if statement to account for an "about to terminate the volume scan"
    * event generated by the AVSET algorithm. - Stein
    *
    * 20080709 Ward For AVSET early termination - added last_ele_flag check.
    *
    * Default Grid_is_full: 99.7 % */

   Min_Bins_Thresh   = ceil(MAX_AZM_BINS
                       * rate_out->qpe_adapt.dp_adapt.Grid_is_full
                       * 0.01);

   if((*num_bins_filled                   >= Min_Bins_Thresh) ||
      (elev_ind                           >= max_ntilts)      ||
      (inbuf->radial[0].bdh.last_ele_flag == TRUE))
   {
      *get_next_elev = FALSE;

      /* Destroy the hybrid list, as it is no longer needed. */

      hybrid_current = NULL;

      if(hybrid_list != NULL)
      {
         destroy_list(&hybrid_list);

         #ifdef QPERATE_DEBUG
            fprintf(stderr, "DESTROY LIST ............\n");
         #endif
      }
   }

   #ifdef QPERATE_DEBUG
      fprintf(stderr, "Exit build_RR_Polar_Grid() ..........\n");
   #endif

} /* end build_RR_Polar_Grid() ============================== */

/******************************************************************************
    Filename: qperate_build_RainfallRate.c

    Description:
    ============
      qperate_terminate() is the Termination Signal Handler.

      Code cribbed from ~/src/cpc001/tsk024/hci_agent.c

    Inputs:
       int signal - the signal sent (SIGTERM = 15)
       int flag   - signal type (3)?

    Change History
    ==============
    DATE        VERSION    PROGRAMMER      NOTES
    ----------  -------    ----------      -----------------------
    02/12/2008    0000     Ward            Initial implementation
******************************************************************************/

int qperate_terminate(int signal, int flag)
{
   RPGC_log_msg(GL_INFO, "qperate_terminate() called\n");

   return 0;

} /* end qperate_terminate() =================================== */
