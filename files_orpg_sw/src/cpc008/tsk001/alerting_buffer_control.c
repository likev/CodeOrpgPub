/* RCS info */
/* $Author: steves $ */
/* $Locker:  $ */
/* $Date: 2014/03/14 21:30:44 $ */
/* $Id: alerting_buffer_control.c,v 1.8 2014/03/14 21:30:44 steves Exp $ */
/* $Revision: 1.8 $ */
/* $State: Exp $ */

#include<infr.h>
#include<rss_lb.h>             /* remote access via LB_ routines          */

#include<orpg.h>
#include<a309.h>
#include<rpg.h>

#include "alerting.h"

/* Macro defintions and enumerations. */
#define REQ_MSG_MXSIZE   	300
#define HDROFFSET          	8
#define RLENOFBLK          	2
#define RALRTAREA          	3
#define RNUMCATS           	4
#define NCHARPTR		((sizeof(Graphic_product)/sizeof(short))+2)
#define INITNDX			(NCHARPTR+1)

/* Static globals. */
static int Initialized = 0;
static int LB_fd;
static LB_status Status;
static LB_check_list *Check_list = NULL;
static short *Request_msg = NULL;
static unsigned int Alert_req_update_time = 0;

/* Function Prototypes. */
static int Process_alerting( int vsn );
static void Read_alert_req( );
static int Build_product( int vscnix );
static void Update_alert_definitions( int line );
static char* Convert_time( time_t time_value );
static int Get_buffers( int vscnix );
static int Build_expected_bufs( int vscnix );
static int Alert_prd_header( int line, short *buf, int vscnix );
static int Total_length( int i, int *buf, int vscnix );
static int Flag_bufs( short *active_cats, int vscnix, int user);
static int Buffer_balance( int bid, int vscnix );
static int Setup_usergrd( short *usergrd, int areanum, int usernum );
static int Update_alert_data( );

