/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2011/03/08 17:21:26 $
 * $Id: qperate_comp_RateZ.c,v 1.6 2011/03/08 17:21:26 ccalvert Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */

/*** Local Include Files  ***/

#include "qperate_func_prototypes.h"

/******************************************************************************
    Filename: qperate_comp_RateZ.c

    Description:
    ============
       compute_RateZ() function calculates the rainfall rate R(Z) for each
    sample volume based on the smoothed reflectivity Z_processed.
    For the calculation of R(Z), the following applies for each sample volume:

       1. Z_processed greater than THRESHOLD(Maximum Reflectivity) then
          Z_processed is set equal to THRESHOLD(Maximum Reflectivity), and

       2. Z_processed equal to QPE_NODATA results in a R(Z) equal to
          QPE_NODATA.

    NOTE: compute_RateZ() is based on version 1 of NSSL's dual-Polarimetric
          QPE algorithm coded by John Krause, NSSL, January 2003.

          An alternative formulation is to convert dBZ to mm^6/m^3:

             Z_mm6_m3 = powf (10.0, Z_processed / 10.0);

          and then convert mm^6/m^3 to a rate in mm/hr;

             RateZ = powf (Z_mm6_m3 / Z_mult, 1 / Z_power);
                   = powf (Z_mm6_m3 /  300.0, 1 /     1.4);

          or more explicitly (where dBZ = Z_processed):

          From the legacy formula:

            Z_convert = 300.0 * powf (R, 1.4)

          Replacing Z with dBZ:

            powf (10.0, Z_processed / 10.0) = 300.0 * powf (R, 1.4)

          Taking log10 of both sides:

            log10(powf (10.0, Z_processed / 10.0)) = log10(300.0 *
                                                           powf (R, 1.4))

          Using the multiplicative log rule on the right hand side:

            (Z_processed / 10.0) = log10(300.0) + log10(powf (R, 1.4))

          Using the power log rule on the right hand side:

            (Z_processed / 10.0) = log10(300.0) + 1.4 * log10(R)

          Isolating R on one side:

            ((Z_processed / 10.0) - log10(300.0)) / 1.4 = log10(R)

          Multiplying the top/bottom of the left hand side by 10.0:

            (Z_processed - (10.0 * log10(300.0))) / (10.0 * 1.4) = log10(R)

          Raising both sides to a power to get R by itself:

            powf(10.0, (Z_processed - (10.0 * log10(300.0))) / (10.0 * 1.4))=R

          R = powf(10.0, (Z_processed - (10.0 * log10(300.0))) / (10.0 * 1.4));

    Inputs:
       float       Z_processed - the smoothed reflectivity, in dBZ.
       Rate_Buf_t* rate_out    - the rate output buffer

    Return:
       The rainfall rate R(Z), in mm/h.

    AEL 3.2.2 B and F.

    Change History
    ==============
    DATE        VERSION    PROGRAMMER         NOTES
    ----        -------    ----------         -----
    12/09/06    0000       Cham Pham          Initial implementation for
                                              dual-polarization project
                                              (ORPG Build 11).
******************************************************************************/

float compute_RateZ (float Z_processed, Rate_Buf_t* rate_out)
{
   float RateZ = QPE_NODATA;
   char  msg[200]; /* stderr msg */

   #ifdef QPERATE_DEBUG
      fprintf ( stderr, "Beginning compute_RateZ() .........\n" );
   #endif

   /* Compute a rainfall rate R(Z) based on the reflectivity data (Z) value. */

   if(Z_processed != QPE_NODATA)
   {
      /* Default Refl_min: -32.0 dBZ */

      if(Z_processed < rate_out->qpe_adapt.dp_adapt.Refl_min)
      {
         /* The reflectivity is too low, so it's probably noise.
          * Try at a higher elevation. */

         return (QPE_NODATA);
      }
      else if(Z_processed > rate_out->qpe_adapt.dp_adapt.Refl_max)
      {
         /* Default Refl_max:  53.0 dBZ
          * Default   Z_mult: 300
          * Default  Z_power:   1.4
          *
          * The reflectivity exceeds 53 dBZ, which might happen for a
          * freak rain. Cap it at Refl_max. 52.77 dBZ -> a rate of 100 mm/hr.
          */

         RateZ = powf (10.0, (rate_out->qpe_adapt.dp_adapt.Refl_max -
                             (10.0 * log10(rate_out->qpe_adapt.dp_adapt.Z_mult))) /
                             (10.0 * rate_out->qpe_adapt.dp_adapt.Z_power));
      }
      else /* Refl_min <= Z_processed <= Refl_max */
      {
         RateZ = powf (10.0, (Z_processed -
                             (10.0 * log10(rate_out->qpe_adapt.dp_adapt.Z_mult))) /
                             (10.0 * rate_out->qpe_adapt.dp_adapt.Z_power));
      }

      if(RateZ < 0.0)
      {
         sprintf(msg, "compute_RateZ: RateZ %f < 0.0, Z_processed %f\n",
                 RateZ, Z_processed);
         RPGC_log_msg(GL_INFO, msg);
         #ifdef QPERATE_DEBUG
            fprintf(stderr, msg);
         #endif

         return (QPE_NODATA);
      }

   } /* end if Z_processed was not QPE_NODATA */

   /* If Z_processed was QPE_NODATA, we'll return a default rate of Rate_Z */

   return (RateZ);

} /* end compute_RateZ() ==================================== */

