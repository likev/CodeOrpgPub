/* RCS info */
/* $Author: steves $ */
/* $Locker:  $ */
/* $Date: 2009/09/03 21:08:48 $ */
/* $Id: srmrmrv_buffer_control.c,v 1.5 2009/09/03 21:08:48 steves Exp $ */
/* $Revision: 1.5 $ */
/* $State: Exp $ */

#include <srmrmrv.h>

/* Macro definitions. */
#define GHEADLNG 		(sizeof(Graphic_product)/sizeof(short))
#define SHEADLNG		(sizeof(Symbology_block)/sizeof(short)) 
#define PKHEADLNG		(sizeof(packet_af1f_hdr_t)/sizeof(short))
#define POFFSET			(GHEADLNG+SHEADLNG+PKHEADLNG)

/* Static global variables. */
static Scan_Summary *Summary = NULL;

/* Function prototypes. */
static void Elev_restart( float  azm );
static int Check_need( float azm );
static void Init_prod_list( float azm, Base_data_header *hdr,  
                            int *no_mem_shed );
static int Obuf_size( float win_rng, int typbuf );
static void Set_prod_list( int typbuf, int *optr, int pbuf_size, 
                           float win_azm, float win_rng, float azm_max, 
                           float azm_min, float rng_max, float rng_min, 
	                   User_array_t *thighres, float azm, 
                           Base_data_header *hdr, int ic, int *ipct, 
                           int srmrvreg );
static void Alg_storm_motion(void);

