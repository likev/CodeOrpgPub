/* RCS info */
/* $Author: steves $ */
/* $Locker:  $ */
/* $Date: 2006/09/12 15:03:35 $ */
/* $Id: bvel8bit_main.c,v 1.1 2006/09/12 15:03:35 steves Exp $ */
/* $Revision: 1.1 $ */
/* $State: Exp $ */

#include <bvel8bit.h>

/*************************************************************************

    Description:
        The main function for the Base Velocity Data Array (256 level) 
        Product Generation task.

    Inputs:
        argc - the number of command line arguments.
        argv - the command line arguments.

    Returns.
        Always returns 0.

*************************************************************************/
int main( int argc, char *argv[] ){

    /* Specify inputs and outputs */
    RPGC_reg_io( argc, argv );

    /* Register scan summary array */
    RPGC_reg_scan_summary();

    /* Register for site info adaptation data. */
    RPGC_reg_site_info( &Siteadp.rda_lat );

    /* Tell system we are ready to go ...... */
    RPGC_task_init( ELEVATION_BASED, argc, argv );

    /* Get the product ID of "BVEL8BIT" */
    if( (DV_prod_id = RPGC_get_id_from_name( "BVEL8BIT" )) < 0 ){

        RPGC_log_msg( GL_ERROR, "RPGC_get_id_from_name() Failed\n" );
        RPGC_hari_kiri();

    }

    /* Do Forever .... */
    while(1){

        RPGC_wait_act( WAIT_DRIVING_INPUT );
        BVEL8bit_buffer_control();

    }

    return 0;

/* End of main() */
} 

