/* RCS info */
/* $Author: cmn $ */
/* $Locker:  $ */
/* $Date: 2008/08/28 17:35:40 $ */
/* $Id: clutprod.c,v 1.7 2008/08/28 17:35:40 cmn Exp $ */
/* $Revision: 1.7 $ */
/* $State: Exp $ */

#define CLUTPROD_MAIN
#include <clutprod.h>

/* Static Function Prototypes. */
static int Clutprod_updt_cd07();
static void Clutprod_timer_handler( int time_parameter );
static void Clutprod_queue_handler( int queued_parameter );
static int Clutprod_get_date_time( int which, int *gen_time_map, int *gen_date_map );
static int Clutprod_updt_cd07( int itc_id, int operation );

/************************************************************************

   Description:
      This module contains the main function for the clutter filter 
      control product producing task.

************************************************************************/
int main( int argc, char *argv[] ){

    /* Local variables */
    int i, status;

    /* Register outputs. */
    RPGC_reg_io( argc, argv );

    /* Get the product ID. */
    CFCprod_id = RPGC_get_id_from_name( "CFCPROD" );

    /* Initialize some flags. */
    Clutinfo.nw_map_request_pending = 0;
    Clutinfo.bypass_map_request_pending = 0;
    Clutinfo.product_request_pending = 0;
    Cbpm_legacy.bpm_avail_legacy = 0;
    Cbpm_orda.bpm_avail_orda = 0;
    Cbpm_orda.cmd_generated = 0;
    Cnwm.nwm_avail_legacy = 0;
    Clm.clm_avail_orda = 0;

    /* Clear the narrow band user request list. */
    for( i = 0; i < MAX_ORDA_PRODUCTS; ++i) 
	Request_list[i] = 0;

    /* Initialize log error services. */
    RPGC_init_log_services( argc, argv );

    /* Register ITC inputs and outputs. */

    /* Flags used in the processing of the CFC product */
    RPGC_itc_out( A304C2, &Map_flags, sizeof(A304c2), ITC_ON_CALL );
    RPGC_itc_in( A304C2, &Map_flags, sizeof(A304c2), ITC_ON_CALL );

    /* Register ITC callback function. */
    RPGC_itc_callback( A304C2, Clutprod_updt_cd07 );

    /* Register for external START_OF_VOLUME_SCAN_DATA event. */
    RPGC_reg_for_external_event( ORPGEVT_START_OF_VOLUME_DATA, Clutprod_queue_handler, 
                                 START_OF_VOLUME_SCAN);

    /* Register for external BYPASS_MAP_RECEIVED event. */
    RPGC_reg_for_external_event( ORPGEVT_BYPASS_MAP_RECEIVED, Clutprod_queue_handler, 
                                 BM_RECEIVED );

    /* Register for external CLUTTER_MAP_RECEIVED event. */
    RPGC_reg_for_external_event( ORPGEVT_CLUTTER_MAP_RECEIVED, Clutprod_queue_handler, 
	                         NW_RECEIVED );

    /* Register for internal EVT_CFCPROD_REPLAY_PRODUCT_REQUEST event. */
    RPGC_reg_for_internal_event( EVT_CFCPROD_REPLAY_PRODUCT_REQUEST, Clutprod_queue_handler, 
                                 REQUEST_BIAS );

    /* Register for NW_MAP_NOT_COMING timer interrupt. */
    RPGC_reg_timer( NW_MAP_NOT_COMING, Clutprod_timer_handler );
    RPGC_reg_timer( BYPASS_MAP_NOT_COMING, Clutprod_timer_handler );

    /* Register for Scan Summary and Volume Status. */
    RPGC_reg_scan_summary();
    RPGC_reg_volume_status( &Vol_stat );

    /* Register for Color Table updates. */
    RPGC_reg_color_table( &Coldat, ADPT_UPDATE_WITH_CALL );

    /* Tell system we are Event driven. */
    RPGC_task_init( EVENT_BASED, argc, argv );

    /* Write ITC_A304C2 since it gets initialized within this module. */
    RPGC_itc_write( A304C2, &status );

    /* Get the rda config (legacy or orda). */
    Rda_config = Clutprod_get_rda_config();
    RPGC_log_msg( GL_INFO, "Initial RDA Configuration: %d\n", Rda_config );

    /* Do initial read of notchwidth/clutter map and bypass map. */
    Clutprod_read_map( READ_BYPASS_MAP );
    Clutprod_read_map( READ_CLUTTER_MAP );

    /* Waiting for event to occur. */
    while(1)
        RPGC_wait_for_event();

} 


