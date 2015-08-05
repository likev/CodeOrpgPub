/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/04/25 17:19:06 $
 * $Id: vwindpro_prod.c,v 1.5 2014/04/25 17:19:06 steves Exp $
 * $Revision: 1.5 $
 * $State: Exp $
 */

#include <vwindpro.h>
#include <packet_10.h>
#include <packet_8.h>
#include <packet_4.h>

/*\///////////////////////////////////////////////////////////////////////////

   Description:
      This module calls various modules to perform the tasks 
      required to generate the VAD Product. 

   Inputs:
      hgt_ewt - Supplemental VAD heights.
      rms_ewt - Supplement VAD RMS values.
      hwd_ewt - Supplemental VAD horizontal wind direction.
      shw_ewt - Supplemental VAD horizontal wind speed.
      iptr - Input buffer (radial) pointer.

   Outputs:
      vadbuf - The vad output buffer.

   Returns:
      Always returns 0.

///////////////////////////////////////////////////////////////////////////\*/
int A31831_vad_prod( float *htg_ewt, float *rms_ewt, float *hwd_ewt, 
                     float *shw_ewt, char *iptr, char *vadbuf ){

    /* Local variables */
    int nhts, htmaxspd, i, maxspd;
    int lensad, lensab, maxdir, lenpsd, bptr, offpsd; 
    int endptr, offsab, offsad;
    float rmaxspd;

    char *sym = vadbuf + sizeof(Graphic_product); 

    /* Set offset to begin filling the product symbology data.
       Express in shorts. */
    bptr = (((int) (sym+sizeof(Symbology_block)-vadbuf))+1)/sizeof(short);

    /* Save a copy of BPTR so we can compute the length of the product 
       symbology data later. */
    offpsd = bptr;

    /* For no feedback to EWT. */
    /* A317r2_choose_height(); */

    /* Draw the background grid */
    A31833_vad_grid( &bptr, (short *) vadbuf );

    /* Label the axis of the product */
    A31834_vad_axis_lbl( &bptr, (short *) vadbuf );

    /* Plot the wind barbs or "NO DATA" message */
    A31835_vad_winds( &bptr, (short *) vadbuf );

    /* Compute the length of the product symbology data in bytes */
    lenpsd = (bptr - offpsd)*sizeof(short);

    /* Save a copy of BPTR so we can compute the length of the site 
       adaptation block later */
    offsab = bptr;

    /* Compute the offset to begin filling the adaptation data, i.e. 
       skip PHBYTES + SAOBYTES for repeating the product header and 
       for overhead for the site adaptation data block */
    offsad = offsab + 
         ((sizeof(Graphic_product) + sizeof(Tabular_alpha_block))/sizeof(short));

     /* Put the adaptable parameters in the product buffer */
    A31836_vad_adapt_data( htg_ewt, rms_ewt, hwd_ewt, shw_ewt, 
                           offsad, &bptr, (short *) vadbuf );

    /* Compute the length of the site adaptation block in bytes */
    lensab = (bptr - offsab)*sizeof(short) ;

    /* Compute the length of the site adaptation data in bytes */
    lensad = (bptr - offsad)*sizeof(short);

    /* Save a copy of BPTR to compute the total message length */
    endptr = bptr;

    /* Find the maximum wind speed for the current volume scan 
       and the corresponding wind direction and height */
    rmaxspd = MISSING;
    maxdir = MISSING;
    htmaxspd = MISSING;
    nhts = A317ve->vnhts[Cvol];
    for( i = 0; i < nhts; ++i ){

	if( A317ve->vshw[Cvol][i] > rmaxspd ){

	    rmaxspd = A317ve->vshw[Cvol][i];
	    maxdir = RPGC_NINT( A317ve->vhwd[Cvol][i] );
	    htmaxspd = RPGC_NINT( A317ve->vhtg[Cvol][i] );
	}

    }

    maxspd = RPGC_NINT( rmaxspd * MPS_TO_KTS );

    /* Fill the product header block */
    A31837_vad_header( (short *) vadbuf, maxspd, maxdir, htmaxspd, 
                       endptr, offsab, lenpsd );

    /* Put a product header block just before the site adaptation 
       data block */
    A31838_vad_adapt_hdr( (short *) vadbuf, offsab, lensab );

    /* Return to caller. */
    return 0;

} /* End of A31831_vad_prod(). */


#define RTPX_39			511
#define LEFT_PIX_39		52.0
#define BOT_PIX_39		32.0
#define NPIXAV_39		460.0
#define FULL_SCREEN_39		512.0
#define LR_SPACE_39		38.33
#define NPIXTI_39		(FULL_SCREEN_39-LEFT_PIX_39-(2.0*LR_SPACE_39))
#define OFFSET_FROM_TOP_39	10
#define MDVOL_39		11
#define SPTI			(LEFT_PIX_39+LR_SPACE_39)
#define RVO_39			(MDVOL_39-1.0)
#define PPVOL			(NPIXTI_39/RVO_39)

/*\////////////////////////////////////////////////////////////////////

   Description:
      This module generates the background GRID for the VAD 
      product. It calls the uniform value unlinked vector packet 
      module several times to set up the appropiate packets. 

   Outputs:
      bptr - offset into the vadbuf array, in shorts.
      vadbuf - The vad output buffer.

   Returns:
      Always returns 0.  

////////////////////////////////////////////////////////////////////\*/
int A31833_vad_grid( int *bptr, short *vadbuf ){


    /* Local variables */
    int c, i, vnhts, new;
    float x, y, yb, xl, xr, yt;

    /* Set new to true since we are first starting to build the vector
       packet block. */
    new = 1;

    /* Set the border color to white */
    c = 6;

    /* The following 4 vectors are the outside border.  Set x (left 
       and right) and Y (top and bottom) initally to zero to build 
       vector packets for the boarder */
    xl = 0.f;
    xr = 0.f;
    yt = 0.f;
    yb = 0.f;

    A31839_counlv_pkt( &new, xl, yt, RTPX_39, yb, c, bptr, vadbuf ) ;
    A31839_counlv_pkt( &new, RTPX_39, yt, RTPX_39, RTPX_39, c, bptr, 
                       vadbuf );
    A31839_counlv_pkt( &new, RTPX_39, RTPX_39, xr, RTPX_39, c, bptr, 
                       vadbuf );
    A31839_counlv_pkt( &new, xl, RTPX_39, xr, yb, c, bptr, vadbuf );

    /* * The next vector is the height axis */
    A31839_counlv_pkt( &new, LEFT_PIX_39, yt, LEFT_PIX_39, RTPX_39, c, 
                       bptr, vadbuf );

    /* The next vector is the time axis */
    y = RTPX_39 - BOT_PIX_39;
    A31839_counlv_pkt( &new, xl, y, RTPX_39, y, c, bptr, vadbuf );

    /* Draw height lines at intervals of GRID_INC.  Make the height 
       lines grey */
    c = 7;

    /* Set new to true since we now want a new color */
    new = 1;
    x = LEFT_PIX_39 + 1.0f;

    /* Compute the Y pixel position.  Call COUNLV_PKT to put the 
       vector in the packet block. */
    xr = RTPX_39 - 1.0f;
    vnhts = A317ve->vnhts[Cvol];
    for( i = 0; i < vnhts; ++i ){

	A318ci->yposition[vnhts-(i+1)] = ((NPIXAV_39/(vnhts + 1)) * (i+1)) +  
                                     OFFSET_FROM_TOP_39;
	y = (float) A318ci->yposition[vnhts-(i+1)];
	A31839_counlv_pkt( &new, x, y, xr, y, c, bptr, vadbuf );

    }

    /* Next... draw tic marks along the time axis.  Set new to true 
       since we now want a new color */
    new = 1;
    yb = RTPX_39 - BOT_PIX_39;

    /* Make tic marks white */
    c = 6;
    yt = yb - 10;

    /* Draw MDVOL tic marks */
    for( i = 0; i < MDVOL_39; ++i ){

	x = i*PPVOL + SPTI;

        /* Call COUNLV_PKT to include this vector in the vector 
           packet block. */
	A31839_counlv_pkt( &new, x, yt, x, yb, c, bptr, vadbuf );

    }

    return 0;

} /* End of A31833_vad_grid() */


#define MXHFLBL			4
#define HCHSZ_34		7.0

