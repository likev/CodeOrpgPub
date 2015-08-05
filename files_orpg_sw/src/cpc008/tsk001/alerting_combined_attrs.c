/* RCS info */
/* $Author: steves $ */
/* $Locker:  $ */
/* $Date: 2009/07/10 16:18:02 $ */
/* $Id: alerting_combined_attrs.c,v 1.5 2009/07/10 16:18:02 steves Exp $ */
/* $Revision: 1.5 $ */
/* $State: Exp $ */

#include <alerting.h>

/* Function Prototypes. */
static int Check_stm_cat( short *active_cats, int *nsc, int *nfc );
static int Volume_cat_test( int user, int area, int vscnix, short *active_cats, 
                            short *thres_code, short *ccat_stat, 
                            int cat_num_storms, int num_fposits, int *cat_feat, 
                            float *comb_att, float *forcst_posits );
static int Vol_cat_check( int dtype, int stmix, int cat_feat[][CAT_NF], 
                          float comb_att[][CAT_DAT], float *forcst_posits, 
                          int threshold_code, int *new_cond, int *delta_thres );
static int Map_stms( short agrid[][BROWS], int cat_num_storms, 
                     float comb_att[][CAT_DAT] );
static int Map_fposits( short agrid[][BROWS], int cat_num_storms, 
                        int num_fposits, float comb_att[][CAT_DAT], 
                        float forcst_posits[][MAX_FPOSITS][FOR_DAT] );
static int Line_clip( float x1, float y1, float x2, float y2, float xmin, 
                      float xmax, float ymin, float ymax, int *intersect );
static int Get_ijlimit( int *fposi, int *fposj, int *mini, int *maxi, 
                        int *minj, int *maxj, int nposits );
static int Determine_storm_centers( int stmix, float az, float rng, 
                                    short bitmap[][BROWS] );


/*////////////////////////////////////////////////////////////////////////
//
//   Description:
//      Map forecasted positions to user/area and process storm 
//      categories.
//
//   Inputs:
//      nstorms - number of storms.
//      numfpos - number of forecast positions.
//      ipr1 - input buffer containing storm information.
//      vscnix - volume scan index.
//
//   Returns:
//      Always returns 0.
//
////////////////////////////////////////////////////////////////////// */        
int A30810_comb_att( int nstorms, int numfpos, int *ipr1, int vscnix ){

    int user, areanum, jj, jjj;
    int numsreq, numfreq;

    /* For All Users. */
    for( user = 0; user < MAX_CLASS1; ++user ){

        /* For All User Areas. */
	for( areanum = 0; areanum < NUM_ALERT_AREAS; ++areanum ){

            /* Determine if this user area is defined. */
	    if( Uaptr[vscnix][areanum][user] != 0) {

                /* Clear storms and forecasted storms directories. */
		for( jj = 0; jj < CAT_MXSTMS; ++jj ){

		    Strmdir[jj] = 0;
		    Fordir[jj] = 0;
		    for (jjj = 0; jjj < MAX_FPOSITS; ++jjj) 
			Fposdir[jj][jjj] = 0;


		}

                /* Has this user requested storm categories? */
		Check_stm_cat( (short *) (Uaptr[vscnix][areanum][user] + ACOFF), 
                               &numsreq, &numfreq);

		if( (numsreq != 0) || (numfreq != 0) ){

		    if( numsreq != 0 ){

                        /* Map the storms to this users area. */
			Map_stms( (void *) (Uaptr[vscnix][areanum][user] + GDOFF), 
                                  nstorms, (void *) (ipr1 + CATT) );

		    }

                    /* Any forecasted categories requested? */
		    if( numfreq != 0 ){

                        /* Map forecasted positions to this users area. */
			Map_fposits( (void *) (Uaptr[vscnix][areanum][user] + GDOFF), 
                                     nstorms, numfpos, (void *) (ipr1 + CATT), 
                                     (void *) (ipr1 + CNFST) );

		    }


                    /* Process storm categories. */
		    Volume_cat_test( user, areanum, vscnix, 
                                     (short *) (Uaptr[vscnix][areanum][user] + ACOFF),
			             (short *) (Uaptr[vscnix][areanum][user] + THOFF), 
			             (short *) (Uaptr[vscnix][areanum][user] + CSTATOFF),
			             nstorms, numfpos, (void *) (ipr1 + CFEA), 
                                     (void *) (ipr1 + CATT), (void *) (ipr1 + CNFST) );

		}

	    }

	}

    }

    return 0;

/* End of A30810_comb_att(). */
} 