/********************************************************************

   Description:
      Timer Trap Receiver routine for Clutter Filter Control product 
      task.

   Inputs:
      time_parameter - timer ID.

********************************************************************/
static void Clutprod_timer_handler( int time_parameter ){

    int i, status, notify_distrib;

    /* Set the NOTIFY_DISTRIB flag to FALSE (0).   This flag is only set 
       if the NW_MAP_NOT_COMING timer or BYPASS_MAP_NOT_COMING timer 
       expire and there are pending product requests. */
    notify_distrib = 0;

    /* NOTCHWIDTH MAP IS NOT COMING timer expiration. */
    if( time_parameter == NW_MAP_NOT_COMING ){

	RPGC_log_msg( GL_STATUS, "Clutter Filter Map is Currently UNAVAILABLE");

        /* Must notify scheduler if there are outstanding product requests. */
	if( Clutinfo.product_request_pending )
	    notify_distrib = 1;
	 
        /* Read itc A304C2. */
	RPGC_itc_read( A304C2, &status );

        /* Clear the NW_MAP_REQUEST_PENDING flag so that it can be asked 
           for again at a later time. */
	Clutinfo.nw_map_request_pending = 0;

        /* Write itc A304C2. */
	RPGC_itc_write( A304C2, &status);

        /* BYPASS MAP IS NOT COMING timer expiration. */

    } 
    else if( time_parameter == BYPASS_MAP_NOT_COMING ){

        RPGC_log_msg( GL_STATUS, "Clutter Bypass Map is Currently UNAVAILABLE" );

        /* Must notify scheduler if there are outstanding product requests. */
	if( Clutinfo.product_request_pending ) 
	    notify_distrib = 1;

        /* Read itc A304C2. */
	RPGC_itc_read( A304C2, &status);

        /* Clear the BYPASS_MAP_REQUEST_PENDING flag so that it can be 
           asked for again at a later time. */
	Clutinfo.bypass_map_request_pending = 0;

        /* Write itc A304C2. */
	RPGC_itc_write( A304C2, &status);

    } 
    else{

        /* Unknown timer expired... Do nothing. */
        LE_send_msg( GL_INFO, "Unknown Timer ID Received: %d\n",
                     time_parameter );

    }

    /* Do we have to tell the scheduler? */
    if( notify_distrib ){

        int num_products = MAX_ORDA_PRODUCTS;

        if( Rda_config == ORPGRDA_LEGACY_CONFIG )
           num_products = MAX_PRODUCTS;

        /* Yes, we do! */
        for( i = 0; i < num_products; ++i ){

            /* Do for each outstanding product request. */
	    if( Request_list[i] != 0 ){

	        Clutprod_request_response( PGM_REPLAY_DATA_UNAVAILABLE, Request_list[i] );
	        Request_list[i] = 0;

	    }

	}

        /* Clear PRODUCT_REQUEST_PENDING flag. */
	Clutinfo.product_request_pending = 0;

    }

    /* Return to calling routine. */
    return;

/* End of Clutprod_timer_handler(). */
} 


