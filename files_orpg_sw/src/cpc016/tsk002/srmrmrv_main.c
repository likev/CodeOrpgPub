/* RCS info */
/* $Author: steves $ */
/* $Locker:  $ */
/* $Date: 2007/06/06 20:19:28 $ */
/* $Id: srmrmrv_main.c,v 1.1 2007/06/06 20:19:28 steves Exp $ */
/* $Revision: 1.1 $ */
/* $State: Exp $ */

#include <srmrmrv.h>

/*************************************************************************

   Description: 
      This module contains the main function for the 
      Storm Relative Mean Radial Velocity product. 

*************************************************************************/
int main( int argc, char *argv[] ){

    /* Initialize log error services. */
    RPGC_init_log_services( argc, argv );

    /* Register Inputs/Outputs. */
    RPGC_reg_io( argc, argv );

    /* Register adapdation blocks and scan summary array. */
    RPGC_reg_color_table( &Colrtbl, BEGIN_VOLUME );

    /* Register for scan summary updates. */
    RPGC_reg_scan_summary();

    /* Register ITC input. */
    RPGC_itc_in( A3CD09, &A3cd09, sizeof(a3cd09), ITC_ON_CALL );

    /* Initialize this task. */
    RPGC_task_init( ELEVATION_BASED, argc, argv );

    /* Get the product ID of the outputs. */
    Srmrvreg_id = RPGC_get_id_from_name( "SRMRVREG" );
    Srmrvmap_id = RPGC_get_id_from_name( "SRMRVMAP" );
    Srmrvreg_code = RPGC_get_code_from_name( "SRMRVREG" );
    Srmrvmap_code = RPGC_get_code_from_name( "SRMRVMAP" );

    if( (Srmrvreg_id < 0) || (Srmrvmap_id < 0) 
                          ||
        (Srmrvreg_code <= 0) || (Srmrvmap_code <= 0) ){

        RPGC_log_msg( GL_ERROR, "Unable to get Product IDs or codes.\n" );
        RPGC_abort_task();

    }

    /* The main loop, which will never end. */
    while(1){

        RPGC_wait_act( WAIT_DRIVING_INPUT );
        Srmrmrv_buffer_control();

    }
    
/* End of main(). */
} 