/*///////////////////////////////////////////////////////////////////
//
//   Description:
//      Counts the number of storm or forecast type categories that
//      are requested for a particular user/area. 
//
//   Inputs:
//      active_cats - array of active categories.
//
//   Outputs:
//      nsc - number of storm categories.
//      nfc - number of forecast categories.
//
//   Returns:
//      Always returns 0.
//
////////////////////////////////////////////////////////////////// */
static int Check_stm_cat( short *active_cats, int *nsc, int *nfc ){

    int i, num_storm_cats, num_forecast_cats;

    /* Initialize the number of storm categories and number of 
       forecast categories. */
    num_storm_cats = 0; 
    num_forecast_cats = 0;

    /* Count the categories in the list of active categories. */
    for( i = 0; i < MAX_ALERT_CATS; ++i ){

	if( ((active_cats[i] >= 8) && (active_cats[i] <= 14))
                                || 
		(active_cats[i] == 16) )
	    num_storm_cats++;

	if( (active_cats[i] >= 25) && (active_cats[i] <= 32) ) 
	    num_forecast_cats++;
	 
    }

    *nsc = num_storm_cats;
    *nfc = num_forecast_cats;

    return 0;

/* End of Check_stm_cat(). */
} 

/*///////////////////////////////////////////////////////////////////////
//
//   Description:
//      Test for category alerts for storm and forecast. 
//
////////////////////////////////////////////////////////////////////// */
static int Volume_cat_test( int user, int area, int vscnix, short *active_cats, 
                            short *thres_code, short *ccat_stat, 
                            int cat_num_storms, int num_fposits, int *cat_feat, 
                            float *comb_att, float *forcst_posits ){

    /* Local variables */
    int new_cond, sav_cond, threshold_code, sexval1, cat_typ, sthresh;
    int l, save_delta, delta_thres, alert_status, cat_code, stmix;
    float saz, sran, selev, sexval, sstmdir, sstmspd;
    char sstmid[4];

    /* Initilaize some variables. */
    delta_thres = 0;

    /* Do For All categories. */
    for( l = 0; l < MAX_ALERT_CATS; ++l ){

	cat_typ = UNDEFINED_CAT_TYPE;
	if( ((active_cats[l] >= 8) && (active_cats[l] <= 14))
                                || 
             (active_cats[l] == 16) )
	    cat_typ = VOLUME_CAT_TYPE;

	if( (active_cats[l] >= 25) && (active_cats[l] <= 32) )
	    cat_typ = FORECAST_CAT_TYPE;

        /* Is the category type part of the volume or forecast groups? */
	if( (cat_typ == VOLUME_CAT_TYPE) || (cat_typ == FORECAST_CAT_TYPE) ){

	    cat_code = active_cats[l];
	    threshold_code = thres_code[l];
	    save_delta = -1;
	    sav_cond = 0;

            /* Initialize some variables. */ 
	    memset( sstmid, ' ', 4 );
	    saz = 0.f;
	    sran = 0.f;
	    sthresh = 0;
	    sexval = 0.f;
	    sexval1 = 0;
	    selev = 0.f;
	    sstmspd = 0.f;
	    sstmdir = 0.f;

            /* Do For All storms. */
	    for( stmix = 0; stmix < cat_num_storms; ++stmix ){

		new_cond = NO_ALERT;

                /* Determine if this storm or the storms forecast position
                   is in the user area. */
		if( ((cat_typ == VOLUME_CAT_TYPE) && (Strmdir[stmix] != 0)) 
                                    || 
		    ((cat_typ == FORECAST_CAT_TYPE) && (Fordir[stmix] != 0)) ){

                    /* Determine if the user's threshold is exceeded by the 
                       current data in the input data. "new_cond" is returned 
                       as 0 if alert conditions not met, as 1 if this category 
                       met the alert conditions. */
		    Vol_cat_check( cat_code, stmix, (void *) cat_feat, 
                                   (void *) comb_att, forcst_posits, 
                                   threshold_code, &new_cond, &delta_thres );

		    if( (delta_thres > save_delta) && (new_cond != NO_ALERT) ){

                        /* Save the elevation and storm stuff.  The storm passed
                           the threshold. */
			save_delta = delta_thres;
			sav_cond = new_cond;
			saz = A308c3.az;
			sran = A308c3.ran;
			selev = A308c3.elevang;
			memcpy( sstmid, A308c3.stmid, 4 );
			sstmspd = A308c3.stmspd;
			sstmdir = A308c3.stmdir;
			sexval = A308c3.exval;
			sexval1 = A308c3.exval1;
			sthresh = A308c3.threshold;

		    }

		}

	    }

            /*  Determine alert status per the category across all storms. */
	    A3081b_alert_status( sav_cond, ccat_stat, l, &alert_status );

            /* Send out alarm if first or ending alert. */
	    if( (alert_status == NEW_ALERT) || (alert_status == END_ALERT) )
		A30817_do_alerting( user, area, stmix, sstmid, saz, sran,
			            sthresh, threshold_code, sexval, sexval1, 
			            alert_status, selev, sstmspd, sstmdir, l, 
			            vscnix );

	}

    }

/* End of Volume_cat_test(). */
    return 0;

} 