/**********************************************************************

   Description:
      Interrupt handler for Clutter Filter Control Product task. 

   Inputs:
      queued_parameter - parameter resulting in the interrupt.

**********************************************************************/
static void Clutprod_queue_handler( int queued_parameter ){
 
    /* Local variables */
    int num_products_orda,gen_date_map, gen_time_map, status;
    int gen_time_status, gen_date_status;

    /* Get the rda config (legacy or orda). */
    Rda_config = Clutprod_get_rda_config();

    /* Read the ITC containing the clutter map product flags 
       BYPASS_MAP_REQUEST_PENDING and NW_MAP_REQUEST_PENDING. 
       This information is needed to service all events. */
    RPGC_itc_read( A304C2, &status);

    LE_send_msg( GL_INFO, "Received Queued Parameter: %d\n", queued_parameter );

    /* Queued parameter is START_OF_VOLUME_SCAN notification. */
    if( queued_parameter == START_OF_VOLUME_SCAN ){

	RPGC_log_msg( GL_INFO, "Processing Start of Volume Event.\n" );

        /* Time to check date/time stamps of Notchwidth and Bypass Maps. */

        /* First, let's check if new Bypass Map is available.  No need to 
           re-request map if a request is already pending. */
	if( !Clutinfo.bypass_map_request_pending ){

            /* Get the Bypass Map generation date/time from the last Bypass 
               Map read (i.e., locally stored values). */
	    status = Clutprod_get_date_time( READ_BYPASS_MAP, &gen_time_map, &gen_date_map );
	    if( status < 0 ) 
		return;
	     
            /* Get the bypass map generation date/time from RDA status. */
	    status = Clutprod_bp_gen_date_time( &gen_time_status, &gen_date_status );
	    if( status < 0 )
		return;
	     
            /* Check date/time stamp in RDA_STATUS against locally stored 
               values.  If no match, need to request new Bypass Map. */
	    if( (gen_time_status != gen_time_map)
                                 || 
                (gen_date_status != gen_date_map) ) {

                /* New map data is available at RDA.   Clear the BPM_AVAIL_xxx 
                   flag to indicate Bypass Map data at RPG no longer valid. */
		if( Rda_config == ORPGRDA_ORDA_CONFIG ) 
		    Cbpm_orda.bpm_avail_orda = 0;

		else 
		    Cbpm_legacy.bpm_avail_legacy = 0;
		 
                /* Request the Bypass Map from the RDA. */
		RPGC_log_msg( GL_INFO, "Bypass Map date/time not current. Requesting new map.\n");
		RPGC_log_msg( GL_INFO, "-->Status/Map Date: %8d/%8d\n", gen_date_status, gen_date_map );
		RPGC_log_msg( GL_INFO, "-->Status/Map Time: %8d/%8d\n", gen_time_status, gen_time_map );
		Clutprod_request_map( DREQ_CLUTMAP, 
                                      "Clutter Bypass Map is Currently UNAVAILABLE.\n" );
	    }

	}

        /* Let's check if new Notchwidth (Clutter) Map is available.  No 
           need to re-request map if a request is already pending. */
	if( !Clutinfo.nw_map_request_pending ){

            /* Get the Notchwidth (Clutter) Map generation date/time from 
               the last Notchwidth (Clutter) map read (i.e., locally stored values). */
	    status = Clutprod_get_date_time( READ_CLUTTER_MAP, &gen_time_map, &gen_date_map );
	    if( status < 0 ) 
		return;

            /* Get the notchwidth map generation date/time stored in RDA status. */
	    status = Clutprod_nw_gen_date_time( &gen_time_status, &gen_date_status );
	    if( status < 0 ) 
		return;

            /* Check date/time stamp in RDA_STATUS against locally stored 
               values.  If no match, need to request new map from RDA. */
	    if( (gen_time_status != gen_time_map)
                               || 
                (gen_date_status != gen_date_map) ){

                 /* New map data is available at RDA.   Clear the NW_(CLM_)AVAIL_xxx flag 
                    to indicate the map data at RPG no longer valid. */
		if( Rda_config ==  ORPGRDA_ORDA_CONFIG ) 
		    Clm.clm_avail_orda = 0;

		else 
		    Cnwm.nwm_avail_legacy = 0;
		
                /* Request the Clutter Map from the RDA. */
		RPGC_log_msg( GL_INFO, "Clutter Filter Map date/time not current. Requesting new map.\n" );
		RPGC_log_msg( GL_INFO, "-->Status/Map Date: %8d/%8d\n", gen_date_status, gen_date_map );
		RPGC_log_msg( GL_INFO, "-->Status/Map Time: %8d/%8d\n", gen_time_status, gen_time_map );

		Clutprod_request_map( DREQ_NOTCHMAP, 
                                      "Clutter Filter Map is Currently UNAVAILABLE" );
	    }

	}

        /* If the Clutter and Bypass maps are available, check product data base 
           to see if there are CFC products.  If not, they need to be generated. */
	if( (Rda_config == ORPGRDA_LEGACY_CONFIG) 
                        && 
            (Cnwm.nwm_avail_legacy == 1) 
                        && 
	    (Cbpm_legacy.bpm_avail_legacy == 1) ) 
	    Clutprod_handle_volstart( MAX_PRODUCTS );

	else if( (Rda_config == ORPGRDA_ORDA_CONFIG) 
                             && 
		 (Clm.clm_avail_orda == 1)
                             && 
		 (Cbpm_orda.bpm_avail_orda == 1) ){

	    num_products_orda = Clm.clm_map_orda.num_elevation_segs;
	    Clutprod_handle_volstart( num_products_orda );

	}

    /* Queued parameter is Bypass Map received (BM_RECEIVED). */
    } 
    else if( queued_parameter == BM_RECEIVED ){

	RPGC_log_msg( GL_INFO, "Processing Bypass Map Received Event.\n" );

        /* Read in the Bypass Map data from LB. */
	status = Clutprod_read_map( READ_BYPASS_MAP );

        /* Exit module on bad status.  Set the availability flag otherwise. */
	if( status < 0 ) 
	    return;
	     
        /* Cancel BYPASS_MAP_NOT_COMING timer. */
	RPGC_cancel_timer( BYPASS_MAP_NOT_COMING, WAIT_FOR_MAP );

        /* Do we have Notchwidth (Clutter) Map data available?  If yes, let's 
           build all products. (NOTE: Whenever new map data is received, 
           we automatically generate all products, regardless of the 
           number and types of narrowband user requests.) */
	if( ((Rda_config == ORPGRDA_ORDA_CONFIG) && (Clm.clm_avail_orda)) 
                                      || 
	    ((Rda_config == ORPGRDA_LEGACY_CONFIG) && (Cnwm.nwm_avail_legacy))){

	    Clutinfo.generate_all_products = 1;
	    Clutprod_generation_control();

            /* Reset the GENERATE_ALL_PRODUCTS flag. */
	    Clutinfo.generate_all_products = 0;

	}

#ifdef CMD_TEST
        else{

             /* Code added to test CMD in a non-operational environment
                without requiring clutter map.  Clutter map is assumed
                bypass map in control everywhere. */
             char *non_op = getenv( "ORPG_NONOPERATIONAL" );

             if( non_op != NULL ){

                /* Generate a clutter map with bypass map in control. */
                if( Clutprod_gen_clutter_map() >= 0 ){

	            Clutinfo.generate_all_products = 1;
	            Clutprod_generation_control();

                    /* Reset the GENERATE_ALL_PRODUCTS flag. */
	            Clutinfo.generate_all_products = 0;

                    /* Reset the clutter map available flag. */
                    Clm.clm_avail_orda = 0;

                }

            }

        }
#endif

    /* Queued parameter is Notchwidth Map received (NW_RECEIVED). */
    } 
    else if( queued_parameter == NW_RECEIVED ){

	RPGC_log_msg( GL_INFO, "Processing Clutter Map Received Event.\n" );

        /* Read in the Notchwidth (Clutter) Map data from LB.  Note that 
           the Notchwidth Map generation date and time must also be read. */
	status = Clutprod_read_map( READ_CLUTTER_MAP );

        /* Exit module on bad status.  Set the availability flag otherwise. */
	if( status < 0 ) 
	    return;
	     
        /* Cancel NW_MAP_NOT_COMING timer if map received was requested 
           (i.e., not UNSOLICITED_NW_RECEIVED). */
	if( !Clutinfo.unsolicited_nw_received ) 
	    RPGC_cancel_timer( NW_MAP_NOT_COMING, WAIT_FOR_MAP );

         /* Do we have Bypass Map data available.  If yes, let's build 
            all products. (NOTE: Whenever new map data is received, we 
            automatically generate all products, regardless of the number 
            and types of narrowband user requests.) */
	if( ((Rda_config == ORPGRDA_ORDA_CONFIG) && (Cbpm_orda.bpm_avail_orda))
                                       || 
	    ((Rda_config == ORPGRDA_LEGACY_CONFIG) && Cbpm_legacy.bpm_avail_legacy) ){

	    Clutinfo.generate_all_products = 1;
	    Clutprod_generation_control();

            /* Reset the GENERATE_ALL_PRODUCTS flag. */
	    Clutinfo.generate_all_products = 0;

	}

    /* Must be a product request from the scheduler. */
    } 
    else if( (queued_parameter >= (MIN_REQUEST + REQUEST_BIAS) ) 
                            && 
             (queued_parameter <= (MAX_REQUEST + REQUEST_BIAS)) ){

	RPGC_log_msg( GL_INFO, "Processing Product Request Received Event.\n" );

        /* Process this user request. */
        if( Rda_config == ORPGRDA_ORDA_CONFIG )
	    Clutprod_process_request( queued_parameter );

        else
	    Clutprod_process_legacy_request( queued_parameter );

    } 
    else{

        /* Unknown queued parameter.... Do nothing. */
        LE_send_msg( GL_INFO, "Unknown Queued Parameter: %d\n", 
                     queued_parameter );

    }

    /* Return to calling routine. */
    return;

/* End of Clutprod_queue_handler() */
} 


