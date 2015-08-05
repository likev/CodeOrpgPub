/* RCS info */
/* $Author: steves $ */
/* $Locker:  $ */
/* $Date: 2014/03/14 21:30:45 $ */
/* $Id: alerting_product_generation.c,v 1.13 2014/03/14 21:30:45 steves Exp $ */
/* $Revision: 1.13 $ */
/* $State: Exp $ */

#include <alerting.h>

/* Function Prototypes. */
static int Alert_message( int area_num, char *stormid, float az, float ran, 
                          int threshold, float exval, int exval1, 
                          int user, short *buf, int cat_idx, 
	                  int vscnix, float strm_spd, float strm_dir );
static int Alarm_msg( int line, int area_num, char *stormid, float az, 
                      float ran, int threshold, int threshold_code, 
                      float exval, int exval1, int alert_status, 
                      short *buf2, short *active_cats, int catix, 
                      int vscnix );
static int Write_alert_packet( int i, int area_num, char *stormid, float az, 
                               float ran, int threshold, float exval, int exval1, 
                               int cat_code, int vscnix, float strm_spd, float strm_dir );
static int Check_paired_prod( int user, float az, float ran, float elevang,
                              float strm_spd, float strm_dir, short *prodreq,
                              short *active_cats, int catix, int vscnix );
static int Request_prods( int user, float az, float ran, float elevang, 
                          int category, int vscnix, float strm_spd, 
                          float strm_dir, int prod_id, int prod_code );
static int Alert_prod_req( short *cpc2msg, int len, int line_index );

/*\////////////////////////////////////////////////////////////////////////
//
//   Description:
//      Perform alerting process for conditions encountered. 
//
//   Inputs:
//      user - index for the user.
//      area_num - alerting grid area number.
//      stmix - storm index.
//      stormid - storm ID.
//      az - azimuth angle of phenomenon (if applicable).
//      ran - range of phenomenon (if applicable).
//      threshold - threshold value.
//      threshold_code - threshold code (1-6) associated with threshold.
//      exval - exceeding value.
//      exval1 - exceeding value.
//      alert_status - alert status (new, continuing, etc).
//      elevang - elevation angle of phenomenon.
//      strm_spd - storm speed associated with phenomenon.
//      strm_dir - storm direction associated with phenomenon.
//      cat_idx - alerting category index.
//      vscnix - volume scan index.      
//
//
//   Returns:
//      Always returns 0.
//
///////////////////////////////////////////////////////////////////////\*/
int A30817_do_alerting( int user, int area_num, int stmix, char *stormid, 
                        float az, float ran, int threshold, int threshold_code, 
                        float exval, int exval1, int alert_status, 
                        float elevang, float strm_spd, float strm_dir, 
                        int cat_idx, int vscnix ){

    int *optr, opstat;

    /* Format packet in UAM if alert_status is NEW_ALERT */
    if( alert_status == NEW_ALERT ){

        if( Uam_ptrs[vscnix][user] != NULL )
	    Alert_message( area_num, stormid, az, ran, threshold, 
	                   exval, exval1, user, (short *) Uam_ptrs[vscnix][user], 
                           cat_idx, vscnix, strm_spd, strm_dir );

    }

    /* Get output buffer for message. */
    optr = RPGC_get_outbuf_by_name( "ALRTMSG", 100*sizeof(int), &opstat );
    if( opstat == NORMAL ){

        /* Send out alarm for alert condition encountered for area and 
           forward buffer. */
	Alarm_msg( user, area_num, stormid, az, ran, threshold, 
		   threshold_code, exval, exval1, alert_status, 
		   (short *) optr, (short *) (Uaptr[vscnix][area_num][user] + ACOFF), 
                   cat_idx, vscnix );

	RPGC_rel_outbuf( optr, FORWARD );

    /* Otherwise abort the alert message. */

    }
    else 
	RPGC_abort_dataname_because( "ALRTMSG", opstat );

    /* For alert status = NEW_ALERT, request product that is paired to the 
       alert category as a one-time product request. */
    if( alert_status == NEW_ALERT ){

        Check_paired_prod( user, az, ran, elevang, strm_spd, strm_dir, 
                           (short *) (Uaptr[vscnix][area_num][user] + PRQOFF), 
                           (short *) (Uaptr[vscnix][area_num][user] + ACOFF),
		           cat_idx, vscnix );
    }

    return 0;

/* End of A30817_do_alerting(). */
} 

#define MAXLNG			5672