/*\////////////////////////////////////////////////////////////////////
//
//   Description:
//      Manages the acquisition and processing of input buffers and 
//      controls the ALERT PROCESSING task. 
//
//   Input:
//      param - parameter which directs algorithm processing.
//
//   Return:
//      There are no return values defined for this function.
//
/////////////////////////////////////////////////////////////////////\*/
void A30811_alerting_ctl( int param ){

    /* Local variables */
    int *ipr, cvsn, istat, dtype, idataid, cvsnix, max_1hr, i;

    /* Process the "INPUT_AVAILABLE" event. */
    if( param == INPUT_AVAILABLE ) {

        /* Get input buffer of alerting data. */
	ipr = RPGC_get_inbuf_any( &idataid, &istat );
	if( istat == NORMAL ){

            /* Initialize data type to improbable value. */
	    dtype = -1;

            /* DO WHILE data type is equal to incoming buf id */
	    for( i = 0; i < NUM_TYPS; ++i ){

                /* If the buffer and incoming type are equal THEN */
		if( Valid_ids[i] == idataid ){

                    /* Set current data type into DTYPE */
		    dtype = i;

                    /*   Exit FOR LOOP - valid data type found. */
		    break;

		}

	    }

            /* Unexpected input data - release input buffer */
            if( i >= NUM_TYPS ){

	        RPGC_rel_inbuf( ipr );
	        return;

            }

            /* Determine if this is a scan that is currently active. */
	    cvsn = RPGC_get_buffer_vol_num( ipr );
	    cvsnix = cvsn % NACT_SCNS;

	    if( Active_scns[cvsnix] == cvsn) {

                /* Determine if this is the first buffer of this scan */
		if( Num_ebufs[cvsnix] == 0 ){

                    /* Set up buffers-expected table for this vscn. */
		    Build_expected_bufs( cvsnix );

                    /* Get product output buffers for requested UAM. 
                       Only call Get_buffers if at least one buffer is expected 
                       for this volume scan. */
		    if( Num_ebufs[cvsnix] != 0) 
			Get_buffers( cvsnix );
		    
		}

                /* If the buffer type was expected then, call processor for the 
                   incoming buffer as per data type. */
		if( Bufs_expected[cvsnix][dtype] == 1) {

                    /* Test for a grid type category (data type 1-3) */
		    if( (dtype >= GRID_BUFFER_MIN) && (dtype <= GRID_BUFFER_MAX) ){

                        LE_send_msg( GL_INFO, "Processing GRID data type (%d)\n", dtype );
			A30851_grid_alert_processing( ipr, dtype, cvsnix );

                    }
 
                    /* Check for VAD buffer (data type 7 ) */
		    else if( dtype == VAD_BUFFER ){
 
                        LE_send_msg( GL_INFO, "Processing VAD data type (%d)\n", dtype );
			A3081j_process_vad_alert( cvsnix, ipr );

                    }

                    /* Check for COMBINED ATTRIBUTES (data type 8) */
		    else if( dtype == CAT_BUFFER ){

                        combattr_t *combattr = (combattr_t *) ipr;
 
                        LE_send_msg( GL_INFO, "Processing CAT data type (%d)\n", dtype );
			A30810_comb_att( combattr->cat_num_storms, 
                                         combattr->num_fposits, ipr, cvsnix );

                    }

                    /* Check for the HYDROMET buffer (datatype 9) */
		    else if( dtype == MAX_RAIN_BUFFER ){

                        /* Apply radar-gage BIAS only if the BIAS_FLAG is set to true */
			if( Hydromet_adj.bias_flag )
			    max_1hr = *(ipr + HYO_SUPL+MAX_HLYACU);

                        else 
			    max_1hr = *(ipr + HYO_SUPL+MAX_HLYACU);
			 
                        LE_send_msg( GL_INFO, "Processing MAX RAIN data type (%d)\n", dtype );
			A3081k_maxrain( max_1hr, cvsnix );

		    }

                    /* Update the counts of buffers done for this vol.scn. */
		    Buffer_balance( idataid, cvsnix );

                    /* Release any open UAM products */
		    Build_product( cvsnix );

		}

	    }

            /* Release the input buffer. */
	    RPGC_rel_inbuf( ipr );

	} else {

            /* Abort the Task */
	    RPGC_abort();

	}

    }
    else if( param == START_OF_VOLUME_SCAN ){

        /* Update the Volume Status information. */
	RPGC_read_volume_status();

        /* Update Alerting Adaptation Data */
	Update_alert_data();

        /* Get ready for the next volume scan */
        LE_send_msg( GL_INFO, "Start of Volume Scan %d Received\n", 
                     Vol_stat.volume_scan );
	Process_alerting( Vol_stat.volume_scan );

    }

    return;

} 

