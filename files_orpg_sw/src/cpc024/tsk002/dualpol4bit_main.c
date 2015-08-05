/* RCS info */
/* $Author: ccalvert $ */
/* $Locker:  $ */
/* $Date: 2009/03/03 18:48:37 $ */
/* $Id: dualpol4bit_main.c,v 1.3 2009/03/03 18:48:37 ccalvert Exp $ */
/* $Revision: 1.3 $ */
/* $State: Exp $ */

#include <dualpol4bit.h>

/*************************************************************************

    Description:
       The main function for a Dual Pol Data Array (256 level) 
       Product Generation task.
 
    Inputs:
       argc - number of command line arguments.
       argv - the command line arguments.

    Returns:
       Always returns 0.
    
*************************************************************************/
int main( int argc, char *argv[] ){

    int p;

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

    /* There is only one input for this process */
    strcpy(INDATA_NAME, "DP_BASE_HC_AND_ML");
   
    /* There are multiple outputs for this process.
       Numbering order is arbitrary.                */

    strcpy(OUTDATA_NAME[0], "ZDR4BIT");
    strcpy(OUTDATA_NAME[1], "HC4BIT");
    strcpy(OUTDATA_NAME[2], "CC4BIT");
    strcpy(OUTDATA_NAME[3], "KDP4BIT");

    for( p = 0; p < N4BIT_PRODS; p++ ) {
       if( (Prod_id[p] = RPGC_get_id_from_name( OUTDATA_NAME[p] )) < 0 ){
          RPGC_log_msg( GL_ERROR, "RPGC_get_id_from_name() Failed #%d...check cfg files.\n",p );
          RPGC_hari_kiri();
       }
    } /* end for all output products */

    /* Do Forever .... */
    while(1){
        RPGC_wait_act( WAIT_DRIVING_INPUT );
        Dualpol4bit_buffer_control();
    }

    return 0;

/* End of main() */
} 