/*\////////////////////////////////////////////////////////////////////////////
//
//   Description:
//      Build alert packets, 8 lines per packet.
//
//   Inputs:
//      area_num - alerting grid area number.
//      stormid - storm ID.
//      az - Azimuth angle of phenomenon (if applicable).
//      ran - Range of phenomenon (if applicable).
//      threshold - threshold value.
//      exval - exceeding value.
//      exval1 - exceeding value.
//      user - index for the user.
//      cat_idx - alerting category index.
//      vscnix - volume scan index.      
//      strm_spd - storm speed associated with phenomenon.
//      strm_dir - storm direction associated with phenomenon.
//
//   Returns:
//      Always returns 0.
//
////////////////////////////////////////////////////////////////////////////\*/
static int Alert_message( int area_num, char *stormid, float az, float ran, 
                          int threshold, float exval, int exval1, 
                          int user, short *buf, int act_cat_idx, 
	                  int vscnix, float strm_spd, float strm_dir ){

    int cat_code;
    short *act_cats = (short *) (Uaptr[vscnix][area_num][user] + ACOFF);

    /* Check for maximum size. */
    if( Ndx[vscnix][user] < MAXLNG ){

        /* Write alert message lines. */
        cat_code = act_cats[act_cat_idx];
	Write_alert_packet( user, area_num, stormid, az, ran, threshold, exval, 
                            exval1, cat_code, vscnix, strm_spd, strm_dir );

        /* Store alert packets. */
	A3081s_store_alert_packet( user, buf, vscnix );

    }

    /* Return to caller routine. */
    return 0;

/* End of Alert_message(). */
} 