/*\////////////////////////////////////////////////////////////////////
//
//   Description:
//      Called when new volume scan starts, to set up the user's 
//      environment for this volume scan. 
//
//   Input:
//      vsn - volume scan number.
//
//   Return:
//      This function always returns 0.
//
/////////////////////////////////////////////////////////////////////\*/
static int Process_alerting( int vsn ){

    int user, areanum, type;
    int area_defined, slot, snix, *mptr, istat1;

    /* First read in the user alert request messages. */
    Read_alert_req();

    /* Calculate the slot number that this volume scan goes in.
       Note:  The slot numbers flip-flop between 0 and 1.  Slot 
       is the next available one to use, snix is the other or
       last one in user (may still be in use.) */
    slot = vsn % 2;
    snix = (slot+1) % 2;

    /*  Determine if all expected buffers have been processed. */
    if( Num_dbufs[slot] != Num_ebufs[slot]) {

        /* Assume the remainder of buffers in this scan are never
           coming and release any open products. */

        /* Clear the user expected buffer flags for this volume
           scan. */
	for( type = 0; type < NUM_TYPS; ++type ){

	    User_ebufs[slot][type][0] = 0;
	    User_ebufs[slot][type][1] = 0;

	}

	Build_product( slot );

    }

    /* Clear the number of done and expected buffers. */
    Num_ebufs[slot] = 0;
    Num_dbufs[slot] = 0;

    /* Set scan number into the correct slot in the table that keeps
       track of which two volume scans are active in alerting. */
    Active_scns[slot] = vsn;

    /* Build table of pointers to scratch buffers that contain the
       user grid definitions. */

    /* Do For All users. */
    for( user = 0; user < MAX_CLASS1; ++user ){

        /* Do For All areas. */
	for( areanum = 0; areanum < NUM_ALERT_AREAS; ++areanum ){

            /* Has this user flag changed? */
	    if( User_chg_flg[user][areanum] == 1) {

                /* Release scratch buffers that are not in use. */
		if( (Uaptr[slot][areanum][user] != Uaptr[snix][areanum][user]) 
                                       && 
		    (Uaptr[slot][areanum][user] != NULL) ){

                    /* This usrs's area changed.  Release output buffer. */
		    RPGC_rel_outbuf( Uaptr[slot][areanum][user], DESTROY );

		}

                /* Get new output buffer. */
		mptr = RPGC_get_outbuf( SCRATCH, 136*sizeof(int), &istat1 );

                /* If buffer status is NORMAL.... */
		if( istat1 == NORMAL ){

                    /* Change user area pointer. */
		    Uaptr[slot][areanum][user] = mptr;

                    /* Set up user area grid. */
		    area_defined = Setup_usergrd( (short *) mptr, areanum, user );

                    /* If area defined flag is clear .... */
		    if( !area_defined ){

                        /* Release output buffer. */
			RPGC_rel_outbuf( mptr, DESTROY );

                        /* Clear this area. */
			Uaptr[slot][areanum][user] = NULL;

		    }
                    else {

                        /* Build user alert directory from user grid. */
			A3081t_setup_direct( (short *) (mptr + ACOFF), 
                                             (short *) (mptr + THOFF), 
                                             (short *) (mptr + PRQOFF), 
                                             (short *) (mptr + CSTATOFF), 
                                             user, areanum );

		    }

                    /* Clear the change flag for this area. */
		    User_chg_flg[user][areanum] = 0;

		}

	    }
            else {

                /* This user's area did not change - use last volume scan's
                   grid .. flip back to the last volume scan in table.  Copy   
                   these pointer to this volume scans pointers. */
		if( (Uaptr[slot][areanum][user] != Uaptr[snix][areanum][user])
                                       && 
		    (Uaptr[slot][areanum][user] != NULL) ) 
		    RPGC_rel_outbuf( Uaptr[slot][areanum][user], DESTROY );
		
		Uaptr[slot][areanum][user] = Uaptr[snix][areanum][user];

	    }

	}

    }

    return 0;

/* End of Process_alerting(). */
} 


/*\////////////////////////////////////////////////////////////////////
//
//   Description:
//      Build one product per volume scan for each user.
//
//   Inputs:
//      vscnix - volume scan index in Uam_ptrs array.
//
//   Return:
//      This function always returns 0.
//
/////////////////////////////////////////////////////////////////////\*/
static int Build_product( int vscnix ){

    int user, type, not_done;

    for( user = 0; user < MAX_CLASS1; ++user ){

	if( Uam_ptrs[vscnix][user] != NULL ){

            /* Determine if this user has unprocessed buffers.  Do Until 
               the logical "not_done" is true, terminate the loop and 
               bypass processing for setting up the product since all 
               the buffers to process are "Not Done". */
	    for( type = 0; type < NUM_TYPS; ++type ){

		not_done = User_ebufs[vscnix][type][0] & (1 << user);
		if( not_done )
		    break;

	    }

            if( not_done )
               continue;

            RPGC_log_msg( GL_INFO, "Building UAM Product for Line %d\n", Lnxref[user] );

	    Alert_prd_header( Lnxref[user], (short *) Uam_ptrs[vscnix][user], vscnix );

            /* Determine if any alert messages in buffer. */
	    if( Ndx[vscnix][user] <= INITNDX ){

		A3081r_no_prd_msg( user, vscnix);
		A3081s_store_alert_packet( user, (short *) Uam_ptrs[vscnix][user], 
			                   vscnix );

	    }

            /* Store block divider, number pages and total length. */
	    Total_length( user, Uam_ptrs[vscnix][user], vscnix);

            /* Release the UAM product buffer. */
	    RPGC_rel_outbuf( Uam_ptrs[vscnix][user], FORWARD );
	    Uam_ptrs[vscnix][user] = NULL;

	}

    }

    return 0;

/* End of Build_product(). */
} 

