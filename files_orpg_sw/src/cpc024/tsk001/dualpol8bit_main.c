/* RCS info */
/* $Author: ccalvert $ */
/* $Locker:  $ */
/* $Date: 2010/03/10 19:12:17 $ */
/* $Id: dualpol8bit_main.c,v 1.4 2010/03/10 19:12:17 ccalvert Exp $ */
/* $Revision: 1.4 $ */
/* $State: Exp $ */

#include <dualpol8bit.h>

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
    strcpy(OUTDATA_NAME[0], "ZDR8BIT");
    strcpy(OUTDATA_NAME[1], "CC8BIT");
    strcpy(OUTDATA_NAME[2], "HC8BIT");
    strcpy(OUTDATA_NAME[3], "KDP8BIT");
#ifdef DUALPOL8BIT_TEST
    strcpy(OUTDATA_NAME[0], "SMZ8BIT"); /*non-operational*/
    strcpy(OUTDATA_NAME[1], "SNR8BIT"); /*non-operational*/
    strcpy(OUTDATA_NAME[2], "PHI8BIT"); /*non-operational*/
    strcpy(OUTDATA_NAME[3], "SDZ8BIT"); /*non-operational*/
#endif

    for( p = 0; p < N8BIT_PRODS; p++ ) {
       if( (Prod_id[p] = RPGC_get_id_from_name( OUTDATA_NAME[p] )) < 0 ){
          RPGC_log_msg( GL_ERROR, "RPGC_get_id_from_name() Failed #%d...check cfg files.\n",p );
          RPGC_hari_kiri();
       }
    } /* end for all output products */

    /* Do Forever .... */
    while(1){
        RPGC_wait_act( WAIT_DRIVING_INPUT );
        Dualpol8bit_buffer_control();
    }

    return 0;

/* End of main() */
} 

