/* RCS info */
/* $Author: steves $ */
/* $Locker:  $ */
/* $Date: 2007/06/28 15:16:48 $ */
/* $Id: a304d8.c,v 1.2 2007/06/28 15:16:48 steves Exp $ */
/* $Revision: 1.2 $ */
/* $State: Exp $ */

#include <veldeal.h>

/* Static global variables. */
static int Jmp_stbin = 0;
static int New_jmpdir = 0;
static int Old_jmpdir = 0;

static int Binnum;

/* Function Prototypes. */
static int Reunfold_radial( int vel_difaz, short *veloc );
static int Correct_veloc( int rptr, int nyq_fold, int paz_goodvel, 
                          int caz_goodvel, short *veloc );


/*******************************************************************

   Description:
      Checks for abnormally large velocity jumps along the radial.
      If jump exists, tries to correct it.

   Inputs:
      veloc - pointer to velocity data.
      binnum - current bin number.
      last_bin_gdvel - last bin on current radial with good velocity.

   Returns:
      Always returns 0. 

*******************************************************************/
int A304d8_jump_check( short *veloc, int binnum, int last_bin_gdvel ){

    /* Local variables. */
    int vel_difrad;
    static int vel_difaz = 0;

    /* Assign this to global variable since it might be needed
       throughout the file. */
    Binnum = binnum;

    while( !A304dj.jmpchk ){

        /* Set the jump check flag to indicate we no longer need to perform
           the check. */
	A304dj.jmpchk = 1;

        /* Set velocity difference to the difference between the velocity   
           at the current bin location and the velocity of the bin   
           location of the last good velocity along the current radial. */
	vel_difrad = veloc[Binnum] - veloc[last_bin_gdvel];

        /* Check if the velocity difference is greater than the maximum   
           velocity difference in range allowed in the final jump check. */
  	if( abs(vel_difrad) > A304dg.veljmp_mxrad ){

            /* If velocity jump flag is true indicating a large velocity jump   
               exists on the current radial and bin number minus jump start   
               bin is less than the maximum number of bins allowed between   
               velocity jumps of opposite sense on the current radial then   
               determine the new jump direction sense. */
	    if( Vel_jump 
                       && 
                ((Binnum - Jmp_stbin) < A304di.th_mxbins_jmp) ){

                if( vel_difrad < 0 )
                    New_jmpdir = -Nyq_intrvl;

                else
  	            New_jmpdir = Nyq_intrvl;

                /* Continue processing when the senses of the new and old jump   
                   directions are equal. */
		if( New_jmpdir == Old_jmpdir ){

                    int index;

		    for( index = Jmp_stbin; index <= Binnum - 1; ++index ){

		        if( veloc[index] > BAD ) 
			    veloc[index] += Old_jmpdir;
			     
		    }

		}

                /* Set vel_jump to false indicating a large velocity jump 
                   no longer exists on the current radial. */
		Vel_jump = 0;
		A304dj.jmpchk = 0;

	    }
            else{

                /* Set vel_jump to true indicating a large velocity jump exists 
                   on the current radial. */
	        Vel_jump = 1;

                if( -vel_difrad < 0 )
		    Old_jmpdir = -Nyq_intrvl;

                else
		    Old_jmpdir = Nyq_intrvl;

		Jmp_stbin = Binnum;

	    }

	}
	
    }

    /* If the previous saved radial velocity data is not BAD, calculate the   
       velocity difference azimuthal. */
    if( A304db.vel_prevaz[A304db.status][Binnum] > BAD ){
  
        vel_difaz = veloc[Binnum] - A304db.vel_prevaz[A304db.status][Binnum];

        /* If the velocity difference azimuthal >= the maximum velocity 
           difference in azimuth allowed in the final jump check, increment 
           the number of bins with large velocity jump in azimuth. */
	if( abs(vel_difaz) >= A304dg.veljmp_mxaz ){

	    ++Bin_lrg_azjump;

            /* If bins with large azimuthal jump >= the threshold, for bins 
               with large azimuthal jump, correct problems by calling Reunfold_radial. */
            if( Bin_lrg_azjump >= A304di.th_bins_lrg_azjmp ){

                Reunfold_radial( vel_difaz, veloc );
	        Bin_lrg_azjump = 0;

            }

        }
        else{

            /* The the number of bins with large azimuth jump to 0. */ 
	    Bin_lrg_azjump = 0;

        }
	
    }
    else {

        /* If the starting bin location of large velocuty jump in azimuth is
           > 2, increment this value by 1. */
	if( Bin_lrg_azjump > 2 ){

	    ++Bin_lrg_azjump;

            /* If bins with large azimuthal jump >= the threshold, for bins 
               with large azimuthal jump, correct problems by calling Reunfold_radial. */
            if( Bin_lrg_azjump >= A304di.th_bins_lrg_azjmp ){

                Reunfold_radial( vel_difaz, veloc );
	        Bin_lrg_azjump = 0;


            }

        }

    }

    return 0;

/* End of A304d8_jump_check() */
} 


