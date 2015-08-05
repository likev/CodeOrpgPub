#include <vwindpro.h>

/*\///////////////////////////////////////////////////////////////////
 
   Description:
      Main routine for the VWINDPRO algorithm and product generator.

      Note:  This also includes the enhanced VAD logic.

   Inputs:
      argc - Number of command line arguments.
      argv - The command line arguments.

////////////////////////////////////////////////////////////////////\*/
int main( int argc, char *argv[] ){

    int itc_status;

    /* Initialize log error services. */
    RPGC_init_log_services( argc, argv );

    /* Register I/O. */
    RPGC_reg_io( argc, argv );

    /* Register adapdation blocks, scan summary array 
       and volume status */
    if( RPGC_reg_color_table( (void *) &Color_data, BEGIN_VOLUME ) < 0 ){

        RPGC_log_msg( GL_ERROR, "Cannot Register Color Table\n" );
        RPGC_hari_kiri();

    }

    RPGC_reg_scan_summary();
    RPGC_reg_volume_status( &Vol_stat );
    RPGC_reg_ade_callback( vad_callback_fx, &Vad.thresh_velocity,
	                   VAD_DEA_NAME, BEGIN_VOLUME );
    RPGC_reg_ade_callback( vad_rcm_heights_callback_fx,
                           (char *) &Prodsel.vad_rcm_heights,
                           VAD_RCM_HEIGHTS_DEA_NAME, BEGIN_VOLUME );
    RPGC_reg_site_info( &Siteadp.rda_lat );

    /* Register ITC input and output. */
    RPGC_itc_out( A3CD97, &Ewt.envwndflg, sizeof(A3cd97), 
                  ITC_WITH_EVENT, ORPGEVT_ENVWND_UPDATE);
    RPGC_itc_out( ENVVAD, &Envvad.parameter_from_vad[0], 
                  sizeof(Envvad), ITC_ON_CALL );
    RPGC_itc_in( A3CD97, &Ewt.envwndflg, sizeof(A3cd97), ITC_ON_CALL );

    /* Initialize this task. */
    RPGC_task_init( VOLUME_BASED, argc, argv );

    /* Get the Product IDs. */
    Vadtmhgt_id = RPGC_get_id_from_name( "VADTMHGT" );
    if( Vadtmhgt_id <= 0 ){

        RPGC_log_msg( GL_ERROR, "No Product ID for VADTMHGT\n" );
        RPGC_hari_kiri();

    }
    else
        RPGC_log_msg( GL_INFO, "VADTMHGT ID: --->%d\n", Vadtmhgt_id );

    Vadparam_id = RPGC_get_id_from_name( "VADPARAM" );
    if( Vadparam_id <= 0 ){

        RPGC_log_msg( GL_ERROR, "No Product ID for VADPARAM\n" );
        RPGC_hari_kiri();

    }
    else
        RPGC_log_msg( GL_INFO, "VADPARAM ID: --->%d\n", Vadparam_id );

    Hplots_id = RPGC_get_id_from_name( "HPLOTS" );
    if( Hplots_id <= 0 ){

        RPGC_log_msg( GL_ERROR, "No Product ID for HPLOTS\n" );
        RPGC_hari_kiri();

    }
    else
        RPGC_log_msg( GL_INFO, "HPLOTS ID: --->%d\n", Hplots_id );
  
    /* Allocate space for data structures.  We do this instead of 
       statically defining in order to take advantage of 
       memory debuggers (e.g., valgrind). */

    /* Allocate space for A317vi_t. */
    A317vi = (A317vi_t *) calloc( 1, sizeof(A317vi_t) );
    if( A317vi == NULL ){

        RPGC_log_msg( GL_ERROR, "calloc Failed for %d Bytes\n",
                      sizeof(A317vi_t) );
        RPGC_hari_kiri();

    }

    /* Allocate space for A317va_t. */
    A317va = (A317va_t *) calloc( 1, sizeof(A317va_t) );
    if( A317va == NULL ){

        RPGC_log_msg( GL_ERROR, "calloc Failed for %d Bytes\n",
                      sizeof(A317va_t) );
        RPGC_hari_kiri();

    }

    /* Allocate space for A317vd_t. */
    A317vd = (A317vd_t *) calloc( 1, sizeof(A317vd_t) );
    if( A317vd == NULL ){

        RPGC_log_msg( GL_ERROR, "calloc Failed for %d Bytes\n",
                      sizeof(A317vd_t) );
        RPGC_hari_kiri();

    }

    /* Allocate space for A317vs_t. */
    A317vs = (A317vs_t *) calloc( 1, sizeof(A317vs_t) );
    if( A317vs == NULL ){

        RPGC_log_msg( GL_ERROR, "calloc Failed for %d Bytes\n",
                      sizeof(A317vs_t) );
        RPGC_hari_kiri();

    }

    /* Allocate space for A317ve_t. */
    A317ve = (A317ve_t *) calloc( 1, sizeof(A317ve_t) );
    if( A317ve == NULL ){

        RPGC_log_msg( GL_ERROR, "calloc Failed for %d Bytes\n",
                      sizeof(A317ve_t) );
        RPGC_hari_kiri();

    }

    /* Allocate space for A317ec_t. */
    A317ec = (A317ec_t *) calloc( 1, sizeof(A317ec_t) );
    if( A317ec == NULL ){

        RPGC_log_msg( GL_ERROR, "calloc Failed for %d Bytes\n",
                      sizeof(A317ec_t) );
        RPGC_hari_kiri();

    }

    /* Allocate space for A318ci_t. */
    A318ci = (A318ci_t *) calloc( 1, sizeof(A318ci_t) );
    if( A318ci == NULL ){

        RPGC_log_msg( GL_ERROR, "calloc Failed for %d Bytes\n",
                      sizeof(A318ci_t) );
        RPGC_hari_kiri();

    }

    /* Do initial read on ITC A3cd97 ... EWT data. */
    RPGC_itc_read( A3CD97, &itc_status );
    if( itc_status >= 0 ){

        RPGC_log_msg( GL_INFO, "A3cd97: \n" );
        RPGC_log_msg( GL_INFO, "--->Envwndflg:       %d\n", Ewt.envwndflg );
        RPGC_log_msg( GL_INFO, "--->Sounding Time: %d\n", Ewt.sound_time );

    }

    /* The main loop, which will never end. */
    while(1){

        RPGC_wait_act( WAIT_DRIVING_INPUT );
        A317a2_vad_buffer_control();

    }

} /* End of main() */

