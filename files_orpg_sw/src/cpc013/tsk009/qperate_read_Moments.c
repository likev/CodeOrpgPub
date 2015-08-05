/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2015/04/21 22:07:16 $
 * $Id: qperate_read_Moments.c,v 1.10 2015/04/21 22:07:16 steves Exp $
 * $Revision: 1.10 $
 * $State: Exp $
 */

/*** Local Include Files  ***/

#include <math.h>
#include <rpgcs.h>
#include "qperate_func_prototypes.h"
#include "dp_lib_func_prototypes.h"
#include <basedata.h> /* HIGH_ATTENUATION_TYPE */

#define MILSEC 1000

/* #define QPERATE_DEBUG */

/******************************************************************************
    Filename: qperate_read_Moments.c

    Description:
    ============
    read_MomentData() reads header info. and dual-pol moment data from
    the DP_MOMENTS_ELEV intermediate product that are needed by QPE Rate
    algorithm.

    Inputs:
       Compact_dp_basedata_elev* inbuf - pointer to input data.

    Outputs:
       int* start_elev_date     - start of surv elevation date
       int* start_elev_time     - start of surv elevation time
       int* end_elev_date     - end of surv elevation date
       int* end_elev_time     - end of surv elevation time
       int *surv_bin_size - reflectivity data gate size in meters
       int* spotblank     - spot blank status

    Change History
    ==============
    DATE        VERSION    PROGRAMMER         NOTES
    --------    -------    ----------         --------------------------
    09/27/07    0000       Cham Pham          Initial version for
                                              dual-polarization project
                                              (ORPG Build 11).
    08/07/14	0001	   Nicholas Cooper    For CCR NA14-00252:
					      Corrected to Surv. time
					      instead of doppler time.

******************************************************************************/

void read_HeaderData (Compact_dp_basedata_elev* inbuf,
                      int* start_elev_date,     int* start_elev_time,
		      int* end_elev_date,	int* end_elev_time,
                      int* surv_bin_size, 	int* spotblank)
{
   int    i;                      /* loop variable              */
   short  got_good_time;          /* TRUE -> already got a time */
   char   msg[200];               /* error message              */
   time_t max_time = 0L;	  /* maximum time of the sweep  */
   time_t min_time = 0L;	  /* minimum time of the sweep  */
   int    max_index = -1;	  /* index of max time of sweep */
   int    min_index = -1;	  /* index of min time of sweep */
   time_t radial_time = 0L;	  /* holder for each radial time*/

   /* Warn if we get > 360 radials. We could get less. */

   if(inbuf->num_radials > MAX_AZM)
   {
      sprintf(msg, "%s inbuf->num_radials %d > MAX_AZM %d\n",
              "read_HeaderData:", inbuf->num_radials, MAX_AZM);
      RPGC_log_msg(GL_INFO, msg);
      #ifdef QPERATE_DEBUG
        fprintf(stderr, msg);
      #endif
   }

   /* Get the elevation time from the start of the elevation/volume.
    * We prefer the radial marked as the start of the elevation/volume,
    * but will take the earliest one, in case that one is missing.
    */

   got_good_time = FALSE; /* start off pessimistic */

   for(i = 0; i < MAX_AZM; i++)
   {
      /* Ignore a bad radial. */

      if(inbuf->radial_is_good[i] == FALSE)
         continue;

      /* Get time from the good radial in seconds */

      radial_time = (((int)inbuf->radial[i].bdh.surv_date - 1) * SECS_PER_DAY) +
                    ((int)inbuf->radial[i].bdh.surv_time / MILSEC);
      
      /* Check if the radial is the first good one */
      if(got_good_time == FALSE)
      {
	 min_time = radial_time;
	 max_time = radial_time;
	 max_index = i;
	 min_index = i;
         got_good_time = TRUE;
      }
      else   /* If it is not the first good radial */
      {
	 if (radial_time < min_time)
	 {
	     /* min_time is the oldest time in the sweep */
	     min_time = radial_time;
	     min_index = i;
	 }
	 else if (radial_time >= max_time)
	 {
	     /* max_time is the oldest time in the sweep */
	     max_time = radial_time;
	     max_index = i;
	 }
      } /* end if first good radial */
   } /* end check over all radials */

   *start_elev_date = (int) inbuf->radial[min_index].bdh.surv_date;
   *start_elev_time = (int) inbuf->radial[min_index].bdh.surv_time;
   *end_elev_date   = (int) inbuf->radial[max_index].bdh.surv_date;
   *end_elev_time   = (int) inbuf->radial[max_index].bdh.surv_time;
   *surv_bin_size   = (int) inbuf->radial[min_index].bdh.surv_bin_size;

   if (SPOT_BLANK_VOLUME & inbuf->radial[min_index].bdh.spot_blank_flag)
      *spotblank = 1;
   else
      *spotblank = 0;

   #ifdef QPERATE_DEBUG
    int year_start   = 0; 
    int month_start  = 0; 
    int day_start    = 0; 
    int hour_start   = 0; 
    int minute_start = 0; 
    int second_start = 0;

    int year_end   = 0; 
    int month_end  = 0; 
    int day_end    = 0; 
    int hour_end   = 0; 
    int minute_end = 0; 
    int second_end = 0; 
    
    /* Convert min/max times from s to m/d/y H:M:S */

    sprintf(msg, "%s Bin size %d : Spot blank flag %d\n",
                "read_HeaderData:", (int) inbuf->radial[min_index].bdh.surv_bin_size,
                inbuf->radial[min_index].bdh.spot_blank_flag);
        fprintf(stderr, msg);

    RPGCS_unix_time_to_ymdhms(min_time, &year_start, &month_start, &day_start,
			      &hour_start, &minute_start, &second_start);
    RPGCS_unix_time_to_ymdhms(max_time, &year_end, &month_end, &day_end,
			      &hour_end, &minute_end, &second_end);

    sprintf(msg, "%s Start(Az %6.2f): %.2d/%.2d/%d %.2d:%.2d:%.2d   End(Az %6.2f): "
                 "%.2d/%.2d/%d %.2d:%.2d:%.2d\n",
                "read_HeaderData:", (double)inbuf->radial[min_index].bdh.azimuth, month_start, day_start,
		year_start, hour_start, minute_start, second_start,
		(double)inbuf->radial[max_index].bdh.azimuth, month_end, day_end, year_end, hour_end, 
                minute_end, second_end);
   	fprintf(stderr, msg);

    sprintf(msg, "%s Start(i= %d): date: %d time (msec) %d   End(i= %d): date: %d time (msec) %d\n",
                "read_HeaderData:", min_index, *start_elev_date, *start_elev_time,
		max_index, *end_elev_date, *end_elev_time);
   	fprintf(stderr, msg);
   #endif



} /* end read_HeaderData() ================================ */