/*\////////////////////////////////////////////////////////////////////

   Description:
      This module LaBeLs the VAD product height and time AXIS. 

   Outputs:
      bptr - The length of the vad product buffer (running
             total), half_words. This is also used as the
             pointer to a half-word in the product buffer
      vadbuf - The vad product buffer.

   Returns:
      Always returns 0.

////////////////////////////////////////////////////////////////////\*/
int A31834_vad_axis_lbl( int *bptr, short *vadbuf ){

    /* Local variables */
    float x, y;
    short label[MXHFLBL];
    int i, c, ht, vnhts, dvol, pvol, nchr, tflg;
    int  ltime, t_bptr = *bptr;

    packet_8_t *p = NULL;
    char *p_chr = NULL;

    /* Label the axis (i.e. write TIME and ALT KFT)
       make the labels white. */
    c = 6; 
    for( i = 0; i < 2; ++i ){

       /* if I is 0, label the time */
	if( i == 0 ){

	    nchr = 4;
	    memcpy( (void *) label, "TIME    ", 8 );
	    y = 490.f;
	    x = 11.f;

	} 
        else {

            /* Else label the height as ALT KFT */
	    nchr = 8;
	    memcpy( (void *) &label, "ALT KFT ", 8 );
	    x = 2.f;
	    y = 2.f;

	}

        /* Assign address for packet 8. */
        p = (packet_8_t *) &vadbuf[t_bptr];

        /* Fill in packet 8 fields. */
	p->hdr.code = 8;
	p->hdr.num_bytes  = (short) (nchr + sizeof(packet_8_data_t));
	p->data.color_val = (short) c;
	p->data.pos_i = x;
	p->data.pos_j = y;

        /* Copy characters to packet. */
        p_chr = ((char *) &vadbuf[t_bptr] + sizeof(packet_8_t));
	memcpy( p_chr, label, nchr );

        /* Increment the buffer pointer. */
	t_bptr += (sizeof(packet_8_t) + nchr)/sizeof(short);

    }

    /* Set y position of where to put the time (pixels). */
    y = 490.f;

    /* Set tflg to true, since we are going to plot times now. */
    tflg = 1;

    /* Do for all volumes of available data, current first then go back 
       in time. */
    for( i = 0; i <= NVOL-1; ++i ){

        /* Determine position within vad data arrays of next volume 
           to process. */
	dvol = Cvol - i;

        /* Make sure position does not become negative. */
	if( dvol < 0 )
	    dvol += NVOL;
	
        /* Determine screen placment for this volume scan. */
	pvol = NVOL - i;

        /* Write time only if it is NOT missing */
	if( (float) A317vd->time[dvol] > MISSING ){

            /* Convert time from milliseconds past 00 GMT to hours and 
               minutes GMT. */
	    A3183a_cnvtime( A317vd->time[dvol]*1000, &ltime);

            /* Compute x-pixel position (centered), then call COCHR_PKT 
               to write time. */
	    x = SPTI + ((pvol-1)*PPVOL) - 2.0*HCHSZ_34;

            A3183b_cochr_pkt( tflg, x, y, 6, ltime, &t_bptr, vadbuf );

	}

    }

    /* Set x pixel location of where to plot the height labels. */
    x = LEFT_PIX_39 - 3.0*HCHSZ_34;

    /* Set tflg to false since we are going to plot heights now. */
    tflg = 0;

    /* Call COCHR_PKT to write the height labels. */
    vnhts = A317ve->vnhts[Cvol];
    for( i = 0; i < vnhts; ++i ){

	ht = A317ve->vhtg[Cvol][i] / 1e3f;
	y = (float) (A318ci->yposition[i] - 4);
	A3183b_cochr_pkt( tflg, x, y, 6, ht, &t_bptr, vadbuf );

    }

    /* Prepare for return to caller. */
    *bptr = t_bptr;

    return 0;
} /* End of A31834_vad_axis_lbl() */


/*\////////////////////////////////////////////////////////////////////////

   Description:
      This module takes VAD WINDS and controls the placement of the 
      wind barbs and "NO DATA" messages within the vad product grid. 
      The NO DATA message  ("ND") written if there is missing data.

   Outputs:
      bptr - The length of the vad product buffer (running
             total), half_words. This is also used as the
             pointer to a half-word in the product buffer

      vadbuf - The vad product buffer.
 
   Returns:
      Always returns 0.

////////////////////////////////////////////////////////////////////////\*/
int A31835_vad_winds( int *bptr, short *vadbuf ){

    /* Local variables */
    float x, y, mxt[NPTS], myt[NPTS];
    int i, j, k, dvol, pvol, nmt, vnhts, vnhts_1;

    /* Set the number of missing times to 0. */
    nmt = 0;

    /* Prevents complier warning. */
    y = 0;

    /* Do For All volumes of available data, current first then go back 
       in time */
    for( i = 0; i <= (NVOL-1); ++i ){

        /* Determine position within vad data arrays of next volume to process */
	dvol = Cvol - i;

        /* Make sure position does not become negative */
	if( dvol < 0 ) 
	    dvol += NVOL;
	
        /* Determine screen placment for this volume scan */
	pvol = NVOL - i;

        /* Compute x (i.e. time) position */
	x = SPTI + ((pvol-1)*PPVOL);

        /* Process vad data for all heights of data */
	vnhts = A317ve->vnhts[dvol];
	vnhts_1 = A317ve->vnhts[Cvol];
	for( j = 0; j < vnhts; ++j ){

	    if( Cvol != dvol ){

		for( k = 0; k < vnhts_1; ++k ){

		    if( A317ve->vhtg[Cvol][k] == A317ve->vhtg[dvol][j] ){

			y = (float) A318ci->yposition[k];
			break; 
		    }

		}

	    }
            else 
		y = (float) A318ci->yposition[j];
	    
            /* Plot the windbarb packet for good data point. */
	    if( A317ve->vshw[dvol][j] > MISSING )
		A3183d_wbarb_pkt( x, y, A317ve->vhwd[dvol][j],
			          A317ve->vshw[dvol][j], A317ve->vrms[dvol][j], 
                                  bptr, vadbuf );

	    else{

                /* Else means... the current data value is missing. 
                   Save the x and y coordinates and increment the number 
                   of missing. */
		mxt[nmt] = x;
		myt[nmt] = y;
		++nmt;

	    }

	}

    }

    /* If missing data is present below or in between any good data on 
       the product, put "NO DATA" labels on it. */
    if( nmt > 0 ){

	for( i = 0; i < nmt; ++i )
	    A3183e_no_data( mxt[i], myt[i], 7, bptr, vadbuf );

    }

    return 0;

} /* End of A31835_vad_winds() */


#define NUMCHAR			50
#define NUMLINES		34
#define L3END			(NUMLINES/2)

