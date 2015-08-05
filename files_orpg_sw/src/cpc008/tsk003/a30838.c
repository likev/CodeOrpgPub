/* RCS info */
/* $Author: steves $ */
/* $Locker:  $ */
/* $Date: 2006/12/18 21:53:03 $ */
/* $Id: a30838.c,v 1.2 2006/12/18 21:53:03 steves Exp $ */
/* $Revision: 1.2 $ */
/* $State: Exp $ */

#include <combattr.h>

/************************************************************************

   Description:
      Correlate each TVS and elevated TVS to the nearest Storm, 
      and assign to each storm the label of the most severe entity 
      associated with it (i.e. TVS, ETVS, or None). 

   Inputs:
      stormotion - 2D array of storm motions.
      ntotpred - number of storms this scan.
      num_tvs - number of TVSs this scan.
      num_etvs - number of ETVSs this scan.
      tvs_main - 2D array of TVS/ETVS attributes.
      tda_adapt - TVS algorithm adaptation data.

   Returns:
      Currently always returns 0.

************************************************************************/
int A30838_correl_tvs( float stormotion[][NSTF_MOT], int ntotpred, 
                       int num_tvs, int num_etvs, float tvs_main[][TVFEAT_CHR], 
                       float *tda_adapt ){

    /* Local variables */
    int i, j, num_feats, temp;
    float corr_dist, distance, distance_x, distance_y, phi, min;
    float theta, x, y;

    /* CALCULATE THE CORRELATION DISTANCE (SQUARED) FOR TVS TO 
       STORM ASSOCIATION. */
    corr_dist = tda_adapt[TDA_SAD] * tda_adapt[TDA_SAD];

    /* LIMIT THE NUMBER OF TVS TO MAX STORMS ALLOWED... */

    /* Computing MIN */
    i = abs(num_tvs) + abs(num_etvs);
    if( i < CATMXSTM )
       num_feats = i;
    else
       num_feats = CATMXSTM;

    /* DETERMINE THE LOCATION OF EACH TVS IN CARTESIAN COORINATES: */
    for( i = 0; i < num_feats; ++i ){

	theta = tvs_main[i][TV_AZM] * DEGTORAD;
	phi = tvs_main[i][TV_BEL] * DEGTORAD;
	x = tvs_main[i][TV_RAN] * sin(theta) * cos(phi);
	y = tvs_main[i][TV_RAN] * cos(theta) * cos(phi);

        /* CORRELATE EACH TVS TO THE NEAREST STORM. */
	min = 999999.f;
	temp = 1;
	for( j = 0; j < ntotpred; ++j ){

	    distance_x = stormotion[j][STF_X0] - x;
	    distance_y = stormotion[j][STF_Y0] - y;
	    distance = (distance_x * distance_x) + 
                       (distance_y * distance_y);

	    if( distance < min ){

		min = distance;
		temp = j;

	    }

	}

        /* ASSIGN TO STORM A TVS.  TVSs NOT WITHIN A THRESHOLD 
           DISTANCE ARE DISCARDED. */
	if( min <= corr_dist ) {

            /* ONLY PROCESS IF STORM DOES NOT ALREADY HAVE A TVS OR 
               ETVS ASSIGNED. */
	    if( Storm_feats[temp][CAT_TVS_TYPE] != 0) 
		continue;
	     
	    Storm_feats[temp][CAT_TVS_TYPE] = tvs_main[i][TV_TYP];

            /* STORE, FOR THE STORM, ATTRIBUTES OF THE BASE OF THE 
               TVS ASSOCIATED WITH IT: */
	    Basechars[temp][TVSB_AZ] = tvs_main[i][TV_AZM];
	    Basechars[temp][TVSB_RN] = tvs_main[i][TV_RAN];
	    Basechars[temp][TVSB_EL] = tvs_main[i][TV_BEL];

	}

    }

    /* RETURN TO PRODUCT CONTROL ROUTINE: */
    return 0;

} 