/*////////////////////////////////////////////////////////////////////////
//
//   Description:
//      This function maps the storms in the combined attributes table
//      to the input user grid.  The output is the storm directory which  
//      One word per storm is set if the sotrm is in the user area.
//
//   Inputs:
//      agrid - user alert grid.
//      cat_num_storms - number of storms in the combined atttributes
//                       table.
//      comb_att - combined attributes data. 
//
//   Returns:
//      Always returns 0.
//
/////////////////////////////////////////////////////////////////////// */
static int Map_stms( short agrid[][BROWS], int cat_num_storms, 
                     float comb_att[][CAT_DAT] ){

    /* Local variables */
    int k;
    float az, ran;

    /* Convert storm center to I/J box indices. */

    /* Do For All storms in the combined attributes table. */
    for( k = 0; k < cat_num_storms; ++k ){

	az = comb_att[k][CAT_AZ];
	ran = comb_att[k][CAT_RNG];
	Determine_storm_centers( k, az, ran, agrid );

    }

    return 0;

/* End of Map_stms(). */
} 

#define MIDPOINT_H			29.5f
#define MIDPOINT_I			29.0f
#define MIDPOINT_J			29.0f

/*///////////////////////////////////////////////////////////////////////
//
//   Description:
//      Map the forecasted positions of an input storm to the input
//      user grid.
//
//   Inputs:
//      agrid - user alert grid. 
//      cat_num_storms - number of storms processed for the combined 
//                       attributes table.
//      num_fposits - number of forecasted positions in table.
//      comb_att - table of combined attributes.
//      forcst_posits - table of storm forcasted positions.
//
//   Returns:
//      Always returns 0.
//
////////////////////////////////////////////////////////////////////// */
static int Map_fposits( short agrid[][BROWS], int cat_num_storms, 
                        int num_fposits, float comb_att[][CAT_DAT], 
                        float forcst_posits[][MAX_FPOSITS][FOR_DAT] ){

    int alertbit, i, j, intersect, kp, have_track, mini, minj;
    int maxi, maxj, ialrt, jalrt, fposi[4], fposj[4], stmix, nposits;
    float minx, miny, maxx, maxy, elrad, azrad, x_pos, y_pos;
    float distx, disty, strmx, strmy, fx_pos[4], fy_pos[4];

    /* Do For All storms ... */
    for( stmix = 0; stmix < cat_num_storms; ++stmix ){

        /* Initialize tables that will contain the I,J grid positions 
           of the storm forecast positions to the 'NO DATA' flag.  The 
           entries will be changed for the positions that have data 
           defined. */
	for( i = 0; i < num_fposits; ++i ){

	    fposi[i] = NODATA;
	    fposj[i] = NODATA;

	}

        /* Convert forecasted positions to I,J box indices. */
	have_track = 0;
	nposits = 0;

	for( kp = 0; kp < num_fposits; ++kp ){

	    x_pos = forcst_posits[stmix][kp][CAT_FX];
	    y_pos = forcst_posits[stmix][kp][CAT_FY];

            /* Save the X and Y positions for the forecast positions in an 
               array to be used in the call to the line clipping routine. */
	    fx_pos[kp] = x_pos;
	    fy_pos[kp] = y_pos;

            /* Does X position exist? */
	    if( x_pos > NODATA ){

                /* Does Y positions exist? */
		if( y_pos > NODATA ){

		    ialrt = RPGC_NINT( x_pos / ISIZE  + MIDPOINT_H );
		    jalrt = RPGC_NINT( MIDPOINT_H - y_pos / JSIZE );

                    /* Set storm track flag and increment positions. */
		    have_track = 1;
		    ++nposits;

                    /* Save the I and J (grid coordinates) of the X,Y (km). */
		    fposi[kp] = ialrt;
		    fposj[kp] = jalrt;

                    /* Check bit map.  Set alertbit if grid element active. */
		    alertbit = RPGCS_bit_test_short( (unsigned char *) &agrid[jalrt-1][0], ialrt-1 );

                    /* If alertbit flag is set - update the forecast directory 
                       one forecast position. */
		    if( alertbit )
			Fposdir[stmix][kp] = 1;

		}

	    }

	}

        /* Set flag in Fordir for storm if at least one forecast position i
           is in an active user area. */
	for( kp = 0; kp < num_fposits; ++kp ){

	    if( Fposdir[stmix][kp] != 0 ){

		Fordir[stmix] = 1;
		goto L400;

	    }

	}

        /* Falling thru to here means either none of individual forecast 
           points falls into a user read, or that there were no defined 
           forecast positions.  We will now determine if the track passes 
           thru any of the designated alert boxes by using the line clipping 
           algorithm. */
	if( !have_track )
	    goto L400;

	Get_ijlimit( fposi, fposj, &mini, &maxi, &minj, &maxj, nposits );

        /* If enough positions are defined check for the line of the track 
           passing through a user box. */
	if( nposits >= 1 ){

            /* Get the X/Y of the storm center as the first point of the track. */
	    azrad = comb_att[stmix][CAT_AZ] * DTR;
	    elrad = comb_att[stmix][CAT_ELCN] * DTR;

	    strmx = comb_att[stmix][CAT_RNG] * sinf(azrad) * cosf(elrad);
	    strmy = comb_att[stmix][CAT_RNG] * cosf(azrad) * cosf(elrad);

	    for( j = minj; j <= maxj; ++j ){

		for( i = mini; i <= maxi; ++i ){

		    alertbit = RPGCS_bit_test_short( (unsigned char *) &agrid[j-1][0], i-1 );

		    if( alertbit ){

                       /* Calculate the X and Y points of the grid box to be used 
                          in the clipping routine. */
			distx = (i - MIDPOINT_I - .5f) * ISIZE;
			disty = (MIDPOINT_J - j + .5f) * JSIZE;

                        /* Compute the max and min of the X and Y points. */
			maxx = distx + ISIZE/2;
			maxy = disty + JSIZE/2;

			minx = distx - ISIZE/2;
			miny = disty - JSIZE/2;

                        /* Call the clipping routine for line intersection. */
			Line_clip( strmx, strmy, fx_pos[nposits-1], fy_pos[nposits-1], 
                                   minx, maxx, miny, maxy, &intersect );

			if( intersect ){
			    Fordir[stmix] = 1;

                            /* The goto is to jump out of loop when one hit is made. 
                               This will save processing of looking any further. */
			    goto L400;

			}

		    }

		}

	    }

            /* A fall thru to here means that the track did not pass thru the 
               box or there were not enough points in the track. */

	}
L400:

	;
    }

    return 0;

/* End of Map_fposits(). */
} 