/*\////////////////////////////////////////////////////////////////////
//
//   Description:
//      This function read the alert request definitions from the
//      ORPGDAT_WX_ALERT_REQ_MSG LB upon initalization and change.
//      The data read are used to (re)initialize the alert grid.
//
//   Return:
//      There are no return values defined for this function.
//
/////////////////////////////////////////////////////////////////////\*/
static void Read_alert_req( ){

   int line, line_idx, area, ind, user, j, k;
   int ret;
   char *time_string;

   /* First time processing. */
   if( !Initialized ){

      /* Initialize array which tracks whether a user's alert request data
         has changed. */
      for( user = 0; user < MAX_CLASS1; user++ )
         for( area = 0; area < NUM_ALERT_AREAS; area++ )
            User_chg_flg[user][area] = 0;

      /* Initialize the LB status structure. */
      Status.attr = (LB_attr *) NULL; 
      Status.n_check = MAX_CLASS1 * NUM_ALERT_AREAS;
      Status.check_list = Check_list;
      ind = 0;
      for( user = 0; user < MAX_CLASS1; user++ ){

         for( area = 1; area <= NUM_ALERT_AREAS; area++ ){

            Check_list[ind].id = user*100 + area;
            ind++;

         }

      }

      /* Initialize the alert definition data and line cross-reference
         for all users. */
      ind = 0;
      for( user = 0; user < MAX_CLASS1; user++ ){

         for( area = 0; area < NUM_ALERT_AREAS; area++ ){

            line = Check_list[ind].id/100;
            line_idx = line - 1;

            /* Must first initialize alert definition to remove old data. */
            for( k = 0; k < NUM_ALERT_AREAS * HW_PER_CAT; k++ )
               Alert_grid[area][line_idx][k] = 0;

            ind++;

         }

         /* Initialize the line cross reference table.  By convention, 
            user is 1 less than the line number (e.g., user 24 refers 
            to line 25). */
         Lnxref[user] = line + 1;

      }

      /* Initially read all alert request messages. */
      LB_fd = ORPGDA_lbfd( ORPGDAT_WX_ALERT_REQ_MSG );
      LB_stat( LB_fd, &Status );
      for( user = 0; user < MAX_CLASS1 * NUM_ALERT_AREAS; user++ ){

         ret = LB_read( LB_fd, (char *) Request_msg,
                        REQ_MSG_MXSIZE*sizeof( short ), Check_list[user].id );

         if( ret < 0 )
            PS_task_abort( "Wx Alert Request Msg Read Failed (%d)\n", ret );

         else if( ret == 0 )
            continue;

         /* Update alert definitions. */
         Update_alert_definitions( (int) Check_list[user].id/100 );

      }

      /* Set the initialize flag to TRUE. */
      Initialized = 1;
  
   }
   else{

      /* Read the alert request message linear buffer for each line only if
         a new message received.  A message length of 0 indicates the line is
         disconnected.  */
      LB_stat( LB_fd, &Status );
      for( user = 0; user < MAX_CLASS1 * NUM_ALERT_AREAS; user++ ){

         /* If the alert request message has be updated, then ... */
         if( (unsigned int) Status.check_list[user].status == LB_MSG_UPDATED ){
                         
            ret = LB_read( LB_fd, (char *) Request_msg,
                           REQ_MSG_MXSIZE*sizeof( short ), Check_list[user].id );

            if( ret < 0 )
               PS_task_abort( "Wx Alert Request Msg Read Failed (%d)\n", ret );

            if( ret != 0 ){

               /* Update alert definitions. */
               area = (Check_list[user].id - (Check_list[user].id/100)*100);
               RPGC_log_msg( GL_INFO, "Alert Request For User %d (Grid: %d) Updated\n",
                             Check_list[user].id/100, area );

               Update_alert_definitions( (int) Check_list[user].id/100 );

            }

            else{

               /* Extract alert area and validate. */
               area = (Check_list[user].id - (Check_list[user].id/100)*100) - 1;
               if( area < 0 || area >= NUM_ALERT_AREAS )
                  PS_task_abort( "Invalid Area (%d ) -- Check_list[%d]:  %d\n", 
                                 area+1, user, Check_list[user].id );

               /* Initialize the alert definition data. */
               line_idx = Check_list[user].id/100 - 1;
               for( j = 0; j < NUM_ALERT_AREAS * HW_PER_CAT; j++ )
                  Alert_grid[area][line_idx][j] = 0;

            }

         }

      }

   }

   /* Notify user of alert request update time. */
   time_string = Convert_time( (time_t) Alert_req_update_time );
   RPGC_log_msg( GL_INFO, "Alert Request Message Updated @ %s\n",
                 time_string );

   return;

/* End of Read_alert_req(). */
}

