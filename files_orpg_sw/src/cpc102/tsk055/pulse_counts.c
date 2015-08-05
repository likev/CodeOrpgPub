/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2005/08/15 17:32:16 $
 * $Id: pulse_counts.c,v 1.3 2005/08/15 17:32:16 steves Exp $
 * $Revision: 1.3 $
 * $State: Exp $
*/


#include <pulse_counts.h>

/* Global Variables. */
extern int Verbose_mode;

/* Static global variables. */
/* PRTs defined in number of 9.6 MHz clock counts. */
static int Pricnt[5][8] = { { 29440, 21248, 14720, 11072, 9344, 8640, 8000, 7360 },
                            { 29632, 21376, 14848, 11136, 9408, 8704, 8064, 7424 },
                            { 29824, 21504, 14912, 11200, 9472, 8768, 8128, 7488 },
                            { 29952, 21632, 14976, 11264, 9536, 8832, 8192, 7552 },
                            { 30144, 21760, 15104, 11328, 9600, 8896, 8256, 7616 } };


/*\/////////////////////////////////////////////////////////////////////////////
//
//   Description:
//      Given the azimuth rate (deg/s) and PRI (4 - 8), determine the pulse counts
//      such that one radial of data does not subtend less than MIN_AZI_WIDTH or
//      more than MAX_AZI_WIDTH.
//
//   Inputs:
//      az_rate - azimuth rate (deg/s)
//      pri - pulse repetition interval number (4 <= pri <= 8)
//
//   Returns:
//      The number of pulses where MIN_AZI_WIDTH <= Azi Width <= MAX_AZI_WIDTH
//
/////////////////////////////////////////////////////////////////////////////\*/
int Pulse_cnt_CD( float az_rate, int pri ){

   int    i, count = 0, p_cnt, num_radials;
   double one_radial_az_width;
   double dopprt, p_cnt_d, sum_p_cnt_d = 0.0;
   

   /* Check for valid PRI.  If invalid, return 0. */
   if( (pri < MIN_DOP_PRI) || (pri > MAX_DOP_PRI) ) 
      return 0;

   for( i = 0; i < MAX_PRICNT; i++ ){

      dopprt = (double) Pricnt[i][ pri-1 ]; 
      dopprt = dopprt/CLK_RATE;
      p_cnt_d = 1.0/(az_rate*dopprt);
      sum_p_cnt_d += p_cnt_d;

      count++;

   }

   p_cnt = (int) (sum_p_cnt_d/count);
   one_radial_az_width = az_rate*dopprt*p_cnt;
   if( one_radial_az_width < MIN_AZI_WIDTH )
      p_cnt++;
  
   else if( one_radial_az_width > MAX_AZI_WIDTH )
      p_cnt--;

   if( Verbose_mode ){

      one_radial_az_width = az_rate*dopprt*p_cnt;
      num_radials = 360.0/one_radial_az_width;

      fprintf( stdout, "Radial width is %f deg.\n", one_radial_az_width );
      fprintf( stdout, "Number of radials per elevation cut: %d\n", num_radials ); 

   }

   return( p_cnt );
   
}

