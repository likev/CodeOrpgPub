/* RCS info */
/* $Author: ccalvert $ */
/* $Locker:  $ */
/* $Date: 2009/03/03 18:48:38 $ */
/* $Id: hhc8bit_main.c,v 1.2 2009/03/03 18:48:38 ccalvert Exp $ */
/* $Revision: 1.2 $ */
/* $State: Exp $ */

#include <hhc8bit.h>

/*************************************************************************

    Description:
       The main function for the Hybrid Hydroclass Data Array (256 level) 
       Product Generation task.
 
    Inputs:
       argc - number of command line arguments.
       argv - the command line arguments.

    Returns:
       Always returns 0.
    
*************************************************************************/
int main( int argc, char *argv[] ){
    char argument[65];
    int i;

    /* Initialize log services */
    RPGC_init_log_services(argc,argv);

    /* Specify inputs and outputs */
    RPGC_reg_io( argc, argv );

    /* Register scan summary array */
    RPGC_reg_scan_summary();

    /* Register for site info adaptation data. */
    RPGC_reg_site_info( &Siteadp.rda_lat );

    /* Tell system we are ready to go ...... */
    RPGC_task_init( VOLUME_BASED, argc, argv );

    /* Get taskname so the correct buffers can be requested */
    for( i = 0; i < argc; i++ ) {
        strcpy(argument, argv[i]);
        if(strcmp( argument, "-T") == 0) {
            strcpy(argument, argv[i+1]);
            break;
        }
    }
    if(strcmp(argument, "hhc8bit") == 0) {
        strcpy(INDATA_NAME, "QPEHHC"); 
        strcpy(OUTDATA_NAME, "HHC8BIT");
    } 

    /* Get the product ID for this instance of hhc8bit */
    if( (Prod_id = RPGC_get_id_from_name( OUTDATA_NAME )) < 0 ){

        RPGC_log_msg( GL_ERROR, "RPGC_get_id_from_name() Failed\n" );
        RPGC_hari_kiri();

    }

    /* Do Forever .... */
    while(1){

        RPGC_wait_act( WAIT_DRIVING_INPUT );
        Hhc8bit_buffer_control();

    }

    return 0;

/* End of main() */
} 

