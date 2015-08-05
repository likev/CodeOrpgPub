/*****************************************************************
   This file interfaces to the NOVAS libraries to calculate the 
   position of the Sun for Suncheck tests.
**************************************************************** */

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <orpgsun.h>

/* Function Prototypes. */
static void Get_tdt_julian_date( double *current_tdt_julian_date, 
                                 double DELTAT );
static void Get_tdt_julian_date_at_time( double *tdt_julian_date, 
                                         time_t desired_time,
                                         double DELTAT );

/***********************************************************************

   Description:
      This function uses the NOVAS software library to compute
      the current position of the Sun on the local horizon.
 
   Inputs:
      here - The point on the surface of the earth we are at.
  
      deltat - The difference between terrestrial dynamical time
               system 1 time, and universal time.
  
   Outputs:
      *SunAz - The returned sun azimuth, in degrees from true north.
  
      *SunEl - The returned sun elevation, in degrees above the 
               local horizon.
 
**************************************************************************/
void ORPGSUN_NovasComputePos( site_info here, double *SunAz, double *SunEl ){

    /* The earth and sun body structures.  See NOVAS documentation for 
       more information. */
    body earth, sun;

    /* The right ascension, and declination of the sun */
    double ra, declination;

    /* The current terrestrial dynamical time (TDT) julian date */
    double current_tdt_julian_date;

    /* The Sun's zenith distance, measured from the local vertical. */
    double zenith_distance;

    /* The Sun's azimuth is measured from the north */
    double azimuth;

    /* rar -right ascension. decr -declination.  In the equatorial 
       coordinate system */
    double rar, decr;

    /* Distance to the sun in AU. */
    double distanceAU;

    /* Call NOVAS subroutine set_body to setup Sun and Earth body structures. */
    set_body(0, 10, "Sun", &sun);
    set_body(0, 3, "Earth", &earth);

    Get_tdt_julian_date_at_time( &current_tdt_julian_date, time(NULL), 
                                 ORPGSUN_DELTAT );

    /*  Call NOVAS routine topo_planet to get the right ascension and 
        declination of the Sun in topographic/equatorial coordinate system. */
    topo_planet( current_tdt_julian_date, &sun, &earth, ORPGSUN_DELTAT,
                 &here, &ra, &declination, &distanceAU );

    /* Call NOVAS subroutine equ2hor to convert the Sun's position to local 
       horizon coordinates. */
    equ2hor( current_tdt_julian_date, ORPGSUN_DELTAT,
             0.0 /* x_ephemeris */, 
             0.0 /* y_ephemeris */,
             &here, ra, declination,
             1 /* normal refraction */,
             &zenith_distance,
             &azimuth,
             &rar, &decr );

    /* Convert zenith_distance reference point so that the Sun's elevation 
       is referenced to the local horizon. */
    *SunEl = 90.0 - zenith_distance;

    /* Azimuth needs no conversion, its local horizon reference is already 
       true north. */
    *SunAz = azimuth;

}