/*\///////////////////////////////////////////////////////////////////////////

   Description:
      This module takes the VAD ADAPTable DATA and puts them 
      in the product buffer in the format designed for the pup 
      alphanumeric terminal. 

   Inputs:
      htg_ewt - Height values AGL for up to NELVEWT elevation
                scans per volume scan used solely for the
                supplemental VADs which are in the Environmental
                Winds Table.
      rms_ewt - RMS values for up to NELVEWT elevation scans per
                volume scan used solely for the supplemental
                VADs which are in the Environmental Winds Table.
      hwd_ewt - Wind direction values for up to NELVEWT
                elevation scan per volume scan used solely for
                the supplemental VADs which are in the
                Environmental Winds Table.
      shw_ewt - Horizontal wind speeds for up to NELVEWT
                elevation scans per volume scan used solely for
                the supplemental VADs which are in the
                Environmental Winds Table.
      offsad - Adaptation data offset.

   Outputs:
      bptr - Pointer to VAD output buffer (running total), half_words. 
      vadbuf - The VAD output buffer.

   Returns:
      Always returns 0.

///////////////////////////////////////////////////////////////////////////\*/
int A31836_vad_adapt_data( float *htg_ewt, float *rms_ewt, float *hwd_ewt, 
                           float *shw_ewt, int offsad, int *bptr, short *vadbuf ){

    /* Local variables */
    int i, j, t_bptr, sum, beg_new_tab, extra_pages, update_page_num;
    float temp;

    static char cvad[NUMLINES][NUMCHAR]; 

    /* Temporary buffer to store product line.  Need 1 additional character
       to account for trailing '\0'. */
    static char temp_s[NUMCHAR+1];

    /* Initial the cvad buffer. */
    for( i = 0; i < NUMLINES; ++i )
        memset( &cvad[i][0], ' ', NUMCHAR ); 

    /* Set running pointer BPTR to OFFSAD */
    t_bptr = offsad;

    /* Build the internal file which will contain the adaptable parameters 
       table. Convert the adaptable parameters back to the units which 
       they were in when our copy was made of them. Our copy of the 
       adaptable parameters are in common area A317VA */
    sprintf( temp_s, "   ADAPTABLE PARAMETERS - WIND PROFILE            " );
    memcpy( &cvad[0][0], temp_s, strlen(temp_s) );

    sprintf( temp_s, "   VAD ANALYSIS SLANT RANGE       %5.1f    NMI     ", 
             A317va->vad_rng / NM_TO_M );
    memcpy( &cvad[3][0], temp_s, strlen(temp_s) );

    sprintf( temp_s, "   BEGINNING AZIMUTH ANGLE        %5.1f    DEGREE  ",
             A317va->azm_beg );
    memcpy( &cvad[5][0], temp_s, strlen(temp_s) );

    sprintf( temp_s, "   ENDING AZIMUTH ANGLE           %5.1f    DEGREE  ",
             A317va->azm_end );
    memcpy( &cvad[7][0], temp_s, strlen(temp_s) );

    sprintf( temp_s, "   NUMBER OF PASSES               %5d           ",
             A317va->fit_tests );
    memcpy( &cvad[9][0], temp_s, strlen(temp_s) );

    sprintf( temp_s, "   RMS THRESHOLD                  %5.1f    KNOTS  ",
             A317va->th_rms * MPS_TO_KTS );
    memcpy( &cvad[11][0], temp_s, strlen(temp_s) );

    sprintf( temp_s, "   SYMMETRY THRESHOLD             %5.1f    KNOTS  ",
             A317va->tsmy * MPS_TO_KTS );
    memcpy( &cvad[13][0], temp_s, strlen(temp_s) );

    sprintf( temp_s, "   DATA POINTS THRESHOLD          %5d           ",
             A317va->minpts );
    memcpy( &cvad[15][0], temp_s, strlen(temp_s) );

    sprintf( temp_s, "   ALTITUDES SELECTED                             " );
    memcpy( &cvad[21][0], temp_s, strlen(temp_s) );
   

    /* Set sum equal to number of selected VAD heights. */
    sum = A317ve->vnhts[Cvol];

    sprintf( temp_s, "        %5d  %5d  %5d  %5d  %5d  %5d  ",
             (int) RPGC_NINT(A317ve->vhtg[Cvol][0]),
             (int) RPGC_NINT(A317ve->vhtg[Cvol][1]),
             (int) RPGC_NINT(A317ve->vhtg[Cvol][2]),
             (int) RPGC_NINT(A317ve->vhtg[Cvol][3]),
             (int) RPGC_NINT(A317ve->vhtg[Cvol][4]),
             (int) RPGC_NINT(A317ve->vhtg[Cvol][5]) );
    memcpy( &cvad[22][0], temp_s, strlen(temp_s) );

    if( sum > 6 ){

        sprintf( temp_s, "        %5d  %5d  %5d  %5d  %5d  %5d  ",
                 (int) RPGC_NINT(A317ve->vhtg[Cvol][6]),
                 (int) RPGC_NINT(A317ve->vhtg[Cvol][7]),
                 (int) RPGC_NINT(A317ve->vhtg[Cvol][8]),
                 (int) RPGC_NINT(A317ve->vhtg[Cvol][9]),
                 (int) RPGC_NINT(A317ve->vhtg[Cvol][10]),
                 (int) RPGC_NINT(A317ve->vhtg[Cvol][11]) );
         memcpy( &cvad[23][0], temp_s, strlen(temp_s) );

    }

    if( sum > 12 ){

        sprintf( temp_s, "        %5d  %5d  %5d  %5d  %5d  %5d  ",
                 (int) RPGC_NINT(A317ve->vhtg[Cvol][12]),
                 (int) RPGC_NINT(A317ve->vhtg[Cvol][13]),
                 (int) RPGC_NINT(A317ve->vhtg[Cvol][14]),
                 (int) RPGC_NINT(A317ve->vhtg[Cvol][15]),
                 (int) RPGC_NINT(A317ve->vhtg[Cvol][16]),
                 (int) RPGC_NINT(A317ve->vhtg[Cvol][17]) );
        memcpy( &cvad[24][0], temp_s, strlen(temp_s) );

    }

    if( sum > 18 ){

        sprintf( temp_s, "        %5d  %5d  %5d  %5d  %5d  %5d  ",
                 (int) RPGC_NINT(A317ve->vhtg[Cvol][18]),
                 (int) RPGC_NINT(A317ve->vhtg[Cvol][19]),
                 (int) RPGC_NINT(A317ve->vhtg[Cvol][20]),
                 (int) RPGC_NINT(A317ve->vhtg[Cvol][21]),
                 (int) RPGC_NINT(A317ve->vhtg[Cvol][22]),
                 (int) RPGC_NINT(A317ve->vhtg[Cvol][23]) );
        memcpy( &cvad[25][0], temp_s, strlen(temp_s) );

    }

    if( sum > 24 ){

        sprintf( temp_s, "        %5d  %5d  %5d  %5d  %5d  %5d  ",
                 (int) RPGC_NINT(A317ve->vhtg[Cvol][24]),
                 (int) RPGC_NINT(A317ve->vhtg[Cvol][25]),
                 (int) RPGC_NINT(A317ve->vhtg[Cvol][26]),
                 (int) RPGC_NINT(A317ve->vhtg[Cvol][27]),
                 (int) RPGC_NINT(A317ve->vhtg[Cvol][28]),
                 (int) RPGC_NINT(A317ve->vhtg[Cvol][29]) );
        memcpy( &cvad[26][0], temp_s, strlen(temp_s) );

    }
    
    sprintf( temp_s, "   OPTIMUM SLANT RANGE            %5.1f          ",
             A317va->vad_rng/NM_TO_M );
    memcpy( &cvad[28][0], temp_s, strlen(temp_s) );

    /* Set the tabular block divider and number of pages. */ 
    vadbuf[t_bptr] = -1;
    t_bptr++;
    vadbuf[t_bptr] = 2;

    /* Save the location for the number of pages in the product. */
    update_page_num = t_bptr;

    /* Set the index where the tabular data will be stored. */
    t_bptr++;
    beg_new_tab = t_bptr;

    /* Call function to add more tab pages. */
    temp = A317va->vad_rng;
    A3183g_vad_winds_info( htg_ewt, rms_ewt, hwd_ewt, shw_ewt, 
                           temp, beg_new_tab, &t_bptr, vadbuf, 
                           &extra_pages );

    vadbuf[update_page_num] = (short) (extra_pages + 2);

    /* Store 1st page of adaptation data. */
    for( j = 0; j < NUMLINES/2; ++j ){

	vadbuf[t_bptr] = NUMCHAR;
	++t_bptr;

	memcpy( &vadbuf[t_bptr], &cvad[j][0], NUMCHAR );
        t_bptr += NUMCHAR/2;

    }

    /* End of table flag */
    vadbuf[t_bptr] = -1;
    ++t_bptr;

    /* Store 2nd page of adaptation data. */
    for( j = L3END; j < NUMLINES; ++j ){

	vadbuf[t_bptr] = NUMCHAR;
	++t_bptr;

	memcpy( &vadbuf[t_bptr], &cvad[j][0], NUMCHAR );
	t_bptr += NUMCHAR/2;

    }

    vadbuf[t_bptr] = -1;
    ++t_bptr;

    /* Prepare for return to caller. */
    *bptr = t_bptr;

    return 0;

} /* End of A31836_vad_adapt_data(). */


/*\////////////////////////////////////////////////////////////////////

   Descritpion:
      This module sets all the header fields in the VAD product 
      header. 

   Inputs:
      maxspd - maximum wind speed for current volume
               scan (knots)
      maxdir - wind direction corresponding to the
               maximum wind speed (degrees)
      htmaxspd - Height of maximum wind speed for current
                 volume scan (feet).
      endptr - Product buffer pointer to the end of the
               buffer. ENDPTR*2 is the buffer length in bytes.
      offsab - Adaptation block offset.
 
   Outputs:
      vadbuf - VAD product buffer.
      lenspd - Product symbology data length, bytes.

   Returns:
      Always returns 0.

////////////////////////////////////////////////////////////////////\*/
int A31837_vad_header( short *vadbuf, int maxspd, int maxdir, 
                       int htmaxspd, int endptr, int offsab, 
                       int lenpsd ){

    /* Local variables */
    int num_blocks, offset;
    Graphic_product *phd = (Graphic_product *) vadbuf;
    Symbology_block *sym =
        (Symbology_block *) (((char *) vadbuf) + sizeof(Graphic_product));

    /* Initialize product header to zeros. */
    memset( phd, 0, sizeof(Graphic_product) );

    /* Set number of blocks in product. */
    num_blocks = 4;

    /* Initialize the product header and description block. */
    RPGC_prod_desc_block( vadbuf, Hplots_id, Volno );

    /* Set color threshold table */
    phd->level_1 = Color_data.thresh[VADRMS][0];
    phd->level_2 = Color_data.thresh[VADRMS][1];
    phd->level_3 = Color_data.thresh[VADRMS][2];
    phd->level_4 = Color_data.thresh[VADRMS][3];
    phd->level_5 = Color_data.thresh[VADRMS][4];
    phd->level_6 = Color_data.thresh[VADRMS][5];
    phd->level_7 = Color_data.thresh[VADRMS][6];
    phd->level_8 = Color_data.thresh[VADRMS][7];
    phd->level_9 = Color_data.thresh[VADRMS][8];
    phd->level_10 = Color_data.thresh[VADRMS][9];
    phd->level_11 = Color_data.thresh[VADRMS][10];
    phd->level_12 = Color_data.thresh[VADRMS][11];
    phd->level_13 = Color_data.thresh[VADRMS][12];
    phd->level_14 = Color_data.thresh[VADRMS][13];
    phd->level_15 = Color_data.thresh[VADRMS][14];
    phd->level_16 = Color_data.thresh[VADRMS][15];

    /* Call A3183C_MAX_LEVELS to build the max data levels for product. */
    A3183c_max_levels( maxspd, maxdir, htmaxspd, vadbuf );

    /* Store offset to the product block */
    offset = sizeof(Graphic_product)/sizeof(short);
    RPGC_set_prod_block_offsets( vadbuf, offset, 0, offsab );

    /* Product symbology block: */
    sym->divider = -1;
    sym->block_id = 1; 
    sym->n_layers = 1;
    sym->layer_divider = -1;

    /* Store product symbology data layer length in bytes */
    RPGC_set_product_int( &sym->data_len, lenpsd);

    /* Store length of the product symbology block in bytes.  Length 
       includes block data, divider, code and length fields (i.e., 
       sizeof(Symbology_block). */
    lenpsd += sizeof(Symbology_block);
    RPGC_set_product_int( &sym->block_len, lenpsd );

    /* Subtract the size of the product header and description block and 
       convert to bytes. */
    endptr *= sizeof(short);
    endptr -= sizeof(Graphic_product);

    /* Store total length of the message */
    RPGC_prod_hdr( vadbuf, Hplots_id, &endptr );

    return 0;

} /* End of A31837_vad_header(). */


