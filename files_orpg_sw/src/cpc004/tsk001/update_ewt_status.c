/*
 * RCS info
 * $Author: cmn $
 * $Locker:  $
 * $Date: 2008/09/15 12:50:55 $
 * $Id: update_ewt_status.c,v 1.2 2008/09/15 12:50:55 cmn Exp $
 * $Revision: 1.2 $
 * $State: Exp $
 */

#include <veldeal.h>

/************************************************************************

   Description: 
      This module checks the availability of the ewt data for the 
      MPDA algorithm.  It is a stripped down version of a304db.ftn. 

   Inputs:
      input_buf - pointer to input radial.

************************************************************************/
void update_ewt_status( void *input_buf ){

    /* Local variables */
    int itc_status, from_vad = 0, from_hci = 0;
    static int sounding_avail = 0, last_update = -1;

    Base_data_header *radhdr = (Base_data_header *) input_buf;


    /* If at beginning of elevation or volume, check if new environmental 
       wind table data is available.  Only required if radial has Doppler data.  
       Also initialize some variables. */
    if( ((radhdr->status == GOODBEL) 
                    || 
         (radhdr->status == GOODBVOL)) 
                  && 
         ((radhdr->n_dop_bins != 0) 
                  && 
          (radhdr->nyquist_vel != 0)) ){

        /* Update the environmental wind table data if sounding data available. */
	RPGC_itc_read( ENVVAD, &itc_status );
	if( itc_status == RPGC_NORMAL ){

	    if( Envvad.parameter_from_vad[0] == QUE4_ENVWNDVAD )
		sounding_avail = 1;
	     
	}

	from_vad = 0;
	from_hci = 0;

	if( ((sounding_avail) 
                && 
            (Envvad.parameter_from_vad[1] != last_update))
                              || 
             (Env_wnd_tbl_updt == 1) ){

	    Env_wnd_tbl_updt = 0;
	    if( (Envvad.parameter_from_vad[0] == QUE4_ENVWNDVAD) 
                                      && 
		(Envvad.parameter_from_vad[1] != last_update))
		from_vad = 1;

	    else 
		from_hci = 1;

            /* New environmental wind data is available */
	    RPGC_itc_read( A3CD97, &itc_status );

            /* If status is not normal, clear SOUNDING_AVAIL */
	    if( itc_status != RPGC_NORMAL )
		sounding_avail = 0;

	    else{

		if( from_vad == 1 )
		    last_update = Envvad.parameter_from_vad[1];
		 
                /* Report update if required. */
		if( !Valid_soundings ){

                    /* ;** Send a message saying new VAD data has arrived. */
		    if( from_vad == 1 ) 
		        LE_send_msg( GL_STATUS, 
                              "New Environmental Winds Data Received from VAD\n" );

		    else
			LE_send_msg( GL_STATUS,
	                      "New Environmental Winds Data Received from HCI\n" );
		     
		    Valid_soundings = 1;

		}

	    }

	}

    A304dg.sounding_avail = Valid_soundings;

    }

    /* Return to caller. */

} 

