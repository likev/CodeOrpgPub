/********************************************************************

    This module is for task prfbmap.

********************************************************************/

/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2012/04/05 21:25:54 $
 * $Id: prfbmap_main.c,v 1.1 2012/04/05 21:25:54 steves Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

#define PRFBMAP_MAIN_C
#include <prf_bitmap.h>

/* ******************************************************************** */
/* The main function for the PRF Bit Map product task. */
/* ******************************************************************** */
int main( int argc, char *argv[] ){

    /* Register inputs and outputs */
    RPGC_reg_io( argc, argv );

    /* Register the scan summery array and volume status */
    RPGC_reg_scan_summary();
    RPGC_reg_volume_status( &Vol_stat );

    /* Initialize supporting environment. */
    RPGC_task_init( ELEVATION_BASED, argc, argv );

    /* Task activation loop */
    while(1){

       RPGC_wait_act( WAIT_DRIVING_INPUT );
       A30541_buffer_control();

    }

    exit (0);

/* End of main(). */
}
