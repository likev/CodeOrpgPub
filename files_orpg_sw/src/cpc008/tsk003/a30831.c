/* RCS info */
/* $Author: steves $ */
/* $Locker:  $ */
/* $Date: 2008/02/04 20:09:37 $ */
/* $Id: a30831.c,v 1.3 2008/02/04 20:09:37 steves Exp $ */
/* $Revision: 1.3 $ */
/* $State: Exp $ */

#include <combattr.h>

/* Function Prototypes. */
static int Comb_att( void *optr );

/**************************************************************************
   Description:
     This module is the buffer control routine for the "Combined 
     Attributes" task.  The Combined Attributes task processes 
     buffers from the MDA, TDA, Storm_Centroids, Storm_Track 
     Forecast, and Hail algorithms. It correlates the storms from
     the SCIT algorithms with the data from MDA, TDA and Hail and 
     builds an output buffer containing all the attributes of 
     storm cells  are needed by Alerting and by any product that 
     that requires the Combined Attributes formatted as `Graphic 
     Attributes.

***************************************************************************/
int A30831_buffer_control(){

    char *optr = NULL, *iptr = NULL;
    int rqdat, all_bufs_ok, opstat, lastelflag, elindx, last_elindx;
    int buf_valid, i, j, buf_vol, temp_buf_vol;

    /* READ THE INPUT BUFFER. */
    iptr = RPGC_get_inbuf_any( &rqdat, &opstat );
    if( opstat == NORMAL ){

        /* DETERMINE THE PROPER INDEX INTO TIBUF FOR THIS INPUT BUFFER */
	buf_valid = 0;
	for( i = 0; i < NIBUFS; i++ ){

	    if( Bufmap[i] == rqdat ){

                /* THE BUFFER IS VALID INPUT FOR THIS PROCESS. */
		buf_valid = AVAIL;

                /* FOR THE MDA INPUT, DETERMINE IF IT IS FROM THE LAST 
                   ELEVATION */
		if( rqdat == Bufmap[MDATTNN_ID] ){

		    lastelflag = RPGC_is_buffer_from_last_elev( iptr, 
                                                                &elindx,
                                                                &last_elindx );

                    /* RELEASE THE MDA BUFFER IF ITS NOT FROM THE LAST 
                       ELEVATION */
		    if( !lastelflag )
			RPGC_rel_inbuf( iptr );

                    else{

                        /* BEFORE SAVING THIS MDA INPUT BUFFER POINTER, 
                           MAKE SURE WE DON'T ALEADY HAVE AN ACTIVE POINTER. 
                           THAT COULD HAPPEN IF A VOLUME RESTARTS BEFORE ALL 
                           INPUTS ARE RECEIVED OR ONE OR MORE INPUTS WERE NOT 
                           AVAILABLE IN THE PREVIOUS VOLUME. */
                        if( Tibuf[i] == NULL )
			    Tibuf[i] = (int *) iptr;
                        else{

                            RPGC_rel_inbuf( Tibuf[i] ); 
			    Tibuf[i] = (int *) iptr;
			
                            RPGC_abort();

                        }

                    }
		     
		}
                else{

                    /* BEFORE SAVING THIS NON-MDA INPUT BUFFER POINTER, 
                       MAKE SURE WE DON'T ALEADY HAVE AN ACTIVE POINTER. 
                       THAT COULD HAPPEN IF A VOLUME RESTARTS BEFORE ALL 
                       INPUTS ARE RECEIVED OR ONE OR MORE INPUTS WERE NOT 
                       AVAILABLE IN THE PREVIOUS VOLUME. */
		    if( Tibuf[i] == NULL ) 
			Tibuf[i] = (int *) iptr;

		    else{

                        RPGC_rel_inbuf( Tibuf[i] );
			Tibuf[i] = (int *) iptr;

			RPGC_abort();

                    }

		}

	    }

	}

	if( !buf_valid ) 
	    RPGC_abort();

        /* CHECK TO SEE IF ALL INPUT BUFFERS HAVE BEEN RECEIVED */
	all_bufs_ok = 1;
	for( i = 0; i < NIBUFS; i++ ){

	    if( Tibuf[i] == NULL ){

		all_bufs_ok = 0;
                break;

            }

	}

        /* GET OUTPUT BUFFER FOR COMBINED-ATTRIBUTES. 
           --------------------------------------------------- 
           PROCEED TO COMBINED-ATTRIBUTES MAIN PROCESSING ROUTINE ONLY 
           IF ALL OUTPUT & INPUT BUFFERS WERE RETRIEVED SUCCESSFULLY. */
	if( all_bufs_ok ){

            /* SANITY CHECK .... ARE ALL BUFFERS FROM THE SAME VOLUME SCAN? */
            buf_vol = RPGC_get_buffer_vol_num( Tibuf[0] );
            for( i = 1; i < NIBUFS; i++ ){

                temp_buf_vol = RPGC_get_buffer_vol_num( Tibuf[i] );
                if( buf_vol != temp_buf_vol ){

                  RPGC_log_msg( GL_INFO, "Buf(0) Vol: %d != Buf(%d) Vol: %d\n",
                                buf_vol, i, temp_buf_vol );
                  RPGC_log_msg( GL_INFO, "--->Releasing Input Buffers and Aborting\n" );

                  for( j = 0; j < NIBUFS; j++ ){

                     RPGC_rel_inbuf( Tibuf[j] );
                     Tibuf[j] = NULL;

                  }                

                  RPGC_abort();   

                  all_bufs_ok = 0;
                  return 0;

               }     

            }

	    optr = RPGC_get_outbuf_by_name( "COMBATTR", CAT_OBUF_SIZE, &opstat);
	    if( opstat == NORMAL ){

                /* PROCESS COMBINED ATTRIBUTES */
		Comb_att( optr );

                /* FORWARD RESULTS SINCE ALL BUFFERS WERE RETRIEVED SUCCESSFULLY */
		RPGC_rel_outbuf( optr, FORWARD );

                /* RELEASE AND REINITIALIZE INPUT POINTERS: */
		for( i = 0; i < NIBUFS; i++ ){

		    RPGC_rel_inbuf( Tibuf[i] );
		    Tibuf[i] = NULL;

		}

                /* RESET FLAG INDICATING OUTPUT BUFFERS RETRIEVED SUCCESSFULLY: */
		all_bufs_ok = 0;


	    }
            else{

                /* IF OUTPUT BUFFER NOT SUCCESSFULLY RETRIEVED, ABORT. */
	        RPGC_abort_because( opstat );

            }

	}

    }

    /* RETURN TO CALLER. */
    return 0;

} 