/*\////////////////////////////////////////////////////////////////////////////
//
//   Description:
//      Build alert packets, 8 lines per packet.
//
//   Inputs:
//      user - user index.
//      area_num - alerting grid area number.
//      stormid - storm ID.
//      az - azimuth angle of phenomenon (if applicable).
//      ran - range of phenomenon (if applicable).
//      threshold - threshold value.
//      exval - exceeding value.
//      exval1 - exceeding value.
//      active_cats - table of active alert categories.
//      cat_idx - alerting category index.
//      vscnix - volume scan index.      
//      strm_spd - storm speed associated with phenomenon.
//      strm_dir - storm direction associated with phenomenon.
//
//   Returns:
//      Always returns 0.
//
////////////////////////////////////////////////////////////////////////////\*/
static int Write_alert_packet( int user, int area_num, char *stormid, float az, 
                               float ran, int threshold, float exval, int exval1, 
                               int cat_code, int vscnix, float strm_spd, float strm_dir ){
 
    /* Initialized data */
    static char blank4[] = "    ";
    static char *tvs_label[ ] = { "            ", 
                                  "ELEVATED TVS",
                                  "TVS         " };
    static char *ccats[] = { "                         ",
                             "VELOCITY                 ",
                             "COMP REFLECTIVITY        ",
                             "ECHO TOPS                ",
                             "                         ",
                             "                         ",
                             "VIL                      ",
                             "VAD                      ",
                             "MAX HAIL SIZE            ",
                             "                         ",
                             "TVS                      ",
                             "MAX STORM REFL           ",
                             "PROB HAIL                ",
                             "PROB SVR HAIL            ",
                             "STORM TOP                ",
                             "MAX 1-HR. RAINFALL       ",
                             "MDA STRNGTH RANK         ",
                             "                         ",
                             "                         ",
                             "                         ",
                             "                         ",
                             "                         ",
                             "                         ",
                             "                         ",
                             "                         ",
                             "FORECAST MAX HAIL SIZE   ",
                             "                         ",
                             "FORECAST TVS             ",
                             "FORECAST MAX STORM REFL  ",
                             "FORECAST PROB HAIL       ",
                             "FORECAST PROB SVR HAIL   ",
                             "FORECAST STORM TOP       ",
                             "FORECAST MDA STRNGTH RANK" };

    static char *cunits[] = { "                         ",
                              "KNOTS                    ",
                              "DBZ                      ",
                              "K-FEET                   ",
		              "                         ",  
		              "                         ",  
		              "KG/M**2                  ",
                              "KNOTS                    ",
                              "INCHES                   ",
		              "                         ",  
		              "                         ",  
                              "DBZ                      ",
                              "PROB %                   ",
                              "PROB %                   ",
 		              "K-FEET                   ",
		              "INCHES                   ",
		              "                         ",  
		              "                         ",  
		              "                         ",  
		              "                         ",  
		              "                         ",  
		              "                         ",  
		              "                         ",  
		              "                         ",  
		              "                         ",  
                              "INCHES                   ",
		              "                         ",  
		              "                         ",  
                              "DBZ                      ",
                              "PROB %                   ",
                              "PROB %                   ",
                              "K-FEET                   ",
		              "                         " }; 
 

    int i, tmp_dir, tmp_spd;
    float ran_nm, texval, xthresh;

    /* Initialize Tbuf array. */
    for( i = 0; i < TLINES; i++ )
       memset( &Tbuf[i][0], ' ', NROWS_BYTES );

    /* Write line 1.  Note:  We need to add 1 to "area_num"
       since this variable is normally used as an array index. */
    sprintf( &Tbuf[0][0], "  ALERT AREA #       %5d", area_num+1 );

    /* Write line 2. */

    /* If alert azimuth is dependent on grid information, ..... */
    if( (cat_code >= 1) && (cat_code <= 6) )
       sprintf( &Tbuf[1][0], "  ALERT BOX AZIMUTH  %5.1f DEG", az );

    /* Else if we are unsure what the azimuth is dependent upon or
       in the case of MDA and TVS, the azimuth is to the base of the
       phenomenon. */
    else if( (cat_code == 10) || (cat_code == 27) || (cat_code == 7)
                               || 
             (cat_code == 15) || (cat_code == 16) || (cat_code == 32) )
       sprintf( &Tbuf[1][0], "  AZIMUTH            %5.1f DEG", az );

    /* Else the alert azimuth is dependent on storm cell information. */
    else 
       sprintf( &Tbuf[1][0], "  STORM CELL AZIMUTH %5.1f DEG", az );

    /* Line 3. */
    ran_nm = ran * KM_TO_NM;

    /* If alert range is dependent on grid information. */
    if( (cat_code >= 1) && (cat_code <= 6) )
       sprintf( &Tbuf[2][0], "  ALERT BOX RANGE    %5.1f NM ", ran_nm );

    /* Else if we are unsure what the range is dependent upon or in 
       the case of MDA and TVS, the range is to the base of the 
       phenomenon. */
    else if( (cat_code == 10) || (cat_code == 27) || (cat_code == 7)
                               || 
             (cat_code == 15) || (cat_code == 16) || (cat_code == 32) )
       sprintf( &Tbuf[2][0], "  RANGE              %5.1f NM ", ran_nm );

    /* Else the alert azimuth is dependent on storm cell information. */
    else 
       sprintf( &Tbuf[2][0], "  STORM CELL RANGE   %5.1f NM ", ran_nm );

    /* Line 4. */
    sprintf( &Tbuf[3][0], "  ALERT CATEGORY     %s", ccats[cat_code] );

    /* Check category type for lines 5 & 6. */

    /* Check for TVS; It has character exceeding value. */
    if( (cat_code == 10) || (cat_code == 27) ){

        sprintf( &Tbuf[4][0], "  THRESHOLD          %s", tvs_label[threshold] );

        /* Write line 6 for the exceeding value as integer. */
        sprintf( &Tbuf[5][0], "  EXCEEDING VALUE    %s", tvs_label[exval1] );

    /* Check for MDA, Prob Hail and Prob SVR Hail; They have integer   
       exceeding values. */
    } 
    else if( ((cat_code >= 12) && (cat_code <= 13)) 
                            || 
             ((cat_code >= 29) && (cat_code <= 30)) 
                            || 
	     (cat_code == 16) || (cat_code == 32) ){
        sprintf( &Tbuf[4][0], "  THRESHOLD          %5d %s", threshold, 
                 ccats[cat_code] );

        /* Write line 6 for the exceeding value as integer. */
        sprintf( &Tbuf[5][0], "  EXCEEDING VALUE    %5d %s", exval1, cunits[cat_code] );;

    }
    else if( (cat_code == 8) || (cat_code == 25) ){

        /* Check for maximum expected hail size category; float threshold. */

        /* Unscale the threshold and exceeding value. */
	xthresh = threshold / 4.f;
	texval = exval / 4.f;

        sprintf( &Tbuf[4][0], "  THRESHOLD          %6.2f  %s", xthresh,
                 ccats[cat_code] );

        /* Write line 6 for the maximum expected hail size category. */
        sprintf( &Tbuf[5][0], "  EXCEEDING VALUE %9.2f %s", texval, cunits[cat_code] );

    } 
    else if( cat_code == 15 ){

        /* Check for the max-rainfall category: float threshold. */

        /* Unscale the threshold and exceeding value. */
	xthresh = threshold / 10.f;
	texval = exval / 10.f;

        sprintf( &Tbuf[4][0], "  THRESHOLD          %5.1f %s", xthresh,
                 ccats[cat_code] );

        /* Write line 6 for the max-rainfall category. */
        sprintf( &Tbuf[5][0], "  EXCEEDING VALUE %8.1f %s", texval,
                 ccats[cat_code] );

    }
    else{

        /* Write the lines for all other categories. */
        sprintf( &Tbuf[4][0], "  THRESHOLD          %5d %s", threshold,
                 ccats[cat_code] );

        /* Write line 6 - the exceeding value as real number. */
        sprintf( &Tbuf[5][0], "  EXCEEDING VALUE %8.1f %s", exval,
        cunits[cat_code] );
    }

    /* Write line 7. */

    /* When the storm ID is blank, only print out the storm ID header. */
    if( strcmp( stormid, blank4 ) == 0 )
        sprintf( &Tbuf[6][0], "%s", stormid );

    else {

        /* When the storm ID is valid, print out the storm ID and storm 
           motion. */
	tmp_dir = RPGC_NINT( strm_dir );
	tmp_spd = RPGC_NINT( strm_spd*MPS_TO_KTS );
	sprintf( &Tbuf[6][0], "  STORM ID           %sSTORM MOTION   %3d/%3d DEG/KTS",
                 stormid, tmp_dir, tmp_spd );
    }

    /* Write line 8.  This is a blank line. */
    sprintf( &Tbuf[7][0], "                     " );

    /* Update the number of lines in this page. */
    It[vscnix][user] += 8;

    /* Return to caller. */
    return 0;

/* End of Write_alert_packet(). */
} 