/*//////////////////////////////////////////////////////////////////////

   Description:
      This is the main buffer control module of the storm relative mean 
      radial velocity product. It obtains base radial data buffers and 
      calls modules to extract information and then passes that information 
      onto other modules to process the data. 

/////////////////////////////////////////////////////////////////////\*/
int Srmrmrv_buffer_control(){

    int abort, first_time, no_mem_shed, ostat, rstat;
    int ref_flag, wid_flag, vel_flag, idelta, istart;
    int needed, idbuf, opbuf;
    float azm;
    Base_data_header *radial;

    /* Initialize ABORT flag to false. ABORT is used to determine if the 
       current run was ABORTED for any reason */
    abort = 0;

    /* Initialize "no_memory_shed" flag...set if all requests memory  loadshed. */
    no_mem_shed = 0;

    /* Initialize first time flag to true. */
    first_time = 1;

    while(1){

        /* Get first good radial */
        radial = (Base_data_header *) RPGC_get_inbuf_by_name( "BASEDATA", &ostat );

        if( ostat == NORMAL ){

            /* Ok, we got a radial, save radial status and check if radial is good. */
	    rstat = radial->status;
	    if( rstat > GOODTHRHI ){

               /* If radial is not good, release this buffer and get another. */
	       RPGC_rel_inbuf( radial );
	       continue;

	    }

            /* OK..... we have found the first "good" base radial. */

            /* Check for disabled moments. */
	    RPGC_what_moments( radial, &ref_flag, &vel_flag, &wid_flag );
	    if( !vel_flag ){

                /* Velocity moment disabled...Abort processing. */
	        RPGC_rel_inbuf( radial );
	        RPGC_abort_because( PROD_DISABLED_MOMENT );
	        return(-1);

            }

            /* Break out of "while()" loop.  We have a good radial. */
            break;

	}
        else{

           /* Input stream terminated. */
           RPGC_log_msg( GL_INFO, "Input Stream Terminated.  Aborting ....\n" );
           RPGC_abort();
           return(-1);

        }

    }

    Volnum = RPGC_get_buffer_vol_num( radial );
    Elev_indx = RPGC_get_buffer_elev_index( radial );

    /* Get the azimuth angle, start angle, and delta angle */
    azm = radial->azimuth;
    idelta = radial->delta_angle;
    istart = radial->start_angle;

    /* Set up the product list table from the information in the 
       product request message. */
    Init_prod_list( azm, radial, &no_mem_shed );

    /* Execute module only if the products were requested */
    if( Tnumprod > 0 ){

        /* Do while not end of elevation */
        while( 1 ){

            if( rstat <= GOODTHRHI ){

                /* Get the azimuth angle, start angle, and delta angle. */
	        azm = radial->azimuth;
	        idelta = radial->delta_angle;
	        istart = radial->start_angle;

                /* Check if this radial is needed by any product. */
		needed = Check_need( azm );

                /* If needed, process this radial. */
		if( needed )
		    Srmrmrv_process( azm, radial, idelta, istart );

            }

	    if( (Onumprod > 0) && (rstat == GOODINT) ){

                /* Drop to end of if block, and get next radial */

                /* check if radial is beginning of an elevation */
	    } 
            else if( (rstat == GOODBEL) || (rstat == GOODBVOL) ) {

	        if( first_time )
		    first_time = 0;

                else{

                    /* If the radial status is start of elevation without previous 
                       radial being the end of a elevation, an antenna restart has 
                       occured. assuming of course this is not the first time through. */
		    Elev_restart( azm );

                }

                /* If the number of outstanding products is zero, or the end of the 
                   elevation or volume was detected, release all outstanding products 
                   and exit module. */
	    } 
            else if( (Onumprod == 0) || (rstat == GENDEL) || (rstat == GENDVOL) ){ 

	        RPGC_rel_inbuf( radial );

                /* Release products only if there are some left. */
	        if( Onumprod != 0 ){

                    /* Set idbuf and opbuf to complete all unfinished products and 
                       send them out. */
	            idbuf = -1;
	            opbuf = FORWARD;
	            Srmrmrv_release_prod( idbuf, opbuf );

                }

                /* Keep processing radials until the end of the scan. */
                while(1){

                    if( (rstat != GENDEL) && (rstat != GENDVOL) ){ 

	                radial = RPGC_get_inbuf_by_name( "BASEDATA", &ostat );
		        if( ostat == NORMAL ){

		            rstat = radial->status;
		            RPGC_rel_inbuf( radial );
		            continue;

		        }

                    }

	            return(0);

	        }

            }

            /* Not end of processing... so, release this input buffer, and get 
               another. */
            RPGC_rel_inbuf( radial );
            first_time = 0;
            radial = RPGC_get_inbuf_by_name( "BASEDATA", &ostat );

            /* If buffer operation status not normal, destroy output buffers and 
               exit module. */
            if( ostat != NORMAL ){

	        idbuf = -1;
	        opbuf = DESTROY;
	        Srmrmrv_release_prod( idbuf, opbuf );
                RPGC_log_msg( GL_INFO, "Input Stream Terminated.  Aborting .... \n" );
	        RPGC_abort();
	        return(-1);

            } 
            else{

	        rstat = radial->status;
	        continue;

	    }

            break;

        } /* End of "while(1) loop. */

    /* else... no products requested to be generated. */
    } 
    else{

        /* Check if all requests memory loadshed. */
	RPGC_rel_inbuf( radial );
	if( no_mem_shed )
	    RPGC_abort_because( PROD_MEM_SHED );

	else
	    abort = 1;
	    
    } 

    /* If ABORT got set to true, request abortion. */
    if( abort )
	RPGC_abort();

    return(0);

/* End of Srmrmrv_buffer_control(). */
} 

/*\////////////////////////////////////////////////////////////////////

   Description:
      This module re-initializes the product control table in the 
      event of an elevation scan restart. 

   Inputs:
      azm - azimuth angle of first radial. 

////////////////////////////////////////////////////////////////////\*/
static void Elev_restart( float azm ){

    /* Local variables */
    int ip;

    /* Check all of the products that were initially set up in the product 
       control list */
    for( ip = 0; ip < Tnumprod; ++ip ){

        /* Reinitialize only the products that haven't already been completed 
           and released. */
	if( (Tbufctl[ip].outbuf != NULL) 
                         && 
            (Tbufctl[ip].status != RELEASED) ){

            /* Initialize: length of the RLE encoded bytes,...... */
	    Tbufctl[ip].bufcnt = POFFSET;
	    Tbufctl[ip].nrleb = 0;
	    Tbufctl[ip].maxneg = 129;
	    Tbufctl[ip].maxpos = 129;
	    Tbufctl[ip].num_radials = 0;
	    Tbufctl[ip].status = 0;

	    if( Tbufctl[ip].include_zero_deg == 1 ){

                /* If first good radial falls within a window, set flag to yes. */
		if( (azm >= (float) Tbufctl[ip].min_azm) 
                                    || 
                    (azm <= (float) Tbufctl[ip].max_azm) )
		    Tbufctl[ip].first_radial = 1;
		
	    } 
            else{

                /* If first good radial falls within a window, set flag to yes. */
		if( (azm >= (float) Tbufctl[ip].min_azm)
                                    && 
                    (azm <= (float) Tbufctl[ip].max_azm) )
		    Tbufctl[ip].first_radial = 1;
		
	    }

	}

    }

/* End of Elev_restart(). */
} 