/*************************************************************************

   Description:
     This function uses the NOVAS software library to compute the position
     of the Sun on the local horizon, but for an input time, instead of the
     current time.
 
   Inputs:
      here - The point on the surface of the earth we are at.
      time_in - The UTC time in seconds that we need to compute
                the Sun's position.
 
   Outputs:
      *SunAz - The returned sun azimuth, in degrees from true north.
      *SunEl - The returned sun elevation, in degrees above the
               local horizon.
 
**************************************************************************/
void ORPGSUN_NovasComputePosAtTime( site_info here, double *SunAz, 
                                    double *SunEl, time_t time_in ){

    /* The earth and sun body structures.  See NOVAS documentation for 
       more information. */
    body earth, sun;

    /* The right ascension, and declination of the sun */
    double ra, declination;

    /* The terrestrial dynamical time (TDT) julian date */
    double tdt_julian_date;

    /* Zenith distance, measured from the local vertical. */
    double zenith_distance;

    /* The Sun's azimuth is measured from the north */
    double azimuth;

    /* rar -right ascension. decr -declination. In equatorial 
       coordinate system */
    double rar, decr;

    /* The distance to the Sun, units of AU, not returned by this function. */
    double distanceAU;

    /* Call NOVAS subroutine set_body to setup Sun and Earth body structures. */
    set_body( 0, 10, "Sun", &sun );
    set_body( 0, 3, "Earth", &earth );

    /* Get the TDT date for the input time. */
    Get_tdt_julian_date_at_time( &tdt_julian_date, time_in, ORPGSUN_DELTAT );

    /* Call NOVAS routine topo_planet to get the right ascension and 
       declination of the Sun in topographic/equatorial coordinate 
       system. */
    topo_planet( tdt_julian_date, &sun, &earth, ORPGSUN_DELTAT, &here,
                 &ra, &declination, &distanceAU );

    /* Call NOVAS subroutine equ2hor to convert the Sun's position to
       local horizon coordinates. */
    equ2hor( tdt_julian_date, ORPGSUN_DELTAT,
             0.0 /* x_ephemeris */, 
             0.0 /* y_ephemeris */,
             &here, ra, declination, 1 /* normal refraction */,
             &zenith_distance,
             &azimuth,
             &rar, &decr );

    /* Convert zenith_distance reference point so that the Sun's
       elevation is referenced to the local horizon. */
    *SunEl = 90.0 - zenith_distance;

    /* Azimuth needs no conversion, its local horizon reference
       is already true north. */
    *SunAz = azimuth;

} /* End of ORPGSUN_NovasComputePosAtTime() */


/***********************************************************************

   Description:
      This function gets the tdt (Terrestrial Dynamic Time, or TT time)
      Julian Date at the time desired.
  
   Outputs:
      tdt_julian_date - The tdt date is returned here, corresponding 
      to the desired time.
 
   Inputs: 
      desired_time - The time in UT1 (UTC) to get julian date for.
                     Can use time(NULL) to get the UTC time on an RDA
 
      DELTA_T - The difference between TT and UT1, maintain from 
                Navy data reports.  Currently about 67 seconds 
                difference between the time scales.

***********************************************************************/
static void Get_tdt_julian_date_at_time( double *tdt_julian_date,
                                         time_t desired_time,
                                         double DELTA_T ){

    /* result (struct tm) - broken down time */
    struct tm result;

    /* Get TDT date for the input time using NOVAS */

    /* time_in is in the UT1 time system (UT1 is approximately UTC)
       Since TT = UT1 + DELTAT. Correct the input desired time,
       to be on the TT time scale. */
    desired_time += DELTA_T;

    /* Now get broken down time on  TT time scale. */
    gmtime_r(&desired_time, &result);

    /* Use the NOVAS routine to get the julian date on TT time scale.
       TT is approximately TDT. */
    *tdt_julian_date = julian_date(
            result.tm_year + 1900, result.tm_mon + 1, result.tm_mday,
            (double) result.tm_hour +
            ((double) result.tm_min) / 60.0 + ((double) result.tm_sec) /
            (3600.0));

} /* End of Get_tdt_julian_date_at_time(). */


/***********************************************************************

   Description:
      This function calculates the current tdt (Terrestrial Dynamic 
      Time) Julian Date.
 
   Outputs:
      current_tdt_julian_date - The current tdt julian date.
  
   DELTA_T - Passed to Get_tdt_julian_date_at_time for description.

***********************************************************************/
static void Get_tdt_julian_date( double *current_tdt_julian_date, 
                                 double DELTA_T ){

    /* The current time in UNIX Epoch (UTC) */
    time_t tnow;

    /* Get the current time. */
    tnow = time(NULL);

    /* Calculate the tdt_julian_date for the current time, using 
       Get_tdt_julian_date_at_time(). */
    Get_tdt_julian_date_at_time( current_tdt_julian_date, tnow, DELTA_T );

} /* End of Get_tdt_julian_date() */
