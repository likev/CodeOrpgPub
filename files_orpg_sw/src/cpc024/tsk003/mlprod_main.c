/* RCS info */
/* $Author: ccalvert $ */
/* $Locker:  $ */
/* $Date: 2009/03/03 18:48:39 $ */
/* $Id: mlprod_main.c,v 1.2 2009/03/03 18:48:39 ccalvert Exp $ */
/* $Revision: 1.2 $ */
/* $State: Exp $ */

#include <mlprod.h>

/*************************************************************************

    Description:
       The main function for the Melting Layer Product Generation task.
 
    Inputs:
       argc - number of command line arguments.
       argv - the command line arguments.

    Returns:
       Always returns 0.
    
*************************************************************************/
int main( int argc, char *argv[] ){

    /* Initialize log services */
    RPGC_init_log_services(argc,argv);

    /* Specify inputs and outputs */
    RPGC_reg_io( argc, argv );

    /* Register scan summary array */
    RPGC_reg_scan_summary();

    /* Register for site info adaptation data. */
    RPGC_reg_site_info( &Siteadp.rda_lat );

    /* Tell system we are ready to go ...... */
    RPGC_task_init( ELEVATION_BASED, argc, argv );

    strcpy(INDATA_NAME, "DP_BASE_HC_AND_ML");
    strcpy(OUTDATA_NAME, "MLPROD");

    /* Get the product ID for this instance of mlprod */
    if( (Prod_id = RPGC_get_id_from_name( OUTDATA_NAME )) < 0 ){

        RPGC_log_msg( GL_ERROR, "RPGC_get_id_from_name() Failed\n" );
        RPGC_hari_kiri();

    }

    /* Do Forever .... */
    while(1){
        RPGC_wait_act( WAIT_DRIVING_INPUT );
        mlprod_buffer_control();
    }

    return 0;

/* End of main() */
} 

