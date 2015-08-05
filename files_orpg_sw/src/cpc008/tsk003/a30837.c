/* RCS info */
/* $Author: steves $ */
/* $Locker:  $ */
/* $Date: 2006/12/18 21:53:01 $ */
/* $Id: a30837.c,v 1.2 2006/12/18 21:53:01 steves Exp $ */
/* $Revision: 1.2 $ */
/* $State: Exp $ */

#include <combattr.h>

/**********************************************************************
   Description: 
      Stores azimuth, range and elevation angle for each TVS and ETVS.
      The total number of TVS'S + ETVS'S are stored in CAT_NUM_RCM
      table.

   Inputs:
      num_tvs - number of TVSs
      num_etvs - number of ETVSs
      tvs_main - 2D array of TVS/ETVS attributes.

   Outputs:
      cat_num_rcm - contains the number of TVSs for RCM.
      cat_tvst - CAT table containing TVS/ETVS information.

   Returns:
      Currently always returns 0.

**********************************************************************/
int A30837_fill_cat( int num_tvs, int num_etvs, float tvs_main[][TVFEAT_CHR], 
                     int *cat_num_rcm, float cat_tvst[][CAT_NTVS] ){

    /* Local variables */
    int total, i, j;

    /* INITIALIZE TVS TABLE.  THE MDA TABLES ARE INITIALIZED 
       IN correlate_mda_features.c */
    for( j = 0; j < CATMXSTM; ++j ){

	for( i = 0; i < CAT_NTVS; ++i )
	    cat_tvst[j][i] = 0.f;

    }

    /* LOOP THRU NUMBER OF TVSs AND ETVSs: STORE AZIMUTH, RANGE 
       AND ELEVATION ANGLE. */
    total = abs(num_tvs) + abs(num_etvs);
    for( i = 0; i < total; ++i ){

	cat_tvst[i][CAT_TVSAZ] = tvs_main[i][TV_AZM];
	cat_tvst[i][CAT_TVSRN] = tvs_main[i][TV_RAN];
	cat_tvst[i][CAT_TVSEL] = tvs_main[i][TV_BEL];

    }

    /* STORE TOTAL NUMBER OF TVS'S FOR RCM. */
    cat_num_rcm[RCM_TVS] = abs(num_tvs);

    /* RETURN TO CALLER */
    return 0;

} 

