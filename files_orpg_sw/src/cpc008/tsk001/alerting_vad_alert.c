/* RCS info */
/* $Author: aamirn $ */
/* $Locker:  $ */
/* $Date: 2008/01/04 17:03:46 $ */
/* $Id: alerting_vad_alert.c,v 1.2 2008/01/04 17:03:46 aamirn Exp $ */
/* $Revision: 1.2 $ */
/* $State: Exp $ */

#include <alerting.h>

#define IR			1.21
#define RE			6371.0

/*//////////////////////////////////////////////////////////////////
//
//   Description:
//      Process VAD alert.
//
//   Inputs:
//      vscnix - volume scan index.
//      VAD_alert - contains the VAD Alert data.
//
//   Returns:
//      Always returns 0.
//
///////////////////////////////////////////////////////////////// */
int A3081j_process_vad_alert( int vscnix, int *VAD_alert ){

    /* Local variables */
    int i, j, dtype, stmix, new_cond, threshold, exval1;
    int alert_status, threshold_code, cat_cde, cat_idx;
    float stmdir, stmspd, az, ran, hgt, exval, slrng, elevang;
    char stmid[4];

    /* Set up data type variable for VAD. */
    dtype = VAD_BUFFER;
    
    /* Initialize variables. */
    memset( stmid, ' ', 4 );
    stmspd = -1.f;
    stmdir = -1.f;
    elevang = 0.f;
    az = 0.f;
    ran = 0.f;
    stmix = 0;

    /* Do For All users. */
    for( i = 0; i < MAX_CLASS1; ++i ){

        /* Do For All alert areas. */
	for( j = 0; j < NUM_ALERT_AREAS; ++j ){

	    if( Uaptr[vscnix][j][i] != NULL ){

                /* Get information for this data type. */
		cat_idx = A30859_get_cat( (short *) (Uaptr[vscnix][j][i] + ACOFF), 
			                  (short *) (Uaptr[vscnix][j][i] + THOFF), 
			                  dtype, &cat_cde, &threshold_code );

                /* If alert category is active.... */
		if( cat_cde != 0 ){

		    threshold = AH_get_alert_threshold( cat_cde, threshold_code );
		    exval1 = VAD_alert[PASP];
		    exval = (float) exval1;

		    if( VAD_alert[PASP] >= threshold ){

			new_cond = NEW_ALERT;

                        /* Calculate the elevation angle for one-time request. */
			hgt = VAD_alert[PAHT] / KM_TO_FT;

			slrng = (float) VAD_alert[PARG];
			elevang = asinf((hgt - (slrng*slrng) / 
				IR*RE) / slrng);
			elevang /= DEGTORAD;

		    }
                    else 
			new_cond = NO_ALERT;

                    /* Determine the alert status for this category. */
		    A3081b_alert_status( new_cond, (short *) (Uaptr[vscnix][j][i] + CSTATOFF),
			                 cat_idx, &alert_status );

                    /* Determine if we are to send alert message. */
		    if( (alert_status == NEW_ALERT) 
                                      || 
                        (alert_status == END_ALERT) ){

                        /* Do alerting. */
			A30817_do_alerting( i, j, stmix, stmid, az, ran, threshold, 
                                            threshold_code, exval, exval1, alert_status, 
                                            elevang, stmspd, stmdir, cat_idx, vscnix );

		    }

		}

	    }

	}

    }

    return 0;

/* End of A3081j_process_vad_alert(). */
} 

