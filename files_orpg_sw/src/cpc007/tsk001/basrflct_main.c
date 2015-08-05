/* RCS info */
/* $Author: steves $ */
/* $Locker:  $ */
/* $Date: 2007/07/10 15:57:43 $ */
/* $Id: basrflct_main.c,v 1.1 2007/07/10 15:57:43 steves Exp $ */
/* $Revision: 1.1 $ */
/* $State: Exp $ */

#define MAIN
#include <basrflct.h>

/************************************************************************

   Description:
      Main function for the BASE REFLECTIVIY product task.

************************************************************************/
int main( int argc, char *argv[] ){

    int pindx;

    /* Register inputs and outputs. */
    RPGC_reg_io( argc, argv );

    /* Register color table adaptation block. */
    if( RPGC_reg_color_table((void *) &Color_data, BEGIN_VOLUME) < 0 ){

        RPGC_log_msg( GL_ERROR, "Cannot Register Color Table\n");
        RPGC_hari_kiri();

    }

    /* Register for scan summary updates. */
    RPGC_reg_scan_summary();

    /* Tell system we are elevation-based. */
    RPGC_task_init( ELEVATION_BASED, argc, argv );

    /* Initialize the product IDs from the product names. */
    for( pindx = 0; pindx < NUMPRODS; pindx++ ){

        Prod_info[pindx].prod_id = RPGC_get_id_from_name( Prod_info[pindx].pname );
        if( Prod_info[pindx].prod_id <= 0 ){

            RPGC_log_msg( GL_ERROR, "No Product ID for %s\n", Prod_info[pindx].pname );
            RPGC_hari_kiri();

        }

    }

    /* Waiting for activation. */
    while(1){

        RPGC_wait_act( WAIT_DRIVING_INPUT );
        A30711_buffer_control();
  
    }

    return 0;

/* End of main(). */
} 

