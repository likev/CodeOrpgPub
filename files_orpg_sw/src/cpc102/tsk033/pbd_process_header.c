/*
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2010/03/17 21:40:09 $
 * $Id: pbd_process_header.c,v 1.1 2010/03/17 21:40:09 ccalvert Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */

#include <pbd.h>

/* Value for undefined azimuth and elevation. */
#define UNDEFINED_AZI		1.e20  	
#define UNDEFINED_ELE		1.e20

/* Azimuth incremant tolerance. */
#define DELTA_AZI_TOLERANCE 	(2.0f)
#define CENTER_AZI          	(0.5f)

/* Elevation Delta tolerance */
#define DELTA_ELV_TOLERANCE 	(0.5f)

/* Degrees to Radians conversion. */
#define DEG_TO_RAD 		(0.0174533f)

/* Used in the height equation:  (Index of refraction)*(Earth's Radius) */
#define IRRE            	(1.21*6371.0)

/* Used in the height equation: 70,000 ft in km. */
#define TOPHGT_KM       	(21.21)

/* Radial count in current elevation. */
static int Radial_cnt = 0;	

/* Set in Find_start_and_delta_angles; used for delta azimuth checking */
static float Delta_azimuth;	

/* Starting time and date of current volume. */
static int Volume_time;		
static int Volume_date;		

/* Number of elevations in current volume; must be set before a radial
   is processed; must be less than ECUTMAX. */
static int Number_elevations;	

/* Target elevation table. Values are in degrees. */
static float Target_elevation [ECUTMAX];

/* The RPG elevation index of this elevation. */
static int Rpg_elevation_index;	

/* Used for recording the previous azimuth */
static float Old_azi = UNDEFINED_AZI;

/* Local functions. */
static int Find_start_and_delta_angles( float azi, Base_data_header *rpg_hd,
                                        float half_azimuth_spacing );
static void Read_and_update_current_vcp_table( Vol_stat_gsm_t *vs_gsm );
static void Process_angle_data( Generic_basedata_t *gbd, 
                                Base_data_header *rpg_hd );
static void Process_block_headers( Generic_basedata_t *gbd, 
                                   Base_data_header *rpg_hd );
static int Process_moment_block_headers( Generic_basedata_t *gbd, 
                                         Base_data_header *rpg_hd );
static int Get_cut_type( int vcp, int elev_num );

/*******************************************************************

   Description:   
      This function sets up parameters for the current volume and 
      current cut when a new cut starts. 

   Inputs:        
      gdr - pointer to Generic_basedata_t radial data structure.

   Outputs:
     vs_gsm - volume-based general status message.
     rpg_num_elevs - number of elevation cuts.

   Returns:
      This function returns 0 if the basedata is OK and all 
      parameters are set up successfully. It returns -1 
      otherwise. 

*******************************************************************/
int PH_process_new_cut ( Generic_basedata_t *gbd,   /* RDA radial data message header */
                         Vol_stat_gsm_t *vs_gsm,    /* volume status message */
                         int *rpg_num_elevs         /* number of elevations cuts */) {

    static int volume_ok = 0;
    static int vcp_num, vcp_ind;

    Generic_basedata_header_t *bdh = (Generic_basedata_header_t *) &gbd->base;
    Generic_vol_t *vhd = NULL;


    /* Extract the volume header from the radial message.   Note: This block should
       always be present. */
    vhd = (Generic_vol_t *) ORPGGDR_get_data_block( (char *) gbd, ORPGGDR_RVOL );
    if( vhd == NULL ){

       LE_send_msg( GL_ERROR, "Volume Header Missing From Radial Message .....\n" );
       return (-1);

    }

#ifdef AVSET_TEST
   /* Check the Terminate_cut status.  If at beginning
      of volume, reset the flag. */
   if( (bdh->status == GOODBVOL) && (bdh->elev_num == 1) )
      Terminate_cut = 0;

   /* If the Terminate_cut flag is set, modify the radial 
      status values. */
   if( Terminate_cut == 1 ){

      if( bdh->status == GOODBEL )
         bdh->status = GOODBELLC;

      if( bdh->status == GENDEL ){

         bdh->status = GENDVOL;
         Terminate_cut = 2;

      }

   }
   else if( Terminate_cut == 2 ){

      /* Set end of volume radial status.   No need to process any 
         more radials until next start of volume. */
      return -1;

   }

#endif

    /* If not at beginning of volume scan or not at beginning of elevation scan,
       then ... */
    if( (bdh->status != GOODBEL)
                     &&
	(bdh->status != GOODBVOL) 
                     &&
	(bdh->status != GOODBELLC) ){	

	if (volume_ok) {

            /* Clear volume_ok flag for next radial.  If not
               beginning of volume radial, do not process or any
               subsequent radials until beginning of volume 
               encountered. */
            if( bdh->status == GENDVOL )
               volume_ok = 0;

	    return (0);

	}
	else 
	    return (-1);

    }

    /* A new volume starts */
    if( (bdh->status == GOODBVOL) && (bdh->elev_num == 1) ){

	int wx_found, i;
	short wx_mode_vcp;

	/* Get new VCP */
	vcp_num = vhd->vcp_num;
	if( (vcp_ind = ORPGVCP_index( vcp_num )) < 0 ){

           /* No match is found */
	    LE_send_msg (GL_STATUS | LE_RPG_WARN_STATUS, "VCP Number (%d) NOT In VCP Table\n",
			vcp_num);
	    volume_ok = 0;
	    return (-1);

	}

	/* Number of elevations of this volume. */
	Number_elevations = ORPGVCP_get_num_elevations( vcp_num );
	if( (Number_elevations <= 0)
                       || 
	    (Number_elevations > ECUTMAX) ){

	    LE_send_msg (GL_STATUS | LE_RPG_WARN_STATUS, 
		"Number Of Elevations (%d) Incorrect In VCP %d\n",
		Number_elevations, vcp_num);
	    volume_ok = 0;
	    return (-1);

	}

	/* Set volume scan start time/date.  */
	Volume_time = bdh->time;
	Volume_date = bdh->date;

	/* Set weather mode for this volume by matching weather mode */
        /* NOTE:  Currently, maintenance VCPs are defined in either
           the clear air table or the precipitation table.  Therefore,
           we check the vcp number to see if it is a maintance vcp.  If
           so, we set weather mode the MAINTENANCE MODE. */
	wx_found = 0;		/* not found */
	for (i = 1; i <= WXMAX; i++) {

	    int j;

	    for (j = 0; j < WXVCPMAX; j++) {

                wx_mode_vcp = ORPGVCP_get_wxmode_vcp( i, j );
		if (wx_mode_vcp == vcp_num) {

                    PBD_rad_wx_mode = i;
                    if( vcp_num <= 255 )
		       PBD_weather_mode = i;
                    else
		       PBD_weather_mode = MAINTENANCE_MODE;

		    wx_found = 1;
		    break;

		}

	    }

	    if (wx_found)
		break;

	}

	if (wx_found == 0) {

	    LE_send_msg (GL_STATUS | LE_RPG_WARN_STATUS, 
                         "No Weather Mode Defined For VCP %d\n", vcp_num);
	    volume_ok = 0;
	    return (-1);

	}

	volume_ok = 1;

        /* Increment the volume scan sequence number. */
        PBD_volume_seq_number++;

        /* Update the volume scan number (modulo PBD_MAX_SCANS) */
        PBD_volume_scan_number = PBD_volume_seq_number % PBD_MAX_SCANS;
        if( PBD_volume_scan_number == 0 )
           PBD_volume_scan_number = PBD_MAX_SCANS;

        /* Set weather mode field in volume-based general status message. */
        /*
          NOTE: The weather mode definition for the RPG is different than
                the weather mode definition for narrowband users.
        */
        vs_gsm->mode_operation = PBD_weather_mode;

        /* Set time/date fields in volume-base general status message. */
        vs_gsm->cv_time = bdh->time;
        vs_gsm->cv_julian_date = bdh->date;
      
        /* Set number of elevations and volume coverage pattern number
           in volume-base general status message. */
        PBD_vcp_number = vcp_num;
        vs_gsm->vol_cov_patt = vcp_num;
        vs_gsm->num_elev_cuts = Number_elevations;

        /* Set the expect volume scan duration. */
        vs_gsm->expected_vol_dur = ORPGVCP_get_vcp_time( vcp_num );

	/* Fill in target elevation angles and corresponding elevation
           index in volume-based general status message. */
	for (i = 0; i < Number_elevations; i++) {

	   /* Fill in local target elevation table for this volume. */
	    Target_elevation [i] = (float) ORPGVCP_get_elevation_angle( vcp_num, i );                                                         
	    vs_gsm->elevations[i] = Round( (Target_elevation [i]*10.0) );
            vs_gsm->elev_index[i] = ORPGVCP_get_rpg_elevation_num( vcp_num, i );

        }

        /* Clear remaining array values. */
        for( i = Number_elevations; i < MAX_CUTS; i++ ){

           vs_gsm->elevations[i] = 0;
           vs_gsm->elev_index[i] = 0;

        }

        /* Set rpg elevation index of last cut in vcp. */
        *rpg_num_elevs = ORPGVCP_get_rpg_elevation_num( vcp_num, 
                                                        Number_elevations - 1 );
        /* Reassign current if vcp number or weather mode has changed
         from last volume scan.  vcp_table contains the adaptation data
         version of the current VCP. */
        if( (PBD_vcp_number != PBD_old_vcp_number)
                             ||
            (PBD_weather_mode != PBD_old_weather_mode) ){

           Vcp_struct *vcp_table = (Vcp_struct *) ORPGVCP_ptr( vcp_ind );
         
           LE_send_msg( GL_INFO, "Updating Current VCP Data With Adaptation Data\n" );
           vs_gsm->current_vcp_table = *vcp_table;

           /* Set rpgvcpid.  Note this is the index for unit indexed arrays. */
           vs_gsm->rpgvcpid = vcp_ind + 1;

           if( PBD_weather_mode != PBD_old_weather_mode ){

              /* Report weather mode upon change. */
              if( PBD_weather_mode == PRECIPITATION_MODE )
                 LE_send_msg(GL_STATUS | LE_RPG_GEN_STATUS,
                    "Weather Mode is now PRECIPITATION/SEVERE WEATHER (A)\n");

              else if( PBD_weather_mode == CLEAR_AIR_MODE )
                 LE_send_msg(GL_STATUS | LE_RPG_GEN_STATUS,
                    "Weather Mode is now Clear Air (B)\n");

              else if( PBD_weather_mode == MAINTENANCE_MODE )
                 LE_send_msg(GL_STATUS | LE_RPG_GEN_STATUS,
                    "Weather Mode is now MAINTENANCE (M)\n");

           }
              
        }
        else if( PBD_volume_status_updated ){

           /* Update the current VCP table if it has been updated by
              another process (e.g., HCI) and the VCP number is the same. */
           Read_and_update_current_vcp_table( vs_gsm );

        }


    }
    /* If the start of elevation - last elevation radial status received, 
       reset Number_elevations. */
    if( bdh->status == GOODBELLC )
       Number_elevations = bdh->elev_num;

    /* Has the vcp number changed unexpectedly. */
    if (vcp_num != vhd->vcp_num) {	/* verify vcp number */

	LE_send_msg (GL_STATUS | LE_RPG_WARN_STATUS, "VCP (%d) Changed Unexpectedly - Was %d\n",
			vhd->vcp_num, vcp_num);
	volume_ok = 0;

    }

    if (volume_ok == 0) {

        /* Restart the volume scan. */
        PH_send_rda_control_command( (int) 1, (int) CRDA_RESTART_VCP,
                                     (int) PBD_ABORT_INPUT_DATA_ERROR );
	LE_send_msg (GL_INFO, "Waiting for the next volume (VCP: %d, Elev: %d)\n",
                     vhd->vcp_num, bdh->elev_num); 
	return (-1);

    }

    /* A new cut is started. Determine waveform type of this cut. */
    Get_cut_type( vcp_num, bdh->elev_num - 1 );
    LE_send_msg( GL_INFO, "New Cut: VCP: %d, Elev #: %d, Waveform: %d, Split Cut: %d\n",
                 vcp_num, bdh->elev_num, PBD_waveform_type, PBD_split_cut );

    /* Initialize the spot blanking bitmap. */
    PBD_spot_blank_bitmap = 0;

    /* Get the RPG elevation index. */
    Rpg_elevation_index = (int) ORPGVCP_get_rpg_elevation_num( vcp_num, 
                                                               bdh->elev_num-1 );
    PBD_current_elev_num = bdh->elev_num;

    /* If status is GOODBVOL and elevation number is not 1, change
       status to GOODBEL. This is to support double beginning of
       volume flags. */
    if( (bdh->status == GOODBVOL) && (bdh->elev_num != 1) )
       bdh->status = GOODBEL;

    return (0);

/* PH_process_new_cut() */
}