/*\//////////////////////////////////////////////////////////////////////

   Description:
      This module checks to see which products need the current radial. 
      If a product needs the radial, a flag is set. If the product status 
      indicate that the product is completed, it is then released. 

   Inputs:
      azm - azimuth angle of radial.

   Returns:
      1 is needed, 0 if not needed.

/////////////////////////////////////////////////////////////////////\*/
static int Check_need( float azm ){

    /* Local variables */
    int needed, ip;

    /* Initialize flag indicating current radial is needed by at least one 
       product to FALSE */
    needed = 0;

    /* Check all the products set up in the product control list. */
    for( ip = 0; ip < Tnumprod; ++ip ){

        /* Only check products which haven't been completed and released. */
	if( Tbufctl[ip].status != RELEASED ) {

            /* If the product is a window product, we have to see if the radial 
               is in the window, otherwise we can assume that until the end of 
               elevation scan, the radial is needed by any full elevation scan 
               product. */
	    if( Tbufctl[ip].prod_id == Srmrvreg_id ){

                /* Check if the window includes the zero degree azimuth. */
		if( Tbufctl[ip].include_zero_deg == 1 ){

                    /* Finally check if the radial is in the window. */
		    if( (azm >= (float) Tbufctl[ip].min_azm) 
                                  || 
                        (azm <= (float) Tbufctl[ip].max_azm) ){

                        /* If it is, then set the NEED flag in the control 
                           list to YES */
			Tbufctl[ip].need = 1;

                        /* Set flag indicting that this radial is need by 
                           at least one product */
			needed = 1;

		    } 
                    else if( (Tbufctl[ip].status == 1) 
                                     && 
			     (Tbufctl[ip].first_radial == 0) ){

                        /* Else, radial is not needed by the current product, 
                           if the status of this product is open, and the elevation 
                           scan did not begin in the middle of the window, then 
                           the product is done */
			Srmrmrv_release_prod( ip, FORWARD );

		    }

		} 
                else{

                    /* Else the window does not include the zero degree azimuth. */
		    if( (azm >= (float) Tbufctl[ip].min_azm)
                                      && 
                        (azm <= (float) Tbufctl[ip].max_azm) ){

                        /* Set appropiate flags if the radial is within a window. */
			Tbufctl[ip].need = 1;
			needed = 1;

		    } 
                    else if( (Tbufctl[ip].status == 1) 
                                    && 
			     (Tbufctl[ip].first_radial == 0) ){

                       /* Else, radial is not needed by the current product, if the 
                          status of this product is open, and the elevation scan 
                          did not begin in the middle of the window, then the product
                          is done. */
			Srmrmrv_release_prod( ip, FORWARD );

		    }

		}

	    } 
            else {

                /* Else the product is a full elevation scan type, and the current 
                   radial is needed by it. */
		Tbufctl[ip].need = 1;
		needed = 1;
	    }

	}

    }

    return (needed);

/* End of Check_need(). */
} 

#define REGION_INDEX		0
#define MAP_INDEX		1

