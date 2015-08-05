/* RCS info */
/* $Author: steves $ */
/* $Locker:  $ */
/* $Date: 2009/03/05 20:19:26 $ */
/* $Id: basvgrid.c,v 1.4 2009/03/05 20:19:26 steves Exp $ */
/* $Revision: 1.4 $ */
/* $State: Exp $ */

#include <basvgrid.h>

#define SCALEFAC		65536
#define MULT 			4.0f

/* Function Prototypes. */
static void Init_grid( short *mapvel );
static void Hdr_stuff( void *radial, int *nstat, float *az, int *elno, 
                       float *el, int *rssd, int *strtbin, int *lastbin, 
                       int *dopmode );
static void Cart_map_vel( short *vel, int rssd, float az, int strtbin, 
                          int *lastbin, short mapvel[][NACOL], int *maxmax, 
                          int *maxi, int *maxj );

/*\/////////////////////////////////////////////////////////
//
//   Description:
//      Acquisition of input and output buffers and call 
//      principle processing routines. 
//
/////////////////////////////////////////////////////////\*/
void Basvgrid_buffer_control( void ){

    int *ipr = NULL, *popr = NULL;
    int elno, rssd, istat, nstat, ostat;
    int ref_flag, wid_flag, vel_flag;
    int pbsize, dopmode, obuflag, lastbin, strtbin;

    float el, az;

    short *vel_data = NULL;
    Base_data_header *hdr = NULL;

    /* Data initializations */
    obuflag = 0;
    popr = NULL;

    /* Get radial */
    ipr = RPGC_get_inbuf_by_name( "COMBBASE", &istat);

    if( istat == RPGC_NORMAL ) {

        /* Check for disabled moments */
	RPGC_what_moments( (Base_data_header *) ipr, &ref_flag, 
                           &vel_flag, &wid_flag );
	if( !vel_flag ){

            /* Velocity disabled...do ABORT processing */
	    RPGC_rel_inbuf( ipr );
	    RPGC_abort_because( PROD_DISABLED_MOMENT );
	    return;

	}

    }

    /* Acquire all needed input buffers to completely process a volume 
       scan ( on radial basis ). */

    /* DO for each elevation scan */
    while(1){

        if( istat == RPGC_NORMAL ){

            /* Call routine to get radial status flag, azimuth, el. no, 
               start and end radial bins and Doppler mode */
	    Hdr_stuff( ipr, &nstat, &az, &elno, &el, &rssd, 
                       &strtbin, &lastbin, &dopmode );

            /* If the beginning of the volume or an elevation */
	    if( nstat == GOODBVOL ) {

                /* Get output buffer size */
	        pbsize = (4 + (NACOL*NAROW)/2) * sizeof(int);

                /* Get output buffer if not already obtained */
	        if( !obuflag ){

		    popr = RPGC_get_outbuf_by_name( "BASEVGD", pbsize, &ostat);

		    if( ostat == RPGC_NORMAL )
		        obuflag = 1;

		    else{

                        /* Set NSTAT to bad end of elevation to stop processing */
		        nstat = BENDEL;
		        obuflag = 0;

                        /* Call abort routine (no mem) */
		        if( ostat == NO_MEM ) 
			    RPGC_abort_because( PROD_MEM_SHED );

		        else 
			    RPGC_abort();
                  
		    }

	        }

                /* Initialize output buffer, and max data level */
	        if( obuflag ){

		    Init_grid( (short *) (popr + OGRID) );
		    *(popr + OMAX) = 0;
		    *(popr + OMAXI) = 0;
		    *(popr + OMAXJ) = 0;

                    /* Initialize Doppler mode */
		    *(popr + OMODE) = dopmode;

	        }

	    }

            /* Check to see if good radial...if not...skip over processing */
	    if( (nstat <= GOODTHRHI) && (obuflag) ) {

                /* Cartesian map max Doppler value on to alerting grid */
                hdr = (Base_data_header *) ipr;
                vel_data = (short *) (((char *) ipr) + hdr->vel_offset);
	        Cart_map_vel( vel_data, rssd, az, strtbin, &lastbin, 
                              (void *) (popr + OGRID), (void *) (popr + OMAX), 
                              (void *) (popr + OMAXI), (void *) (popr + OMAXJ) );

	    }

            /* Release input buffer */
	    RPGC_rel_inbuf( ipr );

            /* If the end of the elevation ...release output buffer */
	    if( (nstat == GENDEL) || (nstat == BENDEL) ){

	        if ( obuflag ){

		    RPGC_rel_outbuf( popr, FORWARD );
		    obuflag = 0;

	        }

	    }
            else {

                /* Otherwise get the next input buffer */
	        ipr = RPGC_get_inbuf_by_name( "COMBBASE", &istat );
	        continue;

	    }

            break;
            /* END DO ( while not the end of the volume scan ) */

        }
        else {

            /* Abnormal termination...ABORT */
	    RPGC_abort();

            /* Release output buffer if have one to release */
	    if( obuflag ){

	        RPGC_rel_outbuf( popr, FORWARD );
	        obuflag = 0;

	    }

            break;

        }

    }

/* End of Basvgrid_buffer_control() */
} 

