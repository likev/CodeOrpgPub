/* RCS info */
/* $Author: steves $ */
/* $Locker:  $ */
/* $Date: 2007/06/28 15:16:52 $ */
/* $Id: a304dv.c,v 1.3 2007/06/28 15:16:52 steves Exp $ */
/* $Revision: 1.3 $ */
/* $State: Exp $ */

#include <veldeal.h>

/* Global Variables. */
static short *Save_veltmp;		/* Array of rejected velocities. */

/* Function Prototypes. */
static int Replace_points( short *veloc, int binnum, int last_bin_gdvel );


/*****************************************************************

   Description:
      If the first check fails to place velocity(binnum) in the 
      proper Nyquist interval or there are no good bins within
      number_bins_first_check, a velocity for comparision is
      acquired from either a sounding, if available, or a 9-pt
      average of surrounding bins.

      If velocity(binnum) can not be successfully placed in the 
      proper Nyquist interval, the velocity at binnum is rejected.
      The velocity is saved for possible reinsertion into the 
      radial.

   Inputs:
      radhdr - pointer to radial header.
      veloc - pointer to velocity data.
      binnum - current bin number.
      last_bin_gdvel - last bin on current radial with good
                       velocity.

   Return:
      The last bin on the current radial with good velocity.

*****************************************************************/
int A304dv_find_other_means( short *radhdr, short *veloc, int binnum,
                             int last_bin_gdvel ){

    static int first_time = 1;

    /* First time and one-time initialization. */
    if( first_time ){

        first_time = 0;
    
        Save_veltmp = (short *) calloc( 1, MAX_CON_REJ*sizeof(short) );
        if( Save_veltmp == NULL ){

           RPGC_log_msg( GL_ERROR, "calloc Failed for Save_veltmp\n" );
           RPGC_hari_kiri();

        }

    }

    /* Call A304d4 to compute the nine-point average. */
    A304d4_comp_9ptavg( veloc, radhdr, binnum, last_bin_gdvel );

    /* If the no check flag is not set, increment the number of consecutive
       bins rejected, store the velocity data into the array of rejected
       velocities and set the velocity data to the below threshold value. */
    if( !A304dj.no_chk_flg ){

	Save_veltmp[Con_bins_rej] = veloc[binnum];
	++Con_bins_rej;

        /* Save the removed velocity for replacement after this radial is 
           processed. */
	Deleted_bin[Num_deleted] = (short) binnum;
	Deleted_vel[Num_deleted] = veloc[binnum];
	Num_deleted++;

        /* Set to below threshold. */
	veloc[binnum] = BELOW_THR;

        /* If the number of consecutive bins rejected is equal to the   
           maximum number of consecutive velocities rejected before   
           replacement commences, set the no check flag to true and call    
           module to replace the velocity points.  Perform error checking   
           along and between radials with call to A304d8. */
	if( Con_bins_rej == A304di.th_conbin_rej ){

  	    Replace_points( veloc, binnum, last_bin_gdvel );

            /* Set the flag for radial jump checking and call A304d8 to   
               check for large radial and azimuthal jumps of velocity. */
            if( (binnum - last_bin_gdvel) <= A304di.num_bin_fstchk )
  		A304dj.jmpchk = 0;
	    
            else 
  		A304dj.jmpchk = 1;
	    
            A304d8_jump_check( veloc, binnum, last_bin_gdvel );
  	    last_bin_gdvel = binnum;

	}

    }
    else{

        /* If the no check flag is true, set the bin location of the last   
           good velocity on the current radial to the current bin location   
           on the radial.  Call A304d8 to do error checking.  Reinitialize
           the number of bins rejected to 0. */
	A304d8_jump_check( veloc, binnum, last_bin_gdvel );
	last_bin_gdvel = binnum;
	Con_bins_rej = 0;

    }

    return last_bin_gdvel;

/* End of A304dv_find_other_means() */
} 


