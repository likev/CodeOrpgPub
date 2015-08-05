

/* RCS info */
/* $Author: steves $ */
/* $Locker:  $ */
/* $Date: 2014/03/14 21:30:44 $ */
/* $Id: alerting.c,v 1.4 2014/03/14 21:30:44 steves Exp $ */
/* $Revision: 1.4 $ */
/* $State: Exp $ */

#define ALERTING
#include <alerting.h>

/* Function Prototypes. */
static void Initialize_alerting();

/*************************************************************************

    The main function for the ALERTING task. 

*************************************************************************/
int main( int argc, char *argv[] ){

    int i;

    /* Initialize log error services. */
    RPGC_init_log_services( argc, argv );

    /* ** Specify inputs and events. */
    RPGC_reg_io( argc, argv );

    /* Specify outputs having events associated with them. */
    Alert_msg_id = RPGC_out_data_by_name_wevent( "ALRTMSG", 
                                                 ORPGEVT_WX_ALERT_MESSAGE );
    Alert_prod_id = RPGC_out_data_by_name_wevent( "ALRTPROD", 
                                                  ORPGEVT_WX_USER_ALERT_MSG );
    
    RPGC_log_msg( GL_INFO, "Alert Message ID: %d, Alert Product ID: %d\n",
                  Alert_msg_id, Alert_prod_id );

    /* Register adapdation blocks. the scan summery array 
       and volume status */
    RPGC_reg_ade_callback( vad_rcm_heights_callback_fx, 
                           (char *) &Prodsel.vad_rcm_heights, 
                           VAD_RCM_HEIGHTS_DEA_NAME, BEGIN_VOLUME );
    RPGC_reg_ade_callback( hydromet_adj_callback_fx, &Hydromet_adj.time_bias, 
                           HYDROMET_ADJ_DEA_NAME, BEGIN_VOLUME );
    RPGC_reg_site_info( &Siteadp.rda_lat );

    /* Register the scan summary array and volume status */
    RPGC_reg_scan_summary();
    RPGC_reg_volume_status( &Vol_stat );

    /* Task is volume based. */
    RPGC_task_init( VOLUME_BASED, argc, argv );

    /* Register for external event (Start of Volume) */
    RPGC_reg_for_external_event( ORPGEVT_START_OF_VOLUME_DATA, 
                                 A30811_alerting_ctl, 
                                 START_OF_VOLUME_SCAN );

    /* Register the internal event (EVT_ANY_INPUT_AVAILABLE) */
    RPGC_reg_for_internal_event( EVT_ANY_INPUT_AVAILABLE, A30811_alerting_ctl, 
                                 INPUT_AVAILABLE );

    /* Get the Product IDs from the product names. */
    for( i = 0; i < NUM_TYPS; i++ ){

        int id = RPGC_get_id_from_name( Valid_bufs[i] );

        if( id <= 0 ){

            RPGC_log_msg( GL_ERROR, "Unable to Get Product ID for %s\n",
                          Valid_bufs[i] );
            exit(0);

        }

        Valid_ids[i] = id;

    }

    /* Call the initialization modules for the functional areas. */
    Initialize_alerting();
    ABC_initialize_alerting_buffer_control();

    /* Wait for events .... */
    while(1)
        RPGC_wait_for_event();

    return 0;

}

/*////////////////////////////////////////////////////////////////////////
//
//   Description:
//      Initialization module for the alerting function.
//
////////////////////////////////////////////////////////////////////////*/
static void Initialize_alerting(){

    int i, j, k;

    /* Do some initialization. */
    for( j = 0; j < NACT_SCNS; j++ ){

        for( k = 0; k < NUM_ALERT_AREAS; k++ ){

            for( i = 0; i < MAX_CLASS1; i++ )
                Uaptr[j][k][i] = 0;

        }

    }

    for( j = 0; j < NACT_SCNS; j++ ){

        for( i = 0; i < MAX_CLASS1; i++ ){

            Uam_ptrs[j][i] = 0;
            Ndx[j][i] = 0;
            Np[j][i] = 0;
            It[j][i] = 0;

        }

    }

    for( j = 0; j < NACT_SCNS; j++ ){

        Num_ebufs[j] = 0;
        Num_dbufs[j] = 0;
        Active_scns[j] = 0;

        for( i = 0; i < NUM_TYPS; i++ ) {

            Bufs_expected[j][i] = 0;
            Bufs_done[j][i] = 0;

        }

    }


/* End of Initialize_alerting(). */
}
