#include <orpg.h>
#include <dpprep_isdp.h>
#include <infr.h>


int main( int argc, char *argv[] ){

   int ret;
   Dpp_isdp_est_t Isdp_est;

   /* Open the LB. */
   if( (ret = ORPGDA_open( DP_ISDP_EST, LB_READ )) < 0 ){
  
      fprintf( stderr, "ORPGDA_open( DP_ISDP_EST ) Failed (%d)\n", ret );
      return -1;

   }

   /* Read the LB. */
   ret = ORPGDA_read( DP_ISDP_EST, (char *) &Isdp_est.isdp_est, 
                      sizeof(Dpp_isdp_est_t), DP_ISDP_EST_MSGID );

   /* Display error message. */
   if( ret <= 0 ){

      fprintf( stderr, "ORPGDA_read Failed (%d)\n", ret );
      return -1;

   }
   
   /* Write the information to screen. */
   fprintf( stderr, "ISDP Estimate:  %d @ %02d/%02d/%02d %02d:%02d:%02d\n", 
            Isdp_est.isdp_est, Isdp_est.isdp_mm, Isdp_est.isdp_dd, Isdp_est.isdp_yy, 
            Isdp_est.isdp_hr, Isdp_est.isdp_min, Isdp_est.isdp_sec );

   return 0;

}
