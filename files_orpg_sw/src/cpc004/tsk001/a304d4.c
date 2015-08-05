/* RCS info */
/* $Author: steves $ */
/* $Locker:  $ */
/* $Date: 2008/04/11 18:36:06 $ */
/* $Id: a304d4.c,v 1.5 2008/04/11 18:36:06 steves Exp $ */
/* $Revision: 1.5 $ */
/* $State: Exp $ */

#define FLOAT
#include <rpgcs_coordinates.h>
#include <rpgcs_data_conversion.h>

#include <veldeal.h>

/* Global Variables. */
static double Avg_vel;                      /* Average of the velocities. */
static short Num_in_avg;                    /* Number in 9-pt average. */
static short Save_gd_vel[NUM_AVG];          /* Velocities used in the average. */

static int Binnum;                          /* Current bin number. */

/* Function Prototypes. */
static int Bad_avg( short *radhdr, short *veloc, int last_bin_gdvel );
static int Good_avg( short *veloc );
static int Check_against_sounding( Base_data_header *radhdr, short *veloc );

/****************************************************************
   Description:
      Computes a 9-pt average using valid velocity data within 
      NUM9PT_LKBK on the previous radial and within NUM9PT_LKAHD 
      on a previous radial from the current bin number. 

   Inputs:
      veloc - pointer to velocity data.
      radhdr - pointer to radial header.
      binnum - current bin number.
      last_bin_gdvel - last bin on current radial with good 
                       velocity.

   Returns:
      Always returns 0.

****************************************************************/
int A304d4_comp_9ptavg( short *veloc, short *radhdr, int binnum,
                        int last_bin_gdvel ){

    /* Local variables */
    int index, sumvel, index_beg, index_end;
    int status  = A304db.status;

    short *vel_prevaz = &A304db.vel_prevaz[status][0];

    /* Initialize the number in average and the sum of velocities. */
    Num_in_avg = 0;
    sumvel = 0;

    /* Set to global variable since it might be used throughout
       this file. */
    Binnum = binnum;

    /* Continue processing when current radial contains valid data. */
    if( !A304dj.not_wibins_flg ){

        /* Computing MAX */
  	if( A304dd.fst_good_dpbin > (Binnum - NUM9PT_LKBK) )
            index_beg = A304dd.fst_good_dpbin;
  
        else
            index_beg = Binnum - NUM9PT_LKBK;

  	for( index = index_beg; index <= Binnum-1; ++index ){

            /* If current velocity is good, calculate the sum of the 
               velocities. */
  	    if( veloc[index] > BAD ){

	        sumvel += veloc[index];

                /* Store the current velocity data into the save good velocity   
                   array. */
  		Save_gd_vel[Num_in_avg] = veloc[index];

                /* Increment the number in average counter by one. */
  		Num_in_avg++;
  
            }

	}

    }

    /* Continue processing when previous radial contains valid data. */
    /* Computing MIN */
    if( A304db.lstgd_dpbin_prevaz < (Binnum + NUM9PT_LKAHD - 1) )
        index_end = A304db.lstgd_dpbin_prevaz;

    else
        index_end = Binnum + NUM9PT_LKAHD - 1;

    for (index = Binnum; index <= index_end; ++index) {

        /* If the previous saved radial velocity is GOOD, calculate the   
           sum of velocities. */
	if (vel_prevaz[index] > BAD ){
 
	    sumvel += vel_prevaz[index]; 

            /* Store the previous velocity data into the save good velocity  
               array to be used in the 9-pt average calculations. */
	    Save_gd_vel[Num_in_avg] = vel_prevaz[index];

            /* Increment the number in average counter by one. */
	    Num_in_avg++;

	}

    }

    /* If the number in average counter is greater than 0, calculate the
       average of velocities and call Good_avg else call
       Bad_avg. */
    if( Num_in_avg > 0 ){

        Avg_vel = (float) sumvel / (float) Num_in_avg;
  	Good_avg( veloc );

    } 
    else 
        Bad_avg( radhdr, veloc, last_bin_gdvel );
     
    return 0;

/* End of A304d4_comp_9ptavg() */
} 