/*******************************************************************

      Description:
	Function setting up the RPG base data header. 

      Inputs:
        gbd - RDA radial data message.
 
      Outputs:
        rpg_basedata - RPG base data buffer.

      Returns:
	This function returns 0 on success or -1 on failure.

*******************************************************************/
int PH_process_header( Generic_basedata_t *gbd, char *rpg_basedata ){        

   Base_data_header *rpg_hd;
   float ftmp;

   rpg_hd = (Base_data_header *) rpg_basedata;

   /* Setup the radial count and reset azimuth number at beginning of 
      elevation/volume. */
   if( (gbd->base.status == GOODBEL) 
                     || 
       (gbd->base.status == GOODBVOL) 
                     ||
       (gbd->base.status == GOODBELLC) ){

      Radial_cnt = 0;
      rpg_hd->sc_azi_num = gbd->base.azi_num;
      rpg_hd->azi_num = 0;

   }

   /* Increment the radial count. */
   Radial_cnt++;

   /* Set the radar name. */
   memcpy( rpg_hd->radar_name, gbd->base.radar_id, 4 );

   /* Increment the azimuth number. This number is used for data
      sequencing checks. */
   rpg_hd->sc_azi_num = gbd->base.azi_num;
   rpg_hd->azi_num++;
   if( PBD_saved_ind >= 0 )
      rpg_hd->sc_azi_num = PBD_Z_hdr[PBD_saved_ind].azi_num;

   /* Set the azimuth resolution and azimuth index value. */
   rpg_hd->azm_reso = gbd->base.azimuth_res;
   rpg_hd->azm_index = gbd->base.azimuth_index;

   /* Initialize the message type field and initialize the message size. */
   rpg_hd->msg_type = 0;
   rpg_hd->msg_len = (sizeof(Base_data_header) + 1)/sizeof(short);

   /* Radial collection date/time. */
   rpg_hd->time = gbd->base.time;
   rpg_hd->date = gbd->base.date;

   /* Volume begin time (sec past midnight) and volume begin date
      (Modified Julian) .*/
   rpg_hd->begin_vol_time = Volume_time;
   rpg_hd->begin_vol_date = (unsigned short) Volume_date; 

   /* Radial status; must be assigned earlier to be used later. */
   rpg_hd->status = gbd->base.status;

   /* Assign RDA elevation number.  This is the cut sequence number
      of the VCP. */
   rpg_hd->elev_num = gbd->base.elev_num;

   /* Set the number of moments (additional items other than the standard
      three moments) to 0.  This value will be set when the data is moved. */
   rpg_hd->no_moments = 0;

   /* Process elevation/azimuth data. */
   Process_angle_data( gbd, rpg_hd );

   /* The elevation number must be greater than 0 and less than or equal the
      number expected. */
   if( (rpg_hd->elev_num == 0)
                || 
       (rpg_hd->elev_num > Number_elevations) ){

      /* Send status information.  Do not process this radial. */
      LE_send_msg (GL_STATUS | LE_RPG_WARN_STATUS, "Unexpected Number Of Elevations (%d)\n", 
                   rpg_hd->elev_num);
      return (-1);

   }

   /* Set last elevation flag if last elevation cut of the VCP or the 
      radial status indicates its the last elevation in the cut. */
   if( (rpg_hd->elev_num == Number_elevations) 
                         || 
       (rpg_hd->status == GOODBELLC)){

      rpg_hd->last_ele_flag = 1;
      if( rpg_hd->status == GOODBELLC )
         rpg_hd->status = GOODBEL;

   }
   else
      rpg_hd->last_ele_flag = 0;

   /* Set the RPG elevation index and target elevation.  Note the target
      elevation is a scaled integer (i.e., ang*10).  The RPG elevation
      index treats split cuts as same elevation cut. */
   rpg_hd->rpg_elev_ind = Rpg_elevation_index;

   ftmp = 10. * Target_elevation [rpg_hd->elev_num - 1];
   rpg_hd->target_elev = Round (ftmp);

   /* Set the PBD_DONT_SEND_RADIAL mask in the PBD_start_volume_required
      or PBD_start_elevation_required flag since we don't want to 
      process subsequent radials.  The current radial is being processed
      now.  Setting this mask will not prevent this radial from being
      processed, only subsequent radials. */
   if( (PBD_start_volume_required & PBD_SEND_RADIAL) )
      PBD_start_volume_required = (PBD_DONT_SEND_RADIAL | 1);

   if( (PBD_start_elevation_required & PBD_SEND_RADIAL) )
      PBD_start_elevation_required = (PBD_DONT_SEND_RADIAL | 1);

   /* Set algorithm control flag and the aborted volume scan. */
   rpg_hd->pbd_alg_control = (char) PBD_alg_control; 
   rpg_hd->pbd_aborted_volume = (char) PBD_aborted_volume; 
   if( rpg_hd->pbd_aborted_volume < 0 )
      rpg_hd->pbd_aborted_volume = 0;

   /* If the algorithm control flag indicates algorithms should abort for
      new volume scan and start of volume scan is required, then
      flag this radial as good end of volume.  If the start of volume
      scan flag indicates normal, then this radial is probably an 
      unexpected start of volume scan radial. */
   if( PBD_alg_control == PBD_ABORT_FOR_NEW_VV){

      if( PBD_start_volume_required != (PBD_PROCESS_NORMAL | 0) ){

         LE_send_msg( GL_INFO, "Setting Radial Status To GENDVOL\n" );
         rpg_hd->status = GENDVOL;

      }

   }

   /* Volume scan sequence number, 1-PBD_MAX_SCANS and quotient of the volume 
      sequence number (monotonically increasing sequence number). */
   rpg_hd->volume_scan_num = PBD_volume_scan_number;
   rpg_hd->vol_num_quotient = PBD_volume_seq_number/PBD_MAX_SCANS;

   /* Weather mode. Either Precipitation mode, Clear Air mode, or
      Maintenance mode.  */
   rpg_hd->weather_mode = PBD_rad_wx_mode;

   /* PRF sector number and calibration constant. */
   rpg_hd->sector_num = gbd->base.sector_num;

   /* Populate fields in the RPG radial header taken from the Generic_rad_t 
      data structure, the Generic_elev_t data structure and the Generic_vol_t
      data structure. */
   Process_block_headers( gbd, rpg_hd );

   /* Process moment block headers. */
   if( Process_moment_block_headers( gbd, rpg_hd ) < 0 )
      return (-1);

   /* Spot blank flag and bitmap. */
   rpg_hd->spot_blank_flag = gbd->base.spot_blank_flag;

   /* Bit ordering is from left to right .... bit 0 is LSB.  Bit 1
      corresponds to elevation index 1, bit 2 elevation index 2, etc. */
   if ((rpg_hd->spot_blank_flag & 
       (SPOT_BLANK_RADIAL | SPOT_BLANK_ELEVATION)) != 0)
      PBD_spot_blank_bitmap |= (1 << (31 - Rpg_elevation_index));

   /* If radial count is the maximum, set the radial status to end 
      of elevation or volume. */
   if( Radial_cnt == PBD_max_num_radials ){

      LE_send_msg( GL_STATUS | LE_RPG_WARN_STATUS, 
                   "Number Of Radials In RDA Cut %d >= %d --> Forced End Of El/Vol\n",
                   rpg_hd->elev_num, PBD_max_num_radials );  

      if( rpg_hd->last_ele_flag ){

         rpg_hd->status = GENDVOL;

         PBD_start_volume_required = (PBD_DONT_SEND_RADIAL | 1);
         LE_send_msg( GL_INFO, "---> Start Of Volume Required .... Do Not Send Radial\n" );

      }
      else{

         rpg_hd->status = GENDEL;

         PBD_start_elevation_required = (PBD_DONT_SEND_RADIAL | 1);
         PBD_expected_elev_num = ++rpg_hd->elev_num;
         LE_send_msg( GL_INFO, "---> Start Of Elevation Required .... Do Not Send Radial\n" );

      }

      Old_azi = UNDEFINED_AZI;

   }

   /* Set the type of data this radial is ... i.e., BASEDATA, REFLDATA,
      or COMBBASE. */
   if( (PBD_waveform_type == VCP_WAVEFORM_CD)
                       ||
       ((PBD_waveform_type == VCP_WAVEFORM_STP)
                       &&
                (PBD_split_cut)) )
      rpg_hd->msg_type |= COMBBASE_TYPE;

   else if( (PBD_waveform_type == VCP_WAVEFORM_BATCH)
                       ||
            (PBD_waveform_type == VCP_WAVEFORM_CDBATCH)
                       ||
            (PBD_waveform_type == VCP_WAVEFORM_STP) )
      rpg_hd->msg_type |= BASEDATA_TYPE;

   else if( PBD_waveform_type == VCP_WAVEFORM_CS )
      rpg_hd->msg_type |= REFLDATA_TYPE;

   /* Finish specifying the data type.  Check to see if the data is 
      considered "Super Resolution". */
   if( rpg_hd->azm_reso == BASEDATA_HALF_DEGREE ) 
      rpg_hd->msg_type |= SUPERRES_TYPE;

   /* Check to see if the reflectivity data is high resolution. */
   if( rpg_hd->surv_bin_size == 250 )
      rpg_hd->msg_type |= HIGHRES_REFL_TYPE;

   return (0);

/* End of PH_process_header() */
}