/*\///////////////////////////////////////////////////////////////////

   Description:
      This module obtains the customer requests and filters through
      them to only the storm relative mean radial velocity products
      for the current elevation scan. It then sets up the window
      limits, obtains the product buffer pointer, and calls function
      to actually fill the product control table.

//////////////////////////////////////////////////////////////////\*/
static void Init_prod_list( float azm, Base_data_header *hdr,  
                            int *no_mem_shed ){

    float win_rng, win_azm, rng_max, rng_min, azm_max, azm_min;
    int numbins, typbuf, stat, num_req, icit, ipct, pbuf_size;
    int ostat, num_mem_shed, req_made[2], req_unsat[2];
    int *optr = NULL;

    static User_array_t thighres[MAXSRMRV];

    /* Initialize product list table and product request list. */
    memset( &Tbufctl, 0, sizeof(Tbufctl_t)*MAXSRMRV );
    memset( thighres, 0, sizeof(User_array_t)*MAXSRMRV ); 

    /* Initialize flag indicating forecast data buffer is needed (false). */
    Need_fcst = 0;

    /* Initialize total and outstanding number of products to zero. */
    Tnumprod = 0;
    Onumprod = 0;
    Numwin = 0;
    Numful = 0;

    /* Initialize "no_mem_shed" flag and counter. */
    *no_mem_shed = 0;
    num_mem_shed = 0;
    memset( req_made, 0, 2*sizeof(int) );
    memset( req_unsat, 0, 2*sizeof(int) );

    /* Get customer product request information for this NTR code. */
    stat = RPGC_get_customizing_info( Elev_indx, thighres, &num_req );

    /* Get the scan summary data for this volume. */
    Summary = RPGC_get_scan_summary( Volnum );

    /* If customer information is valid and there is at least one 
       product request, set up product list table. */
    if( (stat == NORMAL) && (num_req != 0) && (Summary != NULL) ){

        /* Do until all requests checked or we reach maximum product capacity. */
	ipct = 0;
	icit = 0;

        while(1){

            /* Only process the storm relative mean radial velocity products. */
	    if( (thighres[icit].ua_prod_code == Srmrvreg_code) 
                                || 
                (thighres[icit].ua_prod_code == Srmrvmap_code) ){

                /* Only process requests for the current elevation scan. */
	        if( thighres[icit].ua_elev_index == Elev_indx ){

		    Elev_ang = RPGCS_get_target_elev_ang( Summary->vcp_number,
		   	                                  Elev_indx );

                    /* Window center azimuth and range in degrees and kilometers. */
		    win_azm = thighres[icit].ua_dep_parm_0 / 10.f;
		    win_rng = thighres[icit].ua_dep_parm_1 / 10.f;

		    win_rng *= NM_TO_KM;

                    /* If the request is a window product, get min/max range and 
                       azimuths. */
		    if( thighres[icit].ua_prod_code == Srmrvreg_code) {

                        /* Get window min/max ranges and azimths in degrees 
                           and kilometers. */
		        RPGCS_window_extraction( win_rng, win_azm, 50.0,
			                         &rng_max, &rng_min, 
                                                 &azm_max, &azm_min );

                        /* Assure that the min or max range does not go beyond 
                           radial limits. */
		        if( rng_min > 230.f ) 
			    rng_min = 229.f;
		    
		        if( rng_max > 230.f) 
			    rng_max = 230.f;
		    
                        /* Set data type of product output buffer. */
		        typbuf = Srmrvreg_id;

		    } 
                    else{

                        /* Else it's a full 360 degree - 230 km product set min/max 
                           ranges and azimuths to include entire data area assure 
                           that the max range does not exceed 70000 ft height limit. */
		        numbins = RPGC_num_rad_bins( (void *) hdr, 230, 4, 2 );

		        numbins *= 4;
		        rng_max = hdr->dop_bin_size * numbins * .001f;
		        if( rng_max > 230.f ) 
			    rng_max = 230.f;
		    
		        rng_min = .25f;
		        azm_min = 0.f;
		        azm_max = 360.f;

                        /* Set data type of product output buffer. */
		        typbuf = Srmrvmap_id;

		    }

                    /* Counter for number of requests. */
		    if( thighres[icit].ua_prod_code == Srmrvreg_code )
		       ++req_made[REGION_INDEX];

                    else
		       ++req_made[MAP_INDEX];

                    /* Get output buffer dimension for this product. */
		    pbuf_size = Obuf_size( win_rng, typbuf );

                    /* Get output buffer for this requested buffer. */
    		    if( typbuf == Srmrvreg_id )
		        optr = RPGC_get_outbuf_by_name( "SRMRVREG", pbuf_size, &ostat );

                    else
		        optr = RPGC_get_outbuf_by_name( "SRMRVMAP", pbuf_size, &ostat );

                    /* Fill product list control table if buffer was obtained. */
		    if( ostat == NORMAL )
		        Set_prod_list( typbuf, optr, pbuf_size, win_azm, 
                                       win_rng, azm_max, azm_min, rng_max, 
                                       rng_min, thighres, azm, hdr, 
                                       icit, &ipct, Srmrvreg_id );
		    else{

                        /* Counter for number of unsatisfied requests */
		        if( thighres[icit].ua_prod_code == Srmrvreg_code )
		             ++req_unsat[REGION_INDEX];

                        else
		            ++req_unsat[MAP_INDEX];

		        ++num_mem_shed;

		    }

	        }

                /* Increment index into customer request information table. */
                ++icit; 

                /* End of the "do until all requests checked or we reach maximum 
                   product capacity. */
	        if( (ipct < MAXSRMRV) && (icit < num_req) ){

                    /* Not done yet- get next request (i.e. go to 100) */
	            continue;

	        } 

            }

            break;

        } /* End of "while(1)" loop. */

        /* Set total and outstanding number of products. */
        Tnumprod = Numwin + Numful;
        Onumprod = Tnumprod;

        /* Set memory shed flag if all requests are memory shed. */
        if( (num_mem_shed == num_req) || (num_mem_shed == MAXSRMRV) ) 
	    *no_mem_shed = 1;

        /* Abort datatypes which have no memory for product generation. */
	if( (req_made[REGION_INDEX] == req_unsat[REGION_INDEX]) 
                                    && 
            (req_made[REGION_INDEX] > 0) ) 
	    RPGC_abort_datatype_because( Srmrvreg_id, PROD_MEM_SHED );
	
	if( (req_made[MAP_INDEX] == req_unsat[MAP_INDEX])
                                 && 
            (req_made[MAP_INDEX] > 0) )
	    RPGC_abort_datatype_because( Srmrvmap_id, PROD_MEM_SHED );
	
        /* If any storm motions are needed from the forcast algorithm output 
           buffer, get the buffer. */
	if( Need_fcst ) 
	    Alg_storm_motion();
	 
    }

/* End of Init_prod_list(). */
} 

