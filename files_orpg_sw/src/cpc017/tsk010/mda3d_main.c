/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/05/13 20:11:03 $
 * $Id: mda3d_main.c,v 1.12 2014/05/13 20:11:03 steves Exp $
 * $Revision: 1.12 $
 * $State: Exp $
 */

/******************************************************************************
 *	Module:         mda3d_main.c					      *
 *	Author:		Yukuan Song					      *
 *   	Created:	January 10, 2003				      *
 *	References:	WDSS MDA Fortran Source code			      *	
 *			AEL						      *
 *									      *
 *      Description:    This routine is the driver function for 3D task       *
 *									      *
 *	Input:		2D features			          	      *
 *	Output:		3D couplets				              * 
 *      returns:     	one                    				      *
 *      Globals:     	none						      *
 *      Notes:       							      *
 ******************************************************************************/

/*	System include file	*/

/*	local include file	*/

#include "mda3d_main.h"
#include "mda3d_acl.h"

#include <mda_adapt.h>
#define EXTERN       /* Causes instantiation of adaptation data object here */
#include "mda_adapt_object.h" 


#define 	PROCESS	1	   /* A boolean which controls the algorithm loop */ 
#define 	SCIT_TIMEOUT_SEC 10 /* Seconds to wait for optional input          */
#define 	VIRTUAL_STRM_DEPTH -999.0 /* initialize the storm depth */

/* declare global parameters */
        cplt_t mda_sf[MESO_MAX_FEAT][MESO_NUM_ELEVS];
        cplt_t cplt[MESO_MAX_NCPLT][MESO_NUM_ELEVS];
	int mda_num_sf[MESO_NUM_ELEVS];
        int current_length;
        int nbr_cplt;
        mda_th_xs_t mda_th_xs[MESO_MAX_NCPLT][MESO_NUM_ELEVS];
	int ncf[MESO_MAX_NCPLT];

	float mda_strm_depth;

	int MDA_2D_MAX_RANK;
	int num_elev;
	float elev_val[MESO_NUM_ELEVS];

	int ttime[MESO_NUM_ELEVS], tdate[MESO_NUM_ELEVS];

	float meso_vd_thr[BASEDATA_DOP_SIZE][99];
        float meso_shr_thr[BASEDATA_DOP_SIZE][99];

int main(int argc,char* argv[]) {

	int rc = 0;		/* return code from function call */

        /* initialize current_length */
	current_length = 0;

	/* set out rank-threshold arrays */
        mda3d_set_table();

        /* Initialize variables */
        mda_strm_depth = VIRTUAL_STRM_DEPTH;
	nbr_cplt = 0;
        num_elev = 0;
	MDA_2D_MAX_RANK = 0;


	LE_send_msg( GL_INFO, "Begin MDA 3D Algorithm ...\n" );

  	/* Register an intermediate product MDA_2D as algorithm input.*/
  	RPGC_in_data(MDA2D,ELEVATION_DATA);

  	/* Register intermediate product CENTATTR as optional input.    */
        RPGC_in_data(CENTATTR, VOLUME_DATA);
        RPGC_in_opt(CENTATTR, SCIT_TIMEOUT_SEC);        

  	/* Output will be put in the linear buffer assigned using the *
  	 * constant MDA3D (see include file) .                        */ 
  	RPGC_out_data(MDA3D, ELEVATION_DATA, INT_PROD);
  
	/*Register site adaptation data */
        rc = RPGC_reg_site_info( &site_adapt  );
        if ( rc < 0 )
        {
          RPGC_log_msg( GL_ERROR, "SITE INFO: cannot register adaptation data callback function\n");
        }

	/* Register algorithm adaptation data */
	rc = RPGC_reg_ade_callback( mda_callback_fx,
                                    &mda_adapt,
                                    MDA_DEA_NAME,
                                    BEGIN_VOLUME );
	if ( rc < 0 )
        {
	  RPGC_log_msg( GL_ERROR, "MDA: cannot register adaptation data callback function\n");
        }

  	/* ORPG task initialization routine. */
	/* MDA always start from the begining of a volume */
  	RPGC_task_init(VOLUME_BASED,argc,argv);

	LE_send_msg( GL_INFO, "Algorithm initialized\n" );
	LE_send_msg( GL_INFO, "and procedding to control loop\n" );

  	/* while loop that controls how long the task will execute. *
  	 * As long as PROCESS remains true, the task will continue. *
         * The task will terminate upon an error condition or       *
  	 * if a prespecified limit has been reached                 */
  	while(PROCESS) {

  	/* Block algorithm until good data is received */
  	RPGC_wait_act(WAIT_DRIVING_INPUT);

  	/* Runing into the algorithm control loop */
  	mda3d_acl();
        } /* END of while(PROCESS) */
} 	/* END of main function                                              */