/*********************************************************************

      Description:
	This function sets up the starting and delta angles for this
	radial. It also checks the azimuth increment of this radial
	and puts it in Delta_azimuth for later use. It finally resets
	rpg_hd->status field to be psuedo end of either elevation or 
	volume, if the accumulated scan angle in this elevation reaches
	360 degrees. rpg_hd->status is reset only if it is GOODINT
	(good internal radial). We don't want to overwrite any other
	status info.

      Inputs:
        azi - azimuth in degrees.
        rpg_hd - RPG base data 
        half_azimuth_spacing - 1/2 the radial azimuth spacing.

      Outputs:
        rpg_hd - RPG base data 

      Returns:
	This function returns 0 on success or -1 on failure.

*********************************************************************/
static int Find_start_and_delta_angles ( float azi, Base_data_header *rpg_hd,
                                         float half_azimuth_spacing ){

    static float st_ang, end_ang;	/* angles of starting and ending 
					   edges of this radial */
    static float scan_angle = 0.0f;	/* accumulate azimuth scan angle 
					   in this elevation */
    float delta;
    int idelta;

    /* First radial in an elevation */
    if (Radial_cnt == 1)
	st_ang = azi - half_azimuth_spacing;

    else
	st_ang = end_ang;

    /* Check for wrap around 360 degrees */
    if (st_ang < 0.)
	st_ang += 360.;

    else if (st_ang >= 360.)
        st_ang -= 360.;

    rpg_hd->start_angle = Round( st_ang*10.0 );

    end_ang = azi + half_azimuth_spacing;
    delta = end_ang - st_ang;
    idelta = Round( end_ang*10.0 ) - rpg_hd->start_angle;

    /* Check for warp around 360 degrees */
    if( delta < 0.0 ){	

	delta += 360.;
	idelta += 3600;

    }

    rpg_hd->delta_angle = idelta;

    /* Save delta for future azimuth change check */
    Delta_azimuth = delta;

    /* Check if this elevation scan reaches 360 degrees */
    if (Radial_cnt == 1)	/* reset scaned angle */
	scan_angle = delta;

    else{

        /* Check if this is a fat radial.  If this is a fat radial, we do not want
           to include in the computation for determining psuedo end of elevation or
           volume since this radial status is also passed to downstream consumers. */
        if ( Delta_azimuth <= DELTA_AZI_TOLERANCE ) {

   	   scan_angle += delta;

	   /* If almost more than 360 degrees are scaned, set pseudo end of 
	      elevation or volume */
	   if (scan_angle >= 360.){

	       if (rpg_hd->status == GOODINT) {

	      	   if (rpg_hd->elev_num == Number_elevations){

		       /* The last elevation */
		       rpg_hd->status = PGENDVOL;
                       LE_send_msg( GL_INFO, "Marking Azi#/El# %d Pseudo EOV\n",
                                    rpg_hd->azi_num, rpg_hd->elev_num );

                   }
		   else{

		       rpg_hd->status = PGENDEL;
                       LE_send_msg( GL_INFO, "Marking Azi#/El# %d/%d Pseudo EOE\n",
                                    rpg_hd->azi_num, rpg_hd->elev_num );

                   }
		   scan_angle = 0.;

	       }

	   }

       }

    }

    return (0);

/* End of Find_start_and_delta_angles() */
}

/*********************************************************************

      Description:
	This function returns the round-off integer of a float point
	number.

      Inputs: 
        r - a floating number.

      Outputs:

      Returns: 
	This function returns the round-off integer of a float point
	number.

**********************************************************************/
int Round ( float r /* a float number */) {

#ifdef LINUX
    return( (int) roundf( r ) );
#else
    if ((float)r >= 0.)
	return ((int)(r + .5));
    else 
	return (-(int)((-r) + .5));
#endif

/* End of Round() */
}

