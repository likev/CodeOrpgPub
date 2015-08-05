/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2009/03/19 19:55:57 $
 * $Id: mda1d_main.c,v 1.8 2009/03/19 19:55:57 ccalvert Exp $
 * $Revision: 1.8 $
 * $State: Exp $
 */

/******************************************************************************
 *	Module:         mda_1d_main.c					      *
 *	Author:		Yukuan Song					      *
 *   	Created:	August 26, 2002					      *
 *	References:	WDSS MDA Fortran Source code			      *	
 *			AEL						      *
 *									      *
 *      Description:	This is the top level module for ORPG Mesocyclone     *
 * 			Detection Algorithm 1D (mda1d) task.It is the 	      *
 *			launching point for all other mda tasks		      *
 *									      *
 *	Input:		one radial at a time as defined in the        	      *
 *			Base_data_radial structure (see the basedata.h)       *
 *	Output:		a formatted MDA 1D intermediate product, which        * 
 *             		will be distributed to MDA 2D as its input            *
 *      returns:     	one                    				      *
 *      Notes:       							      *
 *									      *
 *	Modified: Chris Calvert 11/03 - included callback function and new    *
 *					dea adaptation data format	      *
 *									      *
 ******************************************************************************/

/*include files	*/
#include "mda1d_main.h"
#include <mda_adapt.h>
#define EXTERN  /* Causes instantiation of adaptation data object here */
#include "mda_adapt_object.h"

/* constant defination */
#define 	PROCESS	1	/* A boolean which controls the algorithm loop */ 

/* declare global parameters */
int YES_DO_IT = 1; /* a flag used to access a block only once */

int main(int argc,char* argv[]) {

	int rc; /* return code from function call */

	LE_send_msg( GL_INFO, "Begin MDA 1D Algorithm ...\n" );

  	/* Register base data type COMBBASE as algorithm input, this input *
  	 * consists of radial reflectivity and velocity data               */
  	RPGC_in_data(SR_COMBBASE,RADIAL_DATA);

  	/* Output will be put in the linear buffer assigned using the *
  	 * constant MDA1D (see include file) */ 
  	RPGC_out_data(MDA1D, ELEVATION_DATA, INT_PROD);
  
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

  	/* while loop that controls how long the task will execute. *
  	 * As long as PROCESS remains true, the task will continue. */
  	while(PROCESS) {

  	/* Block algorithm until good data is received */
  	RPGC_wait_act(WAIT_DRIVING_INPUT);

  	/* Runing into the algorithm control loop */
  	mda1d_acl();
        } /* END of while(PROCESS) */
} 	/* END of main function                                              */