/*////////////////////////////////////////////////////////////////////////
//
//   Description:
//      Given the X and Y limits of a line and a window, this subroutine 
//      determines if the line intersects the window. 
//
//      The algorithm used to determine this information is known as the
//      "Cohen-Sutherland Clipping Algorithm". 
//
//      Reference: Fundamentals of Interactive Computer Graphics, by 
//      J.D. Foley and A. Van Dam. Addison-Wesley Publ. Co. July 1984
//
//
//   Inputs:
//      x1 - x coordinate of line endpoint 1.
//      y1 - y coordinate of line endpoint 1.
//      x2 - x coordinate of line endpoint 2.
//      y2 - y coordinate of line endpoint 2.
//      xmin - Minimum window range in the x direction.
//      xmin - Maximum window range in the x direction.
//      ymin - Minimum window range in the y direction.
//      ymin - Maximum window range in the y direction.
//
//   Outputs:
//      intersect - flag indicating whether line intersects window. 
//
//   Returns:
//
/////////////////////////////////////////////////////////////////////// */
static int Line_clip( float x1, float y1, float x2, float y2, float xmin, 
                      float xmax, float ymin, float ymax, int *intersect ){

    int subocode, tmpocode, i, done, accept, reject, outcode[2];
    float x[2], y[2], tmpx, tmpy;

    /* Initialize x(1) and y(1) and x(2) and y(2). */
    x[0] = x1;
    y[0] = y1;
    x[1] = x2;
    y[1] = y2;

    /* Initialize accept, reject and done float to zero. */
    accept = 0;
    reject = 0;
    done = 0;

    /* Repeat until doen (i.e. until clipped endpoints found or it is   
       determined azimuth does not intersect window. */
    while( !done ){

        /* Set outcodes for the endpoints of the azimuth radial. */
	for( i = 0; i < 2; ++i ){

            /* First initialize it to zero. */
	    outcode[i] = 0;

            /* Bit 1 - endpoint is above window. */
	    if( ymax < y[i] ) 
		RPGCS_bit_set( (unsigned char *) &outcode[i], 1 );

            /* Bit 2 - endpoint is below window. */
	    if( y[i] < ymin ) 
		RPGCS_bit_set( (unsigned char *) &outcode[i], 2 );

            /* Bit 3 - endpoint is right of window. */
	    if( xmax < x[i] ) 
		RPGCS_bit_set( (unsigned char *) &outcode[i], 3 );
	    
            /* Bit 4 - endpoint is left of window. */
	    if( x[i] < xmin ) 
		RPGCS_bit_set( (unsigned char *) &outcode[i], 4 );

	}

        /* The line can be tribially rejected if both endpoints are above, 
           below, left, or right of the window. This case is tested by taking 
           the logical and of the outcodes and testing if the result is not 0. */
	reject = outcode[0] & outcode[1];

	if( reject != 0 ){

            /* Line is trivially rejected. */
	    *intersect = 0;
	    done = 1;

	}
        else{

            /* Next test if the line can be trivially accepted.  This case  
               occurs when both endpoints are within the window.  This case is  
               tested by taking the "inclusive OR" of the outcodes and testing   
               if the result is equal to zero. */
	    accept = outcode[0] | outcode[1];
	    if( accept == 0 ){

                /* Line is trivially accepted. */
		*intersect = 1;
		done = 1;

	    }
            else {

                /* Sub-divide the line since at most 1 endpoint is within the window.
                   If (x(1),y(1)) is inside the window, exchange point 1 and 2 and   
                   their outcodes to guarantee point 1 is outside the window. */
		if( outcode[0] == 0 ){

                    /* Swap endpoints - P1 is to be outside window. */
		    tmpx = x[0];
		    tmpy = y[0];

                    /* Put outcode one in temporary outcode. */
		    tmpocode = outcode[0];

                    /* Swap endpoints. */
		    x[0] = x[1];
		    y[0] = y[1];

                    /* Swap outcodes. */
		    outcode[0] = outcode[1];

                    /* Put temporary endpoints into P2. */
		    x[1] = tmpx;
		    y[1] = tmpy;
		    outcode[1] = tmpocode;

		}

                /* First check if point 1 is above the window, if so compute new   
                   x and y at y = ymax. */
		subocode = RPGCS_bit_test( (unsigned char *) outcode, 1 );
		if( subocode ){

		    x[0] += (x[1] - x[0]) * (ymax - y[0]) / (y[1] - y[0]);
		    y[0] = ymax;

		}
                else{

                   /* Else ... check if point 1 is below the window, if so compute new   
                      x and y at y = ymin. */
		    subocode = RPGCS_bit_test( (unsigned char *) outcode, 2 );
		    if( subocode ){

			x[0] += (x[1] - x[0]) * (ymin - y[0]) / (y[1] - y[0]);
			y[0] = ymin;

		    }
                    else{

                        /* Else ... check if point 1 is right of the window, if so compute new   
                           x and y at x = xmax. */
			subocode = RPGCS_bit_test( (unsigned char *) outcode, 3 );
			if( subocode ){

			    y[0] += (y[1] - y[0]) * (xmax - x[0]) / (x[1] - x[0]);
			    x[0] = xmax;

			}
                        else{

                           /* Finally ... check if point 1 is left of the window, if so compute   
                              new x and y at x = xmin. */
			    subocode = RPGCS_bit_test( (unsigned char *) outcode, 4 );
			    if( subocode ){

				y[0] += (y[1] - y[0]) * (xmin - x[0]) / (x[1] - x[0]);
				x[0] = xmin;

			    }

			}

		    }

		}

	    }

	}

    }

    return 0;

/* End of Line_clip(). */ 
} 