/**************************************************************************

   Description:
     This is the main processing routine of the Combined Attributes 
     task. It is called by the Buffer Control routine, and manages 
     the remainder of the generation of the output buffer. 

   Inputs:
      optr - Output buffer pointer.

   Returns:
      Currently always returns 0.

**************************************************************************/
static int Comb_att( void *optr ){

    int i, j;
    int *outbuf = (int *) optr;

    /* CLEAR OUT "STORM_FEATS" AND "BASECHARS" ARRAYS BEFORE 
       PROCESSING ANY STORMS */
    for( j = 0; j < CATMXSTM; ++j) {

	Storm_feats[j][CAT_TVS_TYPE] = 0;
	Storm_feats[j][CAT_MDA_TYPE] = 0;
	for( i = 0; i < NBCHAR; ++i ) 
	    Basechars[j][i] = -999.99f;

    }

    /* CORRELATE TVSs & ETVSs TO NEAREST STORM. */
    A30838_correl_tvs( (void *) (Tibuf[TRFRCATR_ID] + BSM), 
                       *(Tibuf[TRFRCATR_ID] + BNT), 
                       *(Tibuf[TVSATTR_ID] + TNT), 
                       *(Tibuf[TVSATTR_ID] + TNE), 
                       (void *) (Tibuf[TVSATTR_ID] + TAM), 
                       (void *) (Tibuf[TVSATTR_ID] + TAD) );

    /* CORRELATE MDA FEATURES TO NEAREST STORM AND ASSING MDA DATA 
       TO COMBINED ATTRIBUTES TABLE. */
    correlate_mda_features( (tracking_t *) Tibuf[TRFRCATR_ID], Tibuf[MDATTNN_ID], 
                            (void *) (outbuf + CNMDA), (void *) (outbuf + CNRCM) );

    /* ASSIGN DATA TO COMBINED ATTRIBUTES TABLE. */
    A30837_fill_cat( *(Tibuf[TVSATTR_ID] + TNT), *(Tibuf[TVSATTR_ID] + TNE),  
	             (void *) (Tibuf[TVSATTR_ID] + TAM), (void *) (outbuf + CNRCM), 
                     (void *) (outbuf + CNTVS) );

    /* FILL IN THE REMAINDER OF COMBINED ATTRIBUTES TABLE. */
    A30835_bld_outbuf( optr, (void *) (Tibuf[HAILATTR_ID] + BHS), 
                       (void *) (Tibuf[CENTATTR_ID] + BST) );

    /* RETURN TO BUFFER CONTROL ROUTINE. */
    return 0;

} 