/*\////////////////////////////////////////////////////////////////////
//
//   Description:
//      This function initializes the alert grid, then sets the user
//      change flag indicating the grid has been re-initialized.
//
//   Inputs:
//      line - user line index associated with alert definition.
//
//   Return:
//      There are no return values defined for this function.
//
/////////////////////////////////////////////////////////////////////\*/
static void Update_alert_definitions( int line ){

   int cat, grid, k, size, line_idx;
   int area, area_idx, n_cats;
   unsigned short time_msb, time_lsb;

   /* The line index is the line number less 1. */
   line_idx = line - 1;

   /* Extract the alert request update time from header. */
   time_msb = (unsigned short) Request_msg[TMSWOFF];
   time_lsb = (unsigned short) Request_msg[TLSWOFF];

   Alert_req_update_time = (unsigned int) (time_msb << 16) | time_lsb;
   
   /* Get the size of the alert request message. */
   size = Request_msg[ HDROFFSET + RLENOFBLK ]/sizeof( short );

   /* Extract alert area number from alert request messsage.  
      Validate it. */
   area = Request_msg[ HDROFFSET + RALRTAREA ];
   if( area <= 0 || area > NUM_ALERT_AREAS )
      PS_task_abort( "Invalid Area Number In Wx Alert Req Msg (%d )\n", area );

   /* Extract number of alert categories requested and validate. */
   n_cats = Request_msg[ HDROFFSET + RNUMCATS ];
   if( n_cats < 0 || n_cats > MAX_ALERT_CATS )
      PS_task_abort( "Invalid Number Categories in Wx Alert Req Msg (%d)\n", n_cats );

   /* Transfer alert category defintion. */
   area_idx = area - 1;
   for( cat = 0; cat < n_cats * HW_PER_CAT; cat++ )
      Alert_grid[area_idx][line_idx][cat] = 
                      Request_msg[HDROFFSET + RNUMCATS + cat + 1];

   /* Clear out old alert category definitions. */
   for( cat = n_cats*HW_PER_CAT; cat < MAX_ALERT_CATS*HW_PER_CAT; cat++ )
      Alert_grid[area_idx][line_idx][cat] = 0;

   /* Transfer alert grid defintion. */
   grid = MAX_ALERT_CATS * HW_PER_CAT;
   for( k = n_cats * HW_PER_CAT; k < size; k++ ){

      Alert_grid[area_idx][line_idx][grid] = Request_msg[HDROFFSET + RNUMCATS + k + 1];
      grid++;

   }

   /* Set the grid change flag to TRUE indicating the grid definition has
      changed. */
   User_chg_flg[line_idx][area_idx] = 1;

   return;

/* End of Update_alert_definitions(). */
}

/*\//////////////////////////////////////////////////////////////////////
//
//   Description:
//      Returns the alert request message update time (i.e., time from 
//      message header.
//
//   Returns:
//      Alert_req_update_time.
//
///////////////////////////////////////////////////////////////////////\*/
unsigned int ABC_get_update_time(){

   return( Alert_req_update_time );

/* End of ABC_get_update_time(). */
}

/*\////////////////////////////////////////////////////////////////////////
// 
//   Description: 
//      This function converts UNIX time to hrs, minutes, seconds.
//   
//   Inputs:     
//      timevalue - the UNIX time.
//  
//   Returns:    
//      Time string in hr:min:sec format.
//   
////////////////////////////////////////////////////////////////////////\*/
static char *Convert_time( time_t timevalue ){

   int hrs, mins, secs;
   static char time_string[10];
  
   /* Convert the UNIX time to seconds since midnight. */
   timevalue %= 86400;
  
   /* Extract the number of hours. */
   hrs = timevalue/3600;
   if( hrs < 0 || hrs > 24 )
      RPGC_log_msg( GL_INFO, "Alert Request Message Update Time Invalid (%d)\n",
                    timevalue );
  
   /* Extract the number of minutes. */
   timevalue = timevalue - hrs*3600;
   mins = timevalue/60;
   if( mins < 0 || mins > 59 )
      RPGC_log_msg( GL_INFO, "Alert Request Message Update Time Invalid (%d)\n",
                    timevalue );
  
   /* Extract the number of seconds. */
   secs = timevalue - mins*60;
   if( secs < 0 || secs > 59 )
      RPGC_log_msg( GL_INFO, "Alert Request Message Update Time Invalid (%d)\n",
                    timevalue );
  
   /* Convert numbers to ASCII. */
   sprintf( time_string, "%02d:%02d:%02d", hrs, mins, secs );
  
   return (time_string);

/* End of Convert_time(). */
}