/********************************************************************

   Description:
      Attempts to unfold veloc(binnum) to within a threshold of the
      9-pt average.

   Inputs:
      veloc - pointer to velocity data.

********************************************************************/
static int Good_avg( short *veloc ){

    /* Local variables */
    double r2;
    int dif_velavg, tol_minavg, tol_mxstdev, index, stdev, sumsq;
    int avg_40per, i_average;

    i_average = (short) RPGC_NINT( Avg_vel );
    r2 = fabs( Avg_vel - BIASED_ZERO) * .4f;
    avg_40per = RPGC_NINT( r2 );
    if( avg_40per > A304dg.tol_fstchk )
        tol_minavg = avg_40per;
    else
        tol_minavg = A304dg.tol_fstchk;

    dif_velavg = abs( (int) veloc[Binnum] - i_average );

    /* If the difference velocity average is >= the tolerance minimum   
       average, the sum of squares of the velocities is calculated. */
    if( dif_velavg >= tol_minavg ){

	int end_index = Num_in_avg;

	sumsq = 0;
	for (index = 0; index < end_index; ++index) 
	    sumsq += Square[Save_gd_vel[index]];

        /* Calculate the standard deviation and the tolerance of the   
           maximum standard deviation. */

        /* Computing 2nd power. */
	stdev = sqrt( ((double) sumsq / Num_in_avg) - (Avg_vel*Avg_vel) );

        /* Computing MIN */
	if( A304dg.th_max_stdev < (2.0*stdev) )
	   tol_mxstdev = A304dg.th_max_stdev;

        else
	   tol_mxstdev = 2.0*stdev;

        /* If the tolerance of the maximum standard deviation is < the   
           tolerance of the minimum average, A304d3 is called to attempt to 
           minimize the difference. */
	if( tol_mxstdev < tol_minavg ) 
	    A304d3_min_vel_dif( &veloc[Binnum], i_average, tol_minavg );

        else{

            /* If the difference of the velocity average is > the tolerance   
               of the maximum standard deviation, A304d3 is called to attempt   
               to minimize this difference. */
	    if( dif_velavg > tol_mxstdev )
		A304d3_min_vel_dif( &veloc[Binnum], i_average, tol_mxstdev );

            /* If the difference of the velocity average is <= the tolerance of   
               the maximum standard deviation, the no check flag is set. */
	    else 
		A304dj.no_chk_flg = 1;
	    
	}

        /* If the difference velocity average is < the tolerance minimum average,
           the no check flag is set. */

    }
    else 
	A304dj.no_chk_flg = 1;
    
   
    return 0;

/* End of Good_avg() */
} 


/*********************************************************************

   Description:
      Checks up to NUMBER_LOOK_BACK along the same radial and up to 
      NUMBER_LOOK_FORWARD on an adjacent radial to locate a valid 
      velocity.   If a valid velocity is found, A304d3 is called 
      to attempt to minimize the difference to within THRESHOLD_
      DIFFERENCE_UNFOLD_RELAX.  If the velocity difference can be
      minimized to within this threshold, then the no check flag is 
      set.   Otherwise the velocity is rejected as bad.

   Inputs:
      radhdr - pointer to radial header.
      veloc - pointer to velocity data.
      last_bin_gdvel - last bin on current radial with good velocity.

   Returns:
      Currently always returns 0.

*********************************************************************/
static int Bad_avg( short *radhdr, short *veloc, int last_bin_gdvel ){

    /* Local variables */
    int index_beg, index_end, diff;

    short *vel_prevaz = &A304db.vel_prevaz[GOOD_PREV_RADIAL][0];

    /* Check number look back along the radial to locate a valid velocity. */
    if( (Binnum - last_bin_gdvel) <= A304di.num_lkbk ){

        /* If the difference between the current velocity and the velocity 
           at the last good bin location is >= the threshold difference 
           unfold relax value, A304d3 is called to attempt to minimize 
           this difference. */
	diff = veloc[Binnum] - veloc[last_bin_gdvel]; 
	if( abs(diff) >= A304dg.th_diff_unf_relax )
	    A304d3_min_vel_dif( &veloc[Binnum], veloc[last_bin_gdvel], 
                                A304dg.th_diff_unf_relax );

        else{

            /* The difference is < the threshold difference to unfold relax 
               value.  Set the no check flag. */
	    A304dj.no_chk_flg = 1;

        }

    }
    else{

        /* Checks up to the number look forward on an adjacent radial to 
           locate a valid velocity. */
	if( A304db.status == GOOD_PREV_RADIAL ){

	    index_beg = Binnum + NUM9PT_LKAHD - 1;

            /* Computing MIN */
	    if( (Binnum + A304di.num_lkfor - 1) < A304db.lstgd_dpbin_prevaz )
                index_end = Binnum + A304di.num_lkfor - 1;

            else
	       index_end = A304db.lstgd_dpbin_prevaz;

            /* Do Processing Until the beginning index is > the ending index. */
	    while( index_beg <= index_end ){

		if( vel_prevaz[index_beg] > BAD ){

                    /* If the difference between the previous save radial velocity 
                       and the current velocity is >= the threshold difference to
                       unfold relax value, A304d3 is called to attempt to minimize 
                       this difference. */
		    diff = vel_prevaz[index_beg] - veloc[Binnum];
                    if( abs( diff ) >= A304dg.th_diff_unf_relax )
			A304d3_min_vel_dif( &veloc[Binnum], vel_prevaz[index_beg], 
				            A304dg.th_diff_unf_relax );
 
                    else 
			A304dj.no_chk_flg = 1;
		     
		    break;

		}

                /* Increment the beginning index value and continue processing. */
		++index_beg;

	    }

	}

    }

    if( !A304dj.no_chk_flg ){

	if( A304dg.sounding_avail ) 
	    Check_against_sounding( (Base_data_header *) radhdr, veloc );
	 
    }

    return 0;

/* End of Bad_avg() */
} 