/*************************************************************************

      Description:
        This function sets up the radial accounting data.
        Some of the data that is maintained is:
       
           1) Start Date/Time of each elevation
           2) Start/Stop Azimuth of each elevation
           3) Number of radials in each elevation
           4) Pseudo end of elevation/volume
           5) Calibration constant
           6) Nyquist velocity
           7) Weather mode and VCP
           8) Atmospheric attenuation constant

      Inputs:
        rpg_hd - RPG radial header
        rda_msg - RDA radial message

      Outputs:

      Returns: 
        There is no return value defined for this function.

*****************************************************************************/
void PH_radial_accounting( Base_data_header *rpg_hd, char *rda_msg ){

   static int old_accsecnum;
   static Radial_accounting_data *accdata = NULL;
   static float start_elev_azm = 0.0;
   int accvsnum, accelnum, accsecnum;
   int bytes_written;

   /* If at the beginning of volume, then ... */
   if( rpg_hd->status == GOODBVOL && rpg_hd->elev_num == 1 ){

      /* Check if the accounting data pointer is non-zero.  If
         it is, there must have been an unexpected start of volume
         scan.  Free the memory associated with the pointer, and 
         set pointer to NULL. */
      if( accdata != NULL ){

         free( accdata );
         accdata = NULL;

      } 
 
      /* Allocate accounting data buffer. */
      accdata = (Radial_accounting_data *) 
                calloc( 1, sizeof( Radial_accounting_data ) );

      if( accdata == NULL ){

         LE_send_msg( GL_MEMORY, 
            "Calloc Failed.  Accounting Data Unavailable.\n" );
         return;

      }

   }
      
   /* Do no further processing if accdata pointer is NULL. */
   if( accdata == NULL ) return;

   /* Set volume scan number. */
   accvsnum = rpg_hd->volume_scan_num - 1;

   /* Set RDA elevation number. */
   accelnum = rpg_hd->elev_num - 1;

   /* If at beginning of elevation or volume, ..... */
   if( rpg_hd->status == GOODBVOL || rpg_hd->status == GOODBEL ){

      /* Extract the elevation start time/date and starting azimuth. */ 
      accdata->acceltms[accelnum] = rpg_hd->time;
      accdata->acceldts[accelnum] = rpg_hd->date;
      accdata->accbegaz[accelnum] = rpg_hd->azimuth;

      /* Save start of elevation azimuth for radial accounting. */
      start_elev_azm = rpg_hd->azimuth;

      /* Set radial counter to 0, clear the pseudo-end azimuth,
         and initialize the elevation angle sum. */
      accdata->accnumrd[accelnum] = 0;
      accdata->accpendaz[accelnum] = 0.0;
      accdata->accbegel[accelnum] = 0.0;

      /* Initialize all the PRF sector start angles to invalid 
         azimuth (i.e., -1.0). */
      for( accsecnum = 0; accsecnum < MAX_SECTS; accsecnum++ )
         accdata->accsecsaz[accelnum][accsecnum] = -1.0;

      /* Set the old sector number to -1. */
      old_accsecnum = -1;

      if( rpg_hd->status == GOODBVOL && rpg_hd->elev_num == 1){

         /* Extract the volume scan start date. */ 
         accdata->accvsdate = rpg_hd->date;

         /* Extract the calibration constant. */
         accdata->acccalib = rpg_hd->calib_const;

         /* Set the volume coverage pattern. */
         accdata->accvcpnum = rpg_hd->vcp_num;

         /* Set the weather mode. */
         accdata->accwxmode = rpg_hd->weather_mode;

         /* Set the threshold parameter. */
         accdata->accthrparm = rpg_hd->vel_tover;

         /* Clear the number of elevation cuts this volume scan. */
         accdata->accnumel = 0;

      }

   }

   /* If at pseudo end of elevation or volume, .... */
   else if( rpg_hd->status == PGENDVOL || rpg_hd->status == PGENDEL ){
   
      /* Extract pseudo end azimuth. */
      accdata->accpendaz[accelnum] = rpg_hd->azimuth;

   }

   /* If at end of elevation or volume, ... */
   else if( rpg_hd->status == GENDEL || rpg_hd->status == GENDVOL ){
   
      /* Extract the end time and the ending azimuth. */
      accdata->acceltme[accelnum] = rpg_hd->time;
      accdata->acceldte[accelnum] = rpg_hd->date;
      accdata->accendaz[accelnum] = rpg_hd->azimuth;

      /* Set the number of elevation cuts this volume scan. */
      accdata->accnumel = rpg_hd->elev_num;

   }

   /* Increment the number of radials this elevation. */
   accdata->accnumrd[accelnum] += 1; 

   /* Add the elevation angle to the elevation sum. */
   accdata->accbegel[accelnum] += rpg_hd->elevation; 

   /* Extract the atmospheric attenuation constant. */
   accdata->accatmos[accelnum] = rpg_hd->atmos_atten; 

   /* Extract the PRF sector number.  The sector number varies
      between 1 and 3.  Make this number vary between 0 and 2. */
   accsecnum = rpg_hd->sector_num - 1; 

   /* Needed to process some old time NEXRAD data tapes. */
   if( accsecnum < 0 )
      accsecnum = 0;

  /* Compare the sector number with the previous radial sector
     number.  If the sector number has changed, record the 
     starting azimuth angle. */
   if( accsecnum != old_accsecnum )
      accdata->accsecsaz[accelnum][accsecnum] = rpg_hd->azimuth;

   old_accsecnum = accsecnum;

   /* Extract the unambiguous range for this PRF sector. */
   accdata->accunrng[accelnum][accsecnum] = rpg_hd->unamb_range; 

   if( rpg_hd->n_dop_bins > 0 ){

      /* Set the Doppler data resolution. */
      accdata->accdopres = rpg_hd->dop_resolution;

   }

   /* Extract the Nyquist velocity for this PRF sector. */
   accdata->accunvel[accelnum][accsecnum] = rpg_hd->nyquist_vel; 
   
   /* Post an event for radial accounting ... Used primarily by HCI. */
   if( (rpg_hd->status == GOODBVOL) || (rpg_hd->status == GOODBEL) ||
       (rpg_hd->status == GENDEL) || (rpg_hd->status == GENDVOL) ||
       ((accdata->accnumrd[accelnum] % 10) == 0) ){

      Orpgevt_radial_acct_t orpgevt_radial_acct;
      char *data = NULL;
      
      orpgevt_radial_acct.azimuth = (short) Round( (rpg_hd->azimuth*10.0) );
      orpgevt_radial_acct.azi_num = (short) rpg_hd->azi_num;
      orpgevt_radial_acct.elevation = (short) Round( (rpg_hd->elevation*10.0) );
      orpgevt_radial_acct.elev_num = (short) rpg_hd->elev_num;
      orpgevt_radial_acct.radial_status = (short) rpg_hd->status;
      orpgevt_radial_acct.super_res = (short) PBD_super_res_this_elev;
      orpgevt_radial_acct.start_elev_azm = (short) Round( start_elev_azm*10.0 );
      orpgevt_radial_acct.moments = 0;

      /* Determine which moments/data are available in the RDA radial. */
      if( (data = ORPGGDR_get_data_block( rda_msg, ORPGGDR_DREF )) != NULL )
         orpgevt_radial_acct.moments |= RADIAL_ACCT_REFLECTIVITY;

      if( (data = ORPGGDR_get_data_block( rda_msg, ORPGGDR_DVEL )) != NULL )
         orpgevt_radial_acct.moments |= RADIAL_ACCT_VELOCITY;

      if( (data = ORPGGDR_get_data_block( rda_msg, ORPGGDR_DSW )) != NULL )
         orpgevt_radial_acct.moments |= RADIAL_ACCT_WIDTH;

      if( ((data = ORPGGDR_get_data_block( rda_msg, ORPGGDR_DZDR )) != NULL ) 
                                 ||
          ((data = ORPGGDR_get_data_block( rda_msg, ORPGGDR_DRHO )) != NULL )
                                 ||
          ((data = ORPGGDR_get_data_block( rda_msg, ORPGGDR_DPHI )) != NULL ) )
         orpgevt_radial_acct.moments |= RADIAL_ACCT_DUALPOL;

      /* Post the event. */
      EN_post( ORPGEVT_RADIAL_ACCT, (void *) &orpgevt_radial_acct,
               ORPGEVT_RADIAL_ACCT_LEN, (int) 0 );

   }

   /* If at the end of elevation or volume, write this data to 
      linear buffer. */   
   if( rpg_hd->status == GENDEL || rpg_hd->status == GENDVOL ){

      /* Find the average elevation angle for this cut. */
      accdata->accbegel[accelnum] = 
         accdata->accbegel[accelnum]/accdata->accnumrd[accelnum];
     
      bytes_written = ORPGDA_write( ORPGDAT_ACCDATA, 
                                    (char *) accdata, 
                                    sizeof( Radial_accounting_data ),
                                    accvsnum );
     
      /* Check for errors. */
      if( bytes_written <= 0 ) 
         LE_send_msg( GL_ORPGDA(bytes_written), 
                      "Accounting Data Write Failed. Return = %d\n", bytes_written );

      /* Free accdata data memory if at end of volume scan. */
      if( rpg_hd->status == GENDVOL ){

         if( accdata != NULL ){

            free( accdata );
            accdata = NULL;

         }

      }

   }

   /* Return */
   return;

/* End of Radial_accounting() */
}

/*******************************************************************

   Description:
      This function extracts fields from the RDA status message.

   Inputs:
      item - status item.

   Outputs:

   Returns:
      Given the item, if ORPGDARDA returns ORPGRDA_DATA_NOT_FOUND,
      this function returns "UNKNOWN".

*******************************************************************/
int PH_get_rda_status( int item ){

   int status;

   status = ORPGRDA_get_status( item );
   if( status == ORPGRDA_DATA_NOT_FOUND ){

      switch( item ){

         case RS_RDA_STATUS:
            return (RDA_STATUS_UNKNOWN);

         case RS_CONTROL_STATUS:
            return (RDA_CONTROL_UNKNOWN);

         default: 
            return (0);

      /* End of "switch" */
      }

   }

   return( status );

/* End of PH_get_rda_status() */
}

/*******************************************************************

   Description:
      This function issues an RDA control command to either restart
      the volume scan or restart the elevation scan.

   Inputs:
      parameter_1 - Control command (volume restart or elevation
                    restart).
      parameter_2 - If elevation restart, the elevation to restart.

   Outputs:

   Returns:
      Returns negative value if RDA is in LOCAL control, or 0.

*******************************************************************/
int PH_restart_scan( int parameter_1, int parameter_2 ){

   int status, control_status;

   /* Get the RDA status and control status. */
   control_status = PH_get_rda_status( RS_CONTROL_STATUS ); 
   status = PH_get_rda_status( RS_RDA_STATUS );

   /* If RDA is not in LOCAL control and the archive II status is not
      PLAYBACK, then send restart command to the RDA. */
   if( (control_status != RDA_CONTROL_LOCAL) 
                          && 
      !(status & RDA_STATUS_PLAYBACK) ){

      if( ORPGRDA_send_cmd( COM4_RDACOM, PBD_INITIATED_RDA_CTRL_CMD, parameter_1,
                        parameter_2, 0, 0, 0, NULL ) >= 0 ){

         if( parameter_1 == CRDA_RESTART_VCP )
            LE_send_msg( GL_INFO, "Commanding RDA to Restart Volume Scan.\n" );

         else if( parameter_1 == CRDA_RESTART_ELEV ) 
            LE_send_msg( GL_INFO, "Commanding RDA to Restart RDA Elevation Cut %d\n",
                         parameter_2 );

      }
      else
         LE_send_msg( GL_ERROR, "Failed to Send Restart RDA Control Command\n" );

      /* Return normal completion. */
      return (0);

   }

   /* Return -1 if RDA is in LOCAL control or PLAYBACK mode */
   if( (control_status == RDA_CONTROL_LOCAL)
                          ||
       (status & RDA_STATUS_PLAYBACK) ){

      if( control_status == RDA_CONTROL_LOCAL )
         LE_send_msg( GL_INFO, "RDA in LOCAL CONTROL\n" );
      else
         LE_send_msg( GL_INFO, "RDA in PLAYBACK Mode\n" );

      return (-1);

   }
   else
      return (0);

/* End of PH_restart_scan() */
}

