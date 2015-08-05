/* RCS info */
/* $Author: steves $ */
/* $Locker:  $ */
/* $Date: 2011/06/30 15:00:28 $ */
/* $Id: a304dc.c,v 1.8 2011/06/30 15:00:28 steves Exp $ */
/* $Revision: 1.8 $ */
/* $State: Exp $ */

#include <veldeal.h>

int Verbose_mode;

/* Static Global Variables. */
static int Data_reso;                       /* Velocity data resolution. */
static short Nyq_mult[ NUM_INTRVL ];        /* Nyquist co-interval table. */

/************************************************************************ 

   Description: 
      This module determines the need for velocity dealiasing of the 
      passed radial. 

   Inputs:
      inbuf_ptr - pointer to RPG format radial message.

   Returns:
      The radial status.

************************************************************************/
int A304dc_process_radial( void *inbuf_ptr ){

    Base_data_header *radhdr = (Base_data_header *) inbuf_ptr;
    short *veloc = NULL;

    if( radhdr->vel_offset > 0 )
       veloc = (short *) ((char *) inbuf_ptr + radhdr->vel_offset);

    /* Added to support 2D Velocity Dealiasing Field Test. */
    if( (radhdr->status == GOODBVOL) && (radhdr->elev_num == 1) 
                                     && 
                               (Verbose_mode) )
        RPGC_log_msg( GL_STATUS | LE_RPG_INFO_MSG, 
                      "VELDEAL: Velocity Dealiasing Algorithm Being Used\n" );

   /* Check if this radial needs to be dealiased (i.e., the number of Doppler 
      bins must be non-zero and the Nyquist velocity must be non-zero. */
    if( (veloc != NULL) 
                  &&
        (radhdr->n_dop_bins != 0) 
                  && 
        (radhdr->nyquist_vel != 0)
                  && 
        (!A304di.disable_radaz_dealiasing) ){

        /* Has the Nyquist velocity changed since last radial? */
	if( Old_nyvel != radhdr->nyquist_vel ){

	    Old_nyvel = radhdr->nyquist_vel;
	    Data_reso = 3 - radhdr->dop_resolution;
	    Nyvel = radhdr->nyquist_vel * Data_reso * .01f;

            /* Compute Nyquist velocity dependent thresholds. */
	    Define_edit_parm( );

	}

        /* Set first and last good bins, then dealias radial. */
	A304dd.fst_good_dpbin = radhdr->dop_range - 1;
	A304dd.lst_good_dpbin = A304dd.fst_good_dpbin + radhdr->n_dop_bins - 1;
                     
	A304d2_rad_azm_unfold( radhdr, veloc );

        /* Set the velocity dealiasing bit in radial header. */
	radhdr->msg_type |= VEL_DEALIASED_BIT;

    }

    /*Return to caller. */
    return( (int) radhdr->status );

/* End of A304dc_process_radial() */
} 

/*****************************************************************

   Description:
      Defines elevation based constants for velocity dealiasing.
      Set thresholds based on Nyquist velocity change.

   Inputs:
      dopres - Doppler resolution.

   Returns:
      Currently always returns 0.

*****************************************************************/
int Define_edit_parm(){


    /* Local variables */
    int scale, index, intervals;
    float r1, r2, r3, r4;

    /* Set Nyquist velocity dependent thresholds. */
    Nyq_intrvl = Nyvel*2;

    /* Computing MIN */
    r1 = Data_reso * 45.f; 
    r2 = A304di.th_vel_jmp_frad * Nyq_intrvl;
    if( r1 < r2 )
        A304dg.veljmp_mxrad = (int) r1;

    else
        A304dg.veljmp_mxrad = (int) r2;


    A304dg.veljmp_mxaz = A304di.th_vel_jmp_faz * Nyq_intrvl;

    /* Computing MIN */
    r3 = Data_reso * 22.5f;
    r4 = A304di.th_scl_stdev * Nyq_intrvl;
    if( r3 < r4 )
        A304dg.th_max_stdev = (int) r3;

    else
        A304dg.th_max_stdev = (int) r4;

    A304dg.th_def_vel_diff = A304dg.th_max_stdev;
    A304dg.tol_fstchk = A304di.th_diff_unfold * Data_reso;
    A304dg.th_diff_unf_relax = A304di.th_scl_diff_unfold * 
	                       A304dg.tol_fstchk;

    /* Define Nyquist multiples table. */
    scale = 0;
    intervals = Num_intrvl_chk;
    for( index = 0; index < intervals; index += 2 ){

	++scale;
	Nyq_mult[index] = (short) (Nyq_intrvl*scale);
	Nyq_mult[index+1] = -Nyq_mult[index];

    }

    /* Return to calling routine. */
    return 0;

/* End of Define_edit_parm() */
} 

/*******************************************************************

   Description:
      Minimize the velocity difference between velin and velcmp.

   Input:
      velin - pointer to velocity to dealias
      velcmp - comparision velocity.
      veldif_tol - dealiasing tolerance.

   Returns:
      Currently always returns 0.

*******************************************************************/
int A304d3_min_vel_dif( short *velin, short velcmp, int veldif_tol ){

    int index, velout, diff;
    short vel = *velin;

    /* Add/subtract Nyquist intervals to attempt to minimize the differnce. */
    if( vel < velcmp )
	index = 0;

    else
	index = 1;
    
    /* Continue processing until index is greater than the number of Nyquist
       intervals. */
    while(1){

        velout = vel + Nyq_mult[index];

        /* If the diffence between velocity out and velocity compare is within
           the velocity tolerance, transfer velocity out value to velocity in 
           and set the no check flag to true. */
        diff = velout - velcmp;
        if( abs(diff) < veldif_tol ){

	    A304dj.no_chk_flg = 1;
	    *velin = (short) velout;

            /* Done !!!!!!. */
	    break;

        }

        /* Try another Nyquist co-interval. */
        index += 2;
        if( index >= Num_intrvl_chk ){

            /* Done !!!!!!. */
	    break;

        }
    
    }

    /* Return to caller. */
    return 0;

/* End of A304d3_min_vel_dif() */
}

