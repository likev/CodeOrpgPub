/* RCS info */
/* $Author: steves $ */
/* $Locker:  $ */
/* $Date: 2007/07/03 22:46:38 $ */
/* $Id: cmprflct_main.c,v 1.1 2007/07/03 22:46:38 steves Exp $ */
/* $Revision: 1.1 $ */
/* $State: Exp $ */

#include <cmprflct.h>

/*************************************************************************
   The main function for the COMPOSITE REFLECTIVITY POLAR GRID task. 
*************************************************************************/
int main( int argc, char *argv[] ){

    /* Specify inputs and outputs */
    RPGC_reg_io( argc, argv );

    /* Tell system we are volume-based. */
    RPGC_task_init( TASK_VOLUME_BASED, argc, argv );

    /* Waiting for activation. */
    while(1){

       RPGC_wait_act( WAIT_DRIVING_INPUT );
       A30741_buffer_control();

    }

    return 0;

/* End of main(). */
}