/*\////////////////////////////////////////////////////////////////////////////
//
//   Description:
//      Build alert message.
//
//   Inputs:
//      user - user number.
//      area_num - alerting grid area number.
//      stormid - storm ID.
//      az - azimuth angle of phenomenon (if applicable).
//      ran - range of phenomenon (if applicable).
//      exval - exceeding value.
//      exval1 - exceeding value.
//      alert_status - alert status.
//      buf2 - product buffer.
//      active_cats - table of active alert categories.
//      catix - alerting category index.
//      vscnix - volume scan index.      
//
//   Returns:
//      Always returns 0.
//
////////////////////////////////////////////////////////////////////////////\*/
int Alarm_msg( int user, int area_num, char *stormid, float az, 
               float ran, int threshold, int threshold_code, 
               float exval, int exval1, int alert_status, 
               short *buf2, short *active_cats, int catix, 
               int vscnix ){

    /* Local variables */
    short dtype, iran, iaz;
    int cdate, ctime, whole_wd, alngth;

    Scan_Summary *summary;
    Prod_alert_msg_icd *alt_msg = (Prod_alert_msg_icd *) buf2;

    dtype = active_cats[catix];

    /* Get the generation date/time. */
    RPGCS_get_date_time( &ctime, &cdate );

    /* Convert azimuth to integer. */
    iaz = (short) RPGC_NINT( az*10.0 );

    /* Convert the range to NM, scale by 10 and convert to integer. */
    iran = (short) RPGC_NINT( ran*10.0*KM_TO_NM );

    /* Set message code. */
    alt_msg->mhb.msg_code = 9;

    /* Store the generation date of product. */
    alt_msg->mhb.date = (short) cdate;

    /* Store the generation time of product. */
    RPGC_set_product_int( &alt_msg->mhb.timem, ctime);

    /* Set source ID. */
    alt_msg->mhb.src_id = (short) Siteadp.rpg_id;

    /* Set destination ID to line number. */
    alt_msg->mhb.dest_id = (short) Lnxref[user];

    /* Determine number of blocks. */
    alt_msg->mhb.n_blocks = 2;

    /* Set block divider. */
    alt_msg->divider = -1;

    /* Set alert status and area number. */

    /* If alert status is END_ALERT .... */
    if( alert_status == END_ALERT ){

        /* Set code for 'END_ALERT' */
	alt_msg->alert_status = 2;

    /* Else set alert status ..... */
    }
    else
	alt_msg->alert_status = (short) alert_status;

    /* The alert area is the "area_num" plus 1 .... this is because
       "area_num" is used as an array indexing variable and all
        arrays start at 0. */
    alt_msg->alert_area_num = (short) area_num + 1;

    /* Set category. */
    alt_msg->alert_category = active_cats[catix];

    /* Set threshold code, threshold value. */
    alt_msg->thd_code = (short) threshold_code;
    RPGC_set_product_int( &alt_msg->thd_valuem, threshold);

    /* Store the exceeding value. */
    if( ((dtype >= 9) && (dtype <= 10)) || ((dtype >= 12) && (dtype <= 13))
                                        || 
        ((dtype >= 26) && (dtype <= 27)) || ((dtype >= 29) && (dtype <= 30))
                                        || 
         (dtype == 16) || (dtype == 32) ){

	whole_wd = exval1;

    }
    else{

	whole_wd = RPGC_NINT( exval );

    }

    RPGC_set_product_int( &alt_msg->exd_valuem, whole_wd );

    /* Store az/ran. */
    alt_msg->azim = iaz;
    alt_msg->range = iran;

    /* Set storm ID. */
    memcpy( &alt_msg->c1, stormid, 2 );

    /* Store the volume scan number. */
    alt_msg->vol_num = (short) Active_scns[vscnix];

    /* Store the volume scan date */
    summary = RPGC_get_scan_summary( alt_msg->vol_num );
    if( summary != NULL ){

       alt_msg->vol_date = (short) summary->volume_start_date;

       /* Store the volume scan time */
       RPGC_set_product_int( &alt_msg->vol_timem, summary->volume_start_time );

    }

    /* Set alngth to length of block, in bytes. */
    alngth = sizeof(Prod_alert_msg_icd);
    RPGC_set_product_int( &alt_msg->mhb.lengthm, alngth);

    /* Return to caller routine. */
    return 0;

/* End of Alarm_msg(). */
} 