/*******************************************************************

   Description:
      This function sets parameters for the RDA control command.
      If the elevation number is the first elevation of the cut
      or there was a commanded volume restart, send a volume 
      restart RDA control command.  Otherwise, send elevation
      restart.

   Inputs:
      elev_num - Current elevation number.
      command - Desired command.  A 0 indicates no command desired,
                i.e., let this module figure out what to do.
      reason - reason the control command is being sent.

   Outputs:

   Returns:
      Returns return value from PH_restart_scan call, or 0.

*******************************************************************/
int PH_send_rda_control_command( int elev_num, int command, int reason ){

   int parameter_1, parameter_2;
   int ret = 0;

   if( elev_num == 1 || command == CRDA_RESTART_VCP ){

      /* If the elevation to restart is the lowest, issue volume 
         restart rda control command. */
      parameter_1 = (int) CRDA_RESTART_VCP;
      parameter_2 = (int) 0;

   }
   else{ 

      /* If command is for restart elevation and a restart of volume 
         has already been commanded, ignore the restart elevation.   
         In PH_radial_validation(), there are several conditions to 
         cause volume restarts.  This function gets called before by 
         other validation routines (e.g., fat radial check) so if a 
         command to restart volume has already been posted, posting 
         a restart of elevation may cause problems.  Therefore we 
         ignore the restart elevation if a restart volume is required. */
      if( PBD_start_volume_required & 0x1 ){

         LE_send_msg( GL_INFO, "Start of Volume Required.  Ignore Restart Elevation.\n" );
         return 0;

      }
      else{

         /* If the elevation to restart is not the lowest elevation, 
            issue elevation restart. */
         parameter_1 = (int) CRDA_RESTART_ELEV;
         parameter_2 = (int) elev_num;

      }

   }

   /* If control status is LOCAL or the status is PLAYBACK, the following
      call returns number < 0. */
   if( (ret = PH_restart_scan( parameter_1, parameter_2 )) < 0 ){

      /* The RDA is in LOCAL control or in PLAYBACK state so we must wait 
         until the next beginning of volume.  Tasks must be aborted. */ 
      PBD_alg_control = reason | PBD_ABORT_FOR_NEW_VV;
      PBD_start_volume_required = (PBD_SEND_RADIAL | 1);
      if( PBD_aborted_volume != PBD_volume_scan_number )
         LE_send_msg( GL_STATUS | LE_RPG_WARN_STATUS, "RPG Task Cleanup For Volume Scan %d\n",
                      PBD_volume_scan_number );
      PBD_aborted_volume = PBD_volume_scan_number;

   }
   else if ( parameter_1 == CRDA_RESTART_VCP ){

      /* Set start_volume_required flag.  Note the algorithm control flag
         is set because we want the algorithms to abort in this case. */
      PBD_alg_control = reason | PBD_ABORT_FOR_NEW_VV;
      PBD_start_volume_required = (PBD_SEND_RADIAL | 1);
      PBD_start_elevation_required = (PBD_PROCESS_NORMAL | 0);
      PBD_expected_elev_num = 1; 
      if( PBD_aborted_volume != PBD_volume_scan_number )
         LE_send_msg( GL_STATUS | LE_RPG_WARN_STATUS, "RPG Task Cleanup For Volume Scan %d\n",
                      PBD_volume_scan_number );
      PBD_aborted_volume = PBD_volume_scan_number;

   }
   else{

      /* Set start_elevation_required flag.  The elevation cut restarted 
         must match that which was aborted.  Note the algorithm control flag
         is not set because we do not want the algorithms to abort in this case. */
      PBD_start_volume_required = (PBD_PROCESS_NORMAL | 0);
      PBD_start_elevation_required = (PBD_SEND_RADIAL | 1);
      PBD_expected_elev_num = (int) elev_num; 

   }

   /* Return return value from PH_restart_scan. */
   return( ret );

/* End of PH_send_rda_control_command() */
}

/**********************************************************************************

   Description:
      This function processes the RESTART_VOLUME and RESTART ELEVATION commands
      received by pbd.

      If servicing the command, the ORPGEVT_END_OF_VOLUME event gets posted on
      RESTART_VOLUME command or RESTART_ELEVATION command and the elevation 
      index is the lowest.

   Inputs:
      command - the command.  Either RESTART_VOLUME or RESTART_ELEVATION.
 
*********************************************************************************/
int PH_process_restart_command( int command ){

   int ret;
   orpgevt_end_of_volume_t eov;

   switch( command ){

      case RESTART_VOLUME:
      {

         ret = PH_send_rda_control_command( (int) 0, (int) CRDA_RESTART_VCP,
                                            (int) PBD_ABORT_VCP_RESTART_COMMANDED );

         /* Post an end of volume scan event (but with volume aborted). */
         eov.vol_aborted = 1;
         eov.vol_seq_num = PBD_volume_seq_number;
         eov.expected_vol_dur = 0;

         ret = EN_post( ORPGEVT_END_OF_VOLUME, (void *) &eov,
                        ORPGEVT_END_OF_VOLUME_DATA_LEN, (int ) 0 );

         if( ret < 0 )
            LE_send_msg( GL_EN(ret), "Event ORPGEVT_END_OF_VOLUME Failed (%d)\n", ret );

      }
      break;

      case RESTART_ELEVATION:
      {

         if( PBD_current_elev_num == 1 ){

            ret = PH_send_rda_control_command( (int) 0, (int) CRDA_RESTART_VCP,
                                               (int) PBD_ABORT_VCP_RESTART_COMMANDED );

            /* Post an end of volume scan event (but with volume aborted). */
            eov.vol_aborted = 1;
            eov.vol_seq_num = PBD_volume_seq_number;
            eov.expected_vol_dur = 0;

            ret = EN_post( ORPGEVT_END_OF_VOLUME, (void *) &eov,
                           ORPGEVT_END_OF_VOLUME_DATA_LEN, (int ) 0 );

            if( ret < 0 )
               LE_send_msg( GL_EN(ret), "Event ORPGEVT_END_OF_VOLUME Failed (%d)\n", ret );

         }
         else
            PH_send_rda_control_command( PBD_current_elev_num, (int) CRDA_RESTART_ELEV,
                                         PBD_ABORT_VCP_RESTART_COMMANDED );

         break;

      }

      default:
         break;

   /* End of "switch" */
   }

   return (0);

/* End of PH_process_restart_command() */
}