/******************************************************************************
    Filename: qperate_comp_RateZ.c

    Description:
    ============
       create_RateZ_table() creates a lookup table for rate R(Z). It is
    based on the fact that Z_processed comes in .5 dB increments.

    Inputs:
       float*      RateZ_table - RateZ table.
       Rate_Buf_t* rate_out    - buffer with adaptable parameters

    Return:
       The rainfall rate R(Z) table, in mm/h.

    Change History
    ==============
    DATE        VERSION    PROGRAMMER         NOTES
    --------    -------    ----------         ----------------------
    20090331    0000       James Ward         Initial implementation
******************************************************************************/

void create_RateZ_table (float** RateZ_table, Rate_Buf_t* rate_out)
{
   static float old_Refl_min = 0.0;
   static float old_Refl_max = 0.0;
   static float old_Z_mult   = 0.0;
   static float old_Z_power  = 0.0;

   int          make_new_table = FALSE;
   int          i, num_items   = 0;
   char         msg[200]; /* stderr msg */

   /* Default Refl_min: -32.0 dBZ, accuracy: 0.5 */

   if(fabsf(old_Refl_min - rate_out->qpe_adapt.dp_adapt.Refl_min) > 0.5)
   {
       sprintf(msg, "old Refl_min %f != %f new Refl_min, %s\n",
               old_Refl_min,
               rate_out->qpe_adapt.dp_adapt.Refl_min,
               "making a new RateZ table");

       RPGC_log_msg(GL_INFO, msg);
       #ifdef QPERATE_DEBUG
          fprintf(stderr, msg);
       #endif

       make_new_table = TRUE;
   }

   /* Default Refl_max: 53.0 dBZ, accuracy: 0.5 */

   else if (fabsf(old_Refl_max - rate_out->qpe_adapt.dp_adapt.Refl_max) > 0.5)
   {
       sprintf(msg, "old Refl_max %f != %f new Refl_max, %s\n",
               old_Refl_max,
               rate_out->qpe_adapt.dp_adapt.Refl_max,
               "making a new RateZ table");

       RPGC_log_msg(GL_INFO, msg);
       #ifdef QPERATE_DEBUG
          fprintf(stderr, msg);
       #endif

       make_new_table = TRUE;
   }

   /* Default Z_mult: 300, accuracy: 1 */

   else if (fabsf(old_Z_mult - rate_out->qpe_adapt.dp_adapt.Z_mult) > 1)
   {
       sprintf(msg, "old Z_mult %f != %f new Z_mult, %s\n",
               old_Z_mult,
               rate_out->qpe_adapt.dp_adapt.Z_mult,
               "making a new RateZ table");

       RPGC_log_msg(GL_INFO, msg);
       #ifdef QPERATE_DEBUG
          fprintf(stderr, msg);
       #endif

       make_new_table = TRUE;
   }

   /* Default Z_power: 1.4, accuracy: 0.1 */

   else if (fabsf(old_Z_power - rate_out->qpe_adapt.dp_adapt.Z_power) > 0.1)
   {
       sprintf(msg, "old Z_power %f != %f new Z_power, %s\n",
               old_Z_power,
               rate_out->qpe_adapt.dp_adapt.Z_power,
               "making a new RateZ table");

       RPGC_log_msg(GL_INFO, msg);
       #ifdef QPERATE_DEBUG
          fprintf(stderr, msg);
       #endif

       make_new_table = TRUE;
   }

   if(make_new_table)
   {
      if(*RateZ_table != NULL) /* free previous table */
         free(*RateZ_table);

      /* Malloc the new table */

      num_items = (2 * (rate_out->qpe_adapt.dp_adapt.Refl_max -
                        rate_out->qpe_adapt.dp_adapt.Refl_min)) + 1;

      *RateZ_table = (float*) calloc(num_items, sizeof(float));

      /* Fill the new table */

      for(i = 0; i < num_items; i++)
      {
          (*RateZ_table)[i] = compute_RateZ (
                              rate_out->qpe_adapt.dp_adapt.Refl_min + (0.5 * i),
                              rate_out);
      }

      /* Save old values, so we don't do this every volume */

      old_Refl_min = rate_out->qpe_adapt.dp_adapt.Refl_min;
      old_Refl_max = rate_out->qpe_adapt.dp_adapt.Refl_max;
      old_Z_mult   = rate_out->qpe_adapt.dp_adapt.Z_mult;
      old_Z_power  = rate_out->qpe_adapt.dp_adapt.Z_power;
   }
} /* end create_RateZ_table() ==================================== */