/*\////////////////////////////////////////////////////////////////////////
// 
//   Description: 
//      Obtain output buffers for products.                        
//   
//   Inputs:     
//      vscnix - volume scan index.
//
//   Returns:    
//      Always returns 0.                   
//   
////////////////////////////////////////////////////////////////////////\*/
static int Get_buffers( int vscnix ){

    int user, stat, *optr;

    /* Get buffer pointers for the user alert grid product 
       for all the users that have requested the product. 
       Set the pointer table to all NULL.  Set NDX to point 
       to the first 'NUMBER OF CHARACTERS' Position. */
    for( user = 0; user < MAX_CLASS1; user++ ){

	Uam_ptrs[vscnix][user] = NULL;
	Ndx[vscnix][user] = NCHARPTR;
	Np[vscnix][user] = 0;
	It[vscnix][user] = 0;

    }

    for( user = 0; user < MAX_CLASS1; user++ ){

        /* Determine if user has any area defined */
	if( (Uaptr[vscnix][0][user] != 0) 
                         || 
	    (Uaptr[vscnix][1][user] != 0) ){

            /* Obtain output buffer for this user */
	    optr = RPGC_get_outbuf_by_name( "ALRTPROD", 12000, &stat );

            /* Save buffer is good status */
	    if( stat == NORMAL ){

                RPGC_log_msg( GL_INFO, "Obtained UAM Buffer for Line %d\n", user+1 );
		Uam_ptrs[vscnix][user] = optr;

            }
	    else 
		RPGC_abort_dataname_because( "ALRTPROD", stat );
	     
	}

    }

    return 0;

/* End of Get_buffers(). */
} 

/*\////////////////////////////////////////////////////////////////////////
// 
//   Description: 
//      Construct table of indicators for expected input buffers.  
//   
//   Inputs:     
//      vscnix - volume scan index.
//
//   Returns:    
//      Always returns 0.                   
//   
////////////////////////////////////////////////////////////////////////\*/
static int Build_expected_bufs( int vscnix ){

    int user, type, areanum;

    /* Clear the table first. */
    for( type = 0; type < NUM_TYPS; type++ ){

	Bufs_expected[vscnix][type] = 0;
	Bufs_done[vscnix][type] = 0;
	User_ebufs[vscnix][type][0] = 0;
	User_ebufs[vscnix][type][1] = 0;

    }

    Num_ebufs[vscnix] = 0;
    Num_dbufs[vscnix] = 0;

    for( user = 0; user < MAX_CLASS1; user++ ){

	for( areanum = 0; areanum < NUM_ALERT_AREAS; areanum++ ){

            /* Determine if this user has this area defined for this scan. */
	    if( Uaptr[vscnix][areanum][user] != NULL )
		Flag_bufs( (void *) (Uaptr[vscnix][areanum][user] + ACOFF), 
                           vscnix, user );

	}

    }

    /* Count the number of expected buffers for this scan. */
    for( type = 0; type < NUM_TYPS; type++ ){

	if( Bufs_expected[vscnix][type] == 1) 
	    ++Num_ebufs[vscnix];
	 
    }

    return 0;

/* End of Build_expected_bufs(). */
} 