/*\////////////////////////////////////////////////////////////////////////

   Description:
      This module appends COlored UNLinked Vector PacKeTs to the 
      product buffer. 

   Inputs:
      new - variable used to indicate that the current entry into the 
            packet filling subroutine is the first time.

      xl - Left end of a vector, pixels.
      xr - Right end of a vector, pixels.
      yb - The bottom pixel row number of a vector.
      yt - The top pixel row number of a vector.
      c - The color number put in packets.

   Outputs:
      bptr - The length of the vad product buffer (running total), 
             half_words. This is also used as the pointer to a 
             half-word in the product buffer.
      vadbuf - product buffer. 

   Returns:

////////////////////////////////////////////////////////////////////////\*/
int A31839_counlv_pkt( int *new, float xl, float yt, float xr, float yb, 
                       int c, int *bptr, short *vadbuf ){

    /* Local data. */
    int t_bptr = *bptr;

    /* Initialized data */
    static packet_10_hdr_t *p_hdr = NULL;
    static packet_10_data_t *p_data = NULL;
    static int nbytp = 0;

    /* If new = true, then this is the first time we are appending a 
       vector packet to the product buffer. So set up the packet id,
       and color and save the position of the length of the remainder 
       of the vector packet block for future updates */
    if( nbytp == 0 )
	*new = 1;
    
    /* Set new to false for next entry */
    if( *new ){

	*new = FALSE;

        /* Assign address to packet header. */
        p_hdr = (packet_10_hdr_t *) &vadbuf[t_bptr];

        /* Store packed ID */
	p_hdr->code = 10;

        /* Length of the remainder of the vector packet block. */
	p_hdr->num_bytes = 10;

        /* Vector color */
	p_hdr->val = (short) c;

        /* Increment the length of the product buffer by 3 half-words. */
        t_bptr += (sizeof(packet_10_hdr_t)/sizeof(short));

        /* Increment nbytp to indicate not first time anymore. */
        nbytp += sizeof(packet_10_hdr_t);

    } 
    else{

         /* Else means .. this is not the first vector, so update length 
            of the packet block by size of packet_10_data_t bytes. */
	p_hdr->num_bytes += sizeof(packet_10_data_t);

    }

    /* Assign address to packet data. */
    p_data = (packet_10_data_t *) &vadbuf[t_bptr];

    /* Left end x pixel position */
    p_data->beg_i = (short) RPGC_NINT(xl);

    /* Top end y pixel position */
    p_data->beg_j = (short) RPGC_NINT(yt);

    /* Right end x pixel position */
    p_data->end_i = (short) RPGC_NINT(xr);

    /* Bottom end y pixel position */
    p_data->end_j = (short) RPGC_NINT(yb);

    /* Increment length of the product buffer. */
    t_bptr += (sizeof(packet_10_data_t)/sizeof(short));

    /* Prepare to return to caller. */
    *bptr = t_bptr;

    return 0;

} /* End of A31839_counlv_pkt() */


/*\/////////////////////////////////////////////////////////////////////

   Description:
      This module fills the VAD ADAPTable parameters HeaDeR block. 
      It copies the main product header block to a location 
      just before the algorithm adaptation data, and changes a few 
      variables to reflect the differences between the vad product 
      symbology block and the adaptation block. 

   Inputs:
      vadbuf - output buffer
      bptr - pointer to start of tabular block.
      lensad - adaptation data block length, in bytes.

   Returns:
      Always returns 0.

/////////////////////////////////////////////////////////////////////\*/
int A31838_vad_adapt_hdr( short *vadbuf, int bptr, int lensab ){

    /* Local variables */
    Tabular_alpha_block *tab = (Tabular_alpha_block *) &vadbuf[bptr];
    Graphic_product *phd = 
       (Graphic_product *) ((char *) tab + sizeof(Tabular_alpha_block));

    /* Block divider. */
    tab->divider = -1;

    /* Block id */
    tab->block_id = 3;

    /* Length of the site adaptation block */
    RPGC_set_product_int( &tab->block_len, lensab );

    /* Copy the first product header block to here */
    memcpy( phd, vadbuf, sizeof(Graphic_product) );

    /* Change values which now have new meanings. */

    /* Product id of alpha-numeric adaptation data product */
    phd->msg_code = 100;
    phd->msg_len = 0;

    /* Number of blocks and product code */
    phd->n_blocks = 3;
    phd->prod_code = 100;

    /* Length of the site adaption block, subtract overhead of adaptation 
       block overhead in front of the repeated product header block */
    RPGC_set_product_int( &phd->msg_len, (lensab - sizeof(Tabular_alpha_block)) );

    /* Set the tabular offset to 0. */
    RPGC_set_product_int( &phd->tab_off, 0 );

    return 0;

} /* End A31838_vad_adapt_hdr(). */


/*\/////////////////////////////////////////////////////////////

   Description:
       This module CoNVerts TIME expressed in milliseconds 
       past 0000 GMT to time in hours and minutes past 00 GMT 

   Inputs:
      msctime - time in milliseconds past midnight

   Outputs:
      hmtime - hours and minutes past midnight

/////////////////////////////////////////////////////////////\*/
int A3183a_cnvtime( int msctime, int *hmtime ){

    int hr, min, seconds;
 
    /* Divide by 1000 to convert from milliseconds to seconds */
    seconds = msctime / 1e3f + .5f;

    /* Compute the number of hours */
    hr = seconds / 3600;

    /* Compute the remaining number of minutes */
    min = (seconds - hr * 3600) / 60;

    /* Compute the time in hours and minutes */
    *hmtime = hr * 100 + min;

    return 0;

} /* End A3183a_cnvtime() */

#define MAXBYTLBL		4


/*\//////////////////////////////////////////////////////////////////

   Description:
      This module appends the product buffer with COlored 
      CHaRacter PacKeTs 

   Inputs:
      tflg - variable used to denote the "value" to be plotted is 
             a time
      x - A pixel column number.
      y - A pixel row number.
      c - The color number put in packets.
      value - The value of time or height to plot as an axix
              label.

   Outputs:
      bptr - The length of the vad product buffer (running
             total), half_words. This is also used as the
             pointer to a half-word in the product buffer

      vadbuf - The vad product buffer.

//////////////////////////////////////////////////////////////////\*/
int A3183b_cochr_pkt( int tflg, float x, float y, int c, int value, 
                      int *bptr, short *vadbuf ){

    int i, nchr;
    char chr[12], *p_chr;

    int t_bptr = *bptr;
    packet_8_t *p = NULL;

    /* Initialize the string elements to '\0' */
    memset( chr, 0, 12 );

    /* Convert integer value to left justified character string */
    sprintf( chr, "%-d", value );

    /* Store the number of characters in string for return */
    nchr = strlen(chr);

    /* tflg=true means.... the value is a time */
    if( tflg ) {

        /* If less then 4 characters, then shift characters down and put 
           leading zeros before the time to make it have 4 characters */
	if( nchr < MAXBYTLBL ){

	    for( i = 0; i < MAXBYTLBL; ++i ){

		if( (nchr-i) >= 1 )
		    chr[MAXBYTLBL-i-1] = chr[nchr-i-1];

		else 
		    chr[MAXBYTLBL-i-1] = '0';

	    }

            /* Set NCHR to 4, since we have lengthened it */
	    nchr = MAXBYTLBL;
	}

    } 
    else{

        /* If the number of characters is odd, then shift characters down 1 
           byte and put a blank before the first number to make it an even 
           number of bytes. */
	if( (nchr / 2 << 1) != nchr ){

	    for( i = nchr; i >= 1; --i )
		chr[i] = chr[i-1];

	    ++nchr;
	    chr[0] = ' ';

	}

    }

    /* Get address of packet 8. */
    p = (packet_8_t *) &vadbuf[t_bptr];
     
    /* Packet id */
    p->hdr.code = 8;

    /* Length of remainder of write text packet block (not including this 
       half word) in bytes */
    p->hdr.num_bytes = (short) (nchr + sizeof(packet_8_data_t));

    /* Color of the text */
    p->data.color_val = (short) c;

    /* x position */
    p->data.pos_i = (short) RPGC_NINT(x);

    /* y position */
    p->data.pos_j = (short) RPGC_NINT(y);

    /* Put the characters in the product buffer */
    p_chr = (char *) &vadbuf[t_bptr] + sizeof(packet_8_t);
    memcpy( p_chr, chr, nchr );

    /* Increment the length of the product buffer. */
    t_bptr += (sizeof(packet_8_t) + nchr)/sizeof(short);

    /* Prepare for return to caller. */
    *bptr = t_bptr;

    return 0;

} /* End of A3183b_cochr_pkt() */


/*\/////////////////////////////////////////////////////////////////

   Description:
      This modules fills the maximum data level values for the 
      product header.

   Inputs:
      maxspd - Maximum wind speed, in kts.
      maxdir - Maximum wind direction, in deg.
      htmaxpd - Height of the maximum wind speed, in tens of feet.

   Outputs:
      vadbuf - The vad product buffer.

   Returns:
      Always returns 0.

/////////////////////////////////////////////////////////////////\*/
int A3183c_max_levels( int maxspd, int maxdir, int htmaxspd, 
                       short *vadbuf ){

    Graphic_product *phd = (Graphic_product *) vadbuf;

    /* Set the max velocity level to the max wind speed value. 
       Set the max direction level to the max wind direction value.   
       Set the max height level to the max wind speed height value      
       (in tens of feet ). */
    if( (float) maxspd <= MISSING ){

	phd->param_4 = -1;
	phd->param_5 = -1;
	phd->param_6 = -9999;

    } 
    else{

	phd->param_4 = (short) maxspd;
	phd->param_5 = (short) maxdir;
	phd->param_6 = (short) RPGC_NINT( htmaxspd /10.0 );

    }

    return 0;

} /* End of A3183c_max_levels() */