/*\////////////////////////////////////////////////////////////////////////////
//
//   Description:
//      Build product message block for each category alerted.
//
//   Inputs:
//      user - user index.
//      az - azimuth angle of phenomenon (if applicable).
//      ran - range of phenomenon (if applicable).
//      elevang - elevation angle (if applicable).
//      strm_spd - storm speed.
//      strm_dir - storm direction.
//      prodreq - product request buffer.
//      active_cats - table of active alert categories.
//      act_cat_idx - active categories index.
//      vscnix - volume scan index.      
//
//   Returns:
//      Always returns 0.
//
////////////////////////////////////////////////////////////////////////////\*/
static int Check_paired_prod( int user, float az, float ran, float elevang,
                              float strm_spd, float strm_dir, short *prodreq,
                              short *active_cats, int act_cat_idx, int vscnix ){

    int prod_code, prod_id, category;

    /* Determine if the one-time product is requested with the alert. */
    if( prodreq[act_cat_idx] != 0 ){

	category = active_cats[act_cat_idx];

        /* Determine paired-product product code. */
	prod_code = AH_get_alert_paired_prod( category );

        /* Get associated product ID. */
        prod_id = ORPGPAT_get_prod_id_from_code( prod_code );

	if( prod_id != ORPGPAT_ERROR ){

            /* Request paired product(s). */
	    Request_prods( user, az, ran, elevang, category, vscnix, 
                           strm_spd, strm_dir, prod_id, prod_code );

	}

    }

    return 0;

/* End of Check_paired_prod(). */
}

#define OTR_MESSAGE_LENGTH		18
#define OTR_REQUEST_LENGTH		32
#define MAX_PAIRED_PRODUCTS		5