/*//////////////////////////////////////////////////////////////////////
//
//   Description:
//      This function computes the limits of a rectangle within the 
//      alert grid that will enclose the forecast storm track.
//
//   Inputs:
//      fposi - table of forecast I positions.
//      fposj - table of forecast J positions.
//      nposits - number of positions in forecast track.
//
//   Outputs:
//      mini - minimum I position of the track.
//      maxi - maximum I position of the track.
//      minj - minimum J position of the track.
//      maxj - maximum J position of the track.
//
//   Returns:
//      Always returns 0.
//      
///////////////////////////////////////////////////////////////////// */
static int Get_ijlimit( int *fposi, int *fposj, int *mini, int *maxi, 
                        int *minj, int *maxj, int nposits ){

    int i;

    /* Initialize the minumums to a large number and the maximums to 0
       so that the first compare will cause a replace. */
    *mini = 999;
    *minj = 999;

    *maxi = (int) NODATA;
    *maxj = (int) NODATA;

    if( nposits >= 1 ){

	for( i = 0; i < nposits; ++i ){

	    if( fposi[i] > *maxi ) 
		*maxi = fposi[i];
	    
	    if( (fposi[i] < *mini) && (fposi[i] != (int) NODATA) ) 
		*mini = fposi[i];

	    if( fposj[i] > *maxj ) 
		*maxj = fposj[i];
	    
	    if( (fposj[i] < *minj) && (fposj[i] != (int) NODATA) ) 
		*minj = fposj[i];
	    
	}

    }

    return 0;

/* End of Get_ijlimit(). */
} 

