/* RCS info */
/* $Author: steves $ */
/* $Locker:  $ */
/* $Date: 2007/01/09 20:44:59 $ */
/* $Id: basspect_main.c,v 1.1 2007/01/09 20:44:59 steves Exp $ */
/* $Revision: 1.1 $ */
/* $State: Exp $ */

#include <basspect.h>

/**************************************************************

   Description:
      Main function for the BASE SPECTRUM WIDTH product task.

**************************************************************/
int main( int argc, char *argv[] ){

    int rc;

    /* Register inputs and outputs. */
    RPGC_reg_io( argc, argv );

    /* Register color table adaptation block. */
    if( (rc = RPGC_reg_color_table( (void *) &color_data, 
                                     BEGIN_VOLUME ) < 0) ){

        RPGC_log_msg( GL_ERROR, "Cannot Register Color Table\n");
        RPGC_hari_kiri();

    }

    /* Register for scan summary updates. */
    RPGC_reg_scan_summary();

    /* Tell system we are elevation-based. */
    RPGC_task_init( ELEVATION_BASED, argc, argv );

    /* Waiting for activation. */
    while(1){ 

        RPGC_wait_act( WAIT_DRIVING_INPUT );
        a30731_buffer_control();

    }

    return 0;

} 