/*******************************************************************

   Description:
      This function performs radial validation based on the value
      of radial status.  

   Inputs:
      gbd - Generic format RDA radial.

   Outputs:
      vol_aborted - Flag denoting whether previous volume scan 
                    completed or not.
      unexpected_bov - flag, if set, indicates unexpected 
                       beginning of volume.

   Returns:
      It returns PBD_NORMAL on success or, PBD_NEW_FAILURE for new failure,
      or PBD_CONTINUING_FAILURE for continuing failures. 

**********************************************************************/
int PH_radial_validation ( Generic_basedata_t *gbd, int *vol_aborted,
                           int *unexpected_bov ) {

   /* If either start of volume required and beginning of volume not 
      detected or start of elevation required and start of elevation
      not detected, return continuing failure. */
   if( (PBD_start_elevation_required 
                  && 
       ((gbd->base.status != GOODBEL) 
                  && 
        (gbd->base.status != GOODBVOL)
                  &&
        (gbd->base.status != GOODBELLC)))
                      ||
       (PBD_start_volume_required && (gbd->base.status != GOODBVOL)) ){

      /* Is this the first time through this module after a new failure? */ 
      if( (PBD_start_elevation_required & PBD_SEND_RADIAL) ){

         PBD_start_elevation_required = (PBD_DONT_SEND_RADIAL | 1);
         return(PBD_NEW_FAILURE);

      }
      else if( (PBD_start_volume_required & PBD_SEND_RADIAL) ){

         PBD_start_volume_required = (PBD_DONT_SEND_RADIAL | 1);
         return (PBD_NEW_FAILURE);

      } 

      return (PBD_CONTINUING_FAILURE);

   }

   /* Initialize the algorithm control flag to PB_NO_ABORT. */
   PBD_alg_control = PBD_ABORT_NO;

   /* Process case statement based on radial status. */
   switch( gbd->base.status ){

      /* Good intermediate radial. */
      case GOODINT:
      {

         /* Initialize the aborted volume scan number. */
         PBD_aborted_volume = -1;

         /* Verify the cut number against what is expected.  If not the
            same, generate error message and restart VCP.  NOTE:  The
            PBD_start_volume_required and PBD_alg_control flags are set 
            in the function PH_send_rda_control_command. */
         if( (int) gbd->base.elev_num != PBD_current_elev_num ){
   
            /* Mismatch on rda elevation cut number.  Report error. */
            LE_send_msg( GL_STATUS | LE_RPG_WARN_STATUS,
                         "RDA Elev # Sequence Error EL#/EEL# %2d/%2d\n",
                         gbd->base.elev_num, PBD_current_elev_num );
   
            /* Issue volume restart if control is REMOTE and not PLAYBACK. */
            PH_send_rda_control_command( (int) gbd->base.elev_num,
                                         (int) CRDA_RESTART_VCP,
                                         (int) PBD_ABORT_INPUT_DATA_ERROR );

            return( PBD_NEW_FAILURE );

         }

         break;
      }

      /* Good beginning of elevation scan. */
      case GOODBEL:
      case GOODBELLC:
      {

         int restart_vcp = 0;

         /* Initialize the aborted volume scan number. */
         PBD_aborted_volume = -1;

         /* Was start of volume scan required? */
         if ( PBD_start_volume_required ) {

            /* The expected start of volume scan did not occur. */
	    LE_send_msg ( GL_STATUS | LE_RPG_WARN_STATUS, 
                "Start of RDA Elev # %d Received, Start of Volume Scan Expected\n",
                gbd->base.elev_num );
            restart_vcp = 1;

         }

         /* Was start of elevation unexpected? */ 
         else if( !PBD_start_elevation_required ){

            /* The start of elevation was unexpected. */
	    LE_send_msg ( GL_STATUS | LE_RPG_WARN_STATUS, 
                 "Unexpected Start of RDA Elev # %d\n", gbd->base.elev_num );
            restart_vcp = 1;

         }

         /* Verify the elevation cut number. */
         else if( (int) gbd->base.elev_num != PBD_expected_elev_num ){
   
            /* Mismatch on rda elevation cut number. */
            LE_send_msg( GL_STATUS | LE_RPG_WARN_STATUS,
                 "Start of RDA Elev # %d Expected, RDA Elev # %d Received\n", 
                 PBD_expected_elev_num, gbd->base.elev_num );
            restart_vcp = 1;

         }

         /* Is a VCP restart required because of some error? */
         if( restart_vcp ){

            /* Restart VCP if control is REMOTE and not PLAYBACK.  
               NOTE:  The PBD_start_volume_required and PBD_alg_control
               flags are set in the function PH_send_rda_control_command. */
            PH_send_rda_control_command( (int) gbd->base.elev_num,
                                         (int) CRDA_RESTART_VCP,
                                         (int) PBD_ABORT_INPUT_DATA_ERROR );
            return( PBD_NEW_FAILURE );

         }

         /* If here, expected start of elevation occurred. */
         PBD_start_elevation_required = (PBD_NORMAL | 0);

         break;
      }

      /* Good end of elevation scan. */
      case GENDEL:
      {

         /* Initialize the aborted volume scan number. */
         PBD_aborted_volume = -1;

         /* Verify the elevation cut number. */
         if( (int) gbd->base.elev_num != PBD_current_elev_num ){
   
            /* Mismatch on rda elevation cut number. */
            LE_send_msg( GL_STATUS | LE_RPG_WARN_STATUS,
                 "End of RDA Elev # %d Expected, RDA Elev # %d Received\n", 
                 PBD_current_elev_num, gbd->base.elev_num );

            /* Restart VCP if control is REMOTE and not PLAYBACK.  
               NOTE:  The PBD_start_volume_required and PBD_alg_control
               flags are set in the function PH_send_rda_control_command. */
            PH_send_rda_control_command( (int) gbd->base.elev_num,
                                         (int) CRDA_RESTART_VCP,
                                         (int) PBD_ABORT_INPUT_DATA_ERROR );
            return( PBD_NEW_FAILURE );

         }

         /* Normal end of elevation.  Set the start of elevation flag and 
            the expected elevation number. */
         PBD_start_elevation_required = (PBD_NORMAL | 1);
         PBD_expected_elev_num = PBD_current_elev_num + 1;

         break;
      }

      /* Good end of elevation scan. */
      case GENDVOL:
      {

         /* Initialize the aborted volume scan number. */
         PBD_aborted_volume = -1;

         /* Verify the elevation cut number. */
         if( (int) gbd->base.elev_num != PBD_current_elev_num ){
   
            /* Mismatch on rda elevation cut number. */
            LE_send_msg( GL_STATUS | LE_RPG_WARN_STATUS,
                 "End of RDA Elev # %d Expected, RDA Elev # %d Received\n", 
                 PBD_current_elev_num, gbd->base.elev_num );

            /* Restart VCP if control is REMOTE and not PLAYBACK.  
               NOTE:  The PBD_start_volume_required and PBD_alg_control
               flags are set in the function PH_send_rda_control_command. */
            PH_send_rda_control_command( (int) gbd->base.elev_num,
                                         (int) CRDA_RESTART_VCP,
                                         (int) PBD_ABORT_INPUT_DATA_ERROR );
            return( PBD_NEW_FAILURE );

         }

         /* Set the start of volume flag and the expected elevation number. */
         PBD_start_volume_required = (PBD_SEND_RADIAL | 1);
         PBD_expected_elev_num = 1;

         break;
      }
   
      /* Good beginning of volume scan. */
      case GOODBVOL:
      {

         /* Was the start of volume scan expected? */
         if( !PBD_start_volume_required ){

            /* Unexpected start of volume scan. Set algorithm control to 
               abort for algorithm clean-up in preparation for new volume. */
            PBD_aborted_volume = PBD_volume_scan_number - 1;
            if( PBD_aborted_volume == 0 )
               PBD_aborted_volume = MAX_SCAN_SUM_VOLS;

            /* Tell the operator about the aborted volume scan. */ 
	    LE_send_msg ( GL_STATUS | LE_RPG_WARN_STATUS, 
                          "Unexpected Start of Volume Scan %d\n",
                          PBD_volume_scan_number );
            LE_send_msg( GL_STATUS | LE_RPG_WARN_STATUS,
                         "RPG Task Cleanup For Volume Scan %d\n",
                         PBD_aborted_volume );

            /* Set the algorithm control flag so downstream consumers of
               basedata can abort/clean-up. */
            PBD_alg_control = PBD_ABORT_UNEXPECTED_VCP_RESTART | PBD_ABORT_FOR_NEW_VV;
            *unexpected_bov = 1;
            *vol_aborted = 1;

         }
         else if( PBD_aborted_volume == (PBD_volume_scan_number - 1) ){

            /* If PBD_aborted_volume is set because of some other reason, set
               the "vol_aborted" flag. */
            *vol_aborted = 1;

         }
         else
            PBD_aborted_volume = -1;

         /* The start of volume scan occurred as required. */
         PBD_start_volume_required = (PBD_PROCESS_NORMAL | 0);
         PBD_start_elevation_required = (PBD_PROCESS_NORMAL | 0);

         break;
      }

   /* End of "switch" */
   }

   /* Successful return. */
   return (PBD_NORMAL);

/* End of PD_radial_validation() */
}

/*********************************************************************************

  Description:
      Checks whether volume status has been updated (meaning updated by some
      process other than pbd).

      If the volume status has been updated, the volume status is read.  The
      VCP number in the volume status is checked against the current VCP number.
      If there is a match, the VCP data in the volume status is copied to the
      the local copy of volume status.

   Inputs:
      vs_gsm - pointer to local copy of volume status.

   Returns:
      There is no return value defined for this function.

*********************************************************************************/
void Read_and_update_current_vcp_table( Vol_stat_gsm_t *vs_gsm ){

   int ret;
   static Vol_stat_gsm_t vol_status;

   /* If the volume status data hasn't been updated to outside processes,
      there is nothing to do. */
   if( !PBD_volume_status_updated )
      return;

   PBD_volume_status_updated = 0;

   /* Read the Volume Status. */
   ret = ORPGDA_read( ORPGDAT_GSM_DATA, (char *) &vol_status, sizeof(Vol_stat_gsm_t),
                      VOL_STAT_GSM_ID );
   if( ret <= 0 ){

      LE_send_msg( GL_INFO, "Read_and_update_current_vcp_table ORPGDA_read Failed (%d)\n",
                   ret );
      return;

   }

   /* Check the vcp number in volume status. */
   if( vol_status.vol_cov_patt != vs_gsm->vol_cov_patt )
      return;

   /* Copy the current volume scan table to local volume status. */
   memcpy( (void *) &vs_gsm->current_vcp_table, (void *) &vol_status.current_vcp_table,
           sizeof( Vcp_struct ) );

   LE_send_msg( GL_INFO, "Updated Current VCP Table From Volume Status Update\n" );

/* End of Read_and_update_current_vcp_table(). */
}