/*/////////////////////////////////////////////////////////////////////////
//
//   Description:
//      Determine if the input category meets the requested threshold.  
//      Also, sets up the storm parameters needed in the alert message. 
//
//   Inputs:
//      cat_code - the alert category code.
//      stmix - storm index.
//      cat_feat - table of associated severe features.
//      comb_att - table of combined attributes.
//      forcst_posits - table of forecast positions.
//      threshold_code - threshold code index
//
//   Outputs:
//      new_cond - new alert condition.
//      delta_thres - delta between thresholds.
//
//
//////////////////////////////////////////////////////////////////////// */
static int Vol_cat_check( int cat_code, int stmix, int cat_feat[][CAT_NF], 
                          float comb_att[][CAT_DAT], float *forcst_posits, 
                          int threshold_code, int *new_cond, int *delta_thres ){

    /* Local variables */
    int tdata;

    /* This translation table is indexed by the data value that 
       is in the combined attribute table in order to set a 
       data value (really category code) to be tested against 
       the corresponding value in the alert threshold table. */
    static int trans_tvs[4] = { 0, 0,2,1 };

    /* Initialize 'test data' variable and 'new condition' flag. */
    tdata = 0;
    *new_cond = NO_ALERT;

    /* Store the storm speed and direction in the global structure. */
    A308c3.stmspd = comb_att[stmix][CAT_FSPD];
    A308c3.stmdir = comb_att[stmix][CAT_FDIR];

    /* Set up stormid in global structure. */
    memcpy( A308c3.stmid, &cat_feat[stmix][CAT_SID], 4 );

    /* Process alert for category codes 9-10, 12-13, 26-27, OR 29-30: 
       Note: These are the PROB HAIL, PROB SVR HAIL, MDA, and TVS 
             categories for the volume and forecast group. */
    if( (cat_code == 10) || ((cat_code >= 12) && (cat_code <= 13)) 
                            || 
        (cat_code == 27) || ((cat_code >= 29) && (cat_code <= 30)) 
                            || 
        (cat_code == 16) || (cat_code == 32) ){

        /* Test for MDA.  Window info is elev, az, ran of MDA feature base. */
	if( (cat_code == 16) || (cat_code == 32) ){

	    tdata = comb_att[stmix][CAT_SRMDA];
	    A308c3.elevang = comb_att[stmix][CAT_ELMDA];
	    A308c3.az = comb_att[stmix][CAT_AZ];
	    A308c3.ran = comb_att[stmix][CAT_RNG];

	}

        /* Test for PROB HAIL.  Window info is maxz, elev, az and ran of storm. */
	if( (cat_code == 12) || (cat_code == 29) ){

	    tdata = cat_feat[stmix][CAT_POH];

	    A308c3.elevang = comb_att[stmix][CAT_ELVXZ];
	    A308c3.az = comb_att[stmix][CAT_AZ];
	    A308c3.ran = comb_att[stmix][CAT_RNG];

	}

        /* Test for PROB SVR HAIL.  Window info is maxz, elev, az and ran of storm. */
	if( (cat_code == 13) || (cat_code == 30) ){

	    tdata = cat_feat[stmix][CAT_POSH];

	    A308c3.elevang = comb_att[stmix][CAT_ELVXZ];
	    A308c3.az = comb_att[stmix][CAT_AZ];
	    A308c3.ran = comb_att[stmix][CAT_RNG];

	}

        /* Test for TVS.  Window info is elev, az and ran of the base of the TVS. */
	if( (cat_code == 10) || (cat_code == 27) ){

	    tdata = trans_tvs[cat_feat[stmix][CAT_TVS]];

	    A308c3.elevang = comb_att[stmix][CAT_ELTVS];
	    A308c3.az = comb_att[stmix][CAT_AZ];
	    A308c3.ran = comb_att[stmix][CAT_RNG];

	}

        /* Set threshold. */
	A308c3.threshold = AH_get_alert_threshold( cat_code, threshold_code );

	A308c3.exval1 = tdata;
	A308c3.exval = 0.f;

        /* Determine if the incoming data (tdata) exceeds the requested 
           threshold. */
	if( (tdata != 0) && (tdata >= A308c3.threshold) ){

            /* Set new_cond flag.  Set requested threshold. */
	    *new_cond = NEW_ALERT;
	    *delta_thres = tdata - A308c3.threshold;

	}

    }
    else if( (cat_code == 8) || (cat_code == 11) || (cat_code == 14) || (cat_code == 25)
                                     || 
	     (cat_code == 28) || (cat_code == 31) ){

        /* Process categories 8, 11, 14, 25, 28, AND 31.  Note: These are for
           MAX STORM Z, MAX HAIL SIZE, and STORM TOP for the volume and forecast
           group categories. */

        /* Test for MAX STORM Z. */
	if( (cat_code == 11) || (cat_code == 28) ){

            /* Save window info.   Window info is elevation, az and ran. */
	    A308c3.elevang = comb_att[stmix][CAT_ELVXZ];
	    A308c3.az = comb_att[stmix][CAT_AZ];
	    A308c3.ran = comb_att[stmix][CAT_RNG];
	    A308c3.exval = comb_att[stmix][CAT_MXZ];

	}

        /* Test for MAX HAIL SIZE. */
	if( (cat_code == 8) || (cat_code == 25) ){

            /* Save window info.   Window info is elevation, az and ran. */
	    A308c3.elevang = comb_att[stmix][CAT_ELVXZ];
	    A308c3.az = comb_att[stmix][CAT_AZ];
	    A308c3.ran = comb_att[stmix][CAT_RNG];
	    A308c3.exval = (float) cat_feat[stmix][CAT_MEHS];

	}

        /* Test for STORM TOP.  The data in comb_att is in km.  
           The threshold in attable is in 1000'S of feet.  The 
           Exceeding value is output in KFT. */
	if( (cat_code == 14) || (cat_code == 31) ){

            /* Save window info -- Use storm centroid elev, az and ran. */
	    A308c3.elevang = comb_att[stmix][CAT_ELVXZ];
	    A308c3.az = comb_att[stmix][CAT_AZ];
	    A308c3.ran = comb_att[stmix][CAT_RNG];
	    A308c3.exval = fabs(comb_att[stmix][CAT_STP] * M_TO_FT );

	}

	A308c3.threshold = AH_get_alert_threshold( cat_code, threshold_code );
	A308c3.exval1 = 0;

        /* Process alert. */
	if( A308c3.exval > (float) A308c3.threshold ){

	    *new_cond = NEW_ALERT;
	    *delta_thres = RPGC_NINT( A308c3.exval - (float) A308c3.threshold );

	}

        /* End of process alert for data type. */

    }

    return 0;

/* End of Vol_cat_check(). */
} 


