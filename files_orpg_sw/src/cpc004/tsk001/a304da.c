/* RCS info */
/* $Author: steves $ */
/* $Locker:  $ */
/* $Date: 2006/12/12 20:38:44 $ */
/* $Id: a304da.c,v 1.3 2006/12/12 20:38:44 steves Exp $ */
/* $Revision: 1.3 $ */
/* $State: Exp $ */

#include <veldeal.h>

/************************************************************************ 
   Description: 
      This module contains the buffer control function.

   Returns:
      Returns -1 on abort, 0 otherwise. 

************************************************************************/
int A304da_vd_buf_cntrl(){

    /* Local variables */
    int *obuf_ptr, *ibuf_ptr;
    int ilength, olength, in_status, itc_status, out_status, radial_status;

    /* Do Until end of elevation or volume. */
    while(1){

        /* Get a radial of RAW BASE DATA. */
        ibuf_ptr = RPGC_get_inbuf_by_name( "RAWDATA", &in_status );
        if( in_status == RPGC_NORMAL ){

            /* If status is OK, get a buffer for the BASEDATA storage. */
	    olength = ilength = RPGC_get_inbuf_len( ibuf_ptr );
	    obuf_ptr = RPGC_get_outbuf_by_name( "BASEDATA", olength, &out_status );

            /* If status is OK, copy input data to output buffer. */
	    if( out_status == RPGC_NORMAL ){

	        memcpy( obuf_ptr, ibuf_ptr, ilength );

                /* Check if adaptation data needs to be updated. */
	        A304db_update_adaptation( (Base_data_header *) ibuf_ptr );

                /* Dealias this radial if required. */
               	radial_status = A304dc_process_radial( obuf_ptr );

                /* Release the input buffer */
	        RPGC_rel_inbuf( ibuf_ptr );

                /* Release the output buffer with FORWARD disposition */
	        RPGC_rel_outbuf( obuf_ptr, FORWARD );

            }
            else{

                /* Status returned from RPGC_get_outbuf_by_name was not RPGC_NORMAL so 
                   release input buffer then ABORT! */
                RPGC_rel_inbuf( ibuf_ptr );
                RPGC_abort_because( out_status );
                return -1;

            }

        }
	else{
 
            /* Status returned from RPGC_get_inbuf_by_name was not RPGC_NORMAL so ABORT!. */
	    RPGC_abort();
	    return -1;

        }
    
        /* If radial status is beginning of volume, output the PCT_OBS ITC data. */
        if( radial_status == GOODBVOL ){

	    Pct.vol_time = -1;
	    Pct.vol_date = -1;
	    Pct.percent_obscured = -1.f;
	    RPGC_itc_write( PCT_OBS, &itc_status );

        }

        /* Process data till end of elevation or volume scan. */
        if( (radial_status != GENDEL) 
                         && 
            (radial_status != GENDVOL) )
	   continue;

        break;

    }

    /* Return to caller. */
    return 0;

/* End of A304da_vd_buf_cntrl() */
} 
