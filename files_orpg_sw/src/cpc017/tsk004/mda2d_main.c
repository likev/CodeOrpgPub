/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/03/19 19:56:48 $
 * $Id: mda2d_main.c,v 1.6 2009/03/19 19:56:48 ccalvert Exp $
 * $Revision: 1.6 $
 * $State: Exp $
 */

/*1234567891234567891234567891234567891234567891234567891234567891234567891234*/
/******************************************************************************
 *	Module:         mda2d_main.c					      *
 *	Author:		Yukuan Song					      *
 *   	Created:	Oct. 30, 2002					      *
 *	References:	WDSS MDA Fortran Source code			      *	
 *			AEL						      *
 *									      *
 *      Description:    This routine takes in the azimuthal shear	      *
 *                      pattern vectors and combines them into		      *
 *                      2D-features.					      *
 *									      *
 *	Input:		azimuthal shear pattern vectors          	      *
 *	Output:		2D featurs				              * 
 *      returns:     	one                    				      *
 *      Globals:     	none						      *
 *      Notes:       							      *
 ******************************************************************************/

/*	System include file	*/

/*	local include file	*/

#include "mda2d_main.h"
#include "mda2d_acl.h"
#include <mda_adapt.h>
#define EXTERN  /* Causes instantiation of adaptation data object here */
#include "mda_adapt_object.h"

#define PROCESS	1	/* A boolean which controls the algorithm loop */ 

/* declare global parameters */
        float meso_vd_thr[BASEDATA_DOP_SIZE][MAX_RANK_RANGE];
        float meso_shr_thr[BASEDATA_DOP_SIZE][MAX_RANK_RANGE];
        int max_feat_flag;

int main(int argc,char* argv[]) {

	int rc; /* return code from function call */

        /* initialize ... */
        max_feat_flag = 0;

	LE_send_msg( GL_INFO, "Begin MDA 2D Algorithm ...\n" );


  	/* Register an intermedial product MDA1D (292) as algorithm input.*/
  	RPGC_in_data(MDA1D,ELEVATION_DATA);

  	/* Output will be put in the linear buffer assigned using the *
  	 * constant MDA2D (see include file) .                        */ 
  	RPGC_out_data(MDA2D, ELEVATION_DATA, INT_PROD);
  
	/* Register adaptation data */
        rc = RPGC_reg_ade_callback( mda_callback_fx,
                                    &mda_adapt,
                                    MDA_DEA_NAME,
                                    BEGIN_VOLUME );
        if ( rc < 0 )
	  RPGC_log_msg( GL_ERROR, "MDA: cannot register adaptation data callback function\n");


  	/* ORPG task initialization routine. */
	/* MDA always start from the begining of a volume */
  	RPGC_task_init(VOLUME_BASED,argc,argv);

	LE_send_msg( GL_INFO, "Algorithm initialized\n" );
	LE_send_msg( GL_INFO, "and procedding to control loop\n" );

	/* set a table for rank threshold */
	mda2d_set_table();

  	/* while loop that controls how long the task will execute. *
  	 * As long as PROCESS remains true, the task will continue. *
         * The task will terminate upon an error condition or       *
  	 * if a prespecified limit has been reached                 */
  	while(PROCESS) {

  	/* Block algorithm until good data is received */
  	RPGC_wait_act(WAIT_DRIVING_INPUT);

  	/* Runing into the algorithm control loop */
  	mda2d_acl();
        } /* END of while(PROCESS) */
} 	/* END of main function                                              */
