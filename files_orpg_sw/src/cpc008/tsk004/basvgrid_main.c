/* RCS info */
/* $Author: steves $ */
/* $Locker:  $ */
/* $Date: 2007/02/02 19:17:41 $ */
/* $Id: basvgrid_main.c,v 1.1 2007/02/02 19:17:41 steves Exp $ */
/* $Revision: 1.1 $ */
/* $State: Exp $ */

#include <basvgrid.h>

int main( int argc, char *argv[] ){

    RPGC_reg_io( argc, argv );

    /* Tell system this task is elevation based. */
    RPGC_task_init( ELEVATION_BASED, argc, argv );

    /* Waiting for activation. */
    while(1){

        RPGC_wait_act( WAIT_DRIVING_INPUT );
        Basvgrid_buffer_control();

    }

    return 0;

/* End of main(). */
} 