/********************************************************************

   Description:
      Get the date/time based on the configuration and the map of 
      interest. 

   Inputs:
      which - Which map data to retrieve.

   Outputs:
      gen_time_map - Generation time of map.
      gen_date_map - Generation date of map.

   Returns:
      0 on success, -1 on error. 

*********************************************************************/
static int Clutprod_get_date_time( int which, int *gen_time_map, 
                                   int *gen_date_map ){

    int status = 0;

    if( Rda_config == ORPGRDA_LEGACY_CONFIG ){

	if( which == READ_CLUTTER_MAP ){

	    *gen_time_map = Cnwm.nwm_map_legacy.time;
	    *gen_date_map = Cnwm.nwm_map_legacy.date;

	}
        else{

	    *gen_time_map = Cbpm_legacy.clby_map_legacy.time;
	    *gen_date_map = Cbpm_legacy.clby_map_legacy.date;

	}

    } 
    else if( Rda_config == ORPGRDA_ORDA_CONFIG ){

	if( which == READ_CLUTTER_MAP ){

	    *gen_time_map = Clm.clm_map_orda.time;
	    *gen_date_map = Clm.clm_map_orda.date;

	} 
        else{

	    *gen_time_map = Cbpm_orda.clby_map_orda.time;
	    *gen_date_map = Cbpm_orda.clby_map_orda.date;

	}

    } 
    else
	status = -1;

    /* Return to Calling Module. */

    return status;

/* End of Clutprod_get_date_time() */
} 

