/* RCS info */
/* $Author: steves $ */
/* $Locker:  $ */
/* $Date: 2007/06/15 19:01:33 $ */
/* $Id: a30835.c,v 1.3 2007/06/15 19:01:33 steves Exp $ */
/* $Revision: 1.3 $ */
/* $State: Exp $ */

#include <combattr.h>

#define INDEX		0
#define SEVERITY	1
#define SORT_DIM        2

#define VIL_WGT 	1
#define POH_WGT  	256 
#define POSH_WGT  	(POH_WGT*128)
#define MDA_WGT 	(POSH_WGT*128)
#define TVS_WGT 	(MDA_WGT*128)


/* Function Prototypes. */
static int Compare( const void *elem1, const void *elem2 );

/**********************************************************************
   Description:
      This routine manages the order in which the storm related data 
      is stored in the output buffer.  All storms with TVSs or ETVs 
      are packed first, followed by those with Mesos, then those with 
      the highest POSH & secondly POH.  Finally storms with none of 
      the above features are packed.  Within a category, the storms are 
      ordered by Cell-Based VIL. 

   Inputs:
      outbuf - pointer to output buffer.
      hailstats - array containing hail information.
      stormain - array containing storm information.

   Returns:
      Currently always returns 0.

***********************************************************************/
int A30835_bld_outbuf( int *outbuf, float hailstats[][NHAL_STS], 
                       float stormain[][NSTM_CHR] ){

    /* Initialized data */
    static int tvs_trans_tab[3] = { 0,2,1 };
    static unsigned int stm_sort_order[CAT_MXSTMS][SORT_DIM];


    /* Local variables */
    int i, j, poh, posh, ntotal, cat_idx, table_index;
    int mda_over_thresh;

    /* INITIALIZE THE NUMBER OF STORMS AND NUMBER OF FORECAST 
       POSITIONS TO ZERO IN OUTPUT BUFFER ..JUST IN CASE THERE 
       ARE NO STORMS .  THIS WILL INSURE THAT NO GARBAGE IS PASSED 
       THROUGH THE OUTPUT BUFFER TO OTHER TASKS. */
    outbuf[CNFP] = 0;
    outbuf[CNS] = 0;

    /* INITIALIZE INDEX COUNTER FOR COMBINED_ATTRIBUTES TABLES */
    cat_idx = 0;
    ntotal = *(Tibuf[TRFRCATR_ID] + BNT);

    /* CALCULATE THE SEVERITY INDEX FOR EACH STORM CELL. 
       THE SEVERITY INDEX IS CALCULATED AS FOLLOWS: 

     EACH TYPE OF PHENOMENON IS GIVEN A WEIGHT, WITH LARGER WEIGHTS 
     GIVEN TO THE MOST SEVERE PHENOMENON. THE WEIGHTS ARE DESIGNED 
     IN SUCH A WAY THAT IF TWO STORMS HAVE THE SAME SEVERITY INDEX, 
     BOTH STORMS HAVE THE SAME ATTRIBUTES. */
    for( i = 0; i < ntotal; ++i ){

	stm_sort_order[i][INDEX] = i;

        /* CHECK THE POSH AND POH FOR BEYOND RANGE VALUE.  IF BEYOND 
           RANGE, TREAT AS 0 FOR PURPOSES OF SEVERITY. */
	posh = RPGC_NINT( hailstats[i][H_PSH] );
	if( posh < 0 )
	    posh = 0;
	 
	poh = RPGC_NINT( hailstats[i][H_POH] );
	if( poh < 0 )
	    poh = 0;
	
        /* For MDA associations, only count them if the feature's 
           strength rank exceeds the minimum display threshold. */
	if( Storm_feats[i][CAT_MDA_TYPE] >= Mda_adapt.min_filter_rank )
	    mda_over_thresh = 1 + Storm_feats[i][CAT_MDA_TYPE] -
		              Mda_adapt.min_filter_rank;

	else 
	    mda_over_thresh = 0;
	
	table_index = Storm_feats[i][CAT_TVS_TYPE];
	stm_sort_order[i][SEVERITY] = (tvs_trans_tab[table_index]*TVS_WGT)
		                    + (mda_over_thresh*MDA_WGT)
                                    + (posh*POSH_WGT) 
                                    + (poh*POH_WGT) 
                                    + (RPGC_NINT(stormain[i][STM_VIL])*VIL_WGT);

    }

    /* SORT THE STORMS BASED ON THE SEVERITY INDEX. NO NEED TO SORT IF THERE 
       ARE FEWER THAN 2 STORMS. */
    if( ntotal > 1 ) 
	qsort( stm_sort_order, ntotal, SORT_DIM*sizeof(int),
               Compare );
     
    /* PACK STORMS INTO COMBINED ATTRIBUTES BUFFER IN SORTED ORDER */

    /* DO FOR ALL STORM CELLS .... */
    for( i = ntotal-1; i >= 0; --i ){

        /* GET STORM CELL INDEX */
	j = stm_sort_order[i][INDEX];

        /* PACK CELL INDEX "J" AT COMBINED ATTRIBUTES INDEX "CAT_IDX". */
	A30836_pack_storm( j, cat_idx, (void *) (Tibuf[CENTATTR_ID] + BST), 
		           *(Tibuf[TRFRCATR_ID] + BNT), 
                           (void *) (Tibuf[TRFRCATR_ID] + BSI), 
                           (void *) (Tibuf[TRFRCATR_ID] + BSM), 
                           (void *) (Tibuf[TRFRCATR_ID] + BSF), 
		           Tibuf[TRFRCATR_ID] + BFA, outbuf + CNS, 
		           (void *) (outbuf + CFEA), (void *) (outbuf + CATT), 
                           outbuf + CNFP, (void *) (outbuf + CNFST), 
                           Tibuf[HAILATTR_ID] + BHL, 
		           (void *) (Tibuf[HAILATTR_ID] + BHS) );
	cat_idx++;
    }

    /* RETURN TO CALLING ROUTINE */
    return 0;

}


/**********************************************************************

   Description:
      Comparision function for the qsort function.

   Inputs:
      elem1 - first element to compare
      elem2 - second element to compare

   Returns:
      -1 is elem1 is less than elem2, 0 if elem1 is equal to elem2 or
      1 if elem1 is equal to elem2.

**********************************************************************/
static int Compare( const void *elem1, const void *elem2 ){

   int *array1 = (int *) elem1;
   int *array2 = (int *) elem2;

   if( array1[SEVERITY] < array2[SEVERITY] )
      return -1;

   else if( array1[SEVERITY] == array2[SEVERITY] )
      return 0;

   return 1;

/* End of Compare(). */
} 