/*\////////////////////////////////////////////////////////////////////////
// 
//   Description: 
//      Build UAM product header and description blocks.            
//   
//   Inputs:     
//      line - line index for this UAM.
//      buf - product buffer for UAM.
//      vscnix - volume scan index.
//
//   Returns:    
//      Always returns 0.                   
//   
////////////////////////////////////////////////////////////////////////\*/
static int Alert_prd_header( int line, short *buf, int vscnix ){

    int volno;
    Graphic_product *phd = (Graphic_product *) buf;

    /* Initialize product header to zero. */
    memset( buf, 0, PHEADLNG );

    /* Assign values to product header. */
    volno = Active_scns[vscnix];
    RPGC_prod_desc_block( buf, Alert_prod_id, volno );

    /* Assign Line index in product header. */
    phd->dest_id = (short) line;

    /*  Store offset to Status Block. */
    RPGC_set_prod_block_offsets( buf, PHEADLNG, 0, 0 );

    return 0;

/* End of Alert_prd_header(). */
} 

/*\////////////////////////////////////////////////////////////////////////
// 
//   Description: 
//      Determine the total length of product.   Fills out product header.
//   
//   Inputs:     
//      user - index into various tables for UAM generation.
//      buf - product buffer for UAM.
//      vscnix - volume scan index.
//
//   Returns:    
//      Always returns 0.                   
//   
////////////////////////////////////////////////////////////////////////\*/
static int Total_length( int user, int *buf, int vscnix ){

    int tlngthx;
    short *tab = (short *) buf;

    if( (It[vscnix][user] < MNOLNS) && (It[vscnix][user] != 0) ){

	tab[Ndx[vscnix][user]] = -1;
	++Ndx[vscnix][user];
	++Np[vscnix][user];
    }

    /* Store 1st divider, number of pages */
    tab += sizeof(Graphic_product)/sizeof(short);
    *tab = -1;
    *(++tab) = (short) Np[vscnix][user];

    /* Determine total length of data written in bytes in output buffer */
    tlngthx = (Ndx[vscnix][user] - 1) * sizeof(short);

    /* Move total length as I*4 word to output buffer */
    RPGC_prod_hdr( buf, Alert_prod_id, &tlngthx );

    return 0;

/* End of Total_length(). */
} 

/*\/////////////////////////////////////////////////////////////////////
//
//   Description:
//      Search requested categories that are input via array 
//      "active_cats", and set the "Bufs_expected" flag for all input 
//      buffers expected for all users for this volume scan. 
//
//   Inputs:
//      active_cats - active alert categories.
//      vscnix - volume scan index.
//      user - user index.
//
//   Returns:
//      Always returns 0.
//
/////////////////////////////////////////////////////////////////////\*/
static int Flag_bufs( short *active_cats, int vscnix, int user){

    int l, cat, stat, bufidx;

    for( l = 0; l < MAX_ALERT_CATS; ++l ){

	if( active_cats[l] != 0 ){

	    cat = active_cats[l];
	    bufidx = -1;

            /* Check for one of the Storm categories. */
	    if( ((cat >= 8) && (cat <= 14))
                            || 
                       (cat == 16) 
                            || 
                ((cat >= 25) && (cat <= 32)) )
		bufidx = CAT_BUFFER;
	     
            /* Check for Low Elevation Velocity (1). */
            else if( cat == 1 )
                bufidx = BVG_BUFFER;

            /* Check for Composite Reflectivity (2). */
            else if( cat == 2 )
                bufidx = CR_BUFFER;

            /* Check for Echo Tops (3). */
            else if( cat == 3 )
                bufidx = ET_BUFFER;

            /* Check for VIL (6). */
            else if( cat == 6 )
                bufidx = VIL_BUFFER;

            /* Check for VAD alert (7). */
            else if( cat == 7 )
                bufidx = VAD_BUFFER;

            /* Check for the Max. Rainfall (15). */
	    else if( cat == 15 )
		bufidx = MAX_RAIN_BUFFER;

            /* If buffer index defined, check if data is
               being produced.  */
            if( bufidx >= 0 ){

  	        stat = RPGC_check_data( Valid_ids[bufidx] );

	        if( stat == NORMAL ){

                    /* Buffer is expected .... */
		    Bufs_expected[vscnix][bufidx] = 1;

                    /* Set this users bit for this buffer type */
		    User_ebufs[vscnix][bufidx][0] |= (1 << user);

                }

	    }

	}

    }

    return 0;

/* End of Flag_bufs(). */
} 