/*\/////////////////////////////////////////////////////////////////////

   Description:
      This module sets the number of byte to allocate for the 
      product buffer. 

   Inputs:
      win_rng - window range, in km.
      buftyp - buffer type (either Srmrvreg_id or Srmrvmap_id).

   Returns:
      Size of the buffer, in bytes. 

/////////////////////////////////////////////////////////////////////\*/
static int Obuf_size( float win_rng, int typbuf ){

    /* Set the maximum number of integer half words allocated for the 
       product buffers. the size is dependent on the range to the center 
       of the window (i.e. more radials closer to the radar in a constant 
       size window as compared to farther from the radar). Also, if the 
       product is a full elevation scan product the product buffer will 
       be even larger. */
    if( typbuf == Srmrvmap_id ){

        /* Product is full screen. */
	return( 48000 );

    }
    /* Else... the product is a window product. */
    else{

	if( win_rng < 20.f ){

            /* Window range is < 20 km. */
	    return( 23000 );

	} 
        else if( win_rng < 36.f ){

            /* Window range is >= 20 km and < 36 km. */
	    return( 29000 );

	} 
        else if( win_rng < 85.f ){

            /* Window range is >= 36 km and < 85 km. */
	    return( 15000 );

	} 
        else{

            /* Window range is >= 85 km. */
	    return( 7800 );
	}

    }

    /* Return to calling routine. */
    return 0;

/* End of Obuf_size(). */
} 