/*/////////////////////////////////////////////////////////////////////////
//
//   Description:
//      Determine storm center azimuth and range from the radar.
//
//   Inputs:
//      stmix - storm index.
//      az - azimuth of storm.
//      rng - range of storm.
//      bitmap - position bitmap.
//   
//   Returns:
//      Always returns 0.
//   
//////////////////////////////////////////////////////////////////////// */
static int Determine_storm_centers( int stmix, float az, float rng, 
                                    short bitmap[][BROWS] ){

    int alertbit, di, dj, ialrt, jalrt, halfcol, halfrow;
    unsigned int ibox, jbox;
    float constant, dr, sf, gbs, stpos, azimuth;

    /* Determine user areas containing storm centers. */

    /* Alert grid box size (km) */
    gbs = BSZKM;

    /* range sample increment (km) */
    dr = 1.f;

    /* Radar column position (grid center). */
    halfcol = BCOLS/2;

    /* Radar row position (grid center). */
    halfrow = BCOLS/2;

    /* Shift factor. */
    sf = 65536.f;

    /* Start position (to center of bin) of current storm (#bins). */
    stpos = (float) rng / dr - .5f;

    /* Compute shift factor. */
    constant = dr / gbs * sf;

    /* Azimuth angle of storm, in radians. */
    azimuth = az * DTR;

    /* Compute I and J box change functions. */
    di = (int) (constant * sinf(azimuth));
    dj = (int) (constant * cosf(azimuth));

    /* Compute I and J alert box position variables. */
    ibox = (float) (halfcol + 1) * sf + ( (float) di * stpos);
    jbox = (float) (halfrow + 1) * sf - ( (float) dj * stpos);

    /* Get the I and J alert box positions. */
    ialrt = RPGC_set_mssw_to_uint( ibox );
    jalrt = RPGC_set_mssw_to_uint( jbox );

    /***************************************************** 
      Now check the bitmap position for alert status. 
     *****************************************************/

    /* Because we are accessing a 0-indexed array, need to subtract
       1 from both indices. */
    jalrt--;
    ialrt--;

    /* Set up users storm directory with indicators for storm centers. */
    alertbit = RPGCS_bit_test_short( (unsigned char *) &bitmap[jalrt][0], ialrt );

    /* Check alert status for bitmap position. */
    if( alertbit )
	Strmdir[stmix] = 1;

    return 0;

/* End of Determine_storm_centers() */
}