/*\/////////////////////////////////////////////////////////////////
//
//   Description:
//      Update "Bufs_done" array if the incoming buffer is one that 
//      expected by this volume scan.  
//
//   Inputs:
//      bid - buffer ID.
//      vscnix - volume scan index.
//
//   Returns:
//      Always returns 0.
//
/////////////////////////////////////////////////////////////////\*/
static int Buffer_balance( int bid, int vscnix ){

    int j, i;

    if( bid != 0 ){

        /* Do Until find a matching buffer-id. */
	for( i = 0; i < NUM_TYPS; ++i ){

	    if( Valid_ids[i] == bid ){

		if( Bufs_expected[vscnix][i] == 1 ){

		    Bufs_done[vscnix][i] = 1;
		    ++Num_dbufs[vscnix];

                    /* Clear the bit in all users expected buffer list */
		    for( j = 0; j < NWDS_MAX_CLASS1; ++j )
			User_ebufs[vscnix][i][j] = 0;

		}

		break;

	    }

	}

    }

    return 0;

/* End of Buffer_balance(). */
} 

/*\//////////////////////////////////////////////////////////////////////////
// 
//   Description:
//      Set up local "USERGRD" from common "ALERT_GRID", ie. get the alert 
//      area bitmaps for 1 user and area from the larger array "ALERT_GRID". 
//
//   Inputs:
//      usergrd - user alert grid.
//      areanum - alert area number.
//      usernum - user number.
//
//   Returns:
//      Returns flag to indicate whether (1) or not (0) the area is defined.
//
//////////////////////////////////////////////////////////////////////////\*/
static int Setup_usergrd( short *usergrd, int areanum, int usernum ){

    /* Local variables */
    int n, nn, bitend, bitstr, num_half_words;

    num_half_words = NUM_ALERT_ROWS*HW_PER_ROW;

    /* Start of alert grid is past the 30 words to define categories. */
    bitstr = MAX_ALERT_CATS*HW_PER_CAT;
    bitend = bitstr + num_half_words - 1;

    /* Do For All halfwords in the alert grid bit map. */
    nn = 0;
    for( n = bitstr; n <= bitend; ++n ){

        /* Set up local user grid block. */
	usergrd[nn] = Alert_grid[areanum][usernum][n];
	++nn;

    }

    for( n = 0; n < num_half_words; ++n ){

	if( usergrd[n] != 0 ){

            /* Terminate loop when get one match, no need to do more. */
	    return 1;

	}

    }

    return 0;

/* End of Setup_usergrd(). */
} 


/*\//////////////////////////////////////////////////////////////////////////
//
//   Description:
//      Updates the alert adaptation data and puts the data in data structure
//      format.
//
//   Returns:
//      Returns the return value of ORPGALT_update_legacy function call. 
//
//////////////////////////////////////////////////////////////////////////\*/
static int Update_alert_data( ){

   int   ret = 0;

   /* Call ORPGALT lib routine to assign the structure values to the data bfr */
   ret = ORPGALT_update_legacy( (char *) &Alttable ); 
   if( ret < 0 ){

      RPGC_log_msg( GL_ERROR, 
                    "Update_alert_data: Problem updating legacy data\n");
      ret = -1;

   }

   return ret;

} /* End update_alert_data() */


/*\//////////////////////////////////////////////////////////////////////////
//
//   Description:
//      Initialization module for the buffer control function.
//
//////////////////////////////////////////////////////////////////////////\*/
void ABC_initialize_alerting_buffer_control( ){

    /* Initialize buffers used for alert processing. */
    if( Check_list == NULL ){

        Check_list = (LB_check_list *) calloc( MAX_CLASS1*NUM_ALERT_AREAS,
                                               sizeof(LB_check_list) );

        if( Check_list == NULL ){

           RPGC_log_msg( GL_ERROR, "calloc Failed for %d Bytes\n",
                         MAX_CLASS1*NUM_ALERT_AREAS*sizeof(LB_check_list) );

           RPGC_hari_kiri();

        }

    }

    if( Request_msg == NULL ){

        Request_msg = (short *) calloc( REQ_MSG_MXSIZE, sizeof(short) );

        if( Request_msg == NULL ){

           RPGC_log_msg( GL_ERROR, "calloc Failed for %d Bytes\n",
                         REQ_MSG_MXSIZE*sizeof(short) );

           RPGC_hari_kiri();

        }

    }

/* End of Init_alerting_buffer_control() */
}
