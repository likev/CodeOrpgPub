/* RCS info */
/* $Author: steves $ */
/* $Locker:  $ */
/* $Date: 2007/06/18 23:19:47 $ */
/* $Id: hybrprod_main.c,v 1.1 2007/06/18 23:19:47 steves Exp $ */
/* $Revision: 1.1 $ */
/* $State: Exp $ */

#include <hybrprod.h>

/*************************************************************************

    Description: This module contains the main function for the 
	         Hybrid Scan Product task (HSR and DHR products) 

*************************************************************************/
int main( int argc, char *argv[] ){

    /* Local variables */
    int rc;

    /* Register inputs and outputs. */
    RPGC_reg_io (argc, argv );

    /* Register ITC input */
    RPGC_itc_in( A3136C3, (char *) &A3136c3.tbupdt, sizeof(A3136C3_t), HYBRSCAN );

    /* Register adapdation blocks and scan summary array. */
    RPGC_reg_color_table( &Color_table,  ADPT_UPDATE_BOV );
    RPGC_reg_scan_summary();

    /* register adaptation data callback functions */
    rc = RPGC_reg_ade_callback( hydromet_adj_callback_fx,
                                &Hyd_adj,
                                HYDROMET_ADJ_DEA_NAME,
                                BEGIN_VOLUME );
    if( rc < 0 ){

      RPGC_log_msg( GL_ERROR, "HYBRPROD: Cannot Register Adaptation Data Callback (%d)\n", rc );
      exit(0);

    }

    RPGC_reg_site_info( &Siteadp.rda_lat );

    /* Initialize this task.  This task is volume-based. */
    RPGC_task_init( TASK_VOLUME_BASED, argc, argv );

    /* The main loop, which will never end. */
    while(1){ 

       RPGC_wait_act( WAIT_DRIVING_INPUT );
       A31431_buffer_controller();

    }

} 