/************************************************************************

   Description:
      When the number of bins rejected exceeds threshold, attempt to    
      replace the points.

   Inputs:
      veloc - pointer to velocity data.
      binnum - current bin number.
      last_bin_gdvel - last bin on current radial with good velocity.

   Returns:
      Always returns 0.

***********************************************************************/
static int Replace_points( short *veloc, int binnum, int last_bin_gdvel ){

    /* Local variables */
    int sumvel, diff, bins_rep;
    int index_beg, index_end, sum_binrep, numavg_praz, index;
    short avg_praz, runavg;

    short *vel_prevaz = &A304db.vel_prevaz[GOOD_PREV_RADIAL][0];

    bins_rep = 0;
    numavg_praz = 0;
    A304dj.no_chk_flg = 0;

    /* Checks if the difference between the bin number and the last 
       bin with good velocity is < the number of bins to look back for 
       the initial radial continuity check. */
    if( (binnum - last_bin_gdvel) < 
        (A304di.num_bin_fstchk + A304di.th_conbin_rej - 1)) {

        /* If the difference between the rejected velocity and the velocity 
           at deviation of nine points allowed, A304d3 is called to attempt 
           to minimize this difference. */
	diff = Save_veltmp[0] - veloc[last_bin_gdvel]; 
	if( abs( diff ) > A304dg.th_def_vel_diff ) 
	    A304d3_min_vel_dif( &Save_veltmp[0], 
                                veloc[last_bin_gdvel], 
                                A304dg.th_def_vel_diff );

        /* If the difference between the rejected velocity and the velocity 
           at the last good bin location is <= the maximum standard 
           deviation of nine points allowed, the no check flag is set. */
	else 
	    A304dj.no_chk_flg = 1;
	
    }

    /* Checks if the previous radial status is good and the "no check flag" is
       false. */
    if( (A304db.status == GOOD_PREV_RADIAL) 
                       && 
               (!A304dj.no_chk_flg) ){

	sumvel = 0;

        /* Computing MIN */
	if( A304db.lstgd_dpbin_prevaz < (binnum + A304di.num_rep_lkahd) )
	    index_end = A304db.lstgd_dpbin_prevaz;

        else
	    index_end = binnum + A304di.num_rep_lkahd;

        /* Computing MAX */
	if( A304db.fstgd_dpbin_prevaz < (binnum - A304di.num_rep_lkbk) )
	    index_beg = binnum - A304di.num_rep_lkbk;

        else
	    index_beg = A304db.fstgd_dpbin_prevaz;

	for (index = index_beg; index <= index_end; ++index ){

            /* If the previous saved radial velocity value is good then the 
               number in average of the previous azimuth is incremented and 
               the sum of the velocities is calculated. */
	    if( vel_prevaz[index] > BAD ){

		++numavg_praz;
		sumvel += vel_prevaz[index];

	    }

	}

        /* If the number in average > 0, compute average. */
	if( numavg_praz > 0 ) 
	    avg_praz = (short) (sumvel / numavg_praz);

	else{

            /* Else the average is the first rejected velocity. */ 
	    avg_praz = Save_veltmp[0];
	
        }

        /* If the difference between the rejected velocity and the average 
           of the previous azimuth is > the threshold difference to unfold
           relax value, A304d3 is called to attempt to minimize this  
           difference. */
	diff = Save_veltmp[0] - avg_praz;
	if( abs(diff) > A304dg.th_diff_unf_relax )
	    A304d3_min_vel_dif( &Save_veltmp[0], avg_praz, 
		                A304dg.th_diff_unf_relax );
	 
    }

    veloc[binnum - Con_bins_rej + 1] = Save_veltmp[0];

    /* Increment the number of bins replaced. */
    ++bins_rep;

    /* Determine the running sum of the bins replaced and set the 
       running average to this value. */
    sum_binrep = Save_veltmp[0];
    runavg = (short) sum_binrep;
    index_beg = binnum - Con_bins_rej + 2;
    for( index = index_beg; index <= binnum; ++index) {

        /* If the difference between the rejected velocity and the running 
           average is >= the threshold difference to unfold relax value,
           A304d3 is called to attempt to minimize this difference. */
	diff = Save_veltmp[bins_rep] - runavg;
	if( abs(diff) >= A304dg.th_diff_unf_relax ) 
	    A304d3_min_vel_dif( &Save_veltmp[bins_rep], 
		                runavg, A304dg.th_diff_unf_relax );
	

        /* Set the velocity to the rejected velocity value. */
	veloc[index] = Save_veltmp[bins_rep];

        /* Increment the running sum of the bins replaced and calculate the 
           running average. */
	sum_binrep += Save_veltmp[bins_rep];

        ++bins_rep;
	runavg = (short) (sum_binrep / bins_rep);

    }

    /* Set the number of consecutive bins rejected to 0 and the last bin
       with good velocity equal to the current bin location. */
    Con_bins_rej = 0;
    Num_deleted = (short) (Num_deleted - A304di.th_conbin_rej);
    A304dj.no_chk_flg = 0;

    return 0;

/* End of Replace_points() */
} 