/*\//////////////////////////////////////////////////////////////////////

   Description:
      This module appends Wind BARB PacKeTs to the product buffer 

   Inputs:
      x - x pixel location.
      y - y pixel location.
      wd - Wind direction, in deg.
      ws - Wind speed, in knots.
      rmsl - RMS value.

   Outputs:
      bptr - The length of the vad product buffer (running
             total), half_words. This is also used as the
             pointer to a half-word in the product buffer
      vadbuf - The vad product buffer.

   Returns:
      Always returns 0.

//////////////////////////////////////////////////////////////////////\*/
int A3183d_wbarb_pkt( float x, float y, float wd, float ws,
	              float rmsl, int *bptr, short *vadbuf ){

    /* Local variables */
    float rms_kt;
    int c, intkts;
    packet_4_data_t *p = NULL;

    int t_bptr = *bptr;

    /* Set packet address. */
    p = (packet_4_data_t *) &vadbuf[t_bptr];

    /* Set up wind bard packet ID. */
    p->code = 4;

     /* Length of remainder of wind barb packet block (not including this 
        half word) in bytes */
    p->num_bytes = 10;

    /* Convert rms to knots */
    rms_kt = rmsl * MPS_TO_KTS;
    intkts = (int) rms_kt + 1;

    /* Determine wind barb color */
    c = Color_data.coldat[VADRMS][intkts];

    /* Set up wind barb color */
    p->color_val = (short) c;

    /* x position in pixels */
    p->pos_i = (short) RPGC_NINT(x);

    /* y position in pixels */
    p->pos_j = (short) RPGC_NINT(y);

    /* Wind direction degrees clockwise from north */
    p->wind_dir = (short) RPGC_NINT(wd);
    if( p->wind_dir == 360 )
        p->wind_dir = 0;

    /* Wind speed */
    p->wind_spd = (short) RPGC_NINT( ws * MPS_TO_KTS );

    /* Increment length of product buffer. */
    *bptr = t_bptr + sizeof(packet_4_data_t)/sizeof(short);

    return 0;

} /* End of A3183d_wbarb_pkt() */


/*\////////////////////////////////////////////////////////////////////

   Description:
      This module appends write text (uniform value) packets 
      to the product buffer. The text message is "ND" 

   Inputs:
      x - A pixel column number.
      y - A pixel row number.
      c - Horizontal character size, in pixels.

   Outputs:
      bptr - The length of the vad product buffer (running
             total), half_words. This is also used as the
             pointer to a half-word in the product buffer
      vadbuf - the vad product buffer.

   Returns:
      Always returns 0.

////////////////////////////////////////////////////////////////////\*/
int A3183e_no_data( float x, float y, int c, int *bptr, short *vadbuf ){

    int t_bptr = *bptr;
    char *p_chr = NULL; 
    packet_8_t *p = (packet_8_t *) &vadbuf[t_bptr];

    /* Packet code. */
    p->hdr.code = 8;

    /* There are 8 more bytes after this packet position */
    p->hdr.num_bytes = 8;

    /* Text color */
    p->data.color_val = (short) c;

    /* X and Y position. Adjust the coordinates so that the message is 
       centered on the time-height intersection */
    p->data.pos_i = (short) RPGC_NINT(x - 7.0);
    p->data.pos_j = (short) RPGC_NINT(y - 4.5);

    /* Put "NO DATA" in the packet (i.e. buffer) */
    p_chr = (char *) p + sizeof(packet_8_t);
    memcpy( p_chr, "ND", 2 );

    /* Prepare to return to caller. */
    *bptr = t_bptr + (sizeof(packet_8_t)+strlen("ND"))/sizeof(short);

    return 0;

} /* End of A3183e_no_data(). */


#define MAXDATALINES			14
#define HEADLINES			3
#define NA_FLAG				-999.0

/*\/////////////////////////////////////////////////////////////////////////

   Description:
      This module places the VAD algorithm output into the tabular 
      alphanumeric block of the VWP product, just before the VAD 
      adaptation data.  The data consists of wind related algorith 
      output from up to 30 levels from the VADTMHGT buffer, up to 20 
      levels from the VAD supplemental levels and up to 2 "extra" 
      levels.  The arrays provided this function contain the data 
      sorted from lowest level to highest. 

   Inputs:
      beg - Pointer into VAD output buffer where
            algorithm output ifrom this function is
            to begin.
      hts - Array of height data in feet.
      u - Array of u-component wind speed values in meters per second.
      v - Array of v-component wind speed values in meters per second.
      hwd - Array of horizontal wind directions in degrees.
      hws - Array of horizontal wind speeds in meters per second.
      rms - Array of root mean square deviations in velocities in meters 
            per second.
      vms - Array of vertical wind speeds in meters per second. May 
            contain a flag indicating data not available.
      div - Array of divergence values in 1/hour.  May contain a flag 
            indicating data not available.
      slr - Array of slant ranges in kilometers.
      eidx - Array of elevation indices.
      n - Number of elements in input arrays.
      i4date - Nexrad Julian Date.
      i4time - Volume time in seconds from midnight.

   Outputs:
      end - Pointer to VAD output buffer (running total), halfwords. 
            This is also used as the pointer to a halfword in the 
            product buffer.
      vadbuf - The VAD product output buffer
      pages - Number of pages needed to display the data.

   Returns:
      Always returns 0.

/////////////////////////////////////////////////////////////////////////\*/
int A3183f_vad_alpha_winds( int beg, int *end, short *vadbuf, int i4date, 
                            int i4time, float *hts, float *u, float *v, 
                            float *hwd, float *hws, float *rms, float *vws, 
                            float *div, float *slr, int *eidx, short n, 
                            int *pages ){

    /* Local variables */
    int elev_int, d_time[2], vcpnum, volnum;
    int s1, year, month, day, t_bptr;
    short i2date, x, datalines, hl, pg, ln;
    float elev;
    char chdr[HEADLINES][81];
 
    /* Temporary buffer to hold product line.  Need addition character
       to account for trailing '\0'. */
    char temp_c[81];
    char cvad[81];

    /* Get the volume scan number and VCP.  Used to get elevation angle. */
    volnum = RPGC_get_buffer_vol_num( vadbuf );
    vcpnum = Summary->vcp_number;

    /* Blank all characters of cvad and chdr. */
    memset( cvad, ' ', 80 );
    for( hl = 0; hl < HEADLINES; hl++ )
       memset( &chdr[hl][0], ' ', 80 );

    /* Format the date and time for display and insert into header */
    i2date = (short) i4date;
    RPGCS_julian_to_date( i2date, &year, &month, &day );
    d_time[0] = i4time / 3600;
    d_time[1] = (i4time - d_time[0] * 3600) / 60;

    /* We need to subtract off the century from the year before
       the year is output. */
    if( year >= 2000 )
       year -= 2000;

    else
        year -= 1900;

    /* Write the header lines into the character buffer */
    sprintf( temp_c, 
       "                    VAD Algorithm Output  %2.2d/%2.2d/%2.2d  %2.2d:%2.2d",
       month, day, year, d_time[0], d_time[1] );
    memcpy( &chdr[0][0], temp_c, strlen(temp_c) );

    sprintf( temp_c, 
       "    ALT      U       V       W    DIR   SPD   RMS     DIV     SRNG    ELEV " );
    memcpy( &chdr[1][0], temp_c, strlen(temp_c) );

    sprintf( temp_c,
       "   100ft    m/s     m/s    cm/s   deg   kts   kts    E-3/s     nm      deg" );
    memcpy( &chdr[2][0], temp_c, strlen(temp_c) );

    /* Set running pointer to BEG */
    t_bptr = beg;

    /* Initialize the array index */
    x = 0;

    /* Determine the number of pages needed to display the data */
    *pages = (n - 1) / MAXDATALINES + 1;

    /* Loop for all pages. */
    s1 = (short) (*pages);
    for( pg = 0; pg < s1; ++pg ){

        /* Determine the number of data lines to output this page. */
	datalines = n - x;
	if( datalines > MAXDATALINES ) 
	    datalines = MAXDATALINES ;

        /* Don't write any header lines if there are no data lines. */
	if( datalines > 0 ){

	    for( hl = 0; hl < HEADLINES; ++hl ){

		vadbuf[t_bptr] = 80;
		++t_bptr;

		memcpy( &vadbuf[t_bptr], &chdr[hl][0], 80 );

                /* t_bptr is an index to short array. */
		t_bptr += 40;

	    }

	}

        /* Loop for all data lines. */
	for( ln = 0; ln < datalines; ++ln ){

	    vadbuf[t_bptr] = 80;
	    ++t_bptr;

            /* Convert the elevation index into the elevation angle */
	    elev_int = RPGCS_get_target_elev_ang( vcpnum, eidx[x] );
	    elev = (float) elev_int / 10.f;

            /* It is possible that the VWS (vertical wind speed) and the 
               DIV (divergence) fields may be not applicable.  This code 
               assumes that if one is NA the other is NA.  Place the wind 
               information into the buffer, converting to proper output units. */
	    if( (vws[x] == NA_FLAG) || (div[x] == NA_FLAG) ){

		sprintf( cvad, 
                   "    %3.3d  %6.1f  %6.1f     NA    %3.3d   %3.3d  %4.1f      NA    %6.2f   %4.1f", 
                   (int) RPGC_NINT(hts[x]/100.0), u[x], v[x], (int) RPGC_NINT(hwd[x]),
                   (int) RPGC_NINT(hws[x]*MPS_TO_KTS), rms[x]*MPS_TO_KTS, slr[x]*KM_TO_NM, 
                   elev );
                
                cvad[strlen(cvad)] = ' ';

	    } 
            else{

		sprintf( cvad, 
                   "    %3.3d  %6.1f  %6.1f  %6.1f   %3.3d   %3.3d  %4.1f  %8.4f  %6.2f   %4.1f", 
                   (int) RPGC_NINT(hts[x]/100.0), u[x], v[x], vws[x]*100.0, 
                   (int) RPGC_NINT(hwd[x]), (int) RPGC_NINT(hws[x]*MPS_TO_KTS), 
                   rms[x]*MPS_TO_KTS, div[x]*1000.0,
                   slr[x]*KM_TO_NM, elev );

                cvad[strlen(cvad)] = ' ';
                
	    }

	    memcpy( &vadbuf[t_bptr], cvad, 80 );

            /* t_bptr is an index to short array. */
	    t_bptr += 40;

            /* Increment the array index */
	    x++;

	}

        /* Place a page divider into the buffer */
	vadbuf[t_bptr] = -1;
	++t_bptr;

    }

   /* Reset the buffer pointer. */
   *end = t_bptr;

    return 0;

} /* End of A3183f_vad_alpha_winds(). */

