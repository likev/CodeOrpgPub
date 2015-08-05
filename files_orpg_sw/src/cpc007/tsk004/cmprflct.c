/* RCS info */
/* $Author: steves $ */
/* $Locker:  $ */
/* $Date: 2008/02/27 15:24:53 $ */
/* $Id: cmprflct.c,v 1.2 2008/02/27 15:24:53 steves Exp $ */
/* $Revision: 1.2 $ */
/* $State: Exp $ */

#include <cmprflct.h>

/* Static Global Variables. */
static int Endvscan;
static int Begvscan;
static int Rfspt;
static int Rf460end;

/* Macro definitions. */
#define PBUF_SIZE		331208
#define NRADS			360
#define NBINS			460
#define POLGRID_SIZE		(NRADS*NBINS*sizeof(short))	/* Bytes. */

/* Function Prototypes. */
static void A30742_product_generation_control( char *radial, char *pbufptr );

/***************************************************************************

   Description:
      Buffer control routine for the Composite Reflectivity Polar Grid 
      task.

***************************************************************************/
void A30741_buffer_control(){

    int opstat, ref_flag, wid_flag, vel_flag;
    char *pbufptr = NULL, *ptr = NULL;

    /* Initialization. */
    Endvscan = 0;
    Begvscan = 1;

    /* Request an output buffer. */
    pbufptr = (char *) RPGC_get_outbuf_by_name( "CRPG", PBUF_SIZE, &opstat );

    /* Process further only if output buffer was retrieved successfully. */
    if( opstat == NORMAL ){

        /* Request input radial buffers and process until the end of volume
           scan is reached. */
	ptr = (char *) RPGC_get_inbuf_by_name( "REFLDATA", &opstat );

        /* If the input data stream has been retrieved normally, continue
           processing. */
	if( opstat == 0 ){

	    RPGC_what_moments( (Base_data_header *) ptr, &ref_flag, &vel_flag, 
                               &wid_flag );
	    if( !ref_flag ){

                /* Reflectivity moment disabled, do abort processing. */
		RPGC_rel_inbuf( ptr );
		RPGC_rel_outbuf( pbufptr, DESTROY );
		RPGC_abort_because( PROD_DISABLED_MOMENT );
		return;

	    }

	}

        while(1){

	    if (opstat == 0) {

                /* Call the product generation control routine. */
	        A30742_product_generation_control( ptr, pbufptr );

                /* Release the input buffer. */
	        RPGC_rel_inbuf( ptr );

                /* If not end of volume scan, request the next radial and repeat
                   the process. */
	        if( Endvscan ){

                    /* End of volume scan reached.   Release and forward polar
                       grid. */
		    RPGC_rel_outbuf( pbufptr, FORWARD );
                    break;

	        } 
                else{

		    ptr = (char *) RPGC_get_inbuf_by_name( "REFLDATA", &opstat );
		    continue;

	        }

            /* If input radial not successfully retrieved, release and destroy
               the output buffer. */
	    } 
            else{

	        RPGC_rel_outbuf( pbufptr, DESTROY );
	        RPGC_abort();
                break;

	    }

        }

    }
    else{

        /* Abort processing. */
	if( opstat == NO_MEM )
	    RPGC_abort_because( PROD_MEM_SHED );

	else 
	    RPGC_abort();
	
    }

    /* Return to main. */
    return;

/* End of A30741_buffer_control(). */
} 

/***************************************************************************

   Description:
      Product Generation Control routine for COMPOSITE REFLECTIVITY 
      Polar Grid program 

   Inputs:
      radial - input radial.

   Outputs:
      pbufptr - product buffer.

****************************************************************************/
static void A30742_product_generation_control( char *radial, char *pbufptr ){

    /* Initialized data */
    static int irad = 0, last_rad = 0;

    /* Local variables */
    int i, prev_rad, surv_range;
    unsigned int deltabin, ib_msw, ibin;
    float rbin, scaled_delta;

    Base_data_header *headr = (Base_data_header *) radial;
    short *refl = (short *) (radial + headr->ref_offset);
    Crpg_t *grid = (Crpg_t *) pbufptr;

    /* If radial status is beginning of volume or if no radials have been
       processed yet, then ... */
    if( (Begvscan) || (headr->status == GOODBVOL) ){

        /* Initialize polar grid to 0's. */
	memset( grid->polgrid, 0, POLGRID_SIZE );

        /* Reset beginning of volume scan to "false". */
	Begvscan = 0;

        /* Store the calibration constant in the polar grid's buffer header. */
	grid->calib_const = headr->calib_const;

    }

    /* Perform individual radial processing. */

    /* Check radial status .... */
    if( (headr->status >= GOODTHRLO) && (headr->status <= GOODTHRHI) ){

        /* Initialize the offsets to the beginning and the end of the       
           reflectivity data in the input radial. */
	surv_range = headr->surv_range;
	Rfspt = surv_range - 1;
	Rf460end = Rfspt + headr->n_surv_bins - 1;

        /* Find polar grid's coordinate for each incoming radial. */
	irad = (int) headr->azimuth;

        /* Set-up last_rad and prev_rad if beginning of elevation. */
	if( (headr->status == GOODBEL) || (headr->status == GOODBVOL) ){

	    last_rad = irad;
	    prev_rad = irad;

	} 
        else{

            /* Calculate what the previous radial index should be. */
	    prev_rad = irad - 1;

	    if( prev_rad < 0 ) 
		prev_rad = 359;
	     
	}

        /* Find (scaled) delta*increment for each bin projected to polar   
           grid (portion over 2**16 is integer bin no. projected to). */
	scaled_delta = headr->cos_ele * 65536.f;
	deltabin = (int) scaled_delta;

        /* Find coordinate of first bin in polar grid. */
	rbin = (surv_range - .5f) * scaled_delta + 65536.f;
	ibin = (unsigned int) rbin;

        /* Build Composite Reflectivity by replacing value at coordinate   
           position in polar grid with mapped bin vlaue if bin value greater. */
	for( i = Rfspt; i <= Rf460end; ++i ){

	    ib_msw = RPGC_set_mssw_to_uint( ibin ) - 1;
	    if( grid->polgrid[irad][ib_msw] < refl[i] ) 
		grid->polgrid[irad][ib_msw] = refl[i];

            /* Increment bin coordinate. */
	    ibin += deltabin;

	}

        /* Check if previous azimuth slot was missed because radials are   
           more than 1 degree apart. */
	if( prev_rad != last_rad ){

            /* Repeat processing using previous index. */
	    ibin = (unsigned int) rbin;
	    for( i = Rfspt; i <= Rf460end; ++i ){

		ib_msw = RPGC_set_mssw_to_uint( ibin ) - 1;
		if( grid->polgrid[prev_rad][ib_msw] < refl[i] ) 
		    grid->polgrid[prev_rad][ib_msw] = refl[i];
		
                /* Increment bin coordinate. */
		ibin += deltabin;

	    }

	}

        /* Test for last radial in the volume scan and set flags if found. */
	if( headr->status == GENDVOL ) 
	    Endvscan = 1;

    }

    /* Save last radial index processed. */
    last_rad = irad;

    return;

/* End of A30742_product_generation_control(). */
} 


