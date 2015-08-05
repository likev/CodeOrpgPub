/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2012/03/12 13:43:25 $
 * $Id: qperate_comp_RateZ_Zdr.c,v 1.7 2012/03/12 13:43:25 ccalvert Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */

/*** Local Include Files  ***/

#include"qperate_func_prototypes.h"

/******************************************************************************
    Filename: qperate_comp_RateZ_Zdr.c

    Description:
    ============
       compute_RateZ_Zdr() function calculates the rainfall rate R(Z, Zdr)
       for each sample volume based on the smoothed reflectivity data
       Z_processed and Zdr.

       For the calculation of R(Z, Zdr) , the following applies for each
       sample volume:

       1. Any Z value equal to NO DATA results in a R(Z, Zdr) equal to
          NO DATA

       2. Any Zdr value equal to NO DATA results in a R(Z, Zdr) equal to NO DATA

       3. Any Z value is greater than 53 dbZ, then it is capped to 53 dbZ before
          it becomes linear Z.

    NOTE: compute_RateZ_Zdr() is based on version 2 of NSSL's dual-Polarimetric
          QPE algorithm coded by John Krause, NSSL, March 19, 2007.

    See also AEL 3.2.2 K

    Inputs:
       float       Z_processed - smoothed reflectivity, in dBZ.
       float       Zdr         - unprocessed differential reflectivity, in dBZ
       Rate_Buf_t* rate_out    - rate output buffer

    Return:
       The rainfall rate R(Z, Zdr), in mm/h.

    Change History
    ==============
    DATE        VERSION    PROGRAMMER         NOTES
    ---------   -------    ----------         -----
    06 Aug 07    0001      Jihong Liu         Initial implementation for
                                              dual-polarization project
                                              (ORPG Build 11).

    15 Nov 07    0002      Stein/Pham         Discovered that ZZdr_Zmult_Coeff
                                              has already been multiplied by
					      0.01, which we were doing again.
					      We removed the multiplication
					      from the code.
                                              Converted the smooth reflectivity
                                              from dBZ units to dB units

    11 Dec 04    0003      Stein              As per telecon with Murnan,
                                              Krauss, Berkowitz on 6 Dec 07,
					      Zdr is no longer being averaged.
					      We'll just use a single linear
					      value.  Removed the hydro class
					      array input parameter.
******************************************************************************/

float compute_RateZ_Zdr (float Z_processed, float Zdr, Rate_Buf_t* rate_out)
{
   float Z_linear   = QPE_NODATA;
   float Zdr_linear = QPE_NODATA;
   float RateZ_Zdr  = QPE_NODATA;

   char  msg[200]; /* stderr msg */

   #ifdef QPERATE_DEBUG
      fprintf (stderr, "Beginning compute_RateZ_Zdr() .........\n");
   #endif

   /* Compute Average Zdr linear - 11 Dec 07: NO LONGER AN AVERAGE       */
   /* AvgZdr_linear = compute_Avg_Zdr( Pol_Zdr, hydro_class, azm, rng ); */

   /* Convert Zdr (processed) to Zdr linear */

   if (Zdr != QPE_NODATA)
      Zdr_linear = powf ( 10., ( Zdr * 0.1 ) );

   /* Compute rainfall rate R(Z, Zdr) for each sample volume based on the
      processed reflectivity data (Z) and averaged (Zdr) value. */

   if(Z_processed != QPE_NODATA && Zdr_linear != QPE_NODATA)
   {
      /* Default Refl_min: -32.0 dBZ */

      if(Z_processed < rate_out->qpe_adapt.dp_adapt.Refl_min)
      {
         /* The reflectivity is too low, so it's probably noise.
          * Try at a higher elevation. */

         return (QPE_NODATA);
      }
      else if (Z_processed > rate_out->qpe_adapt.dp_adapt.Refl_max)
      {
         /* Default Refl_max: 53.0 dBZ
          *
          * The reflectivity exceeds 53 dBZ, which might happen for a
          * freak rain. Cap it at Refl_max. 52.77 dBZ -> a rate of 100 mm/hr.
          */

         Z_linear = powf(10., (rate_out->qpe_adapt.dp_adapt.Refl_max * 0.1));
      }
      else /* Refl_min <= Z_processed <= Refl_max */
      {
         /* Convert the processed reflectivity data from dBZ units to dB units
          * and stored in Z_linear */

         Z_linear = powf(10., (Z_processed * 0.1));
      }

      /* Default    Zdr_z_mult:  0.0067
       * Default   Zdr_z_power:  0.927
       * Default Zdr_zdr_power: -3.43
       */

      RateZ_Zdr = rate_out->qpe_adapt.dp_adapt.Zdr_z_mult *
                  powf(Z_linear,
                       rate_out->qpe_adapt.dp_adapt.Zdr_z_power) *
                  powf(Zdr_linear,
                       rate_out->qpe_adapt.dp_adapt.Zdr_zdr_power);

      if(RateZ_Zdr < 0.0)
      {
         sprintf(msg, "%s RateZ_Zdr %f < 0.0, Z_linear %f, Zdr_linear %f\n",
                 "compute_RateZ_Zdr:",
                 RateZ_Zdr, Z_linear, Zdr_linear);
         RPGC_log_msg(GL_INFO, msg);
         #ifdef QPERATE_DEBUG
            fprintf(stderr, msg);
         #endif

         return(QPE_NODATA);
      }

   } /* end if Z_processed and Zdr_linear are not QPE_NODATA */

   return(RateZ_Zdr); /* we return QPE_NODATA if Z or Zdr is QPE_NODATA */

} /* end compute_RateZ_Zdr() ==================================== */
