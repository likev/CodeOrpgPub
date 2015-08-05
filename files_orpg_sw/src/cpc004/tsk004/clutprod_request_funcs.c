/* RCS info */
/* $Author: steves $ */
/* $Locker:  $ */
/* $Date: 2014/03/13 19:48:24 $ */
/* $Id: clutprod_request_funcs.c,v 1.5 2014/03/13 19:48:24 steves Exp $ */
/* $Revision: 1.5 $ */
/* $State: Exp $ */

#include <clutprod.h>

/*******************************************************************************

   Description:
      Request a map from RDA.  

   Inputs:
      requested_map - Map to be requested.
      error_phrase - Error message in the event there is a problem 
                     requesting the map.

   Returns:
      1 on success, 0 if some error occurred.

*******************************************************************************/
int Clutprod_request_map( int requested_map, char *error_phrase ){

    /* Local variables */
    int wb_connected, status, successful;

    /* Clear the success of operation flag. */
    successful = 0;

    /* If wideband primary connected, request Map from RDA. */
    status = Clutprod_is_wb_connected( &wb_connected );
    if( wb_connected && (status >= 0) ){

        /* Clear the filter status flag. */
	status = 0;

        /* Determine which map is being requested.  Process differently. */
	if( requested_map == DREQ_CLUTMAP ) {

            /* Set the BYPASS_MAP_REQUEST_PENDING flag and activate the 
                BYPASS_MAP_NOT_COMING timer. */

            /* Read itc A304C2. */
	    RPGC_itc_read( A304C2, &status );

	    Clutinfo.bypass_map_request_pending = 1;

            /* Write itc A304C2. */
	    RPGC_itc_write( A304C2, &status );
	    RPGC_set_timer( BYPASS_MAP_NOT_COMING, WAIT_FOR_MAP );

	}
        else{

            /* Set the NW_MAP_REQUEST_PENDING flag and activate the 
               NW_MAP_NOT_COMING timer. */

            /* Read itc A304C2. */
	    RPGC_itc_read( A304C2, &status );
	    Clutinfo.nw_map_request_pending = 1;

            /* Write itc A304C2. */
	    RPGC_itc_write( A304C2, &status);
	    RPGC_set_timer( NW_MAP_NOT_COMING, WAIT_FOR_MAP );

	}

        /* Request the map that is asked for. */
	status = ORPGRDA_send_cmd( COM4_REQRDADATA, APP_INITIATED_RDA_CTRL_CMD,
                                   requested_map, 0, 0, 0, 0, NULL );

        /* If everything went OK, set SUCCESSFUL. */
	if( status >= 0 )
	    successful = 1;
	
        else {

            /* Write System Status Log message telling operator that 
               data is unavailable. */
	    RPGC_log_msg( GL_STATUS | LE_RPG_GEN_STATUS, error_phrase );

	}

        /* The wideband is either not connected or we are playing back 
           data.  In either case, can't make request now. */

    } 
    else{

        /* Only report status if status is not being filtered. */
	if( !status ){

	    RPGC_log_msg( GL_STATUS | LE_RPG_GEN_STATUS, error_phrase );
	    successful = 0;

	}

    }

    /* Return to calling routine. */
    return successful;

/* End of Clutprod_request_map() */
}

/*********************************************************************

   Description:
      Notifies scheduler when a user requested product can not be 
      generated.  

   Inputs:
      reason_code - reason the product could not be generated.
      request_bit_map - product dependent parameters for request.

*********************************************************************/
void Clutprod_request_response( int reason_code, short request_bit_map ){

    int i;

    static short dep_params[NUM_PROD_DEPENDENT_PARAMS];

    /* Write the response reason to task log. */
    switch( reason_code ){

        case PGM_REPLAY_DATA_UNAVAILABLE:
            RPGC_log_msg( GL_INFO,
                 "Product Not Generated Because PGM_REPLAY_DATA_UNAVAILABLE\n" );
            break;

        case PGM_MEM_LOADSHED:
            RPGC_log_msg( GL_INFO,
                 "Product Not Generated Because PGM_MEM_LOADSHED\n" );
            break;

        case PGM_INVALID_REQUEST:
             RPGC_log_msg( GL_INFO,
                 "Product Not Generated Because PGM_INVALID_REQUEST\n" );
            break;

        default:
            reason_code = PGM_REPLAY_DATA_UNAVAILABLE;
            RPGC_log_msg( GL_INFO,
                 "Product Not Generated Because PGM_REPLAY_DATA_UNAVAILABLE\n" );
            break;

    /* End of "switch" statement. */
    }

    /* Initialize the product dependent parameters. */
    for( i = 0; i < NUM_PROD_DEPENDENT_PARAMS; i++ )
        dep_params[i] = PARAM_UNUSED;

    /* Set the one and one product dependent parameter for this product. */
    dep_params[0] = (short) request_bit_map;

    /* Tell scheduler that product not coming. */
    RPGC_product_replay_request_response( CFCPROD, reason_code, dep_params );

/* Clutprod_request_response() */
} 


