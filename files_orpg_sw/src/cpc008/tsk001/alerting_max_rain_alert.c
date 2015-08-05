/* RCS info */
/* $Author: aamirn $ */
/* $Locker:  $ */
/* $Date: 2008/01/04 17:03:43 $ */
/* $Id: alerting_max_rain_alert.c,v 1.2 2008/01/04 17:03:43 aamirn Exp $ */
/* $Revision: 1.2 $ */
/* $State: Exp $ */

#include <alerting.h>

/*//////////////////////////////////////////////////////////////////
//
//   Description:
//      Check for a maximum rainfall alert.
//
//   Inputs:
//      maxaccu - maximum accumulation value.
//      vscnix - volume scan index.
//
//   Returns:
//      Always returns 0.
//
///////////////////////////////////////////////////////////////// */
int A3081k_maxrain( int maxaccu, int vscnix ){

    /* Local variables */
    int i, j, new_cond, dtype, threshold, cat_cde, cat_idx;
    int alert_status, stmix, exval1, threshold_code;
    float az, ran, stmdir, stmspd, exval, elevang;
    char stmid[4];

    /* Incoming accumulation data is mm scaled by 10.  Convert to
       inches scaled by 10. */

    /* Set up the data type variable to equal the category code. */
    dtype = MAX_RAIN_BUFFER;

    /* Clear the variables that are used by the "DO ALERTING" routine. */
    memset( stmid, ' ', 4 );
    stmspd = -1.f;
    stmdir = -1.f;
    az = 0.f;
    ran = 0.f;
    stmix = 0;
    exval = 0.f;
    exval1 = 0;
    elevang = 0.f;

    /* Do For All users. */
    for( i = 0; i < MAX_CLASS1; ++i ){

        /* Do For All alert areas. */
	for( j = 0; j < NUM_ALERT_AREAS; ++j ){

            /* If a user area is defined .... */
	    if( Uaptr[vscnix][j][i] != NULL ) {

                /* Get information to determine category. */
		cat_idx = A30859_get_cat( (short *) (Uaptr[vscnix][j][i] + ACOFF),
			                  (short *) (Uaptr[vscnix][j][i] + THOFF),
			                  dtype, &cat_cde, &threshold_code );

                /* Is the category set? */
		if( cat_cde != 0 ){

                    /* Set threshold to code from adaptation data. */
		    threshold = AH_get_alert_threshold( cat_cde, threshold_code );

                    /* Convert maximum accumulation to in*10. */
		    exval = (float) RPGC_NINT( (float) maxaccu / (IN_TO_MM*10.0) );

                    /* Is maximum rainfall greater than the alert threshold? */
		    if( exval >= (float) threshold ){

                        /* Condition code is new alert. */
			new_cond = NEW_ALERT;

                    /* No alert */
		    }
                    else 
			new_cond = NO_ALERT;

		    A3081b_alert_status( new_cond, (short *) (Uaptr[vscnix][j][i] + CSTATOFF),
			                 cat_idx, &alert_status );
		   
                    if( (alert_status == NEW_ALERT) || (alert_status == END_ALERT) ){

                        /* "DO ALERTING" for this category. */
			A30817_do_alerting( i, j, stmix, stmid, az, ran, threshold, 
                                            threshold_code, exval, exval1, alert_status, 
                                            elevang, stmspd, stmdir, cat_idx, vscnix );

		    }

		}

	    }

	}

    }

    return 0;

/* End of A3081k_maxrain(). */
} 

