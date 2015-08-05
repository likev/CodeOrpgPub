/* RCS info */
/* $Author: jeffs $ */
/* $Locker:  $ */
/* $Date: 2014/03/18 17:04:39 $ */
/* $Id: alerting_helpers.c,v 1.5 2014/03/18 17:04:39 jeffs Exp $ */
/* $Revision: 1.5 $ */
/* $State: Exp $ */

#include "alerting.h"

/*\////////////////////////////////////////////////////////////////////////
// 
//   Description: 
//      Build User Alert directory from alert grid.                
//   
//   Inputs:     
//      usernum - user number.
//      areanum - alert area number.
//
//   Outputs:
//      active_cats - array of active categories.
//      thresh - array of alert thresholds.
//      prodreq - array of product request flags.
//      cstat - status.
//      
//  
//   Returns:    
//      Time string in hr:min:sec format.
//   
////////////////////////////////////////////////////////////////////////\*/
int A3081t_setup_direct( short *active_cats, short *thresh, 
                         short *prodreq, short *cstat, 
                         int usernum, int areanum ){

    int k, k3;

    for( k = 0; k < MAX_ALERT_CATS; k++ ){

        /* Calculate index to Alert_grid. */
	k3 = k*HW_PER_CAT;

        /* Fill the active category, threshold, product-request and
           initialize the current status. */
	active_cats[k] = Alert_grid[areanum][usernum][k3];
	thresh[k] = Alert_grid[areanum][usernum][k3+1];
	prodreq[k] = Alert_grid[areanum][usernum][k3+2];
	cstat[k] = 0;

    }

    return 0;

/* End of A3081t_setup_direct(). */
} 

/*\///////////////////////////////////////////////////////////////////////
//
//   Description:
//      Returns index into the active_cats array.  The index corresponds
//      to the active_cats element which matches data type "dtype".
//
//   Inputs:
//      active_cats - array of active categories.
//      thresh - array of alert threshold.
//      dtype - data type.
//
//   Outputs:
//      cat_cde - alert category code.
//      thrcode - alert threshold code.
//      
//   Returns:
//      The index into the "active_cats" array corresponding to "dtype".
//   
//////////////////////////////////////////////////////////////////////\*/
int A30859_get_cat( short *active_cats, short *thresh, int dtype, 
                    int *cat_cde, int *thrcode ){

    int i, cat_idx;

    *thrcode = 0;
    *cat_cde = 0;

    for( i = 0; i < MAX_ALERT_CATS; ++i ){

        cat_idx = i;

        switch( dtype ){

            /* Base Velocity Grid. */
            case BVG_BUFFER:
            {
                if( active_cats[i] == VELOCITY_ALERT )
                    *cat_cde = VELOCITY_ALERT;
                break;
            }

            /* Composite Reflectivity Cartesian Grid. */
            case CR_BUFFER:
            {
                if( active_cats[i] == CR_ALERT )
                    *cat_cde = CR_ALERT;
                break;
            }

            /* Echo Tops Grid. */
            case ET_BUFFER:
            {
                if( active_cats[i] == ET_ALERT )
                    *cat_cde = ET_ALERT;
                break;
            }

            /* VIL Grid. */
            case VIL_BUFFER:
            {
                if( active_cats[i] == VIL_ALERT )
                    *cat_cde = VIL_ALERT;
                break;
            }

            /* Velocity/Azimuth Display Data. */
            case VAD_BUFFER:
            {
                if( active_cats[i] == VAD_ALERT )
                    *cat_cde = VAD_ALERT;
                break;
            }

            /* One-Hour Precipitation Data. */
            case MAX_RAIN_BUFFER:
            {
                if( active_cats[i] == MAX_RAIN_ALERT )
                    *cat_cde = MAX_RAIN_ALERT;
                break;
            }

            /* All others not covered above. */
            default:
               
               /* Categories requiring combined attributes data
                  do not call this module. */
               break;

        /* End of switch( dtype ). */
        }

        /* If the category code is defined, return the category threshold
           value. */
        if( *cat_cde != 0 ){

  	    *thrcode = thresh[cat_idx];
            break;

        }

    /* End of "for loop". */
    }
     
    /* Return the index of active_cats.  This index points to an alert
       category which requires processing of the data specified by dtype. */
    return cat_idx;

/* End of A30859_get_cat() */
}