#define GHEADLNG 		(sizeof(Graphic_product)/sizeof(short))
#define SHEADLNG		(sizeof(Symbology_block)/sizeof(short)) 
#define PKHEADLNG		(sizeof(packet_af1f_hdr_t)/sizeof(short))
#define POFFSET			(GHEADLNG+SHEADLNG+PKHEADLNG)

/*\///////////////////////////////////////////////////////////////////////

   Description:
      This module sets-up the product control table with information 
      received from the customer request table. It also derives some 
      constants and flag for each product and saves them in the 
      product control table. 

   Inputs:
      typbuf - Type of request (either Srmrvreg_id or Srmrvmap_id).
      optr - Pointer to output product buffer.
      pbuf_size - Size of output product buffer, in bytes. 
      win_azm - Request window azimith, deg.
      win_rng - Request window range, km.
      azm_max - Window maximum azimuth, deg.
      azm_min - Window minimum azimuth, deg.
      rng_max - Window maximum range, km.
      rng_min - Window minimum range, km.
      azm - radial azimuth.
      hdr - pointer to base data header. 
      ic - 
      ipct - 
      srmrvreg - 

   Outputs:
      thighres -

///////////////////////////////////////////////////////////////////////\*/
static void Set_prod_list( int typbuf, int *optr, int pbuf_size, 
                           float win_azm, float win_rng, float azm_max, 
                           float azm_min, float rng_max, float rng_min, 
	                   User_array_t *thighres, float azm, 
                           Base_data_header *hdr, int ic, int *ipct, 
                           int srmrvreg ){

    int ip;
    float r;

    /* Initialized data.  Indexed by [Doppler Resolution][Weather Mode].  
       That is, [0.5][Clear Air], [0.5][Precipitation], [1.0][Clear Air],
       [1.0][Precipitation]. */
    static int coluindx[2][2] =	{ {25, 3},
                                  {27, 5} };

    /* Fill product control table: */ 
    ip = *ipct;

    /* Buffer data type. */
    Tbufctl[ip].prod_id = typbuf;

    /* Buffer length counter. */
    Tbufctl[ip].bufcnt = POFFSET;

    /* Buffer output pointer. */
    Tbufctl[ip].outbuf = (char *) optr;

    /* Product buffer length in shorts. */
    Tbufctl[ip].maxi2_in_obuf = pbuf_size/sizeof(short);

    /* Window azimuth angle, degrees. */
    r = win_azm * 10.f;
    Tbufctl[ip].center_azm = RPGC_NINT( r );

    /* Window slant range, NM * 10. */
    Tbufctl[ip].center_range = thighres[ic].ua_dep_parm_1;

    /* Storm speed to remove, tenths of m/s.  Incoming data is in 
       kts*10, convert to m/s*10.  Then the calculations are done in 
       metric--header info in English. */
    Tbufctl[ip].storm_speed = thighres[ic].ua_dep_parm_3;
    if( thighres[ic].ua_dep_parm_3 > 0 ){

	r = (float) thighres[ic].ua_dep_parm_3 / MPS_TO_KTS;
	Tbufctl[ip].storm_speed = RPGC_NINT(r);

    }

    /* Save the incoming data in kts*10 to put in product header. */
    Tbufctl[ip].kstorm_speed = thighres[ic].ua_dep_parm_3;

    /* Storm direction to remove, degrees. */
    Tbufctl[ip].storm_dir = thighres[ic].ua_dep_parm_4;

    /* Check if forecast data is needed, set flag if it is. */
    if( thighres[ic].ua_dep_parm_3 < 0) {

	Tbufctl[ip].storm_info = -1;
	Need_fcst = 1;

    } 
    else
	Tbufctl[ip].storm_info = 0;

    /* NTR message code. */
    Tbufctl[ip].prod_code = thighres[ic].ua_prod_code;

    /* Minimum azimuth angle, deg. */
    Tbufctl[ip].max_azm = RPGC_NINT(azm_max);

    /* Maximum slant range, km */
    Tbufctl[ip].max_range = RPGC_NINT(rng_max);

    /* Minimum azimuth angle, deg. */
    Tbufctl[ip].min_azm = RPGC_NINT(azm_min);

    /* Minimum slant range, km. */
    Tbufctl[ip].min_range = RPGC_NINT(rng_min);

    /* Bin number of input radials minimum slant range. */
    r = rng_min * 1000 / hdr->dop_bin_size;
    Tbufctl[ip].min_bin = RPGC_NINT(r);

    /* Bin number of input radials maximum slant range. */
    r = rng_max * 1000 / hdr->dop_bin_size;
    Tbufctl[ip].max_bin = RPGC_NINT(r);

    /* Color table depends on weather mode and velocity resolution. */
    Dop_reso = hdr->dop_resolution;
    Tbufctl[ip].color_index = 
                coluindx[Dop_reso-1][hdr->weather_mode-1];
    RPGC_log_msg( GL_INFO, "Dop Res: %d, Weather Mode: %d, Color Table: %d\n",
                  Dop_reso, hdr->weather_mode, Tbufctl[ip].color_index );

    /* Maximum and minimum radial velocity found in region after storm 
       motion removal, scaled velocity value. initialize to mid-point 
       of the velocity scale (i.e.129). */
    Tbufctl[ip].maxneg = 129;
    Tbufctl[ip].maxpos = 129;

    /* If min_azm > max_azm, then the window includes the zero degree 
       azimuth angle. */
    if( azm_min > azm_max ){

        /* Zero degree include flag */
	Tbufctl[ip].include_zero_deg = 1;

        /* If first good radial falls within a window, set flag to yes. */
	if( (azm > azm_min) || (azm < azm_max) )
	    Tbufctl[ip].first_radial = 1;
	 
    } 
    else{
 
	Tbufctl[ip].include_zero_deg = 0;

        /* If first good radial falls within a window, set flag to yes. */
	if( (azm > azm_min) && (azm < azm_max) ) 
	    Tbufctl[ip].first_radial = 1;
	
    }

    /* Keep count of the number of window products and the number of 
       full elevation scan products requested. */
    if( typbuf == Srmrvreg_id ){

	++Numwin;

        /* Set number of bins to compact base radial to get specified 
           resolution. */

	Tbufctl[ip].bin_incr = 500 / hdr->dop_bin_size;

    } 
    else{

	++Numful;

        /* Set number of bins to compact base radial to get specified 
           resolution. */
	Tbufctl[ip].bin_incr = 1000 / hdr->dop_bin_size;

    }

    /* Maximum bin number of compacted output radial, minimum bin is 1. */
    r = (Tbufctl[ip].max_bin - Tbufctl[ip].min_bin) / (float) Tbufctl[ip].bin_incr;
    Tbufctl[ip].maxobin = RPGC_NINT(r);

    /* Increment the product counter. */
    ip++;
    *ipct = ip;

/* End of Set_prod_list(). */
} 