/******************************************************************************
    Filename: qperate_read_Moments.c

    Description:
    ============
    read_Moments() reads the dual-pol moments from the DP_MOMENTS_ELEV
    intermediate product for one [azm][bin].

    Moment reading code is based on RPGCS_radar_data_conversion() in:

       ~/src/cpc101/lib004/rpgcs_data_conversion.c

    Note: Fetching moment data on demand won't work if we're averaging Zdr,
          which requires a range of Zdr values around a bin.

    Inputs:
       Compact_dp_basedata_elev* inbuf - pointer to the input data.
       int                       azm   - azimuth you want the moments for
       int                       bin   - bin you want the moments for

    Outputs:
       Moments_t*       bin_moments     - one [azm][bin] filled with moments
       unsigned short*  max_no_of_gates - maximum number of gates for all the moments

    Change History
    ==============
    DATE        VERSION    PROGRAMMER         NOTES
    --------    -------    ----------         ----------------------
    20080311    0000       James Ward         Initial version
    20080515    0001       James Ward         Genericized it more,
                                              as KDP and RHO
                                              went to SHORT_MOMENT_DATA.
    20080515    0002       James Ward         Moved melting layer moments
                                              into 1 beam_center_top read
                                              by build_RR_Polar_Grid()
    20101221    0003       James Ward         Added attenuation flag
******************************************************************************/