/**************************************************************************

   Description:
      If a wind table or sounding is available, a radial component of the 
      ambient wind is computed for the height and azimuth of the radar 
      measurement.  The velocity difference between the ambient wind radial 
      component and the radar measurement is minimized to within the default 
      velocity difference threshold, if possible. 

   Inputs:
      radhdr - pointer to radial header.
      veloc - pointer to velocity data.

   Returns:
      Always returns 0.

***************************************************************************/
static int Check_against_sounding( Base_data_header *radhdr, short *veloc ){

    /* Local variables */
    int hgt_of_data, found_entry;
    float radial_proj, rng_of_data, elevation, height;

    int level_above, level_below;
    short ambient_comp;

    /* Calculate the slant range of the sample bin in question, in m. */
    rng_of_data = ((float) Binnum + 0.5) * radhdr->dop_bin_size;

    /* Calculate the height of the sample bin above the ground in Kft 
       because the EWT is indexed in whole Kft. */
    elevation = radhdr->elevation;
    RPGCS_height_f( rng_of_data, elevation, &height );
    hgt_of_data = (int) (height * M_TO_KFT);

    /* Take the minimum of the height and the Max EWT entry. */
    if( hgt_of_data > Max_ewt_entry )
        hgt_of_data = Max_ewt_entry;

    /* If the wind table has no valid entries, then Max_ewt_entry is -1.
       Ensure hgt_of_data is non-negative. */
    if( hgt_of_data < 0 )
        hgt_of_data = 0;

    /* Search for a valid wind table entry closest to the height of the sample
       volume. */
    found_entry = 0;

    /* Check that the entries into the environmental wind table are valid. */
    if( Ewt.newndtab[hgt_of_data][NCOMP] != MTTABLE_INT )
	found_entry = 1;

    else{

        /* Search for valid entry. */
	level_above = hgt_of_data + 1;
	level_below = hgt_of_data - 1;

        /* Search for valid entry above the height of the sample volume. */
        while(1){

	    if( (level_above <= Max_ewt_entry) 
                       && 
                (Ewt.newndtab[level_above][NCOMP] != MTTABLE_INT) ){

                /* Found valid entry.  Set height of sample volume equal to the    
                   closest valid entry in the environmental wind table. */
	        found_entry = 1;
	        hgt_of_data = level_above;

                /* Search for valid entry below the height of the sample volume. */
	    }
            else if( (level_below >= Min_ewt_entry)
                           && 
                     (Ewt.newndtab[level_below][NCOMP] != MTTABLE_INT) ){

	        hgt_of_data = level_below;
	        found_entry = 1;

	    }

            /* Continue to search for valid entry if one not already found. */
	    if( !found_entry ){

	        --level_below;
	        ++level_above;
	        if( (level_above <= Max_ewt_entry) || (level_below >= Min_ewt_entry) ) 
		   continue;

                break;
	    }

            break;

	}

    }

    /* Calculate radial projection of sounding entry if valid entry was found. */
    if( found_entry ){

	double coselv = radhdr->cos_ele;
	double sinazm = radhdr->sin_azi;
	double cosazm = radhdr->cos_azi;
     
        int reso = RPGCS_get_velocity_reso( (int) radhdr->dop_resolution );
        int diff;

	radial_proj = ( (Ewt.newndtab[hgt_of_data][ECOMP] * sinazm) + 
                        (Ewt.newndtab[hgt_of_data][NCOMP] * cosazm) ) * coselv;

        /* Convert to scaled and biased units, then apply the velocity bias
           used by the dealiasing algorithm. */
	ambient_comp = (short) RPGCS_ms_to_velocity( reso, radial_proj );
        ambient_comp += VEL_BIAS;

        /* Check velocity in question and radial projection of sounding entry 
           against velocity difference threshold. */
        diff = veloc[Binnum] - ambient_comp;
	if( abs(diff) >= A304dg.th_def_vel_diff ){

            /* Call A304d3 to try and minimize velocity difference if diference
               is above threshold. */
	    A304d3_min_vel_dif( &veloc[Binnum], ambient_comp, A304dg.th_def_vel_diff );

	}
        else 
	    A304dj.no_chk_flg = 1;
	 
    }

    return 0;

/* End of Check_against_sounding() */
} 