/*\////////////////////////////////////////////////////////////////////////////
//
//   Description:
//      Construct a one-time product request block for the four Severe
//      Weather Analysis products.
//
//   Inputs:
//      user - user index.
//      az - azimuth angle of phenomenon (if applicable).
//      ran - range of phenomenon (if applicable).
//      elevang - elevation angle (if applicable).
//      category - the alert category.
//      catix - alerting category index.
//      vscnix - volume scan index.      
//      strm_spd - storm speed.
//      strm_dir - storm direction.
//      prod_id - product ID.
//      prod_code - product code.
//
//   Returns:
//      Always returns 0.
//
////////////////////////////////////////////////////////////////////////////\*/
static int Request_prods( int user, float az, float ran, float elevang, 
                          int category, int vscnix, float strm_spd, 
                          float strm_dir, int prod_id, int prod_code ){

    /* Local variables */
    int j, cdate, ctime, i4wrd, n_prods, length;
    short paired_prods[MAX_PAIRED_PRODUCTS];
    float taz, ranp1, ranp2;

    static short buf[100]; 

    /* Cast "buf" to data structure holding the alert-paired product 
       request(s). */
    Alert_prod_req_t *prod_req = (Alert_prod_req_t *) buf;

    /* Product Request Description Block. */
    Pd_request_products *req = (Pd_request_products *) 
                   (buf + sizeof(Prod_msg_header_icd)/sizeof(short));

    /* Initialization. */
    memset( buf, 0, 100*sizeof(short) );
    n_prods = 0;

    /* Message code field = 0 means product request. */

    /* Store the generation date and time of product. */
    RPGCS_get_date_time( &ctime, &cdate );
    prod_req->phd.date = (short) cdate;

    RPGC_set_product_int( &prod_req->phd.timem, ctime );

    /* Set the length of the message. */
    RPGC_set_product_int( &prod_req->phd.lengthm, OTR_MESSAGE_LENGTH );

    /* Set source ID. */
    prod_req->phd.src_id = (short) Siteadp.rpg_id;

    /* Set destination ID to line # (user index) */
    prod_req->phd.dest_id = (short) Lnxref[user];

    /* Determine the number of products associated with the product ID. */
    if( prod_id < 0 ){

        n_prods = ORPGPAT_get_num_dep_prods( prod_id );
        if( (n_prods <= 0) || (n_prods > MAX_PAIRED_PRODUCTS) ){

            RPGC_log_msg( GL_INFO, "Unable to Request Alert-Paired Products\n" );
            RPGC_log_msg( GL_INFO, "--->Product ID %d Invalid or # Associated Product Too Many %d.\n",
                          prod_id, n_prods );
            return 0;

        }
        else{

           for( j = 0; j < n_prods; j++ )
	      paired_prods[j] = (short) ORPGPAT_get_dep_prod( prod_id, j );

        }

    }
    else{

        n_prods = 1;
        paired_prods[0] = prod_code;

    }

    /* Set number of blocks.  This is always the number of product plus the
       the request header (i.e., n_prods + 1). */
    prod_req->phd.n_blocks = n_prods + 1;

    /* Create Product Request Description Block for all products. */
    for( j = 0; j < n_prods; ++j ){

        /* Enter divider and number of bytes in block. */
	req->divider = -1;
	req->length = sizeof(Pd_request_products);

        /* Set product code and flag bits. */
	req->prod_id = paired_prods[ j ];
	req->flag_bits = 0;

        /* Set sequence number to -13 indicating product came from alerting. */
	req->seq_number = -13;

        /* Set the number of products requested. */
	req->num_products = 1;

        /* Set request interval. */
	req->req_interval = 1;

        /* Store the generation time of product. */
	i4wrd = Active_scns[vscnix];
	RPGC_set_product_int( &req->VS_start_time, i4wrd );

        /* Store azimuth and range of window. */
	req->params[0] = (short) RPGC_NINT( az*10.0 );
	req->params[1] = (short) RPGC_NINT( ran*10.0*KM_TO_NM );

        /* Store storm speed and direction. */
	if( strm_spd <= 0.f ){

	    req->params[3] = -1;
	    req->params[4] = -1;

	}
        else {

            /* Convert to knots and scale speed and direction. */
	    req->params[3] = strm_spd * MPS_TO_KTS * 10.0;
	    req->params[4] = strm_dir * 10.0;

	}

        /* Scale and store elevation angle. */
	req->params[2] = (short) RPGC_NINT( elevang*10.0 );

        /* Special processing for X-section products. */
        if( (req->prod_id == 50) || (req->prod_id == 51)
                             ||
            (req->prod_id == 85) || (req->prod_id == 86) ){

            /* Calculate the azimuth and range of the two points for xsect.
               Create the points by adding and subtracting a delta from the
               range of the alert phenomemon.  The azimuth for both points
               same. */
            ranp1 = ran - 46.f;
            taz = az;

            /* Check if the line goes through the radar and adjust azimuth. */
            if( ranp1 < 0.0 ){

                ranp1 = fabs(ranp1);
                taz = az + 180.0;
                if( taz > 360.0 )
                    taz += -360.0;

            }

            /* Store the azimuth and range for point 1 in buffer. */
            req->params[0] = (short) RPGC_NINT( taz*10.0 );
            req->params[1] = (short) RPGC_NINT( ranp1*10.0*KM_TO_NM );

            /* Calculate the azimuth and range of point 2. */
            ranp2 = ran + 46.f;
            if( ranp2 > 230.0 )
                ranp2 = 230.0;

            req->params[2] = (short) RPGC_NINT( az*10.0 );
            req->params[3] = (short) RPGC_NINT( ranp2*10.0*KM_TO_NM );

        }

        /* Set the altitude for the VAD (sin wave) to the lowest altitude
           that VAD algorithm is to generate.  Use the lowest altitude
           in adaptation data. */
        if( req->prod_id == 84 )
            req->params[2] = (short) (Prodsel.vad_rcm_heights.vad[0] / 1000);

        /* Store the alert category. */
	req->params[5] = category;

        /* Go to beginning of the next block. */
	req++;

    }

    /* Request product if there are products to request. */
    if( n_prods > 0 ){

        /* Convert request to RPG product request message format. */
        length = OTR_MESSAGE_LENGTH + n_prods*OTR_REQUEST_LENGTH;
        Alert_prod_req( buf, length, Lnxref[user] );

    }

    return 0;

/* End of Request_prods() */
} 