/*\/////////////////////////////////////////////////////////////
//
//   Description:
//      Initialize output array and compute "constant" 
//      Cartesian mapping variables. 
//
//   Inputs:
//      mapvel - the Cartesian grid.
//
//////////////////////////////////////////////////////////////\*/
static void Init_grid( short *mapvel ){

    static int init = 0;

    /* Local variables */
    int n, nboxes;

    /* Compute loop end */
    nboxes = NACOL*NAROW;

    /* Initialize velocity array. */
    for( n = 0; n < nboxes; ++n ) 
	mapvel[n] = (short) init;

    /* Get constant Cart map parameters */
    A308c4.colfact = SCALEFAC * ((NACOL/2)+1);
    A308c4.rowfact = SCALEFAC * ((NAROW/2)+1);

/* End of Init_grid() */
} 


/*///////////////////////////////////////////////////////////////////
//   
//   Description:
//      Get the radial status, azimuth, elevation, elevation #, 
//      starting and ending radial bin numbers and Doppler mode. 
//
//   Inputs:
//      radial - pointer to radial message.
//
//   Outputs:
//      nstat - radial status.
//      az - azimuth angle, in deg.
//      elno - elevation number.
//      el - elevation angle, in deg.
//      rssd - doppler bin size, in m.
//      strtbin - starting bin (0 indexed).
//      lastbin - ending bin (0 indexed).
//      dopmode = Doppler resolution.    
//
//   Returns:
//      Always returns 0.
//
////////////////////////////////////////////////////////////////// */
static void Hdr_stuff( void *radial, int *nstat, float *az, int *elno, 
                       float *el, int *rssd, int *strtbin, int *lastbin, 
                       int *dopmode ){
 
    float rsskm;
    static float benchel;
    int n_dop_bins;

    Base_data_header *hdr = (Base_data_header *) radial;

    /* Get the azimuth angle. */
    *az = hdr->azimuth;

    /* Get the elevation angle. */
    *el = hdr->elevation;

    /* Get the radial status, the Doppler range interval and           
       the Doppler velocity resolution. */
    *nstat = hdr->status;
    *rssd = hdr->dop_bin_size;
    *dopmode = hdr->dop_resolution;

    /* Compute the size of one 1 km. */
    rsskm = ((float) *rssd) * M_TO_KM * MULT;

    /* If the elevation has changed...re-compute Cartesian 
       mapping parameters. */
    if( hdr->elevation != benchel ){

	A308c4.colel = ((rsskm * cosf(hdr->elevation * DEGTORAD)) / ABOXWDTH) * SCALEFAC;
	A308c4.rowel = ((rsskm * cosf(hdr->elevation * DEGTORAD)) / ABOXHGHT) * SCALEFAC;

	benchel = hdr->elevation;

    }

    /* Get the elevation number. */
    *elno = hdr->elev_num;

    /* Get the start and end of the Doppler radial field */
    n_dop_bins = hdr->n_dop_bins;
    if( n_dop_bins > BASEDATA_VEL_SIZE )
        n_dop_bins = BASEDATA_VEL_SIZE;

    *strtbin = hdr->dop_range - 1;
    *lastbin = *strtbin + n_dop_bins - 1;

/* End of Hdr_stuff().*/
} 