/*\/////////////////////////////////////////////////////////////////////////////
//
//   Description:
//      Given the azimuth rate (deg/s) and PRI (1 - 3), determine the pulse counts
//      such that one radial of data does not subtend less than MIN_AZI_WIDTH or
//      more than MAX_AZI_WIDTH.
//
//   Inputs:
//      az_rate - azimuth rate (deg/s)
//      pri - pulse repetition interval number (1 <= pri <= 3)
//
//   Returns:
//      The number of pulses where MIN_AZI_WIDTH <= Azi Width <= MAX_AZI_WIDTH
//
/////////////////////////////////////////////////////////////////////////////\*/
int Pulse_cnt_CS( float az_rate, int pri ){

   int    p_cnt, num_radials, i;
   double one_radial_az_width;
   double survprt, sum_p_cnt_d = 0.0, p_cnt_d;

   /* Check for valid PRI.  If invalid, return 0. */
   if( (pri < MIN_SURV_PRI) || (pri > MAX_SURV_PRI) ) 
      return 0;

   for( i = 0; i < MAX_PRICNT; i++ ){

      survprt = (double) Pricnt[i][ pri-1 ];
      survprt = survprt/CLK_RATE;
      p_cnt_d = 1.0/(az_rate*survprt);
      sum_p_cnt_d += p_cnt_d;

   }

   p_cnt = (int) ((sum_p_cnt_d/MAX_PRICNT) + 0.5);
   one_radial_az_width = az_rate*survprt*p_cnt;

   if( one_radial_az_width < MIN_AZI_WIDTH )
      p_cnt++;
  
   else if( one_radial_az_width > MAX_AZI_WIDTH )
      p_cnt--;

   if( Verbose_mode ){

      one_radial_az_width = az_rate*survprt*p_cnt;
      num_radials = 360.0/one_radial_az_width;

      fprintf( stdout, "Radial width is %f deg.\n", one_radial_az_width );
      fprintf( stdout, "Number of radials per elevation cut: %d\n", num_radials ); 

   }

   return( p_cnt );
   
}

/*\/////////////////////////////////////////////////////////////////////////////
//
//   Description:
//      Given the azimuth rate (deg/s), Surveillance PRI (1 - 3), Doppler PRI 
//      (4 - 8) and Surveillance pulse counts, determine the pulse counts
//      such that one radial of data does not subtend less than MIN_AZI_WIDTH or
//      more than MAX_AZI_WIDTH.
//
//   Inputs:
//      az_rate - azimuth rate (deg/s)
//      spri - Surveillance pulse repetition interval number (1 <= pri <= 3)
//      spc - Suveillance pulse counts
//      dpri - Doppler pulse repetition interval number (1 <= pri <= 3)
//
//   Returns:
//      The number of pulses where MIN_AZI_WIDTH <= Azi Width <= MAX_AZI_WIDTH
//
/////////////////////////////////////////////////////////////////////////////\*/
int Pulse_cnt_BATCH( float az_rate, int spri, int spc, int dpri ){

   double surv_dwell, one_pulse_az_width, one_radial_az_width;
   double dopprt, survprt;
   double sum_p_cnt_d = 0, p_cnt_d;
   int    i, p_cnt, num_radials;

   /* Check for valid PRI.  If invalid, return 0. */
   if( (dpri < MIN_DOP_PRI) || (dpri > MAX_DOP_PRI) ) 
      return 0;

   /* Check for valid PRI.  If invalid, return 0. */
   if( (spri < MIN_SURV_PRI) || (spri > MAX_SURV_PRI) ) 
      return 0;

   for( i = 0; i < MAX_PRICNT; i++ ){

      dopprt = ((double) Pricnt[i][ dpri-1 ]) / CLK_RATE;
      survprt = ((double) Pricnt[i][ spri-1 ])/ CLK_RATE;

      /* For BATCH, account for transisition from low PRF to high PRF
         (i.e., add 1 low PRF interval to surveillance dwell time). */
      surv_dwell = survprt*((double) (spc+1));
      one_pulse_az_width = az_rate*dopprt;
      p_cnt_d = ((1.0 - (surv_dwell*az_rate))/one_pulse_az_width); 

      sum_p_cnt_d += p_cnt_d;

   }

   p_cnt = (int) ((sum_p_cnt_d/MAX_PRICNT) + 0.5);
   one_radial_az_width = az_rate*survprt*p_cnt;

   if( one_radial_az_width < MIN_AZI_WIDTH )
      p_cnt++;
  
   else if( one_radial_az_width > MAX_AZI_WIDTH )
      p_cnt--;

   if( Verbose_mode ){

      one_radial_az_width = az_rate*(dopprt*(p_cnt-1) + surv_dwell);
      num_radials = 360.0/one_radial_az_width;

      fprintf( stdout, "Radial width is %f deg.\n", one_radial_az_width );
      fprintf( stdout, "Number of radials per elevation cut: %d\n", num_radials ); 

   }

   return( p_cnt );

}