/*\///////////////////////////////////////////////////////////////////////
//  
//   Description:
//      Determine category exceeded threshold this time.
//
//   Inputs:
//      new_cond - new condition (alert) flag.
//      catix - category index.
//
//   Outputs:
//      cstat - array of current alerting status, by category.
//      alert_status - current alerting status.
//
//   Returns:
//      Always returns 0.
//      
///////////////////////////////////////////////////////////////////////\*/
int A3081b_alert_status( int new_cond, short *cstat, 
                         int catix, int *alert_status ){

    if( new_cond != NO_ALERT ){

        /* Met Alert conditions this time determine if NEW or CONTINUING */
	if( cstat[catix] == NEW_ALERT ){

            /* Last Volume had the alert so current statue is OLD */
	    cstat[catix] = OLD_ALERT;
	    *alert_status = OLD_ALERT;

	}
        else if( cstat[catix] == NO_ALERT ){

            /* Previous Volume has no alert in this category */
	    cstat[catix] = NEW_ALERT;
	    *alert_status = NEW_ALERT;

	}
        else if( cstat[catix] == OLD_ALERT ){

            /* Previous volume had this category as OLD alert */
	    cstat[catix] = OLD_ALERT;
	    *alert_status = OLD_ALERT;

	}
        else if( cstat[catix] == END_ALERT ) {

            /* Previous volume ENDED this alert */
	    cstat[catix] = NEW_ALERT;
	    *alert_status = NEW_ALERT;
	}

    }
    else {

        /* This Category did not pass threshold this volume 
           Determine if an alert needs to be cancelled */
	if( (cstat[catix] == NEW_ALERT) 
                          || 
            (cstat[catix] == OLD_ALERT) ){

	    cstat[catix] = END_ALERT;
	    *alert_status = END_ALERT;

	}
        else {

	    cstat[catix] = NO_ALERT;
	    *alert_status = NO_ALERT;

	}

    }

    return 0;

/* End of A3081b_alert_status(). */ 
} 

/*\////////////////////////////////////////////////////////////////
//
//   Description:
//      Format 'No Alert' message line.
//
//   Inputs:
//      user - user index
//      vscnix - volume scan index.
//
//   Returns:
//      Always returns 0.
//
//////////////////////////////////////////////////////////////\*/
int A3081r_no_prd_msg( int user, int vscnix ){

    int j, len;
    char *spt = (char *) Tbuf;

    /* Format strings */
    static char *fmt = " NO NEW ALERTS THIS SCAN";

    /* Write line 1. */
    strcpy( spt, fmt );
    len = strlen( fmt );
    spt[len] = ' ';

    /* Write remainder lines as blank. */
    spt += CHAR_PER_LN;
    for( j = 1; j < NOLNS; ++j ){

        /* Write blank line. */
        memset( spt, ' ', CHAR_PER_LN );

        /* Increment index by 80 characters. */
        spt += CHAR_PER_LN;

    }

    /* Determine # I*2 words of characters per line. */
    It[vscnix][user] += NOLNS;

    return 0;

/* End of A3081r_no_prd_msg(). */
} 

/*\//////////////////////////////////////////////////////////////////////////
//
//   Description:
//      Return the alert paired-product code based on category code and 
//      threshold code.
//
//   Inputs:
//      cat_code - alert category.
//
//   Returns:
//      Returns paired-product code or -1 on error.
//
//////////////////////////////////////////////////////////////////////////\*/
int AH_get_alert_paired_prod( int cat_code ){

   /* Validate the category code. */
   if( (cat_code <= 0) || (cat_code > ALERT_THRESHOLD_CATEGORIES) )
       return -1;
   
   return( (int) Alttable.data[cat_code-1].prod_code );

/* End of AH_get_alert_paired_prod(). */
}


/*\//////////////////////////////////////////////////////////////////////////
//
//   Description:
//      Return the alert threshold based on category code and threshold 
//      code.
//
//   Inputs:
//      cat_code - alert category.
//      thresh_code - threshold_code (a number in the range 1-6).
//
//   Returns:
//      Returns threshold or -1 on error.
//
//////////////////////////////////////////////////////////////////////////\*/
int AH_get_alert_threshold( int cat_code, int thresh_code ){

   /* Validate the category code. */
   if( (cat_code <= 0) || (cat_code > ALERT_THRESHOLD_CATEGORIES) )
       return -1;

    /* Return the threshold value based on alert category and threshold
       code. */
    switch( thresh_code ){

        case 1:
        {
            return( (int) Alttable.data[cat_code-1].thresh_1 );
        }

        case 2:
        {
            return( (int) Alttable.data[cat_code-1].thresh_2 );
        }

        case 3:
        {
            return( (int) Alttable.data[cat_code-1].thresh_3 );
        }

        case 4:
        {
            return( (int) Alttable.data[cat_code-1].thresh_4 );
        }

        case 5:
        {
            return( (int) Alttable.data[cat_code-1].thresh_5 );
        }

        case 6:
        {
            return( (int) Alttable.data[cat_code-1].thresh_6 );
        }

        default:
            break;

   }

   return -1;

/* End of AH_get_alert_threshold(). */ 
}