/*///////////////////////////////////////////////////////////////////////
//
//   Desscription:
//      This routine takes polar radial data and transforms it onto a 
//      Cartesian grid box array.  For each value that is mapped 
//      (ie. a maximum) a current Doppler velocity value is 
//      determined for a particular alert grid box. The value remains 
//      in the box until surpassed (in magnitude) by bin value of 
//      another Doppler velocity bin value. 
//
//   Inputs:
//
//
//////////////////////////////////////////////////////////////////////\*/
static void Cart_map_vel( short *vel, int rssd, float az, int strtbin, 
                          int *lastbin, short mapvel[][NACOL], int *maxmax, 
                          int *maxi, int *maxj ){


    /* Local variables */
    int tempmax1, i, j, n, ih, jh, nbin, enmax, stmax;
    int tempmax, xazfunc, yazfunc;
    float azrad, rsskm, stpos;

    /* Function Body */
    rsskm = ((float) rssd) * (M_TO_KM*MULT);
    azrad = az * DTR;
    ih = jh = 0;

    /* This is not the best way to do this ... it is done this
       way to match the FORTRAN version .... there is a problem
       if strtbin is something other than 0. */
    stpos = rsskm * (float) strtbin + 0.5;

    xazfunc = (int) (A308c4.colel * sinf(azrad));
    yazfunc = (int) (A308c4.rowel * cosf(azrad));

    i = A308c4.colfact + (int) ((float) xazfunc * stpos);
    j = A308c4.rowfact - (int) ((float) yazfunc * stpos);

    /* Process only the bins to achieve a range coverage of 115 km 
       since the alert category is specified to only cover 115 km. 
       The last bin to process will be the minimum of the max bin to get 
       the required range and the last good bin designed in the radial . */
    if( 460 < *lastbin )
       *lastbin = 460;
    *lastbin /= 4;

    /* Loop through velocity data ... */
    for( nbin = strtbin; nbin < *lastbin; ++nbin ){

        /* Compute start and end of string of four */
        stmax = nbin * 4;
	enmax = stmax + 3;

	tempmax = 0;

        /* Compute the grid indices ... assumes unit indexed. */
	ih = RPGC_set_mssw_to_uint( i );
	jh = RPGC_set_mssw_to_uint( j );

        /* Do from start to end of string */
	for( n = stmax; n <= enmax; ++n ){

            /* If velocity is good... */
	    if( vel[n] > RDRNGF ){

		tempmax1 = vel[n];

                /* If current vel is negative...adjust to get magnitude */
		if (tempmax1 < 129) 
		    tempmax1 = 258 - tempmax1;

                /* If the vel is greater than the current temp max...
                   re-set it */
		if( tempmax1 > tempmax ) 
		    tempmax = tempmax1;

	    }

	}

        /* Is temp-max zero...if it isn't, compare it to the box max */
	if( tempmax != 0 ){

	    if( tempmax > mapvel[jh-1][ih-1] ){

		mapvel[jh-1][ih-1] = (short) tempmax;

                 /* Compare to get maximum-maximum and its position */
		if( tempmax > *maxmax ){

		    *maxmax = tempmax;
		    *maxi = ih;
		    *maxj = jh;

		}

	    }

            /* Increment I and J counters */

	}

	i += xazfunc;
	j -= yazfunc;

    }

/* End of Cart_map_vel() */
} 