/**********************************************************************************

   Description:
      This function sets fields in the RPG radial header relating to azimuth
      and elevation.

   Inputs:
      gbd - Generic basedata radial.

   Outputs:
      rpg_hd - RPG radial header.

   Returns:
      There is no return value defined for this function.

**********************************************************************************/
static void Process_angle_data( Generic_basedata_t *gbd, Base_data_header *rpg_hd ){


   /* Needed for delta azimuth and delta elevation tolerance checks. */
   static float az_tolerance = DELTA_AZI_TOLERANCE;
   static float elv_tolerance_warning = DELTA_ELV_TOLERANCE;
   static float elv_tolerance_alarm = DELTA_ELV_TOLERANCE;
   static float half_azimuth_spacing = 0.5f;

   float diff;
   float azi;

   /* Setup the radial count and reset azimuth number at beginning of
      elevation/volume. */
   if( (gbd->base.status == GOODBEL) || (gbd->base.status == GOODBVOL) ){

      double dtemp;

      /* Set the value for half of the azimuth spacing.  This value is used in
         Find_start_and_delta_angles(). */
      if( gbd->base.azimuth_res == BASEDATA_ONE_DEGREE )
         half_azimuth_spacing = 0.5f;

      else
         half_azimuth_spacing = 0.25f;

      if( DEAU_get_values( "pbd.azm_tolerance", &dtemp, 1 ) < 0 ){

         LE_send_msg( GL_ERROR, "DEAU_get_values( pbd.azm_tolerance ) Failed\n" );
         az_tolerance = DELTA_AZI_TOLERANCE;

      }
      else
         az_tolerance = (float) dtemp;

      if( DEAU_get_values( "pbd.elv_tolerance_warning", &dtemp, 1 ) < 0 ){

         LE_send_msg( GL_ERROR, "DEAU_get_values( pbd.elv_tolerance_warning ) Failed\n" );
         elv_tolerance_warning = DELTA_ELV_TOLERANCE;

      }
      else
         elv_tolerance_warning = (float) dtemp;

      if( DEAU_get_values( "pbd.elv_tolerance_alarm", &dtemp, 1 ) < 0 ){

         LE_send_msg( GL_ERROR, "DEAU_get_values( pbd.elv_tolerance_alarm ) Failed\n" );
         elv_tolerance_alarm = DELTA_ELV_TOLERANCE;

      }
      else
         elv_tolerance_alarm = (float) dtemp;

      /* Make sure the warning level is less than or equal to alarm level. */
      if( elv_tolerance_warning > elv_tolerance_alarm )
         elv_tolerance_warning = elv_tolerance_alarm;

   }
   else if( (gbd->base.status == GENDEL) || (gbd->base.status == GENDVOL) ){

      /* Reset Old_azi at end of elevation or volume. */
      Old_azi = UNDEFINED_AZI;

   }

   /* Get the radial azimuth. */
   azi = gbd->base.azimuth;

   /* Check for duplicates. */
   if( Old_azi != UNDEFINED_AZI ){

      diff = azi - Old_azi;

      if (diff < 0.)
         diff = -diff;

      if (diff < .001)
         azi = Old_azi + .001;
   }

   Old_azi = azi;

   /* Assign the azimuth angle. */
   rpg_hd->azimuth = azi;

   /* Set sine and cosine of the azimuth angle. */
   rpg_hd->sin_azi = sinf(azi * DEG_TO_RAD);
   rpg_hd->cos_azi = cosf(azi * DEG_TO_RAD);

   /* Set start and delta angles of this azimuth. */
   Find_start_and_delta_angles( azi, rpg_hd, half_azimuth_spacing );

   /* Get the radial elevation. */
   rpg_hd->elevation = gbd->base.elevation;

   if( rpg_hd->elevation > 180.0f )
      rpg_hd->elevation -= 360.0f;

   /* Compare with expected elevation. */
   diff = (float) rpg_hd->elevation - Target_elevation [rpg_hd->elev_num - 1];

   if (diff < 0.)
      diff = -diff;

   /* If difference greater than threshold, report error. */
   if( diff >= elv_tolerance_warning ){

      /* Send status message - not implemented. */
      LE_send_msg( GL_STATUS | LE_RPG_WARN_STATUS,
                   "Elevation Tolerance EXCEEDED (AZ/EL/PEL):  %6.1f/%5.1f/%5.1f",
                   rpg_hd->azimuth, rpg_hd->elevation,
                   Target_elevation [rpg_hd->elev_num - 1] );

      /* We assume anything greater than the alarm level is a problem with
         the encoder and therefore isn't a true elevation reading. */
      if( diff >= elv_tolerance_alarm ){

         /* Reset elevation angle to the target. */
         rpg_hd->elevation = Target_elevation [rpg_hd->elev_num - 1];

      }

   }

   /* Set the sine and cosine of the elevation angle. */
   rpg_hd->sin_ele = sinf( rpg_hd->elevation * DEG_TO_RAD );
   rpg_hd->cos_ele = cosf( rpg_hd->elevation * DEG_TO_RAD );

   /* Has a FAT RADIAL been detected? */
   if ( Delta_azimuth > az_tolerance ) {

      /* Azimuth difference greater than tolerance. */
      LE_send_msg( GL_STATUS | LE_RPG_WARN_STATUS, 
                   "Azimuth Tolerance EXCEEDED (EL/AZ/DELTA):  %4.1f/%6.1f/%6.1f",
                   rpg_hd->elevation, rpg_hd->azimuth, Delta_azimuth );

      /* If the elevation to restart is the lowest, issue volume
         restart rda control command. */
      if( gbd->base.elev_num == 1 )
         PH_send_rda_control_command( (int) gbd->base.elev_num,
                                      (int) CRDA_RESTART_VCP,
                                      (int) PBD_ABORT_INPUT_DATA_ERROR );

      /* If the elevation to restart is not the lowest, issue elevation
         restart. */
      else
         PH_send_rda_control_command( (int) gbd->base.elev_num,
                                      (int) CRDA_RESTART_ELEV,
                                      (int) PBD_ABORT_INPUT_DATA_ERROR );


   }

/* End of Process_angle_data(). */
}

/**********************************************************************************

   Description:
      This function sets fields in the RPG radial header from information in
      the radial block header, the elevation block header and the volume block
      header.

   Inputs:
      gbd - Generic basedata radial.

   Outputs:
      rpg_hd - RPG radial header.

   Returns:
      There is no return value defined for this function.

**********************************************************************************/
static void Process_block_headers( Generic_basedata_t *gbd, Base_data_header *rpg_hd ){

   Generic_rad_t *rad_hdr = NULL;
   Generic_vol_t *vol_hdr = NULL;
   Generic_elev_t *elev_hdr = NULL;

   /* Transfer data from radial block header. */
   rad_hdr = (Generic_rad_t *) ORPGGDR_get_data_block( (char *) gbd, ORPGGDR_RRAD );
   if( rad_hdr != NULL ){

      rpg_hd->unamb_range = rad_hdr->unamb_range;
      rpg_hd->nyquist_vel = rad_hdr->nyquist_vel;
      rpg_hd->horiz_noise = rad_hdr->horiz_noise;
      rpg_hd->vert_noise = rad_hdr->vert_noise;

   }
   else{

      /* This should never happen ..... */
      LE_send_msg( GL_INFO, "Radial Does Not Contain Radial Data Block\n" );

      rpg_hd->unamb_range = 0;
      rpg_hd->nyquist_vel = 0;
      rpg_hd->horiz_noise = 0.0f;
      rpg_hd->vert_noise = 0.0f;

   }

   /* Transfer data from elevation block header. */
   elev_hdr = (Generic_elev_t *) ORPGGDR_get_data_block( (char *) gbd, ORPGGDR_RELV );
   if( elev_hdr != NULL ){

      rpg_hd->calib_const = elev_hdr->calib_const;
      rpg_hd->atmos_atten = elev_hdr->atmos;

   }
   else{

      /* This should never happen ..... */
      LE_send_msg( GL_INFO, "Radial Does Not Contain Elevation Data Block\n" );
      rpg_hd->calib_const = 0.0f;
      rpg_hd->atmos_atten = 0;

   }

   /* Transfer data from volume block header. */
   vol_hdr = (Generic_vol_t *) ORPGGDR_get_data_block( (char *) gbd, ORPGGDR_RVOL );
   if( vol_hdr != NULL ){

      rpg_hd->vcp_num = vol_hdr->vcp_num;
      rpg_hd->version = (vol_hdr->major_version << 8) + vol_hdr->minor_version;
      rpg_hd->latitude = vol_hdr->lat;
      rpg_hd->longitude = vol_hdr->lon;
      rpg_hd->height = (unsigned short) vol_hdr->height;
      rpg_hd->feedhorn_height = (unsigned short) vol_hdr->feedhorn_height;
      rpg_hd->horiz_shv_tx_power = vol_hdr->horiz_shv_tx_power;
      rpg_hd->vert_shv_tx_power = vol_hdr->vert_shv_tx_power;
      rpg_hd->sys_diff_refl = vol_hdr->sys_diff_refl;
      rpg_hd->sys_diff_phase = vol_hdr->sys_diff_phase;

   }
   else{

      /* This should never happen ..... */
      LE_send_msg( GL_INFO, "Radial Does Not Contain Volume Data Block\n" );
      rpg_hd->vcp_num = 0;
      rpg_hd->version = 0;
      rpg_hd->latitude = PBD_latitude;
      rpg_hd->longitude = PBD_longitude;
      rpg_hd->height = PBD_rda_height;
      rpg_hd->feedhorn_height = 20.0f;
      rpg_hd->horiz_shv_tx_power = 0.0f;
      rpg_hd->vert_shv_tx_power = 0.0f;
      rpg_hd->sys_diff_refl = 0.0f;
      rpg_hd->sys_diff_phase = 0.0f;

   }

/* End of Process_block_headers(). */
}