void read_Moments(Compact_dp_basedata_elev* inbuf, int azm, int bin,
                  Moments_t* bin_moments, unsigned short* max_no_of_gates)
{
   /* Initialize. The melting layer follows azm, and is independent of bin,
    * but this bin could be the first time we've touched this azm. */

   bin_moments->CorrelCoef = QPE_NODATA;
   bin_moments->Zdr        = QPE_NODATA;
   bin_moments->Z          = QPE_NODATA;
   bin_moments->Kdp        = QPE_NODATA;
   bin_moments->HydroClass = (short) QPE_NODATA; /* AEL 3.1.1 */
   bin_moments->attenuated = FALSE;

   /* Ignore a bad radial */

   if(inbuf->radial_is_good[azm] == FALSE)
      return;

   /* 1. DP_MOMENTS_ELEV holds 11 moments, but QPE doesn't use:
    *    PHI, SDP, SDZ, SMV, or SNR.
    * 2. ML requires special handling, as we extract 4 values instead of
    *    just the 1 bin. It is SHORT_MOMENT_DATA, as BYTE_MOMENT_DATA
    *    was found to be too small for the precision needed.
    * 3. ML, KDP, and RHO are SHORT_MOMENT_DATA, all others are
    *    BYTE_MOMENT_DATA.
    * 4. ML and EHC accept 0, 1 as valid values, all others replace with
    *    QPE_NODATA.
    * 5. Along the way we collect the largest gate statistic.
    *
    * 20090323 Ward HCA no longer sends 0, 1 as valid hydroclasses. */

   bin_moments->HydroClass = (short) RPGC_NINT(
                             get_moment_value(inbuf->radial[azm].ehc_moment,
                             bin, QPE_NODATA, QPE_NODATA, max_no_of_gates));

   bin_moments->Kdp        = get_moment_value(inbuf->radial[azm].kdp_moment,
                             bin, QPE_NODATA, QPE_NODATA, max_no_of_gates);

   bin_moments->CorrelCoef = get_moment_value(inbuf->radial[azm].rho_moment,
                             bin, QPE_NODATA, QPE_NODATA, max_no_of_gates);

   bin_moments->Z          = get_moment_value(inbuf->radial[azm].smz_moment,
                             bin, QPE_NODATA, QPE_NODATA, max_no_of_gates);

   bin_moments->Zdr        = get_moment_value(inbuf->radial[azm].zdr_moment,
                             bin, QPE_NODATA, QPE_NODATA, max_no_of_gates);

   if(inbuf->radial[azm].bdh.msg_type & HIGH_ATTENUATION_TYPE)
      bin_moments->attenuated = TRUE;

} /* end read_Moments() ==================================================== */

/******************************************************************************
    Filename: qperate_read_Moments.c

    Description:
    ============
    get_moment_value() gets a moment value from the input buffer.

    Inputs:
       unsigned char* input       - pointer to the generic moment
       int            bin         - bin you want the moments for
       float          zero_value  - what to return if the moment is 0.
       float          one_value   - what to return if the moment is 1.

    Outputs:
       unsigned short* max_no_of_gates - maximum number of gates for all moments

    Returns:
       The moment value

    For some moments, 0 and 1 are valid values. For other moments, they
    indicate a NO DATA condition.

    Change History
    ==============
    DATE        VERSION    PROGRAMMER         NOTES
    --------    -------    ----------         ----------------------
    20080311    0000       James Ward         Initial version
******************************************************************************/

float get_moment_value(unsigned char* input,      int bin,
                       float          zero_value, float one_value,
                       unsigned short* max_no_of_gates)
{
   float             val = QPE_NODATA;
   Generic_moment_t* gm  = (Generic_moment_t*) input;

   if(gm->no_of_gates <= bin) /* moment not be filled out to enough bins */
   {
      val = QPE_NODATA;
   }
   else if(gm->data_word_size == BYTE_MOMENT_DATA)
   {
      if(gm->gate.b[bin] == 0)
         val = zero_value;
      else if(gm->gate.b[bin] == 1)
         val = one_value;
      else
         val = ((float) gm->gate.b[bin] - gm->offset) / gm->scale;
   }
   else /* SHORT_MOMENT_DATA */
   {
      if(gm->gate.u_s[bin] == 0)
         val = zero_value;
      else if(gm->gate.u_s[bin] == 1)
         val = one_value;
      else
         val = ((float) gm->gate.u_s[bin] - gm->offset) / gm->scale;
   }

   /* Collect the largest gate statistic */

   if(gm->no_of_gates > *max_no_of_gates)
      *max_no_of_gates = gm->no_of_gates;

   return(val);

} /* end get_moment_value() ================================ */

/******************************************************************************
    Filename: qperate_read_Moments.c

    Description:
    ============
       print_Moment() prints a moment for debugging purposes.

    Inputs: int        azm         - the azimuth
            int        bin         - the bin
            Moments_t* bin_moments - the moment to be printed

    Return: The moment printed to stdout

    Change History
    ==============
    DATE        VERSION   PROGRAMMER          NOTES
    --------    -------   ----------          ---------------
    20110113    0001      James Ward          Initial version
******************************************************************************/

void print_Moment(int azm, int bin, Moments_t* bin_moments)
{
  fprintf(stderr, "-------------------------------------------\n");
  fprintf(stderr, "       azm: %d\n",  azm);
  fprintf(stderr, "       bin: %d\n",  bin);
  fprintf(stderr, "attenuated: %hd\n", bin_moments->attenuated);
  fprintf(stderr, "CorrelCoef: %f\n",  bin_moments->CorrelCoef);
  fprintf(stderr, "HydroClass: %hd\n", bin_moments->HydroClass);
  fprintf(stderr, "       Kdp: %f\n",  bin_moments->Kdp);
  fprintf(stderr, "         Z: %f\n",  bin_moments->Z);
  fprintf(stderr, "       Zdr: %f\n",  bin_moments->Zdr);

} /* end print_Moment() ================================ */