/*\/////////////////////////////////////////////////////////////////////

   Description:
       This module sets the storm motion value removed from the 
       radial velocity to the storm motions obtained by the forcast 
       algorithm. If the product request data contains a negative 
       storm motion, then the algorithm motions are desired. If 
       the product is a window, the closest storm to the window 
       center is selected. If the product is a full elevation scan 
       product, the average storm motion of all storms is used. 
       If the algorithm data indicates that there are no 
       continuing storms, a zero velocity will be used. 

/////////////////////////////////////////////////////////////////////\*/
static void Alg_storm_motion(void){

    /* Local variables */
    int min_ix, del_time, i, ip, vol_time;
    int itc_status;
    float min_dist, dist, xc, yc, xs, ys, r1, r2;

    vol_time = Summary->volume_start_time;

    /* Read the itc containing A3CD09 data */
    RPGC_itc_read( A3CD09, &itc_status );
    if( itc_status == NO_DATA ) 
	A3cd09.numstrm = 0;
    
    /* Do For All products .... */
    for( ip = 0; ip < Tnumprod; ++ip) {

        /* If storm motion was not specified by the user, the motion 
           field will be negative. */
	if( (Tbufctl[ip].storm_speed < 0) 
                       || 
            (Tbufctl[ip].storm_speed == PARAM_ALG_SET ) ){

            /* First make sure some storm motion data exist, if not 
               then set the storm motion in the control table to zero. */
	    if( (A3cd09.numstrm != 0) && (A3cd09.numstrm <= NSTR_TOT) ) {

                /* If this is a window product, find the nearest storm 
                   to the center. */
		if( Tbufctl[ip].prod_id == Srmrvreg_id ){

                    /* Compute delta time from structure time to current data. */
		    del_time = vol_time - A3cd09.timetag / 1e3f;

                    /* Compute x and y coordinates to the center of the window 
                       in km.  Notice that the center range is in nmx10 as input. */
		    xc = Tbufctl[ip].center_range * 0.10f * 
                         sin(Tbufctl[ip].center_azm * 0.10f * DEGTORAD) * NM_TO_KM;

		    yc = Tbufctl[ip].center_range * 0.10f * 
                         cos(Tbufctl[ip].center_azm * 0.10f * DEGTORAD) * NM_TO_KM;

                    /* Initialize the minimum distance to a storm to a big number. */
		    min_dist = 9999999.f;

                    /* Initialize MIN_IX to keep compiler from warning. */
		    min_ix = 0;

                    /* * check all storms */
		    for( i = 0; i < A3cd09.numstrm; ++i ){

                        /* Save local copy of the storm x and y positions. */
			xs = A3cd09.strmove[i][STR_XP0] + 
                             del_time * A3cd09.strmove[i][STR_XSP] * .001f;
			ys = A3cd09.strmove[i][STR_YP0] + 
                             del_time * A3cd09.strmove[i][STR_YSP] * .001f;

                        /* Compute distance from storm center to our window center. */
			r1 = xc - xs;
			r2 = yc - ys;
			dist = sqrt( (r1*r1) + (r2*r2) );

                        /* If this distance is less than the minumum distance 
                           found so far save this distance as the minimum distance. */
			if( dist < min_dist ){

			    min_dist = dist;

                            /* Save index into structure data of this storm. */
			    min_ix = i;

			}

		    }

                    /* Check if a storm was found within a realistic distance. */
		    if( min_dist < 40.f ){

                        /* Save copy of closest storms speed and direction. */
			Tbufctl[ip].storm_speed = 
                               fabs( A3cd09.strmove[min_ix][STR_SPD] ) * 10.f;

			Tbufctl[ip].kstorm_speed = Tbufctl[ip].storm_speed * MPS_TO_KTS;

			Tbufctl[ip].storm_dir = 
                               fabs( A3cd09.strmove[min_ix][STR_DIR] ) * 10.f;

		    } 
                    else {

                        /* Else... no near storms were found, set up product 
                           to do lowest 8 elevation cuts with average storm 
                           speed accounted for. */
			Tbufctl[ip].storm_speed = A3cd09.avgstspd * 10.f;

			Tbufctl[ip].kstorm_speed = A3cd09.avgstspd * MPS_TO_KTS * 10.f;

			Tbufctl[ip].storm_dir = A3cd09.avgstdir * 10.f;

		    }

		} 
                else {

                    /* Else.... it's a full scan product. */
		    Tbufctl[ip].storm_speed = A3cd09.avgstspd * 10.f;

		    Tbufctl[ip].kstorm_speed = A3cd09.avgstspd * MPS_TO_KTS * 10.f;

		    Tbufctl[ip].storm_dir = A3cd09.avgstdir * 10.f;

		}

	    } 
            else {

                /* Else..... there are no storms to get motion from. */
		Tbufctl[ip].storm_speed = 0;
		Tbufctl[ip].kstorm_speed = 0;
		Tbufctl[ip].storm_dir = 0;

	    }

	}

        /* Make sure storm speed never exceeds 100 m/s. */
	if( Tbufctl[ip].storm_speed > 1000 ) {

	    Tbufctl[ip].storm_speed = 1000;
	    Tbufctl[ip].kstorm_speed = Tbufctl[ip].storm_speed * MPS_TO_KTS;

        }

    }

/* End of Alg_storm_motion(). */
} 



