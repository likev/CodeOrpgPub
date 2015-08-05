/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2011/02/24 14:37:34 $
 * $Id: hca_main.c,v 1.7 2011/02/24 14:37:34 steves Exp $
 * $Revision: 1.7 $
 * $State: Exp $
*/

/******************************************************************************
 *      Module:         main                                                  *
 *      Author:         Brian Klein, Yukuan Song                              *
 *      Created:        May 2007                                              *
 *      References:     NSSL Source code in C++                               *
 *                      ORPG HCA AEL                                          *
 *                                                                            *
 *      Description:    This is the main function of HCA algorithm            *
 *                                                                            *
 *      Change History:                                                       *
 ******************************************************************************/

#include <rpgc.h>
#include "hca_local.h"
#include "hca_adapt.h"
#include "alg_adapt.h"

#define WAITING_TIME 8 /* in second */
#define EXTERN 
#include "hca_adapt_object.h"

int main( int argc, char *argv[] ){

    int rc; /*-- return from function call --*/
   
    /* Specify inputs and outputs */
    RPGC_reg_io( argc, argv );

    /* MLDA(328) is an optional input, waiting 1 sec before treated as unavailable */
    RPGC_in_opt(MLDA,WAITING_TIME);

    /* Register scan summary array */
    RPGC_reg_scan_summary();

    /* Register for hca adaptation data. */
    /*    rc = RPGC_reg_ade_callback(hca_callback_fx,
		          &hca_adapt,
			  "alg.hca",
			  ADPT_UPDATE_BOV );*/
    rc = Hca_callback_fx((void*)&hca_adapt);
 
    if(rc < 0 )
   {
      RPGC_log_msg ( GL_ERROR,
      "HCA: cannot register adaptation data callback function\n" );
   }

    /* Tell system we are ready to go ...... */
    RPGC_task_init(VOLUME_BASED, argc, argv );

    /* Do Forever .... */
    while(1) {

        RPGC_wait_act( WAIT_DRIVING_INPUT);
        rc = Hca_callback_fx((void*)&hca_adapt);
        Hca_buffer_control();

    }
    return 0;
} 