/*\///////////////////////////////////////////////////////////////
//
//   Description:
//      Store alert message packet.
//
//   Inputs:
//      user - user index.
//      buf - product buffer.
//      vscnix - volume scan index.
//
//   Returns:
//      Always returns 0.
//
///////////////////////////////////////////////////////////////\*/
int A3081s_store_alert_packet( int user, short *buf, int vscnix ){

    /* Local variables */
    int jb, idx;

    /* Set index. */
    idx = Ndx[vscnix][user];

    /* Move data to output buffer. */
    for( jb = 0; jb < NOLNS; jb++ ){

	buf[idx] = CHAR_PER_LN;
	++idx;

        /* Move characters to output buffer. */
        memcpy( &buf[idx], &Tbuf[jb][0], NROWS_BYTES );
        idx += I2S_PER_LN;

    }

    /* Update the index to next available buffer word. */
    Ndx[vscnix][user] = idx;

    /* Check for full page. */
    if( It[vscnix][user] >= MNOLNS ){

	Np[vscnix][user]++;
	It[vscnix][user] = 0;
	idx = Ndx[vscnix][user];

        /* Store divider. */
	buf[idx] = -1;
	Ndx[vscnix][user]++;

    }

    return 0;

/* End of A3081s_store_alert_packet(). */
} 

#define MSG_HEADER_SHORTS     9
#define MSG_HEADER_BYTES     (MSG_HEADER_SHORTS*sizeof(short))

