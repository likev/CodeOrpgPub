/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2006/12/18 16:13:13 $
 * $Id: combattr.c,v 1.1 2006/12/18 16:13:13 steves Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

#include <combattr.h>

/*************************************************************************

   Description: 
      This module contains the main function for the Combined Attributes 
      Task.
 
**************************************************************************/
int main( int argc, char *argv[] ){

    int rc, i;

    /* Specify inputs and outputs. */
    RPGC_reg_io( argc, argv );

    /* Register adaptation data */
    rc = RPGC_reg_ade_callback( mda_callback_fx, &Mda_adapt,
                                MDA_DEA_NAME, BEGIN_VOLUME );
    if ( rc < 0 )
        RPGC_log_msg( GL_ERROR, "MDA: cannot register adaptation data callback function\n");

    /* Register scan summary array. */
    RPGC_reg_scan_summary();

    /* Initialize this task.  It is volume based. */
    RPGC_task_init( VOLUME_BASED, argc, argv );

    /* Initialize the buffer tracking array. */
    for( i = 0; i < NIBUFS; ++i ) 
	Tibuf[i] = NULL;

    /* Initialize all the data IDs of the inputs to
       this task. */
    Bufmap[MDATTNN_ID] = RPGC_get_id_from_name( "MDATTNN" );
    Bufmap[CENTATTR_ID] = RPGC_get_id_from_name( "CENTATTR" );
    Bufmap[TRFRCATR_ID] = RPGC_get_id_from_name( "TRFRCATR" );
    Bufmap[HAILATTR_ID] = RPGC_get_id_from_name( "HAILATTR" );
    Bufmap[TVSATTR_ID] = RPGC_get_id_from_name( "TVSATTR" );

    /* Verify all the data IDs were initialize.   Commit
       Hari Kiri on error. */
    for( i = 0; i < NIBUFS; i++ ){

        if( Bufmap[i] < 0 ){

            RPGC_log_msg( GL_ERROR, "Bufmap[%d] Not Initialized\n", i );
            RPGC_hari_kiri();

        }

    }

    /* The main loop, which will never end. */
    while(1){

        RPGC_wait_for_any_data( WAIT_ANY_INPUT );
        A30831_buffer_control( );

    }

} 

