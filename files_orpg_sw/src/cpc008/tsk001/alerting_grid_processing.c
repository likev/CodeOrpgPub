/* RCS info */
/* $Author: steves $ */
/* $Locker:  $ */
/* $Date: 2014/11/10 15:26:44 $ */
/* $Id: alerting_grid_processing.c,v 1.8 2014/11/10 15:26:44 steves Exp $ */
/* $Revision: 1.8 $ */
/* $State: Exp $ */

#include <alerting.h>

/* Macro Definitions. */
#define BBRDST 			0
#define BBRDEN			1
#define EBRDST 			114
#define EBRDEN			115

#define TSZE			2

/* Static global variables. */
static short Reftble[GRENZ+1][TSZE] = { {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
                                        {0, 0}, {0, 0}, {29, 29}, {33, 33}, {37, 37},
                                        {41, 41}, {45, 45}, {49, 49}, {53, 53}, {57, 57},  
                                        {61, 61}, {65, 65}, {69, 69}, {73, 73}, {77, 77},
                                        {81, 81}, {85, 85}, {89, 89}, {93, 93}, {97, 97}, 
                                        {101, 101}, {105, 105}, {109, 109}, {113, 113}, {117, 117},
                                        {121, 121}, {125, 125}, {129, 129}, {133, 133}, {137, 137},
                                        {141, 141}, {145, 145}, {149, 149}, {153, 153}, {157, 157},
                                        {161, 161}, {165, 155}, {169, 169}, {173, 173}, {177, 177}, 
                                        {181, 181}, {185, 185}, {189, 189}, {193, 193}, {197, 197},
                                        {201, 201} };

static short Vettble[GREND+1][TSZE] = { {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
                                        {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, 
                                        {0, 0}, {0, 0}, {0, 0}, {0, 0}, {-1, -1}, 
                                        {3, 3}, {7, 7}, {11, 11}, {15, 15}, {19, 19}, 
                                        {23, 23}, {27, 27}, {31, 31}, {35, 35}, {39, 39}, 
                                        {43, 43}, {47, 47}, {51, 51}, {55, 55}, {59, 59}, 
                                        {63, 63}, {67, 67}, {71, 71}, {75, 75}, {79, 79}, 
                                        {83, 83}, {87, 87}, {91, 91}, {95, 95}, {99, 00}, 
                                        {103, 103}, {107, 107}, {111, 111}, {115, 115} };

/* Function Prototypes. */
static int Process_ref_data( int maxval, int maxr, int maxc, int gridcols, 
                             int gridrows, short bitmap[][HW_PER_ROW], 
                             short *grid, int thresh, int thresh1, 
                             int thrcode, int areanum, int usernum, 
                             short *cstat, int cat_cde, int vscnix );
static int Process_vet_data( int maxval, int maxr, int maxc, int gridcols, 
                             int  gridrows, short *grid, short bitmap[][HW_PER_ROW], 
                             int thresh, int thrcode, int areanum, 
                             int usernum, short *cstat, int cat_cde, 
	                     int dtype, int vscnix );
static int Check_vet_data( short bitmap[][HW_PER_ROW], int *new_cond, int *alrtflag, 
                           float *exval, int thresh, short *grid, 
                           int gridcols, int gridrows, float *aboxaz, 
                           float *aboxrng, int usernum, int areanum );
static int Process_velocity( int maxval, int maxc, int maxr, short grid[][NACOL], 
                             int dopmode, short bitmap[][HW_PER_ROW], int thresh, 
                             int thresh1, int thrcode, int areanum, int usernum, 
                             short *cstat, int cat_cde, int vscnix );
static int Get_az_rng( int ialrt, int jalrt, float *az, float *rng);

/*\//////////////////////////////////////////////////////////////////////
//
//   Description:
//      Call appropriate alert grid type subroutine for each area and 
//      user. 
//
//   Inputs:
//      ipr - input buffer pointer.
//      dtype - data type.
//      vscnix - volume scan index.
//
//   Returns:
//      Always returns 0.
//
/////////////////////////////////////////////////////////////////////\*/
int A30851_grid_alert_processing( int *ipr, int dtype, int vscnix ){

    /* Local variables */
    int thresh, areanum, usernum, thrcode;
    int thresh1, cat_cde, act_cat_idx;
    int *memadd, *memcs;

    /* Do For All Users and Areas. */
    for( usernum = 0; usernum < MAX_CLASS1; ++usernum ){

	for( areanum = 0; areanum < NUM_ALERT_AREAS; ++areanum ){

	    if( Uaptr[vscnix][areanum][usernum] != NULL ){
 
                /* Check the index into active categories for a match on 
                   the data type defined in dtype .. -1 means did not find. */
		act_cat_idx = A30859_get_cat( (short *) (Uaptr[vscnix][areanum][usernum] + ACOFF),
		                    (short *) (Uaptr[vscnix][areanum][usernum] + THOFF), 
		                    dtype, &cat_cde, &thrcode );

                /* Non-zero means match found. */
		if( cat_cde != 0 ){

                    /* The units of the threshold in alttable are in knts. */
		    thresh = AH_get_alert_threshold( cat_cde, thrcode );

                    /* Calculate index into this users grid, cstat. */
		    memadd = Uaptr[vscnix][areanum][usernum] + GDOFF;
		    memcs = Uaptr[vscnix][areanum][usernum] + CSTATOFF;

                    /* If Composite Reflectivity ... call processor. */
                    if( dtype == CR_BUFFER ){

                        /* Convert threshold value to scaled/biased value. */
			thresh1 = RPGCS_dBZ_to_reflectivity( thresh );

			Process_ref_data( *(ipr + CHDR + GRID_MAXVAL), 
			                  *(ipr + CHDR + GRID_MAXJ), 
			                  *(ipr + CHDR + GRID_MAXI), 
			                  *(ipr + CHDR + GRID_COL), 
			                  *(ipr + CHDR + GRID_ROW), 
			                  (void *) memadd, (short *) (ipr + CGRID), 
                                          thresh, thresh1, thrcode, areanum, 
			                  usernum, (short *) memcs, act_cat_idx, vscnix );

                    /* If Echo Tops .. call processor. */
		    }
                    else if( dtype == ET_BUFFER ){

                        /* Add bias into value. */
			thresh++;

			Process_vet_data( *(ipr + OMET - 1), *(ipr + OMETR - 1), 
				          *(ipr + OMETC - 1), *(ipr + ONCO - 1), 
				          *(ipr + ONRO - 1), (void *) (ipr + OVET), 
                                          (void *) memadd, thresh, thrcode, areanum, 
                                          usernum, (short *) memcs, act_cat_idx, dtype, 
                                          vscnix );

                    /* If VIL .. call processor. */
		    } 
                    else if( dtype == VIL_BUFFER ){

			Process_vet_data( *(ipr + OMVI - 1), *(ipr + OMVIR - 1), 
				          *(ipr + OMVIC - 1), *(ipr + ONCO - 1),
				          *(ipr + ONRO - 1), (void *) (ipr + OVVI), 
                                          (void *) memadd, thresh, thrcode, areanum, 
                                          usernum, (short *) memcs, act_cat_idx, dtype, 
                                          vscnix );

                    /* If Low Elevation Velocity .. call processor. */
		    }
                    else if( dtype == BVG_BUFFER ){

                        /* Convert threshold to scaled/biased value. */
			thresh1 = RPGCS_ms_to_velocity( RPGCS_get_velocity_reso( *(ipr + OMODE) ), 
                                                        thresh );

			Process_velocity( *(ipr + OMAX), *(ipr + OMAXI), 
                                          *(ipr + OMAXJ), (void *) (ipr + OGRID), 
                                          *(ipr + OMODE), (void *) memadd, thresh, 
			                  thresh1, thrcode, areanum, usernum, 
                                          (short *) memcs, act_cat_idx, vscnix );

		    }

                    /* End Do Until appropriate data type is found. */

		}

	    }
            /* End of test for active user/area. */

	}

    }

    return 0;

/* End of A30851_grid_alert_processing(). */
} 


/*\////////////////////////////////////////////////////////////////////
//
//   Description: 
//      Do the VIL/Echo-Tops data type alert testing. 
//
//   Inputs:
//      maxval - the maximum incoming array value.
//      maxr - the maximum value row position.
//      maxc - the maximum value column position.
//      gridcols - the number of columns in grid.
//      gridowss - the number of rows in grid.
//      grid - the grid to be alerted on.
//      bitmap - the alert area definition.
//      thresh - the alert threshold value.
//      thrcode - the alert threshold code.
//      areanum - the alert area number.
//      usernum  - the alert user number.
//      cat_idx - active category index.
//      dtype - the data type to be alerted on.
//      vxcnix - volume scan index.
//
//   Outputs:
//      cstat - current alert status.
//
//   Returns:
//      Currently this function returns 0.
//
////////////////////////////////////////////////////////////////////\*/
static int Process_vet_data( int maxval, int maxr, int maxc, int gridcols, 
                             int  gridrows, short *grid, short bitmap[][HW_PER_ROW], 
                             int thresh, int thrcode, int areanum, int usernum, 
                             short *cstat, int cat_idx, int dtype, int vscnix ){

    int new_cond, alrtflag, alertbit, alert_stat, nmaxc, nmaxr;
    float exval, aboxaz, aboxrng;

    /* Initialize data. */
    alrtflag = 0;
    alert_stat = NO_ALERT;
    new_cond = NO_ALERT;

    exval = 0.f;

    /* Check for max grid value greater than the alert threshold. */
    if( maxval >= thresh ){

        /* If it is above the alert threshold...see if in an alert area.  
           If in alert area...send alert. */
	nmaxr = (maxr + 1) / 4 + GRSTART;
	nmaxc = (maxc + 1) / 4 + GRSTART;

	alertbit = RPGCS_bit_test_short( (unsigned char *) &bitmap[nmaxr][0], nmaxc );
	if( alertbit ){

	    new_cond = NEW_ALERT;

            /* Call routine to get azimuth and range of the alert box. */
	    Get_az_rng( nmaxc,  nmaxr, &aboxaz, &aboxrng );
	    exval = (float) maxval;

	}
        else {

            /* Otherwise....Do For All alert grid boxes within 232 km. */
	    Check_vet_data( bitmap, &new_cond, &alrtflag, &exval, 
                            thresh, grid, gridcols, gridrows, 
                            &aboxaz, &aboxrng, usernum, areanum );

	}

    }

    /* Call "DO ALERTING" routine if there is a change in the alert status. */
    A3081b_alert_status( new_cond, cstat, cat_idx, &alert_stat );
    if( (alert_stat == NEW_ALERT) || (alert_stat == END_ALERT) ){

        /* If echo tops type...subtract bias. */
	if( dtype == ET_BUFFER ){

	    exval += -1.0;
	    --thresh;

	}

	if( exval < 0.f ) 
	    exval = 0.f;
	
	A30817_do_alerting( usernum, areanum, 0, "    ", aboxaz, aboxrng, thresh, 
                            thrcode, exval, 0, alert_stat, 0.0, -1.0, -1.0,
		            cat_idx, vscnix );

    }

    return 0;

/* End of Process_vet_data(). */
} 


/*\//////////////////////////////////////////////////////////////////
//
//   Description:
//      Do part 2 of the VIL/Echo Tops data type alert testing. 
//
//   Inputs:
//      bitmap - the alert area definition.
//      thresh - the alert threshold value.
//      grid - the grid to be alerted on.
//      gridcols - the nubmer of grid columns.
//      gridrows - the number of grid rows.
//
//   Outputs:
//      new_cond - new alert condition.
//      alrtflag - the "alerted already" flag.
//      exval - the exceeding data value.
//      aboxaz - the alert box azimuth position.
//      aboxrng - the alert box range position.
//
//   Returns:
//      Always returns 0.
//
//////////////////////////////////////////////////////////////////\*/
static int Check_vet_data( short bitmap[][HW_PER_ROW], int *new_cond, 
                           int *alrtflag, float *exval, int thresh, 
                           short *grid, int gridcols, int gridrows, 
                           float *aboxaz, float *aboxrng, int usernum, 
                           int areanum ){

    int alertbit, i, j, ii, jj, iend, jend, istr, jstr;

    *exval = 0.f;

    /* Do For All alert grid boxes within 232 km. */
    for( j = GRSTART; j <= GREND; ++j ){

	for( i = GRSTART; i <= GREND; ++i ){

            /* Check to see if alerted already. */
	    if( *alrtflag != 1 ){

                /* If not...see if current alert box is set. */
		alertbit = RPGCS_bit_test_short( (unsigned char *) &bitmap[j][0], i );

                /* If the alert box ix set then.... */
		if( alertbit ){

                    /* Get I and J start and end positions (of the 4x4) grid from   
                       precalculated look-up table Vettble...search through   
                       only the 4x4 boxes within the 16x16 alert box. */
		    jstr = Vettble[j][JTYPE] - 1;
		    jend = jstr + 3;

		    istr = Vettble[i][ITYPE] - 1;
		    iend = istr + 3;

                    /* Check for boxes on the edge...adjust. */
		    if( i == GRSTART ){

			istr = BBRDST;
			iend = BBRDEN;

		    }
                    else if( i == GREND ){

			istr = EBRDST;
			iend = EBRDEN;

		    }

		    if( j == GRSTART ){

			jstr = BBRDST;
			jend = BBRDEN;

		    }
                    else if( j == GREND ){

			jstr = EBRDST;
			jend = EBRDEN;

		    }

                    /* Do For All 4x4 boxes in the alert box. */
		    for( jj = jstr; jj <= jend; ++jj ){

			for( ii = istr; ii <= iend; ++ii ){

                            /* If the alert threshold is exceeded...send alert. */
			    if( grid[jj*gridrows + ii] >= thresh ){

                                /* Check to see if alerted already. */
				if( *alrtflag != 1 ){

				    *new_cond = NEW_ALERT;

                                    /* Call routine to get az and range of alert box */
				    Get_az_rng( i, j, aboxaz, aboxrng );

				    *exval = (float) grid[jj*gridrows + ii];

                                    /* Set "alert set" flag */
				    *alrtflag = 1;

				}

                                /* Get the largest exceeding value in alert box */
				if ((float) grid[jj*gridrows + ii] > *exval)
				    *exval = (float) grid[jj*gridrows + ii];

			    }

			} /* End of for( ii = .... ) */

		    } /* End of for( jj = ...... ) */

		} /* End of if(alertbit) .... */

	    } /* End of if( *alrtflag != 1 ) */

	}

    }

    return 0;

/* End of Check_vet_data(). */
} 

#define VEL_OFFSET		2.0
#define VEL_BIAS		63.5
#define VEL_SCALE		2.0
/*\////////////////////////////////////////////////////////////////////////
//
//   Description:
//      Do the lowest elevation Doppler velocity data type alert testing.
//
//   Inputs:
//      maxval - maximum value in the grid.
//      maxc - column where maximum value resides.
//      maxr - row where maximum value resides.
//      grid - low elevation velocity  grid.
//      dopmode - Doppler resolution.
//      bitmap - alert grid.
//      thresh - threshold value.
//      thresh1 - scaled/biased threshold value.
//      thrcode - threshold code.
//      areanum - alert grid number.
//      usernum - user number.
//      cstat - array of current alert status.
//      cat_idx - alert category index.
//      vscnix - volume scan index.
//
//   Returns:
//      Always returns 0.
//
////////////////////////////////////////////////////////////////////////\*/
static int Process_velocity( int maxval, int maxc, int maxr, short grid[][NACOL], 
                             int dopmode, short bitmap[][HW_PER_ROW], int thresh, 
                             int thresh1, int thrcode, int areanum, int usernum, 
                             short *cstat, int cat_idx, int vscnix ){

    /* Local variables. */
    int new_cond, alrtflag, alertbit, i, j, alert_stat;
    float exval, aboxaz, aboxrng;

    /* Init variables. */
    alrtflag = 0;
    alert_stat = NO_ALERT;
    new_cond = NO_ALERT;

    exval = 0.f;

    /* The row and column indices are assumed 1-indexed (vice 0-indexed).
       Therefore we subtract 1 from both indices. */
    maxc--;
    maxr--;

    /* If the current max value exceeds the alert threshold, proceed. */
    if( maxval >= thresh1 ){

        /* Test to see if the max value is in an alert box. */
	alertbit = RPGCS_bit_test_short( (unsigned char *) &bitmap[maxr][0], maxc );

        /* If the max value is in an alert box, send alert. */
	if( alertbit ){

	    new_cond = NEW_ALERT;

            /* Call routine to get azm and range of the alert box. */
	    Get_az_rng( maxc, maxr, &aboxaz, &aboxrng );

	    exval = (float) maxval;

	}
        else{

            /* Do For All alert grid boxes within 232 km. */
	    for( j = GRSTART; j <= GREND; ++j ){

		for( i = GRSTART; i <= GREND; ++i ){

                    /* Check to see if area has been alerted already. */
		    if( alrtflag != 1 ){

                        /* Check to see if box is in alert area. */
			alertbit = RPGCS_bit_test_short( (unsigned char *) &bitmap[j][0], i );

                        /* If box is in alert area, then .... */
			if( alertbit ){

                            /* Check to see if grid box value is above alert threshold. 
                               If it is, alert area. */
			    if( grid[j][i] >= thresh1 ){

				new_cond = NEW_ALERT;

				exval = (float) grid[j][i];

                                /* Call routine to get azm and range of alert box. */
				Get_az_rng( i, j, &aboxaz, &aboxrng );

				alrtflag = 1;

			    }

			}

		    }

		}

	    }

	}

    }

    /* Call "Do Alerting" function if there is a change in the alert status. */
    A3081b_alert_status( new_cond, cstat, cat_idx, &alert_stat );

    if( (alert_stat == NEW_ALERT) || (alert_stat == END_ALERT) ){

        /* Convert exceeding value to a real number. Note: The value in the
           grid is the abs(vel).  The base velocity grid task converts
           vel value 2 to 256 since the velocity range is not symmetric.
           Therefore we can't use RPGCS_velocity_to_ms() since 256 is 
           not a value velocity. */
	exval = (exval - VEL_OFFSET)*(dopmode/VEL_SCALE) - (VEL_BIAS*dopmode);
	if( exval < 0.f )
	    exval = 0.f;

        /* Convert the exceeding value to Englist Units for the alert message. */
	exval *= MPS_TO_KTS;

        /* Call "Do Alerting" function. */
	A30817_do_alerting( usernum, areanum, 0, "    ", aboxaz, aboxrng, thresh, 
                            thrcode, exval, 0, alert_stat, 0.0, -1.0, -1.0,
		            cat_idx, vscnix );

    }

    return 0;

/* End of Process_velocity(). */
} 


/*\////////////////////////////////////////////////////////////////////////
//
//   Description:
//      Process the reflectivity data type alert testing. 
//
//   Inputs:
//      maxval - the maximum incoming array value.
//      maxr - the maximum value row position.
//      maxc - the maximum value column position.
//      gridcols - number of columns in grid.
//      gridrows - number of rows in grid.
//      bitmap - the alert area definition.
//      grid - the grid to be alerted on.
//      thresh - the threshold value.
//      thresh1 - the scaled/biased threshold value.
//      thrcode - the threshold code.
//      areanum - the alert area number.
//      usernum - the alert user number.
//      cat_idx - category index.
//      vxcnix -  volume scan index number.
//
//   Outputs:
//      cstat - current alert status.
//
//   Returns:
//      Always returns 0.
//
////////////////////////////////////////////////////////////////////////\*/
static int Process_ref_data( int maxval, int maxr, int maxc, int gridcols, 
                             int gridrows, short bitmap[][HW_PER_ROW], 
                             short *grid, int thresh, int thresh1, 
                             int thrcode, int areanum, int usernum, 
                             short *cstat, int cat_idx, int vscnix ){

    /* Local variables */
    int new_cond, alrtflag, alertbit, i, j, ii, jj, alert_stat;
    int ni, iend, jend, istr, jstr, nmaxc, nmaxr;
    float exval, aboxaz, aboxrng;

    /* Initialize variables. */
    exval = 0.f;
    alrtflag = 0;
    alert_stat = NO_ALERT;
    new_cond = NO_ALERT;

    /* If the current maxval is greater than the alert threshold. */
    if( maxval >= thresh1 ){

        /* Check to see if the max value grid box is within alert box. 
           Note: nmaxr and nmaxc are 0 indexed values. */
	nmaxr = (maxr - 1) / 4;
	nmaxc = (maxc - 1) / 4;

	alertbit = RPGCS_bit_test_short( (unsigned char *) &bitmap[nmaxr][0], nmaxc );
	if( alertbit ){

	    new_cond = NEW_ALERT;

            /* Call routine to get azimuth and range of alert box. */
	    Get_az_rng( nmaxc, nmaxr, &aboxaz, &aboxrng);

	    exval = (float) maxval;

            /* If it is...alert area. */
	}
        else{

            /* Do For All alert grid boxes within the maximum range. */
	    for( j = GRSTZ; j <= GRENZ; ++j ){

		for( i = GRSTZ; i <= GRENZ; ++i ){

                    /* IR area hasn't been alerted already...proceed. */
		    if( alrtflag != 1 ){

                        /* Check to see if alert box is set. */
			alertbit = RPGCS_bit_test_short( (unsigned char *) &bitmap[j][0], i );

                        /* If the box is in an alert area. */
			if( alertbit ){

                            /* Get I and J start and end of ref grid boxes in alert box   
                               from look-up table Reftble. */
			    jstr = Reftble[j][JTYPE] - 1;
			    jend = jstr + 3;

			    istr = Reftble[i][ITYPE] - 1;
			    iend = istr + 3;

                            /* Do For All 4x4 ref grid boxes in 16x16 alert area. */
			    for( jj = jstr; jj <= jend; ++jj ){

				for( ii = istr; ii <= iend; ++ii ){

                                    /* If the 4x4 grid box value is greater than the alert 
                                       threshold, alert for that area. */

				    if( grid[jj*gridrows + ii] >= thresh1 ){

                                        /* If area hasn't been alerted already...proceed. */
					if( alrtflag != 1 ){

					    new_cond = NEW_ALERT;

                                            /* Call routine to get azimuth and range of the 
                                               alert box. */
					    Get_az_rng( i, j, &aboxaz, &aboxrng );

					    exval = (float) grid[jj*gridrows+ii]; 

                                            /* Set "Alert Set" flag. */
					    alrtflag = 1;

					}

                                        /* Get the largest exceeding value in alert box. */
					if( (float) grid[jj*gridrows + ii] > exval ) 
					    exval = (float) grid[jj*gridrows+ii]; 

				    }


				}

			    }

			}

		    }

		}

	    }

	}

    }

    /* Call "DO ALERTING" routine if it's not an old alert. */
    A3081b_alert_status( new_cond, cstat, cat_idx, &alert_stat );
    if( (alert_stat == NEW_ALERT) || (alert_stat == END_ALERT) ){


        /* Convert scaled/biased exceeding value to real value. */
        ni = (int) RPGC_NINT( exval );
	exval = RPGCS_reflectivity_to_dBZ( ni );
	if( exval < 0.f ) 
	    exval = 0.f;

        /* Call "DO ALERTING" routine. */
	A30817_do_alerting( usernum, areanum, 0, "    ", aboxaz, aboxrng, 
                            thresh, thrcode, exval, 0, alert_stat, 0.0, -1.0,
		            -1.0, cat_idx, vscnix );
    }

    return 0;

/* End of Process_ref_data(). */
} 


/*\///////////////////////////////////////////////////////////////////////
//
//   Description:
//      Determine the azimuth and range of the center of the alert box 
//      with the alert. 
//
//   Inputs:
//      ialrt - alert box I index.
//      jalrt - alert box J index.
//
//   Outputs:
//      az - azimuth (deg) of the alert box center.
//      rng - range (km) of the alert box center.
//
//////////////////////////////////////////////////////////////////////\*/
static int Get_az_rng( int ialrt, int jalrt, float *az, float *rng){

    float distx, disty;

    /* Get x and y box distances. */
    distx = ((float) ialrt - MIDPNTI - .5f) * ISIZE;
    disty = (MIDPNTJ - (float) jalrt + .5f) * JSIZE;

    /* Get the box range , in km. */
    *rng = (float) sqrtf( (float) ((distx*distx) + (disty*disty)) );

    /* Get the box azimuth, in deg. */
    *az = atan2f(distx, disty)/DTR;

    /* If a negative angle...adjust it */
    if( *az < 0.f ) 
	*az += 360.f;

    return 0;

/* End of Get_az_rng(). */
} 