/*\////////////////////////////////////////////////////////////////

   Description:
       This function builds a one-time product request message 
       from the cpc2msg buffer.

   Inputs:
       cpc2msg - Pointer to short containing cpc2msg data.
       len - length, in bytes, of the cpc2msg.
       skip_shorts - number of shorts for start of cpc2msg to 
                     start of message header block.
       line_index - narrowband line index to add to the message 
                    header.  Used by PS_ONETIME.
   
   Return:
       Returns 0.

   Note:
       The cpc2msg buffer length (not the length of the message 
       found in the message header) must be large enough to 
       accommodate the addition of the line index in the message 
       header.

////////////////////////////////////////////////////////////////\*/
static int Alert_prod_req( short *cpc2msg, int len, int line_index ){

   char *prq_msg = NULL;
   Pd_prod_header *msg_hdr = NULL;
   Pd_request_products *prod_req = NULL;
   int num_prods, prod_code; 
   int status = 0, ret, line, i;
   int update_time, length;
   

#ifdef LITTLE_ENDIAN_MACHINE
   /* Convert icdmsg buffer data to corrent ENDIANness.  (This is
      done to simulate message from PUP). */
   MISC_short_swap( cpc2msg, len/2 );
#endif

   /* Convert cpc2msg (i.e., alert-paired product request) from RPG/PUP ICD
      format to internal format.  The internal format product request 
      message is stored in buffer prq_req.  We must later free this memory. */
   status = UMC_from_ICD( (void *) cpc2msg, len, 0, (void *) &prq_msg );    
   if( status < 0 ){

      LE_send_msg( GL_INFO, "Alert-Paired Product Request Failed\n" );
      return (0);

   }

   /* Add the line index to product request message header.  Change
      the length field to account for the line index. */
   msg_hdr = (Pd_prod_header *) prq_msg; 
   msg_hdr->line_ind = line_index;

   /* Re-build cpc2msg buffer from internal format. */
   memcpy( (void *) cpc2msg, (void *) prq_msg, len + sizeof(msg_hdr->line_ind) ); 

   /* Free storage for product request message buffer. */
   free (prq_msg);

   /* Get alert request update time. */
   update_time = (int) ABC_get_update_time();

   /* Change message generation time to alert request update time. */
   msg_hdr = (Pd_prod_header *) cpc2msg;
   ORPGMISC_pack_ushorts_with_value( &msg_hdr->timem, &update_time );
   
   /* Determine the number of products requested. */
   ORPGMISC_unpack_value_from_ushorts( &msg_hdr->lengthm, &length );
   num_prods = (length - sizeof(Pd_msg_header))/sizeof(Pd_request_products);

   /* Extract line index. */
   line = msg_hdr->line_ind;

   /* Inform user about the product(s) just requested. */
   prq_msg = (char *) cpc2msg;
   prod_req = (Pd_request_products *) (prq_msg + ALIGNED_SIZE(sizeof(Pd_prod_header))); 

   for( i = 0; i < num_prods; i++ ){

      int vol_init = 0;
      unsigned int vol_seq_num = 0;

      /* Product request message contains product code requested.  Validate 
         product code.  If invalid, return to caller. */
      prod_code = prod_req->prod_id;
      if( prod_code == 0 ){

         LE_send_msg( GL_ERROR, "Invalid Paired Product Request (Prod Code: %d)\n",
                      prod_code );

         return( 0 );

      }

      /* The product code in the product request message must be converted to 
         product id for one-time request. */
      prod_req->prod_id = ORPGPAT_get_prod_id_from_code( prod_req->prod_id );
      
      /* Validate the product id.  If invalid, return to caller. */
      if( prod_req->prod_id == ORPGPAT_ERROR ){

         LE_send_msg( GL_ERROR, "Invalid Paired Product Request (Prod ID: %d)\n",
                      prod_req->prod_id );
         
         return( 0 );

      }

      if( !vol_init ){

         /* The volume start time in the product request is really volume 
            scan number.  Change the volume scan number to volume sequence 
            number. */
         vol_seq_num = ORPGVST_get_volume_number();
         if( vol_seq_num == ORPGVST_DATA_NOT_FOUND )
            vol_seq_num = 0;

         else{

            int vol_scan_num;

            /* The volume scan number is in the range 1 - MAX_SCAN_SUM_VOLS, 
               inclusive.  We assume that if the volume scan numbers do not match,
               then the vol_seq_num must have incremented. The sequence number
               from ORPGVST_get_volume_number call is not the correct volume. */
            vol_scan_num = ORPGMISC_vol_scan_num( vol_seq_num );

            /* This should not typically happen but must handle it when it does. */
            if( vol_scan_num != prod_req->VS_start_time ){
 
               int vol_diff = 0;

               LE_send_msg( GL_INFO, 
                            "Current Volume Scan (%d) != Requested Volume (%d)\n",
                            vol_scan_num, prod_req->VS_start_time );

               if( vol_scan_num < prod_req->VS_start_time ){

                  /* This handles the case where volume status volume scan number
                     has wrapped around MAX_SCAN_SUM_VOLS from product volume
                     scan number. */
                  vol_diff = (MAX_SCAN_SUM_VOLS - prod_req->VS_start_time) + vol_scan_num;

               }
               else{

                  /* This handles the normal case, i.e., volume scan number from
                     volume status is greater than product volume scan number. */
                  vol_diff = vol_scan_num - prod_req->VS_start_time;

               }

               vol_seq_num = (int) vol_seq_num - vol_diff;
      
            }

         }

         /* We assume for each batch of product requests, the volume scan 
            number does not change, so this conversion need only to be done 
            once. */
         vol_init = 1;

      }

      if( vol_init ){

         /* Change the volume start time field in the product request from volume 
            scan number to volume scan sequence number. */
         prod_req->VS_start_time = (int) vol_seq_num;

      }

      /* Inform user of requested product. */
      LE_send_msg( GL_INFO, 
                   "ALERT-PAIRED PROD REQ: Prod Code %d (ID %d, VSN %d) Sched For Line %d\n",
                   prod_code, prod_req->prod_id, prod_req->VS_start_time, line );
      LE_send_msg( GL_INFO, ">>> Flag Bits %x  Seq # %d  Num Prods %d  Req Int %d\n",
                   prod_req->flag_bits, prod_req->seq_number, prod_req->num_products, 
                   prod_req->req_interval );
      LE_send_msg( GL_INFO, ">>> Params  %d  %d  %d  %d  %d  %d\n",
                   prod_req->params[0], 
                   prod_req->params[1], 
                   prod_req->params[2], 
                   prod_req->params[3], 
                   prod_req->params[4],
                   prod_req->params[5] );

      /*
        Adjust pointer to start of next request.
      */
      prod_req++;

   }

   /*
     Write alert product request to one-time product request LB.
   */
   msg_hdr = (Pd_prod_header *) cpc2msg; 
   ORPGMISC_unpack_value_from_ushorts( &msg_hdr->lengthm, &length );
   ret = ORPGDA_write( ORPGDAT_OT_REQUEST, (char *) msg_hdr, length, ALERT_OT_REQ_MSGID );
   if( ret <= 0 )
      LE_send_msg( GL_INFO, "Alert-Pair Product Request Failed.  Ret = %d\n",
                   ret );

   else{

      /* 
        Post a one-time request event.
      */
      EN_post( ORPGEVT_OT_REQUEST, (void *) NULL, (size_t) 0, (int) 0 );

   }

   return (0);

/* End of Alert_prod_req */ 
}

