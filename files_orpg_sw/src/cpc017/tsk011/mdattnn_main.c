/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2011/09/30 22:17:53 $
 * $Id: mdattnn_main.c,v 1.7 2011/09/30 22:17:53 ccalvert Exp $
 * $Revision: 1.7 $
 * $State: Exp $
 */

/******************************************************************************
 *	Module:         mdattnn_main.c					      *
 *	Author:		Brian Klein					      *
 *   	Created:	Jan. 23, 2003					      *
 *	References:	WDSS MDA Fortran Source code			      *	
 *			AEL						      *
 *									      *
 *      Description:    This program uses mda3d output (3D features) and      *
 *                      attempts to associate them with 3D features from      *
 *                      the previous volume scans to produce tracking         *
 *                      information for each 3D feature.                      *
 *									      *
 *      Notes:       							      *
 ******************************************************************************/

/*	System include files	*/
#include "rpgc.h"
#include "a309.h"

/*	Global include files	*/
#include "mdattnn.h"

#define DEBUG     0

	/* global variables */
	float mda_def_u;      /* Default (right moving) mesocyclone U motion. */
        float mda_def_v;      /* Default (right moving) mesocyclone V motion. */


/*      Prototypes              */
void mdattnn_acl(/*FILE*, FILE*, FILE**/);

/* declare global parameters */
#define PROCESS	1	/* A boolean to control the algorithm loop    */ 
#define TVSATTR_RU 290


int main(int argc,char* argv[]) {

        int i;
        FILE* output_track = NULL;
        FILE* output_fort34 = NULL;
        FILE* output_fort767 = NULL;

	LE_send_msg( GL_INFO, "Begin MDA Track/Trend/NN Algorithm ...\n" );

        /* for testing! */
        if( DEBUG ){
           output_track = fopen("output_ttnn", "w");
           output_fort34 = fopen("output_fort34", "w");
           output_fort767 = fopen("output_fort767", "w");
        }

        /* In lieu of alginit.ftn...  */
        for (i = 0; i < MAX_MDA_FEAT; i++){
           Old_cplt[i].meso_event_id = -1;
           new_cplt[i].meso_event_id = -1;
        }
        Nbr_cplt_trks = 0;
        Nbr_old_cplts = 0;
        Old_date      = 0;
        Old_time      = 0;

	/* Initialize varaibles */
	mda_def_u = 0.0;
	mda_def_v = 0.0;

	nbr_prev_newcplt = 0;
	nbr_first_elev_newcplt = 0;

  	/* Register algorithm input.                                    */

  	RPGC_in_data(MDA3D, ELEVATION_DATA);     /* for mda3d data         */

        RPGC_in_data(TRFRCATR, VOLUME_DATA);  /* for avg storm motion   */
        RPGC_in_opt(TRFRCATR, SCIT_TIMEOUT_SEC);

        RPGC_in_data(TVSATTR_RU, ELEVATION_DATA);   /* for TVS locations      */
        RPGC_in_opt(TVSATTR_RU, TDA_TIMEOUT_SEC);

  	/* Register the output data.                                    */

  	RPGC_out_data(MDATTNN, ELEVATION_DATA, INT_PROD);
  
  	/* Register to read the Scan Summary Array                      */

  	RPGC_reg_scan_summary();

  	/* ORPG task initialization routine.                            */
	/* Always start from the begining of a volume.                  */

  	RPGC_task_init(VOLUME_BASED,argc,argv);

	LE_send_msg( GL_INFO, "Algorithm initialized\n" );
	LE_send_msg( GL_INFO, "and proceeding to control loop\n" );

  	/* while loop that controls how long the task will execute.     */
        /* As long as PROCESS remains true, the task will continue.     */
        /* The task will terminate upon an error condition or           */
  	/* if a prespecified limit has been reached                     */

  	while(PROCESS) {

          /* Wait for input data                                        */

          RPGC_wait_act(WAIT_DRIVING_INPUT);

          /* Input received... proceed with the algorithm...            */

          mdattnn_acl(output_track, output_fort34, output_fort767);

        } /* END of while(PROCESS)                                      */

} /* END of main function                                               */