/*************************************************************************

   Description: 
      This file defines all ORPG InterTask Communication (ITC) callback 
      routines required for the CFCPROD task.  These callback routines move 
      data from the ITC block into a local buffer.

   Inputs:
      itc_id - ITC ID.
      operation - either ITC_READ_OPERATION or ITC_WRITE_OPERATION.

*************************************************************************/
static int Clutprod_updt_cd07( int itc_id, int operation ){

    /* Read operation ... transfer data from ITC to local structure. */
    if( (itc_id == A304C2) && (operation == ITC_READ_OPERATION) ){

	Clutinfo.nw_map_request_pending = Map_flags.nw_map_request_pending;
	Clutinfo.bypass_map_request_pending = Map_flags.bypass_map_request_pending;
	Clutinfo.unsolicited_nw_received = Map_flags.unsolicited_nw_received;

    }
 
    /* Write operation ... transfer data from local structure to ITC. */
    else if( (itc_id == A304C2) && (operation == ITC_WRITE_OPERATION) ){

	Map_flags.nw_map_request_pending = Clutinfo.nw_map_request_pending;
	Map_flags.bypass_map_request_pending = Clutinfo.bypass_map_request_pending;
	Map_flags.unsolicited_nw_received = Clutinfo.unsolicited_nw_received;

    } 
    else 
	RPGC_abort();
  
    return 0;

/* End of Clutprod_updt_cd07() */
} 