#define MAX_DATA_LINES			(MAX_VAD_HTS+MAX_VAD_ELVS+NELVEWT)

/*\/////////////////////////////////////////////////////////////////////////

   Description:
      This module gets wind information at different heights. It 
      then calls A3183F__VAD_ALPHA_WINDS() to add more TAB pages to 
      HPLOTS product. 

   Inputs:
      htg_ewt - Height for the supplemental VADs, in meter.
      rms_ewt - The rms values for supplemental VADs, in m/s.
      hwd_ewt - Horizontal wind direction for supplemental
                VADs, in degree.
      shw_ewt - Horizontal wind speed for the supplemental
                VADs, in m/s.
      vad_slrng - Constant slant range for elevation scan, in meters.
      beg - Pointer to VAD outpur buffer where the
           algorithm output is to begin.

   Outputs:
      end - Pointer to VAD output buffer(running total),
            halfword.
      bufptr - VAD output buffer
      extra_pages - Number of TAB pages being added.

   Returns:
      Always returns 0.

/////////////////////////////////////////////////////////////////////////\*/
int A3183g_vad_winds_info( float *htg_ewt, float *rms_ewt, float *hwd_ewt, 
                           float *shw_ewt, float vad_slrng, int beg, int *end, 
                           short *bufptr, int *extra_pages ){

    /* Local variables */
    int i, j, k, n;
    int lines, index[MAX_DATA_LINES];
    float alt[MAX_DATA_LINES], vws[MAX_DATA_LINES], dive[MAX_DATA_LINES];
    float wdir[MAX_DATA_LINES], uspd[MAX_DATA_LINES], vspd[MAX_DATA_LINES]; 
    float wspd[MAX_DATA_LINES], wrms[MAX_DATA_LINES], slrng[MAX_DATA_LINES];
    double alpha, wnddir;

    j = 0;
    k = 0;
    n = 0;
    lines = 0;

    /* Loop to get wind info based on the order of the height which
       starts from lowest to highest.  The raw data will be collected
       from three source tables. */
    for( i = 0; i < MAX_DATA_LINES; ++i ){

        /* All three source tables are available. */
	if( (n < NELVEWT) && (k < MAX_VAD_ELVS) && (j < MAX_VAD_HTS) ){

            /* VHTG is the lowest height in  three tables. */
	    if( (A317ve->vhtg[Cvol][j] < (A317vd->htg[k]*M_TO_FT+Siteadp.rda_elev)) 
                                      && 
                (A317ve->vhtg[Cvol][j] < (htg_ewt[n]*M_TO_FT+Siteadp.rda_elev)) ){

		if( A317ve->vshw[Cvol][j] != MISSING ){

		    alt[lines] = A317ve->vhtg[Cvol][j];
		    wnddir = wdir[lines] = A317ve->vhwd[Cvol][j];
		    wspd[lines] = A317ve->vshw[Cvol][j];
		    wrms[lines] = A317ve->vrms[Cvol][j];
		    vws[lines] = -999.f;
		    dive[lines] = -999.f;
		    slrng[lines] = A317ve->slrn[j];
		    index[lines] = A317ve->elcn[j];
		    alpha = (wnddir - 180.0) * DEGTORAD;
		    uspd[lines] = wspd[lines] * sin(alpha);
		    vspd[lines] = wspd[lines] * cos(alpha);
		    lines++;

		}

		j++;

            /* HTG is the lowest height in three tables. */
	    } 
            else if( ((A317vd->htg[k]*M_TO_FT+Siteadp.rda_elev) < A317ve->vhtg[Cvol][j])
                                           && 
		     (A317vd->htg[k] < htg_ewt[n]) ){

		if( A317vd->shw[k] != MISSING ){

		    alt[lines] = A317vd->htg[k]*M_TO_FT + Siteadp.rda_elev;
		    wnddir = wdir[lines] = A317vd->hwd[k];
		    wspd[lines] = A317vd->shw[k];
		    wrms[lines] = A317vd->rms[k];
		    vws[lines] = A317vd->svw[k];
		    dive[lines] = A317vd->div[k];
		    slrng[lines] = vad_slrng * .001f;
		    index[lines] = Rpg_elev_ind[k];
		    alpha = (wnddir - 180.0) * DEGTORAD;
		    uspd[lines] = wspd[lines] * sin(alpha);
		    vspd[lines] = wspd[lines] * cos(alpha);
		    lines++;

		}

		k++;
          
            /* HTG_EWT is the lowest height in three tables. */
	    } 
            else if( ((htg_ewt[n]*M_TO_FT+Siteadp.rda_elev) < A317ve->vhtg[Cvol][j])
                                        && 
                     (htg_ewt[n] < A317vd->htg[k]) ){

		if( shw_ewt[n] != MISSING ){

		    alt[lines] = htg_ewt[n]*M_TO_FT + Siteadp.rda_elev;
		    wnddir = wdir[lines] = hwd_ewt[n];
		    wspd[lines] = shw_ewt[n];
		    wrms[lines] = rms_ewt[n];
		    vws[lines] = -999.f;
		    dive[lines] = -999.f;
		    slrng[lines] = A317ec->vad_rng_ewt * .001f;
		    index[lines] = Rpg_elev_ind[n];
		    alpha = (wnddir - 180.0) * DEGTORAD;
		    uspd[lines] = wspd[lines] * sin(alpha);
		    vspd[lines] = wspd[lines] * cos(alpha);
		    lines++;

		}

		n++;

            /* VHTG = HTG and both are the lowest heights. */
	    } 
            else if( (A317ve->vhtg[Cvol][j] == A317vd->htg[k]*M_TO_FT + Siteadp.rda_elev)
                                           && 
		     (A317vd->htg[k] < htg_ewt[n]) ){

		if( A317ve->vshw[Cvol][j] != MISSING ){

		    alt[lines] = A317ve->vhtg[Cvol][j];
		    wnddir = wdir[lines] = A317ve->vhwd[Cvol][j];
		    wspd[lines] = A317ve->vshw[Cvol][j];
		    wrms[lines] = A317ve->vrms[Cvol][j];
		    vws[lines] = -999.f;
		    dive[lines] = -999.f;
		    slrng[lines] = A317ve->slrn[j];
		    index[lines] = A317ve->elcn[j];
		    alpha = (wnddir - 180.0) * DEGTORAD;
		    uspd[lines] = wspd[lines] * sin(alpha);
		    vspd[lines] = wspd[lines] * cos(alpha);
		    lines++;

		}

		if( A317vd->shw[k] != MISSING ){

		    alt[lines] = A317vd->htg[k] * M_TO_FT + Siteadp.rda_elev;
		    wnddir = wdir[lines] = A317vd->hwd[k];
		    wspd[lines] = A317vd->shw[k];
		    wrms[lines] = A317vd->rms[k];
		    vws[lines] = A317vd->svw[k];
		    dive[lines] = A317vd->div[k];
		    slrng[lines] = vad_slrng * .001f;
		    index[lines] = Rpg_elev_ind[k];
		    alpha = (wnddir - 180.0) * DEGTORAD;
		    uspd[lines] = wspd[lines] * sin(alpha);
		    vspd[lines] = wspd[lines] * cos(alpha);
		    lines++;

		}

		j++;
		k++;

               /* VHTG = HTG_EWT and both are the lowest heights. */
	    } 
            else if( (A317ve->vhtg[Cvol][j] == (htg_ewt[n]*M_TO_FT+Siteadp.rda_elev))
                                           && 
		     (A317ve->vhtg[Cvol][j] < (A317vd->htg[k]*M_TO_FT+Siteadp.rda_elev)) ){

		if( A317ve->vshw[Cvol][j] != MISSING ){

		    alt[lines] = A317ve->vhtg[Cvol][j];
		    wnddir = wdir[lines] = A317ve->vhwd[Cvol][j];
		    wspd[lines] = A317ve->vshw[Cvol][j];
		    wrms[lines] = A317ve->vrms[Cvol][j];
		    vws[lines] = -999.f;
		    dive[lines] = -999.f;
		    slrng[lines] = A317ve->slrn[j];
		    index[lines] = A317ve->elcn[j];
		    alpha = (wnddir - 180.0) * DEGTORAD;
		    uspd[lines] = wspd[lines] * sin(alpha);
		    vspd[lines] = wspd[lines] * cos(alpha);
		    lines++;

		}

		if( shw_ewt[n] != MISSING ){

		    alt[lines] = htg_ewt[n] * M_TO_FT + Siteadp.rda_elev;
		    wnddir = wdir[lines] = hwd_ewt[n];
		    wspd[lines] = shw_ewt[n];
		    wrms[lines] = rms_ewt[n];
		    vws[lines] = -999.f;
		    dive[lines] = -999.f;
		    slrng[lines] = A317ec->vad_rng_ewt * .001f;
		    index[lines] = Rpg_elev_ind[n];
		    alpha = (wnddir - 180.0) * DEGTORAD;
		    uspd[lines] = wspd[lines] * sin(alpha);
		    vspd[lines] = wspd[lines] * cos(alpha);
		    lines++;
		}

		j++;
		n++;

            /* HTG_EWT = HTG and both are the lowest heights. */
	    } 
            else if( (htg_ewt[n] == A317vd->htg[k]) 
                                 && 
		     ((A317vd->htg[k]*M_TO_FT+Siteadp.rda_elev) < A317ve->vhtg[Cvol][j]) ){

		if( A317vd->shw[k] != MISSING ){

		    alt[lines] = A317vd->htg[k]*M_TO_FT + Siteadp.rda_elev;
		    wnddir = wdir[lines] = A317vd->hwd[k];
		    wspd[lines] = A317vd->shw[k];
		    wrms[lines] = A317vd->rms[k];
		    vws[lines] = A317vd->svw[k];
		    dive[lines] = A317vd->div[k];
		    slrng[lines] = vad_slrng * .001f;
		    index[lines] = Rpg_elev_ind[k];
		    alpha = (wnddir - 180.0) * DEGTORAD;
		    uspd[lines] = wspd[lines] * sin(alpha);
		    vspd[lines] = wspd[lines] * cos(alpha);
		    lines++;

		}

		if( shw_ewt[n] != MISSING ){

		    alt[lines] = htg_ewt[n]*M_TO_FT + Siteadp.rda_elev;
		    wnddir = wdir[lines] = hwd_ewt[n];
		    wspd[lines] = shw_ewt[n];
		    wrms[lines] = rms_ewt[n];
		    vws[lines] = -999.f;
		    dive[lines] = -999.f;
		    slrng[lines] = A317ec->vad_rng_ewt * .001f;
		    index[lines] = Rpg_elev_ind[n];
		    alpha = (wnddir - 180.0) * DEGTORAD;
		    uspd[lines] = wspd[lines] * sin(alpha);
		    vspd[lines] = wspd[lines] * cos(alpha);
		    lines++;

		}

		n++;
		k++;

            /* Three heights are equal. */
	    } 
            else if( (A317ve->vhtg[Cvol][j] == ((A317vd->htg[k]*M_TO_FT + Siteadp.rda_elev)))
                                           && 
		     (A317vd->htg[k] == htg_ewt[n]) ){

		if( A317ve->vshw[Cvol][j] != MISSING ){

		    alt[lines] = A317ve->vhtg[Cvol][j];
		    wnddir = wdir[lines] = A317ve->vhwd[Cvol][j];
		    wspd[lines] = A317ve->vshw[Cvol][j];
		    wrms[lines] = A317ve->vrms[Cvol][j];
		    vws[lines] = -999.f;
		    dive[lines] = -999.f;
		    slrng[lines] = A317ve->slrn[j];
		    index[lines] = A317ve->elcn[j];
		    alpha = (wnddir - 180.0) * DEGTORAD;
		    uspd[lines] = wspd[lines] * sin(alpha);
		    vspd[lines] = wspd[lines] * cos(alpha);
		    lines++;

		}

		if( A317vd->shw[k] != MISSING ){

		    alt[lines] = A317vd->htg[k]*M_TO_FT + Siteadp.rda_elev;
		    wnddir = wdir[lines] = A317vd->hwd[k];
		    wspd[lines] = A317vd->shw[k];
		    wrms[lines] = A317vd->rms[k];
		    vws[lines] = A317vd->svw[k];
		    dive[lines] = A317vd->div[k];
		    slrng[lines] = vad_slrng * .001f;
		    index[lines] = Rpg_elev_ind[k];
		    alpha = (wnddir - 180.0) * DEGTORAD;
		    uspd[lines] = wspd[lines] * sin(alpha);
		    vspd[lines] = wspd[lines] * cos(alpha);
		    lines++;

		}

		if( shw_ewt[n] != MISSING ){

		    alt[lines] = htg_ewt[n]*M_TO_FT + Siteadp.rda_elev;
		    wnddir = wdir[lines] = hwd_ewt[n];
		    wspd[lines] = shw_ewt[n];
		    wrms[lines] = rms_ewt[n];
		    vws[lines] = -999.f;
		    dive[lines] = -999.f;
		    slrng[lines] = A317ec->vad_rng_ewt * .001f;
		    index[lines] = Rpg_elev_ind[n];
		    alpha = (wnddir - 180.0) * DEGTORAD;
		    uspd[lines] = wspd[lines] * sin(alpha);
		    vspd[lines] = wspd[lines] * cos(alpha);
		    lines++;

		}

		j++;
		k++;
		n++;

	    }

        /* Only HTS and ELEV tables are available.  All the elements in EWT
           table have been selected. */
	} 
        else if( (n >= NELVEWT) && (k < MAX_VAD_ELVS) && (j < MAX_VAD_HTS) ){

            /* HTG is the lowest height in two tables. */
	    if( ((A317vd->htg[k]*M_TO_FT + Siteadp.rda_elev)) < A317ve->vhtg[Cvol][j]) {

		if( A317vd->shw[k] != MISSING ){

		    alt[lines] = A317vd->htg[k]*M_TO_FT + Siteadp.rda_elev;
		    wnddir = wdir[lines] = A317vd->hwd[k];
		    wspd[lines] = A317vd->shw[k];
		    wrms[lines] = A317vd->rms[k];
		    vws[lines] = A317vd->svw[k];
		    dive[lines] = A317vd->div[k];
		    slrng[lines] = vad_slrng * .001f;
		    index[lines] = Rpg_elev_ind[k];
		    alpha = (wnddir - 180.0) * DEGTORAD;
		    uspd[lines] = wspd[lines] * sin(alpha);
		    vspd[lines] = wspd[lines] * cos(alpha);
		    lines++;
		}

		k++;

            /* VHTG is the lowest height in two tables. */
	    } 
            else if( (A317vd->htg[k]*M_TO_FT + Siteadp.rda_elev) > A317ve->vhtg[Cvol][j] ){

		if( A317ve->vshw[Cvol][j] != MISSING ){

		    alt[lines] = A317ve->vhtg[Cvol][j];
		    wnddir = wdir[lines] = A317ve->vhwd[Cvol][j];
		    wspd[lines] = A317ve->vshw[Cvol][j];
		    wrms[lines] = A317ve->vrms[Cvol][j];
		    vws[lines] = -999.f;
		    dive[lines] = -999.f;
		    slrng[lines] = A317ve->slrn[j];
		    index[lines] = A317ve->elcn[j];
		    alpha = (wnddir - 180.0) * DEGTORAD;
		    uspd[lines] = wspd[lines] * sin(alpha);
		    vspd[lines] = wspd[lines] * cos(alpha);
		    lines++;

		}

		j++;

            /* VHTG = HTG and both are the lowest heights. */
	    } 
            else{

		if( A317ve->vshw[Cvol][j] != MISSING ){

		    alt[lines] = A317ve->vhtg[Cvol][j];
		    wnddir = wdir[lines] = A317ve->vhwd[Cvol][j];
		    wspd[lines] = A317ve->vshw[Cvol][j];
		    wrms[lines] = A317ve->vrms[Cvol][j];
		    vws[lines] = -999.f;
		    dive[lines] = -999.f;
		    slrng[lines] = A317ve->slrn[j];
		    index[lines] = A317ve->elcn[j];
		    alpha = (wnddir - 180.0) * DEGTORAD;
		    uspd[lines] = wspd[lines] * sin(alpha);
		    vspd[lines] = wspd[lines] * cos(alpha);
		    lines++;

		}

		if( A317vd->shw[k] != MISSING ){

		    alt[lines] = A317vd->htg[k]*M_TO_FT + Siteadp.rda_elev;
		    wnddir = wdir[lines] = A317vd->hwd[k];
		    wspd[lines] = A317vd->shw[k];
		    wrms[lines] = A317vd->rms[k];
		    vws[lines] = A317vd->svw[k];
		    dive[lines] = A317vd->div[k];
		    slrng[lines] = vad_slrng * .001f;
		    index[lines] = Rpg_elev_ind[k];
		    alpha = (wnddir - 180.0) * DEGTORAD;
		    uspd[lines] = wspd[lines] * sin(alpha);
		    vspd[lines] = wspd[lines] * cos(alpha);
		    lines++;

		}

		j++;
		k++;

	    }

        /* Only EWT and HTS tables are available. All the elements in 
           ELEV table have been selected. */
	} 
        else if( (n < NELVEWT) && (k >= MAX_VAD_ELVS) && (j < MAX_VAD_HTS) ){

            /* VHTG is the lowest height in two tables. */
	    if( A317ve->vhtg[Cvol][j] < ((htg_ewt[n]*M_TO_FT + Siteadp.rda_elev)) ){

		if( A317ve->vshw[Cvol][j] != MISSING ){

		    alt[lines] = A317ve->vhtg[Cvol][j];
		    wnddir = wdir[lines] = A317ve->vhwd[Cvol][j];
		    wspd[lines] = A317ve->vshw[Cvol][j];
		    wrms[lines] = A317ve->vrms[Cvol][j];
		    vws[lines] = -999.f;
		    dive[lines] = -999.f;
		    slrng[lines] = A317ve->slrn[j];
		    index[lines] = A317ve->elcn[j];
		    alpha = (wnddir - 180.0) * DEGTORAD;
		    uspd[lines] = wspd[lines] * sin(alpha);
		    vspd[lines] = wspd[lines] * cos(alpha);
		    lines++;

		}

		j++;

            /* HTG_EWT is the lowest height in two tables. */
	    } 
            else if( A317ve->vhtg[Cvol][j] > ((htg_ewt[n]*M_TO_FT + Siteadp.rda_elev)) ){

		if( shw_ewt[n] != MISSING ){

		    alt[lines] = htg_ewt[n]*M_TO_FT + Siteadp.rda_elev;
		    wnddir = wdir[lines] = hwd_ewt[n];
		    wspd[lines] = shw_ewt[n];
		    wrms[lines] = rms_ewt[n];
		    vws[lines] = -999.f;
		    dive[lines] = -999.f;
		    slrng[lines] = A317ec->vad_rng_ewt * .001f;
		    index[lines] = Rpg_elev_ind[n];
		    alpha = (wnddir - 180.0) * DEGTORAD;
		    uspd[lines] = wspd[lines] * sin(alpha);
		    vspd[lines] = wspd[lines] * cos(alpha);
		    lines++;

		}

		n++;

            /* HTG_EWT = VHTG and both are the lowest heights. */
	    } 
            else{

		if( A317ve->vshw[Cvol][j] != MISSING ){

		    alt[lines] = A317ve->vhtg[Cvol][j];
		    wnddir = wdir[lines] = A317ve->vhwd[Cvol][j];
		    wspd[lines] = A317ve->vshw[Cvol][j];
		    wrms[lines] = A317ve->vrms[Cvol][j];
		    vws[lines] = -999.f;
		    dive[lines] = -999.f;
		    slrng[lines] = A317ve->slrn[j];
		    index[lines] = A317ve->elcn[j];
		    alpha = (wnddir - 180.0) * DEGTORAD;
		    uspd[lines] = wspd[lines] * sin(alpha);
		    vspd[lines] = wspd[lines] * cos(alpha);
		    lines++;

		}

		if( shw_ewt[n] != MISSING ){

		    alt[lines] = htg_ewt[n]*M_TO_FT + Siteadp.rda_elev;
		    wnddir = wdir[lines] = hwd_ewt[n];
		    wspd[lines] = shw_ewt[n];
		    wrms[lines] = rms_ewt[n];
		    vws[lines] = -999.f;
		    dive[lines] = -999.f;
		    slrng[lines] = A317ec->vad_rng_ewt * .001f;
		    index[lines] = Rpg_elev_ind[n];
		    alpha = (wnddir - 180.0) * DEGTORAD;
		    uspd[lines] = wspd[lines] * sin(alpha);
		    vspd[lines] = wspd[lines] * cos(alpha);
		    lines++;

		}

		j++;
		n++;

	    }
       
        /* Only ELEV and EWT tables are available. All the elements in   
           HTS table have been selected. */
	}
        else if( (n < NELVEWT) && (k < MAX_VAD_ELVS) && (j >= MAX_VAD_HTS) ){

            /* HTG is the lowest height in two tables. */
	    if( A317vd->htg[k] < htg_ewt[n] ){

		if( A317vd->shw[k] != MISSING ){

		    alt[lines] = A317vd->htg[k] * M_TO_FT + Siteadp.rda_elev;
		    wnddir = wdir[lines] = A317vd->hwd[k];
		    wspd[lines] = A317vd->shw[k];
		    wrms[lines] = A317vd->rms[k];
		    vws[lines] = A317vd->svw[k];
		    dive[lines] = A317vd->div[k];
		    slrng[lines] = vad_slrng * .001f;
		    index[lines] = Rpg_elev_ind[k];
		    alpha = (wnddir - 180.0) * DEGTORAD;
		    uspd[lines] = wspd[lines] * sin(alpha);
		    vspd[lines] = wspd[lines] * cos(alpha);
		    lines++;

		}

		k++;

            /* HTG_EWT is the lowest height in two tables. */
	    } 
            else if( A317vd->htg[k] > htg_ewt[n] ){

		if( shw_ewt[n] != MISSING ){

		    alt[lines] = htg_ewt[n]*M_TO_FT + Siteadp.rda_elev;
		    wnddir = wdir[lines] = hwd_ewt[n];
		    wspd[lines] = shw_ewt[n];
		    wrms[lines] = rms_ewt[n];
		    vws[lines] = -999.f;
		    dive[lines] = -999.f;
		    slrng[lines] = A317ec->vad_rng_ewt * .001f;
		    index[lines] = Rpg_elev_ind[n];
		    alpha = (wnddir - 180.0) * DEGTORAD;
		    uspd[lines] = wspd[lines] * sin(alpha);
		    vspd[lines] = wspd[lines] * cos(alpha);
		    lines++;

		}

		n++;

            /* HTG_EWT = HTG and both are the lowest heights. */
	    } 
            else{

		if( A317vd->shw[k] != MISSING ){

		    alt[lines] = A317vd->htg[k]*M_TO_FT + Siteadp.rda_elev;
		    wnddir = wdir[lines] = A317vd->hwd[k];
		    wspd[lines] = A317vd->shw[k];
		    wrms[lines] = A317vd->rms[k];
		    vws[lines] = A317vd->svw[k];
		    dive[lines] = A317vd->div[k];
		    slrng[lines] = vad_slrng * .001f;
		    index[lines] = Rpg_elev_ind[k];
		    alpha = (wnddir - 180.0) * DEGTORAD;
		    uspd[lines] = wspd[lines] * sin(alpha);
		    vspd[lines] = wspd[lines] * cos(alpha);
		    lines++;

		}

		if( shw_ewt[n] != MISSING ){

		    alt[lines] = htg_ewt[n]*M_TO_FT + Siteadp.rda_elev;
		    wnddir = wdir[lines] = hwd_ewt[n];
		    wspd[lines] = shw_ewt[n];
		    wrms[lines] = rms_ewt[n];
		    vws[lines] = -999.f;
		    dive[lines] = -999.f;
		    slrng[lines] = A317ec->vad_rng_ewt * .001f;
		    index[lines] = Rpg_elev_ind[n];
		    alpha = (wnddir - 180.0) * DEGTORAD;
		    uspd[lines] = wspd[lines] * sin(alpha);
		    vspd[lines] = wspd[lines] * cos(alpha);
		    lines++;

		}

		k++;
		n++;

	    }

        /* Only HTS table is available. */
	} 
        else if( (n >= NELVEWT) && (k >= MAX_VAD_ELVS) && (j < MAX_VAD_HTS) ){

	    if( A317ve->vshw[Cvol][j] != MISSING ){

		alt[lines] = A317ve->vhtg[Cvol][j];
		wnddir = wdir[lines] = A317ve->vhwd[Cvol][j];
		wspd[lines] = A317ve->vshw[Cvol][j];
		wrms[lines] = A317ve->vrms[Cvol][j];
		vws[lines] = -999.f;
		dive[lines] = -999.f;
		slrng[lines] = A317ve->slrn[j];
		index[lines] = A317ve->elcn[j];
		alpha = (wnddir - 180.0) * DEGTORAD;
		uspd[lines] = wspd[lines] * sin(alpha);
		vspd[lines] = wspd[lines] * cos(alpha);
		lines++;

	    }

	    j++;

        /* Only ELEV table is available. */
	} 
        else if( (n >= NELVEWT) && (k < MAX_VAD_ELVS) && (j >= MAX_VAD_HTS) ){

	    if( A317vd->shw[k] != MISSING ){

		alt[lines] = A317vd->htg[k]*M_TO_FT + Siteadp.rda_elev;
		wnddir = wdir[lines] = A317vd->hwd[k];
		wspd[lines] = A317vd->shw[k];
		wrms[lines] = A317vd->rms[k];
		vws[lines] = A317vd->svw[k];
		dive[lines] = A317vd->div[k];
		slrng[lines] = vad_slrng * .001f;
		index[lines] = Rpg_elev_ind[k];
		alpha = (wnddir - 180.0) * DEGTORAD;
		uspd[lines] = wspd[lines] * sin(alpha);
		vspd[lines] = wspd[lines] * cos(alpha);
		lines++;

	    }

	    k++;

        /* Only HTG_EWT table is available. */
	} 
        else if( (n < NELVEWT) && (k >= MAX_VAD_ELVS) && (j >= MAX_VAD_HTS) ){

	    if( shw_ewt[n] != MISSING ){

		alt[lines] = htg_ewt[n]*M_TO_FT + Siteadp.rda_elev;
		wnddir = wdir[lines] = hwd_ewt[n];
		wspd[lines] = shw_ewt[n];
		wrms[lines] = rms_ewt[n];
		vws[lines] = -999.f;
		dive[lines] = -999.f;
		slrng[lines] = A317ec->vad_rng_ewt * .001f;
		index[lines] = Rpg_elev_ind[n];
		alpha = (wnddir - 180.0) * DEGTORAD;
		uspd[lines] = wspd[lines] * sin(alpha);
		vspd[lines] = wspd[lines] * cos(alpha);
		lines++;

	    }

	    n++;

        /* Break out of the loop. */
	} 
        else 
	    break;
	
    }

    /* Call A3183F_VAD_ALPHA_WINDS() function to add tab pages. */
    *extra_pages = 0;
    if( lines > 0 )
	A3183f_vad_alpha_winds( beg, end, bufptr, A317vd->date, A317vd->time[Cvol], 
                                alt, uspd, vspd, wdir, wspd, wrms, vws, dive, 
                                slrng, index, lines, extra_pages );
    
    return 0;

} /* End of A3183g_vad_winds_info(). */