/*****************************************************************************

   Description:
      Parameter Trap Processor for Clutter Filter Control Product
      requests from narrowband users.   

   Inputs:
      queued_parameter - parameter to service.

   Returns:
      Currently this function always returns 0.

   Notes:
      Supports ORDA configuration only.

*****************************************************************************/
int Clutprod_process_request(int queued_parameter ){

    /* Local variables */
    static short request_bit_map;
    static int gen_date, gen_time;
    static int status;
    static int notify_distrib;
    int i, index;

    /* Get the REQUEST_BIT_MAP from the queued parameter. */
    request_bit_map = (short) (queued_parameter - REQUEST_BIAS);

     /* Validate the REQUEST_BIT_MAP. */
    if( (request_bit_map < 2) || (request_bit_map > 32) ){

        /* Notify the scheduler task of an illegal product request. */
	Clutprod_request_response( PGM_INVALID_REQUEST, request_bit_map );

        RPGC_log_msg( GL_INFO, "Bad Request .... Invalid Product Parameters.\n" );

    } 
    else {

        /* REQUEST_BIT_MAP is valid.  Add the request to the queue. */
        index = 0;
        for( i = 1; i <= MAX_ORDA_PRODUCTS; i++ ){

            if( request_bit_map & (1 << i) ){

                index = i - 1;
                break;

             }

        }

	Request_list[index] = request_bit_map;
	Clutinfo.product_request_pending = 1;

        /* Let's check if Bypass Map data is available.  If so, then .... */
	notify_distrib = 0;
	if( Cbpm_orda.bpm_avail_orda ){

            /* Check date/time stamp in RDA_STATUS against locally stored 
               values. */
	    status = Clutprod_bp_gen_date_time( &gen_time, &gen_date );

	    if( (gen_time != Cbpm_orda.clby_map_orda.time) 
                             || 
                (gen_date != Cbpm_orda.clby_map_orda.date) ){

                /* New map data is available.   Clear the BYPASS_MAP_AVAIL flag 
                   to indicate data at RPG no longer valid. */
		Cbpm_orda.bpm_avail_orda = 0;

                /* Request the Bypass Map from the RDASC. */
		status = Clutprod_request_map( DREQ_CLUTMAP,  
                                               "Clutter Filter Bypass Map is Currently UNAVAILABLE\n" ); 
                LE_send_msg( GL_INFO, "Requesting Bypass Map: Status Time/Date: %d/%d, Map Time/Date: %d/%d\n", 
	                     gen_time, gen_date, Cbpm_orda.clby_map_orda.time, Cbpm_orda.clby_map_orda.date );
		notify_distrib = !status;

	    }

            /* Let's see if there is already an outstanding request for the 
               Bypass Map data. */

	} 
        else if( !Clutinfo.bypass_map_request_pending ){


            /* No request pending.  Need to ask for the Bypass Map data from 
               RDASC. */
	    status = Clutprod_request_map( DREQ_CLUTMAP, 
                                           "Clutter Filter Bypass Map is Currently UNAVAILABLE\n" ); 
            LE_send_msg( GL_INFO, "Requesting Bypass Map ....\n" );
	    notify_distrib = !status;

	}

        /* Let's check if Clutter Map is available.  If so, then ... */
	if( Clm.clm_avail_orda ){

            /* Check date/time stamp in RDA_STATUS against locally stored 
               values. */
	    Clutprod_nw_gen_date_time( &gen_time, &gen_date );
	    if( (gen_time != Clm.clm_map_orda.time) 
                             || 
                (gen_date != Clm.clm_map_orda.date) ) {

                /* New map data is available.   Clear the NW_MAP_AVAIL flag 
                   to indicate data at RPG no longer valid. */
		Clm.clm_avail_orda = 0;

                /* Request the Notchwidth Map from the RDASC via PREPRDAT. */
		status = Clutprod_request_map( DREQ_NOTCHMAP, 
                                               "Clutter Filter Map is Currently UNAVAILABLE\n" );
                LE_send_msg( GL_INFO, "Requesting Clutter Map: Status Time/Date: %d/%d, Map Time/Date: %d/%d\n", 
	                     gen_time, gen_date, Clm.clm_map_orda.time, Clm.clm_map_orda.date );

	    }

            /* Let's see if there is already an outstanding request for the 
               Notchwidth Map data. */
	} else if( !Clutinfo.nw_map_request_pending ){

            /* No request pending.  Need to ask for the Notchwidth Map. */
	    status = Clutprod_request_map( DREQ_NOTCHMAP,  
                                           "Clutter Filter Map is Currently UNAVAILABLE\n" );
            LE_send_msg( GL_INFO, "Requesting Clutter Map ....\n" );
	}

        /* If both maps available and they are the latest and greatest, 
           generate some products. */
        LE_send_msg( GL_INFO, "Clutter Map Available: %d, Bypass Map Available: %d\n",
                     Clm.clm_avail_orda, Cbpm_orda.bpm_avail_orda );
	if ( Clm.clm_avail_orda && Cbpm_orda.bpm_avail_orda )
	    Clutprod_generation_control();

        else {

            /* Send request/response to DISTRIB that product is not coming. */
	    if( notify_distrib || !status ) 
		Clutprod_request_response( PGM_REPLAY_DATA_UNAVAILABLE, request_bit_map );

	}
    }

    /* Return to calling routine. */
    return 0;

/* End of Clutprod_process_request() */
} 
/*****************************************************************************

   Description:
      Parameter Trap Processor for Clutter Filter Control Product
      requests from narrowband users. 

   Inputs:
      queued_parameter - parameter to service.

   Returns:
      Currently this function always returns 0.

*****************************************************************************/
int Clutprod_process_legacy_request(int queued_parameter ){

    /* Local variables */
    static short request_bit_map;
    static int gen_date, gen_time;
    static int status;
    static int notify_distrib;

    LE_send_msg( GL_INFO, "Legacy Product Request Received.\n" );

    /* Get the REQUEST_BIT_MAP from the queued parameter. */
    request_bit_map = (short) (queued_parameter - REQUEST_BIAS);

     /* Validate the REQUEST_BIT_MAP. */
    if( (request_bit_map < 2) || (request_bit_map > 5) ){

        /* Notify the scheduler task of an illegal product request. */
	Clutprod_request_response( PGM_INVALID_REQUEST, request_bit_map );

    } 
    else {

        /* REQUEST_BIT_MAP is valid.  Add the request to the queue. */
	Request_list[request_bit_map - 2] = request_bit_map;
	Clutinfo.product_request_pending = 1;

        /* Let's check if Bypass Map data is available.  If so, then .... */
	notify_distrib = 0;
	if( Cbpm_legacy.bpm_avail_legacy ){

            /* Check date/time stamp in RDA_STATUS against locally stored 
               values. */
	    status = Clutprod_bp_gen_date_time( &gen_time, &gen_date );

	    if( (gen_time != Local_data.loc_bpm_gentime) 
                             || 
                (gen_date != Local_data.loc_bpm_gendate) ){

                /* New map data is available.   Clear the BYPASS_MAP_AVAIL flag 
                   to indicate data at RPG no longer valid. */
		Cbpm_legacy.bpm_avail_legacy = 0;

                /* Request the Bypass Map from the RDASC via PREPRDAT. */
		status = Clutprod_request_map( DREQ_CLUTMAP,  
                                "Clutter Filter Bypass Map is Currently UNAVAILABLE\n" ); 
		notify_distrib = !status;

	    }

            /* Let's see if there is already an outstanding request for the 
               Bypass Map data. */

	} 
        else if( !Clutinfo.bypass_map_request_pending ){

            /* No request pending.  Need to ask for the Bypass Map data from 
               RDASC. */
	    status = Clutprod_request_map( DREQ_CLUTMAP, 
                            "Clutter Filter Bypass Map is Currently UNAVAILABLE\n" ); 
	    notify_distrib = !status;

	}

        /* Let's check if Notchwidth Map is available.  If so, then ... */
	if( Cnwm.nwm_avail_legacy ){

            /* Check date/time stamp in RDA_STATUS against locally stored 
               values. */
	    Clutprod_nw_gen_date_time( &gen_time, &gen_date );
	    if( (gen_time != Cnwm.nwm_map_legacy.time) || (gen_date != Cnwm.nwm_map_legacy.date) ) {

                /* New map data is available.   Clear the NW_MAP_AVAIL flag 
                   to indicate data at RPG no longer valid. */
		Cnwm.nwm_avail_legacy = 0;

                /* Request the Notchwidth Map from the RDASC. */
		status = Clutprod_request_map( DREQ_NOTCHMAP, 
                                             "Clutter Notchwidth Map is Currently UNAVAILABLE\n" );

	    }

            /* Let's see if there is already an outstanding request for the 
               Notchwidth Map data. */
	} else if( !Clutinfo.nw_map_request_pending ){

            /* No request pending.  Need to ask for the Notchwidth Map. */
	    status = Clutprod_request_map( DREQ_NOTCHMAP,  
                                         "Clutter Notchwidth Map is Currently UNAVAILABLE\n" );
	}

        /* If both maps available and they are the latest and greatest, 
           generate some products. */
	if ( Cbpm_legacy.bpm_avail_legacy && Cnwm.nwm_avail_legacy ) 
	    Clutprod_generation_control();
	
        else {

            /* Send request/response to DISTRIB that product is not coming. */
	    if( notify_distrib || !status ) 
		Clutprod_request_response( PGM_REPLAY_DATA_UNAVAILABLE, request_bit_map );

	}
    }

    /* Return to calling routine. */
    return 0;

/* End of Clutprod_process_legacy_request() */
} 