/********************************************************************

   Description:
      Reunfolds radial if bin_with_large_azimutal_jump is greater 
      than threshold_bins_with_large_azimuthal_jump. 

   Inputs:
      vel_difaz - velocity difference in azimith.
      veloc - pointer to velocity data. 

   Returns:
      Always returns 0.

*******************************************************************/
static int Reunfold_radial( int vel_difaz, short *veloc ){

    /* Local variables */
    int nyq_fold, index_beg, index_end, index_back, caz_goodvel, index_frwrd;
    int rptr, first_bin, praz_goodvel, index, num_con_binmiss;

    short *vel_prevaz = &A304db.vel_prevaz[A304db.status][0];
 
    /* Determine which way to unfold the data. */
    if( -vel_difaz < 0 )
        nyq_fold = -Nyq_intrvl;

    else 
        nyq_fold = Nyq_intrvl;

    veloc[Binnum] = (short) (veloc[Binnum] + nyq_fold );

    /* Set the radial pointer to the current bin location and the number
       of consecutive bins missing to 0. */
    rptr = Binnum;
    num_con_binmiss = 0;

    /* Continue processing until the radial pointer is equal to the first
       good bin on the current radial. */
    first_bin = A304dd.fst_good_dpbin;
    while( rptr > first_bin ){

	--rptr;

        /* If velocity is BAD, increment the number of consecutive bins 
           missing. */
	if( veloc[rptr] <= BAD ){

	    ++num_con_binmiss;

            /* If the number of consecutive bins missing is equal to the 
               maximum number of missing velocities allowed in the search for 
               valid velocity when replacing a rejected velocity, terminate 
               reunfolding. */
	    if( num_con_binmiss == A304di.th_mxmiss ) 
	        break;

	}
        else{

            /* If the velocity data is GOOD, the number of consecutive bins   
               missing is set to 0. */
	    num_con_binmiss = 0;
	    praz_goodvel = BAD;
	    caz_goodvel = BAD;

            /* If the previous saved radial velocity data is good, set the 
               previous azimuth good velocity. */
	    if( vel_prevaz[rptr] > BAD ) 
	        praz_goodvel = vel_prevaz[rptr];

            else{

                /* If the previous saved radial velocity data is BAD, initialize   
                   the index pointers so we can go search for a GOOD velocity. */
		index = 0;
		index_frwrd = rptr;
		index_back = rptr;

                /* Continue processing until index is equal to the number of 
                   surrounding bins to search for a valid velocity on the previous 
                   saved radial. */
  		while( index != A304di.num_reunf_prazs ){

		    ++index;
		    ++index_frwrd;

                    /* Checks if the forward index pointer is <= the last good 
                       velocity on the previous saved radial. */
                    if( (index_frwrd <= A304db.lstgd_dpbin_prevaz)
                                          && 
		        (vel_prevaz[index_frwrd] > BAD) ){ 

                        /* If the previous saved radial velocity data is GOOD, 
                           sets the previous azimuth good velocity. */
		        praz_goodvel = vel_prevaz[index_frwrd]; 
			break;

		    }
                    else{

		        --index_back;

                        /* Checks if the backward index pointer is >= the first good   
                          velocity on the previous saved radial. */
	                if( (index_back >= A304db.fstgd_dpbin_prevaz)
                                            && 
			    (vel_prevaz[index_back] > BAD) ){

                            /* If the previous saved radial velocity data is good, 
                               sets the previous azimuth good velocity. */
			    praz_goodvel = vel_prevaz[index_back];
			    break;

		        }

		    }

                } /* End of while( index != A304di.num_reunf_prazs ) */

            }

            /* Checks if the previous azimuth good velocity is not BAD. */
	    if( praz_goodvel > BAD ){

                /* If the velocity data at a previous bin is good, sets 
                   the current azimuth good velocity and the found velocity 
                   current azimuth flag. */
		if( veloc[rptr + 1] > BAD )
		    caz_goodvel = veloc[rptr + 1];

                else{

                    /* If the velocity data is BAD, beginning and ending indices 
                       are determined. */
		    index_beg = 1;

                    /* Computing MIN */
		    if( A304di.num_reunf_cazs < (A304dd.lst_good_dpbin - rptr) )
		        index_end = A304di.num_reunf_cazs;

                    else
	                index_end = A304dd.lst_good_dpbin - rptr;

                    /* Processing continues until the found velocity current azimuth   
                       flag is true or the beginning index is > the ending index. */
                    while( index_beg <= index_end ){

		        if( veloc[index_beg + rptr ] > BAD ){

			    caz_goodvel = veloc[index_beg + rptr];
			    break;

			}

			++index_beg;

                    } /* End of while( index_beg <= index_end ) */

		}

                /* If a good velocity on the current azimuth is found, calls  
                   routine to correct the velocity data, otherwise terminates 
                   reunfolding. */
		if( caz_goodvel > BAD )
		    rptr = Correct_veloc( rptr, nyq_fold, praz_goodvel, caz_goodvel, 
                                          veloc );
                else 
		    break;

	    }
            else{

                /* If the found velocity previous azimuth flag is false, 
                   terminate reunfolding. */
		break;

            }
	     
        }

    } /* End of while( rptr > first_bin ) */

    return 0;

/* End of Reunfold_radial() */
} 