/**********************************************************************************

   Description:
      This function sets fields in the RPG radial header from information in
      the moment block headers. 

   Inputs:
      gbd - Generic basedata radial.

   Outputs:
      rpg_hd - RPG radial header.

   Returns:
      -1 on error, 0 otherwise.

**********************************************************************************/
static int Process_moment_block_headers( Generic_basedata_t *gbd, 
                                         Base_data_header *rpg_hd ){

   Generic_moment_t *mom = NULL;
   int leading_edge_range, diff, first_bin;

   /* Only set a beginning of elevation/ volume. */
   static int max_surv_bins, max_dop_bins, srto70;

   /* Initialize Info data structure. */
   memset( &Info, 0, sizeof(Moment_info_t) );

   /* Get the Reflectivity moment block header. */
   if( PBD_saved_ind >= 0 )
      mom = (Generic_moment_t *) &PBD_saved_ref[PBD_saved_ind].mom;
 
   else
      mom = (Generic_moment_t *) ORPGGDR_get_data_block( (char *) gbd, ORPGGDR_DREF );

   if( mom != NULL ){

      /* Bin size should always be set.  If not, set default bin size. */
      rpg_hd->surv_bin_size = mom->bin_size;
      if( rpg_hd->surv_bin_size <= 0 )
         rpg_hd->surv_bin_size = 1000;
 
      rpg_hd->n_surv_bins = mom->no_of_gates;
      rpg_hd->msg_type |= REF_ENABLED_BIT;

      /* Initialize range to first bin and the bin number to the first bin.  These
         will be set based on "Info". */
      rpg_hd->surv_range = 1;
      rpg_hd->range_beg_surv = 0;

      /* Set the SNR thresholds for reflectivity. */ 
      rpg_hd->surv_snr_thresh = mom->SNR_threshold;

      /* Is there any surveillance data in this radial? */
      if( rpg_hd->n_surv_bins > 0 ){

         /* If first bin is behind radar, adjust the first bin and the number
            of bins;  The first bin must be positive.  If the first bin is in
            front radar, then the number of RDA bins remains the same.  We must
            account for this when we move the data and we pad the front with 0's. */

         /* The leading_edge_range is the range to the beginning of the first
            range bin.  Dividing by the bin size, gives the number of bins
            between 0 range and the start of the data. */
         leading_edge_range = mom->first_gate_range - rpg_hd->surv_bin_size / 2;
         first_bin = Round ((float) leading_edge_range / (float) rpg_hd->surv_bin_size);

         rpg_hd->n_surv_bins += first_bin;

         /* Process accordingly based on whether the first bin is in front, at, or
            behind the radar. */
         if( first_bin < 1 ) {

            Info.rda_surv_bin_off = -first_bin;
            Info.num_surv_bins = rpg_hd->n_surv_bins;

         }
         else {

            Info.rpg_surv_bin_off = first_bin;
            Info.num_surv_bins = rpg_hd->n_surv_bins - first_bin;

         }

         /* Set the surveillance bin size in the Info structure. */
         Info.rda_surv_bin_size = rpg_hd->surv_bin_size;

         /* Set the range to the start of the first surveillance bin, in meters. */
         rpg_hd->range_beg_surv =
                (short) (leading_edge_range - (first_bin*rpg_hd->surv_bin_size));

         /* Set the range to the start of the first surveillance bin, in bins. */
         rpg_hd->surv_range = 1;

      }

   }
   else{

      rpg_hd->surv_bin_size = 1000;
      rpg_hd->n_surv_bins = 0;
      rpg_hd->surv_range = 1;
      rpg_hd->range_beg_surv = 0;

      /* This really is not needed.   Added to be consistent with earlier
         version of this software. */
      rpg_hd->surv_snr_thresh = (short) (ORPGVCP_get_threshold( rpg_hd->vcp_num,
                                                                rpg_hd->elev_num, 
                                                                ORPGVCP_REFLECTIVITY ) * 8.0);

   }

   /* Get the Velocity moment block header. */
   mom = (Generic_moment_t *) ORPGGDR_get_data_block( (char *) gbd, ORPGGDR_DVEL );
   if( mom != NULL ){

      rpg_hd->dop_bin_size = mom->bin_size;
      if( rpg_hd->dop_bin_size <= 0 )
         rpg_hd->dop_bin_size = 250;

      rpg_hd->n_dop_bins = mom->no_of_gates;
      rpg_hd->msg_type |= VEL_ENABLED_BIT;
      rpg_hd->vel_tover = mom->tover;
      rpg_hd->vel_snr_thresh = mom->SNR_threshold;

      rpg_hd->dop_range = 1;
      rpg_hd->range_beg_dop = 0; 

      /* Is there any Doppler data in this radial? */
      if( rpg_hd->n_dop_bins > 0 ){

         /* Range to first bin and number of bins for Doppler. */

         /* The leading_edge_range is the range to the beginning of the first range
            bin.  Dividing by the bin size, gives the number of bins between 0 range
            and the start of the data. */
         leading_edge_range = mom->first_gate_range - rpg_hd->dop_bin_size / 2;
         first_bin = Round ((float) leading_edge_range / (float) rpg_hd->dop_bin_size);

         rpg_hd->n_dop_bins += first_bin;

         /* Process accordingly based on whether the first bin is in front, at, or
            behind the radar. */
         if (first_bin < 1) {

            Info.rda_dop_bin_off = -first_bin;
            Info.num_dop_bins = rpg_hd->n_dop_bins;

         }
         else {

            Info.rpg_dop_bin_off = first_bin;
            Info.num_dop_bins = rpg_hd->n_dop_bins - first_bin;

         }

         /* Set the range to the start of the first Doppler bin, in meters. */
         rpg_hd->range_beg_dop =
             (short) (leading_edge_range - (first_bin*rpg_hd->dop_bin_size));

         /* Set the range to the start of the first Doppler bin, in bins. */
         rpg_hd->dop_range = 1;

         /* We assume the Doppler resolution is either 0.5 or 1.0 m/s. */
         if( mom->scale == 2.0 )
            rpg_hd->dop_resolution = 1;

         else
            rpg_hd->dop_resolution = 2;

      }

   }
   else{

      rpg_hd->dop_bin_size = 250;
      rpg_hd->n_dop_bins = 0;
      rpg_hd->dop_range = 1;
      rpg_hd->dop_resolution = 1;
      rpg_hd->vel_tover = 0;

      /* This really is not needed.   Added to be consistent with earlier
         version of this software. */
      rpg_hd->vel_snr_thresh = (short) (ORPGVCP_get_threshold( rpg_hd->vcp_num,
                                                               rpg_hd->elev_num, 
                                                               ORPGVCP_VELOCITY ) * 8.0);

   }

   /* Get the Spectrum Width moment block header. */
   mom = (Generic_moment_t *) ORPGGDR_get_data_block( (char *) gbd, ORPGGDR_DSW );
   if( mom != NULL ){

      rpg_hd->msg_type |= WID_ENABLED_BIT;
      rpg_hd->spw_tover = mom->tover;
      rpg_hd->spw_snr_thresh = mom->SNR_threshold;

   }
   else{

      rpg_hd->spw_tover = 0;

      /* This really is not needed.   Added to be consistent with earlier
         version of this software. */
      rpg_hd->spw_snr_thresh = (short) (ORPGVCP_get_threshold( rpg_hd->vcp_num,
                                                               rpg_hd->elev_num, 
                                                               ORPGVCP_SPECTRUM_WIDTH ) * 8.0);

   }

   /* Find the height to 70,000 ft.   We want to clip the data along a radial
      to the minimum of 70,000 ft and either 230 km or the unambiguous range
      depending on the waveform.  We also need to clip the data to array
      bounds limit. */
   if( (gbd->base.status == GOODBEL) || (gbd->base.status == GOODBVOL) ){

      float sin_ele;
      float rda_height = PBD_rda_height/1000.0f;

      sin_ele = sin ((double) Target_elevation [rpg_hd->elev_num - 1] * DEG_TO_RAD );
      srto70 = (int) (IRRE*(sqrt( (sin_ele*sin_ele) +
                      2.0*(rda_height + TOPHGT_KM)/IRRE ) - sin_ele));

      max_surv_bins = ((srto70*1000) / rpg_hd->surv_bin_size);

      /* Since we don't know how many reflectivity bins there can be, we
         check the number there are.  If the number there are are less
         than or equal to BASEDATA_REF_SIZE, then this is the limit. */
      if( rpg_hd->n_surv_bins <= BASEDATA_REF_SIZE ){

         if( max_surv_bins > BASEDATA_REF_SIZE )
            max_surv_bins = BASEDATA_REF_SIZE;

      }
      else if( max_surv_bins > MAX_BASEDATA_REF_SIZE )
         max_surv_bins = MAX_BASEDATA_REF_SIZE;

      /* For Doppler, the maximum number of bins is limited by BASEDATA_DOP_SIZE. */
      max_dop_bins = ((srto70*1000) / rpg_hd->dop_bin_size);
      if( max_dop_bins > BASEDATA_DOP_SIZE )
         max_dop_bins = BASEDATA_DOP_SIZE;

   }

   /* Is there any surveillance data in this radial? */
   if( rpg_hd->n_surv_bins > 0 ){

      /* Clip the data a 70,000 ft.  Also make sure there isn't more data
         that we have space allocated for. */
      if( (diff = (rpg_hd->n_surv_bins - max_surv_bins)) > 0 ){

         rpg_hd->n_surv_bins -= diff;
         Info.num_surv_bins -= diff;

      }

   }

   /* Is there any Doppler data in this radial? */
   if( rpg_hd->n_dop_bins > 0 ){

      /* Clip the data a 70,000 ft.  Also make sure there isn't more data
         that we have space allocated for. */
      if( (diff = (rpg_hd->n_dop_bins - max_dop_bins)) > 0 ){

         rpg_hd->n_dop_bins -= diff;
         Info.num_dop_bins -= diff;

      }

   }

   /* If the number of surveillance bins or Doppler bins is negative, set
      the number to 0 with the corresponding beginning range and bin number. */
   if (rpg_hd->n_surv_bins < 0){

      rpg_hd->n_surv_bins = 0;
      rpg_hd->surv_range = 1;
      rpg_hd->range_beg_surv = 0;
      Info.num_surv_bins = 0;

   }

   if (rpg_hd->n_dop_bins < 0){

      rpg_hd->n_dop_bins = 0;
      rpg_hd->dop_range = 1;
      rpg_hd->range_beg_dop = 0;
      Info.num_dop_bins = 0;

   }

   /* Make sure that the space in RPG base data buffer is large enough. */
   if( (rpg_hd->n_surv_bins > (int) MAX_BASEDATA_REF_SIZE)
                           ||
       (rpg_hd->n_dop_bins > (int) BASEDATA_DOP_SIZE) ){
      LE_send_msg( GL_STATUS | LE_RPG_WARN_STATUS,
                   "Too Many Bins Found In RDA Radial Message (%d %d )\n",
                   rpg_hd->n_surv_bins, rpg_hd->n_dop_bins );
      return (-1);

   }

   return 0;

/* End of Process_moment_block_headers(). */
}

/**********************************************************************************

   Description:
      Determines whether the cut specified by vcp and elev_num is part of a 
      Doppler split cut.

   Inputs:
      vcp_num - vcp number.
      elev_num - RDA elevation number (0 indexed).

   Returns:
      -1 on error, 0 otherwise.

**********************************************************************************/
static int Get_cut_type( int vcp_num, int elev_num ){

   int waveform;

   /* Set the waveform type based on VCP definition. */
   PBD_waveform_type = ORPGVCP_get_waveform( vcp_num, elev_num );
   PBD_split_cut = 0;

   /* A CD cut or a STP cut can be part of a split cut. */
   if( PBD_waveform_type == VCP_WAVEFORM_CD )
      PBD_split_cut = 1;

   else if( PBD_waveform_type == VCP_WAVEFORM_STP ){
       
      /* Assume if CD or STP cut, then the previous cut must be a CS cut. */
      if( elev_num >= 1 ){
         
         waveform = ORPGVCP_get_waveform( vcp_num, elev_num-1 );
         if( waveform == VCP_WAVEFORM_CS ){

            /* This is a part of a Doppler split cut. */
            PBD_split_cut = 1;

         }

      }

   }

   return 0;

/* End of Get_cut_type(). */
}