/**********************************************************************

   Description:
      Does a least squares minimization for reunfolding down a radial. 

   Inputs:
      rptr - bin location on current radial.
      nyq_fold - Nyquist folding number.
      paz_goodvel - previous azimuth good velocity.
      caz_goodvel - current azimuth good velocity.
      veloc - pointer to velocity data.

   Outputs:
      rptr - bin location on current radial.

   Returns:
      Bin location on current radial.

***********************************************************************/
static int Correct_veloc( int rptr, int nyq_fold, int paz_goodvel, 
                          int caz_goodvel, short *veloc ){

    /* Local variables */
    int diff, dif_unfsq, difvel_faz, difvel_frad, jump_chkcon;
    int difvel_unfaz, vel_reunfold, index, difvel_unfrad, dif_fsq;

    vel_reunfold = veloc[rptr] + nyq_fold;

    /* Calculate the difference velocity unfolded radial and folded 
       radial values. */
    diff = vel_reunfold - caz_goodvel;
    difvel_unfrad = abs(diff);
    difvel_frad = veloc[rptr] - caz_goodvel;

    /* Calculate the difference velocity unfolded azimuth and folded   
       azimuth values. */
    diff = vel_reunfold - paz_goodvel;
    difvel_unfaz = abs(diff);
    diff = veloc[rptr] - paz_goodvel;
    difvel_faz = abs(diff);
    dif_unfsq = Square[difvel_unfrad] + Square[difvel_unfaz];
    dif_fsq = Square[abs(difvel_frad)] + Square[difvel_faz];

    /* If the difference of the unfolded squared values is <= the difference
       of the folded squared values, set the velocity at radial pointer to
       the velocity reunfolded value and return. */
    if( dif_unfsq <= dif_fsq ){

	veloc[rptr] = (short) vel_reunfold;
        return rptr;

    }

    /* If the difference of the unfolded squared value is > the difference
       of the folded squared value, set the jump check continue flag to 1. */
    jump_chkcon = 1;

    /* Continue processing until the jump check continue flag is 0. */
    while( jump_chkcon ){

        jump_chkcon = 0;

        /* Checks if the difference velocity folded radial is > the  
           maximum velocity difference in range allowed in the final 
           jump check. */
	if( abs(difvel_frad) > A304dg.veljmp_mxrad ){

            /* If the velocity jump flag is 1 indicating a large velocity      
               jump exists on the current radial and radial pointer minus  
               jump start bin is < the maximum number of bins allowed between 
               velocity jumps of opposite sense on the current radial then 
               determine the new jump direction. */
	    diff = rptr - Jmp_stbin;
	    if( Vel_jump && (abs(diff) < A304di.th_mxbins_jmp) ){

	        Vel_jump = 0;
                if( -difvel_frad < 0 )
		    New_jmpdir = -Nyq_intrvl;

                else
		    New_jmpdir = Nyq_intrvl;

                /* Continue processing when the senses of the new and old jump   
                   directions are equal. */
                if( New_jmpdir == Old_jmpdir ){

		    for( index = Jmp_stbin; index <= rptr; ++index){

		        if( veloc[index] > BAD ) 
			    veloc[index] = (short) (veloc[index] + Old_jmpdir );
			     
		    }

                }
                else{

                    /* When new jump direction is not equal to the old jump direction   
                       set jump check continue flag to true. */
		    jump_chkcon = 1;
		     
                }

	    }
            else{

                /* Set velocity jump to 1 indicating a large velocity jump exists
                   on the current radial. */
	        Vel_jump = 1;
		Jmp_stbin = rptr + 1;
                if( difvel_frad < 0 )
		    Old_jmpdir = -Nyq_intrvl;

                else
	            Old_jmpdir = Nyq_intrvl;

	    }

        }
        else{

            /* Set velocity jump to 0 indicating a large velocity jump does not
               exist on the current radial. */
	    Vel_jump = 0;

        }
	     
    } /* End of while( jump_chkcon ) */

    /* When jump check continue flag is 0, set the terminate reunfolding flag
       to true. */
    return( A304dd.fst_good_dpbin );

/* End of Correct_veloc() */
} 

