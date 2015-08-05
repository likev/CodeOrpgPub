/* 
 * RCS info
 * $Author: ccalvert $
 * $Locker:  $
 * $Date: 2010/03/17 21:40:09 $
 * $Id: pbd_process_data.c,v 1.1 2010/03/17 21:40:09 ccalvert Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 */  

#include <pbd.h>

#define MAX_ADD_MOMENTS   	5	/* Maximum additional moments in an RPG 
                                	   radial. */

#define MAX_AZI_DIFF		1.0f 	/* Max match error on azimuth (split cuts) */
#define MAX_ELE_DIFF    	0.5f	/* Max match error on elevation (split cuts) */

/* Constants for accelerating azimuth match */
#define TEST_ANGLE		15.0f	/* For large angle difference test in 
				           Search_for_saved_radial */ 

#define N_JUMP			8	/* Number of radials to skip; We assume that
				           the azimuth differences among a contiguous 
				           N_JUMP radials are always less than 
				           TEST_ANGLE degrees */

/* Static Globals. */
static short RPG_elev_cut_num;		/* RPG elevation cut # for saved radial. */	

/* Local Function Prototypes */
static int Search_for_saved_radial( Generic_basedata_t *gbd );
static int Move_additional_data( char *rda_msg, char *rpg_msg );
static int Add_DP_data( char *gbd, char *rpg_radial, int index, 
                        int fields_added );
static int Insert_data( char *rpg_radial, int add_mom, int offset, 
                        int max_bins, Generic_moment_t *mom );
static int Add_VCP( char *vcp_data, int wx_mode, int where_defined );
static int Verify_rdccon( Vcp_struct *vcp, short *rdccon );


/*******************************************************************

   Description:
      This function saves the reflectivity data and radial header
      information  pertinent to the reflectivity if the radial is
      of continuous surveillance waveform. It adds the saved 
      reflectivity data and header information to the radial if 
      the radial is of the contiunous Doppler waveform with ambiguity 
      resolution. 
   
      If rda_msg->azi_num == 1, then we discard the stored data. 
      This works when the elevation scan is restarted. In the 
      beginning of a volume, the data is also discarded to guarantee 
      that only reflectivity data in the same volume will be used. 
      Because there may not be space for the reflectivity data in 
      the RDA message, we can not copy the inserted ref. field into 
      the RDA message. We, instead, record the index in PBD_saved_ref, 
      PBD_saved_ind, and use data directly from PBD_saved_ref later. Other 
      fields in the RDA header, that corresponds to the inserted 
      reflectivity field, are reset here.

   Inputs:
      rda_msg - RDA Base Data header Message 31 format .... 

   Returns:
      It returns 0 on success or -1 on failure (e.g. the fill
      failed or the radial is of an unknown waveform type).

***********************************************************************/
int PD_process_incomplete_radial ( char *rda_msg ){

   int curr_rpg_elev_cut_num = 0, num_bins;
   Generic_basedata_t *gbd = (Generic_basedata_t *) rda_msg;

   /* Beginning of executable code. */
   PBD_saved_ind = -1;

   /* If at beginning of volume and elevation number is first, then ... */
   if( (gbd->base.status == GOODBVOL) && (gbd->base.elev_num == 1) ){

	PBD_n_saved = 0;
        RPG_elev_cut_num = 0;

   }

   /* If waveform is of contiguous surveillance type, then... */
   if( PBD_waveform_type == VCP_WAVEFORM_CS ){ 

      Generic_moment_t *mom = 
              (Generic_moment_t *) ORPGGDR_get_data_block( (char *) gbd, ORPGGDR_DREF );

      Generic_elev_t *elev = 
              (Generic_elev_t *) ORPGGDR_get_data_block( (char *) gbd, ORPGGDR_RELV );

      if( gbd->base.azi_num == 1 )	/* discard stored data */
         PBD_n_saved = 0;

      if( (mom == NULL) || (elev == NULL) ){

         if( (PBD_data_trans_enabled & BD_REFLECTIVITY) == 0 )
            return (0);

         else{ 

            LE_send_msg( GL_INFO, 
                   "Reflectivity Data/Elevation Header Missing From CS Cut.\n" );
            return (-1);

         }

      }

      /* Check if too many saved radials or too many reflectivity bins. 
         in either case, return error. */
      if( PBD_n_saved >= PBD_max_num_radials ){

         LE_send_msg ( GL_STATUS | LE_RPG_WARN_STATUS, 
                       "Reflectivity Radials >= %d For RDA Cut %d\n",
                       PBD_max_num_radials, gbd->base.elev_num );
 	 return (-1);

      }

      /* Check if the number of surveillance bins in the radial is greater than
         the amount of space we have allocated storage for. */
      if( mom->no_of_gates > MAX_BASEDATA_REF_SIZE ){

         LE_send_msg ( GL_STATUS | LE_RPG_WARN_STATUS, 
                       "Reflectivity Bins >= %d For RDA Cut %d\n",
                       MAX_BASEDATA_REF_SIZE, gbd->base.elev_num );
         mom->no_of_gates = MAX_BASEDATA_REF_SIZE;

      }

      /* Save radial header, and elevation header. */
      memcpy( &PBD_Z_hdr[PBD_n_saved], &gbd->base, sizeof(Generic_basedata_header_t) );
      memcpy( &PBD_Z_elev[PBD_n_saved], elev, sizeof(Generic_elev_t) );
      RPG_elev_cut_num = ORPGVCP_get_rpg_elevation_num( PBD_vcp_number, 
                                                        gbd->base.elev_num - 1 );

      /* For reflectivity data, we assume 8 bit data. */
      if( mom->data_word_size != BYTE_MOMENT_DATA ){

         LE_send_msg( GL_ERROR, "Reflectivity Data Not BYTE Moment Data.\n" ); 
         return(-1);

      }

      /* Save reflectivity moment header and data. */
      memcpy( &PBD_saved_ref[PBD_n_saved].mom, mom, sizeof(Generic_moment_t) + mom->no_of_gates ); 

      /* Initialize the "has" flags. */
      PBD_saved_ref[PBD_n_saved].has_zdr = 0;
      PBD_saved_ref[PBD_n_saved].has_rho = 0;
      PBD_saved_ref[PBD_n_saved].has_phi = 0;

      /* Save the DP data for possible inclusion on the Doppler cut. */

      /* Check for ZDR data. */
      mom = (Generic_moment_t *) ORPGGDR_get_data_block( (char *) gbd, ORPGGDR_DZDR );
      if( (mom != NULL) && (mom->data_word_size == BYTE_MOMENT_DATA) ){

         PBD_saved_ref[PBD_n_saved].has_zdr = 1;

         /* Clip the number of bins. */
         num_bins = mom->no_of_gates;
         if( num_bins > BASEDATA_ZDR_SIZE )
            num_bins = BASEDATA_ZDR_SIZE;

         /* Copy the data to the save area. */
         memcpy( &PBD_saved_ref[PBD_n_saved].zdr_mom, mom, sizeof(Generic_moment_t) + num_bins ); 

      }

      /* Check for PHI data. */
      mom = (Generic_moment_t *) ORPGGDR_get_data_block( (char *) gbd, ORPGGDR_DPHI );
      if( (mom != NULL) && (mom->data_word_size == SHORT_MOMENT_DATA) ){

         PBD_saved_ref[PBD_n_saved].has_phi = 1;

         /* Clip the number of bins. */
         num_bins = mom->no_of_gates;
         if( num_bins > BASEDATA_PHI_SIZE )
            num_bins = BASEDATA_PHI_SIZE;

         /* Copy the data to the save area. */
         num_bins *= sizeof(short);
         memcpy( &PBD_saved_ref[PBD_n_saved].phi_mom, mom, sizeof(Generic_moment_t) + num_bins ); 

      }

      /* Check for RHO data. */
      mom = (Generic_moment_t *) ORPGGDR_get_data_block( (char *) gbd, ORPGGDR_DRHO );
      if( (mom != NULL) && (mom->data_word_size == BYTE_MOMENT_DATA) ){

         PBD_saved_ref[PBD_n_saved].has_rho = 1;

         /* Clip the number of bins. */
         num_bins = mom->no_of_gates;
         if( num_bins > BASEDATA_RHO_SIZE )
            num_bins = BASEDATA_RHO_SIZE;

         /* Copy the data to the save area. */
         memcpy( &PBD_saved_ref[PBD_n_saved].rho_mom, mom, sizeof(Generic_moment_t) + num_bins ); 

      }

      /* Increment the number of radials saved. */
      PBD_n_saved++;

   }

   /* If waveform is of contiguous Doppler with ambiguity resolution type, or 
      Staggered PRT and part of a split cut, then... */
   else if( (PBD_waveform_type == VCP_WAVEFORM_CD)
                           ||
            ((PBD_waveform_type == VCP_WAVEFORM_STP)
                           &&
                       (PBD_split_cut)) ){
 
      int ind;
      
      /* For Doppler split cut and generic radial format, check for reflectivity data
         in the radial and save it if there is.  Reflectivity data only needs to be 
         saved if azimuth resolution is 1/2 deg. */
      if( (int) gbd->base.azimuth_res == HALF_DEGREE_AZM  )
         SR_save_SuperRes_refl_data( (char *) gbd, gbd->base.azimuth, gbd->base.elevation );

      /* We set the status of the first Doppler split cut to GOODBVOL. */
      if( (((gbd->base.status & 0xf) == GOODBEL) || ((gbd->base.status & 0xf) == GOODBELLC))
                          && 
           (gbd->base.elev_num == 2) )
            gbd->base.status = GOODBVOL;

      if ( PBD_n_saved <= 0 ){

         if( (PBD_data_trans_enabled & BD_REFLECTIVITY) == 0 )
            return (0);

         else{

            LE_send_msg( GL_INFO, "There are No Saved Reflectivity Radials .....\n" );
	    return (-1);		/* we started in a Doppler only elevation */

         }

      }

      /* Search for saved radial if the current elevation RPG elevation number
         matches the RPG elevation number of the saved data.  For some VCPs (e.g.
         MPDA), a VCP_WAVEFORM_CD cut might exist without a matching VCP_WAVEFORM_CS
         cut. */ 
      curr_rpg_elev_cut_num = ORPGVCP_get_rpg_elevation_num( PBD_vcp_number,
                                                             gbd->base.elev_num - 1 );
      if( (curr_rpg_elev_cut_num == RPG_elev_cut_num) 
                             &&
          (curr_rpg_elev_cut_num > 0) ){

         /* Find the radial index that matches the Doppler radial. */
         ind = Search_for_saved_radial ( gbd );
         if ( ind < 0 ){

            LE_send_msg ( GL_ERROR, 
                          "Matching Reflectivity Radial Not Found For RDA Cut %d\n",
                          gbd->base.elev_num );
            return (-1);

         }

         /* If reflectivity radial spot blanked, set doppler radial to spot
            blanked. */
         if( (PBD_Z_hdr[ind].spot_blank_flag & SPOT_BLANK_RADIAL) )
            gbd->base.spot_blank_flag = PBD_Z_hdr[ind].spot_blank_flag;

         /* Save index of last reflectivity radial combined.   This
            index will be used in PD_move_data(). */
         PBD_saved_ind = ind;

      }
      
   }

   /* If waveform is of Batch type, Doppler without Ambiguity Resolution or Staggered 
      PRT type (not part of a split cut), then... */
   else if( (PBD_waveform_type == VCP_WAVEFORM_CDBATCH ) 
			    || 
             (PBD_waveform_type == VCP_WAVEFORM_BATCH )
                            ||
             (PBD_waveform_type == VCP_WAVEFORM_STP )){

      /* Do nothing ... just return. */
      return (0);

   }
   else 
      return (-1);	/* unknown wave form type */

   return (0);

/* End of PD_process_incomplete_radial() */
}

/*******************************************************************

   Description:
      This function searches for the index of the nearest radial 
      in the azimuth tables. Elevation and azimuth tolerences are
      checked.

   Inputs:
      gbd - RDA radial data message.

   Returns:
      This function returns the matched index (non-negative) or -1 
      if no satisfied match is found.

********************************************************************/
static int Search_for_saved_radial( Generic_basedata_t *gbd ){

    int ind, i, n_jump;
    float azi, ele, diff, min;

    ind = -1;
    min = 360.0f;

    /* If the azimuth resolution is 1/2 deg, then the number of
       radials to jump needs to double in order to maintain the
       same efficiency. */
    n_jump = N_JUMP;
    if( gbd->base.azimuth_res == HALF_DEGREE_AZM )
       n_jump += n_jump;

    azi = gbd->base.azimuth;

    /* Search for nearest (closest) radial. */
    for (i = 0; i < PBD_n_saved; i++) {

        /* Ensure difference is positive and within 180 degrees. */
	diff = azi - PBD_Z_hdr[i].azimuth;
	while( diff <= -180.0f )
	    diff += 360.0f;

	while( diff > 180.0f )
	    diff -= 360.0f;

	if( diff < 0.0f )
	    diff = -diff;

        /* Save the minimum difference and the index of the radial with
           the minimum difference. */
	if( diff < min ){

	    min = diff;
	    ind = i;

	}

	if( diff > TEST_ANGLE )	 /* jump for higher efficiency */
	    i += n_jump - 1;
    }

    /* Only failure, return error. */
    if (ind == -1)
	return (ind);

    /* Check elevation and azimuth tolerence. */
    ele = gbd->base.elevation;
    if( ele > 180.0f ) ele -= 360.0;
    diff = ele - PBD_Z_hdr[ind].elevation;

    while( diff <= -180.0f )
	diff += 360.0f;

    while( diff > 180.0f )
	diff -= 360.0f;

    if( diff < 0.0f )
	diff = -diff;

    if( min > MAX_AZI_DIFF )
	LE_send_msg (GL_INFO, "WARNING: Bad Azimuth Match (min: %f)\n", min );

    else if( diff > MAX_ELE_DIFF )
	LE_send_msg (GL_INFO, "WARNING: Bad Elevation Match (diff: %f)\n", diff );

    return (ind);

/* End of Search_for_saved_radial() */
}

/*******************************************************************

   Description:
      This function moves the three data fields from the RDA 
      radial data message to the RPG base data structure. In
      RPG data array, the first data corresponds to the 
      measurement at the range of the bin size.
      The data arrays are terminated with BASEDATA_INVALID on
      both near and far range sides if data is missing.

   Inputs:
      gdb - Generic basedata radial message.

   Outputs:
      rpg_radial - RPG basedata buffer.

   Returns:
      This function returns 0 on success or -1 on failure. So far
      there is no failure condition.

********************************************************************/
int PD_move_data( Generic_basedata_t *gbd, char *rpg_radial ){

    unsigned char *ref_src_ptr = NULL, *vel_src_ptr = NULL;
    unsigned char *wid_src_ptr = NULL;
    int radial_index, fields_added, msg_type, i;

    Base_data_header *rpg_hd = (Base_data_header *) rpg_radial;
    Generic_moment_t *mom = NULL;
    Moment_t *ref_dest_ptr = NULL, *vel_dest_ptr = NULL;
    Moment_t *wid_dest_ptr = NULL;

    msg_type = (int) gbd->msg_hdr.type;

    /* Make a copy of the radial index. */
    radial_index = PBD_saved_ind;

    /* Process reflectivity data. */
    if( (Info.num_surv_bins + Info.rpg_surv_bin_off) > (int) MAX_BASEDATA_REF_SIZE ){

	LE_send_msg ( GL_STATUS | LE_RPG_WARN_STATUS, 
                      "Too Many Reflectivity Bins (%d > %d For RDA Cut %d)\n",
		      Info.num_surv_bins, MAX_BASEDATA_REF_SIZE, gbd->base.elev_num );
	Info.num_surv_bins = MAX_BASEDATA_REF_SIZE - Info.rpg_surv_bin_off;

    }

    /* Where we store the reflectivity data. */
    ref_dest_ptr = (Moment_t *) ((short *) rpg_radial + BASEDATA_REF_OFF);

    /* Where we get the reflectivity data. */
    if( PBD_saved_ind >= 0 ){		

       /* Data was part of split cut. */
       ref_src_ptr = (unsigned char *) &PBD_saved_ref[PBD_saved_ind].ref[0];

       /* Indicate that reflectivity data is enabled and is inserted into this 
          radial. */
       rpg_hd->msg_type |= (REF_INSERT_BIT | REF_ENABLED_BIT);

       /* Transfer the saved reflectivity data to RPG radial. */
       for( i = 0; i < Info.num_surv_bins; i++ )	
          ref_dest_ptr[i + Info.rpg_surv_bin_off] = 
                           (Moment_t) ref_src_ptr[i + Info.rda_surv_bin_off];

    }
    else{

       mom = (Generic_moment_t *) ORPGGDR_get_data_block( (char *) gbd, ORPGGDR_DREF );
       if( (mom != NULL) && (mom->data_word_size == BYTE_MOMENT_DATA) ){

          ref_src_ptr = (unsigned char *) &mom->gate.b[0];  
          for( i = 0; i < Info.num_surv_bins; i++ )	
             ref_dest_ptr[i + Info.rpg_surv_bin_off] = 
                                 (Moment_t) ref_src_ptr[i + Info.rda_surv_bin_off];

          /* Indicate that reflectivity is enabled. */
          rpg_hd->msg_type |= REF_ENABLED_BIT;

       }
       else{

          /* The number of data bins is non-zero, the data is not in the 
             expected format or the data is not available so set all bins 
             to below threshold. */
          for( i = 0; i < Info.num_surv_bins; i++ )	
             ref_dest_ptr[i + Info.rpg_surv_bin_off] = 0;

       }

    }

    /* Terminate the array with invalid data. */
    rpg_hd->ref_offset = 0;
    if( Info.num_surv_bins > 0 ){

       if( Info.rpg_surv_bin_off > 0 )
          memset( ref_dest_ptr, 0, Info.rpg_surv_bin_off*sizeof(Moment_t) );

       if( (Info.rpg_surv_bin_off + Info.num_surv_bins) < (MAX_BASEDATA_REF_SIZE-1) )
          ref_dest_ptr[Info.rpg_surv_bin_off + Info.num_surv_bins] = BASEDATA_INVALID;

       /* Set offset, in number of bytes, to reflectivity data in the radial header. */
       rpg_hd->ref_offset = (unsigned short) ((char *) ref_dest_ptr - rpg_radial);

       /* Set the message size dependent on the availability of Reflectivity moment. */
       rpg_hd->msg_len =
           (rpg_hd->ref_offset + (rpg_hd->n_surv_bins*sizeof(Moment_t)) + 1)/sizeof(short);

    }

    /* Process Doppler data (radial velocity and spectrum width). */
    if( (Info.num_dop_bins + Info.rpg_dop_bin_off) > (int) BASEDATA_DOP_SIZE ){

	LE_send_msg ( GL_STATUS | LE_RPG_WARN_STATUS, 
                      "Too Many Doppler Bins (%d > %d For RDA Cut %d)\n",
		      Info.num_dop_bins, BASEDATA_DOP_SIZE, gbd->base.elev_num );
	Info.num_dop_bins = (int) BASEDATA_DOP_SIZE - Info.rpg_dop_bin_off;

    }

    vel_dest_ptr = (Moment_t *) ((short *) rpg_radial + BASEDATA_VEL_OFF);
    wid_dest_ptr = (Moment_t *) ((short *) rpg_radial + BASEDATA_SPW_OFF);

    mom = (Generic_moment_t *) ORPGGDR_get_data_block( (char *) gbd, ORPGGDR_DVEL );
    if( (mom != NULL) && (mom->data_word_size == BYTE_MOMENT_DATA) ){

       vel_src_ptr = (unsigned char *) &mom->gate.b[0];  
       for (i = 0; i < Info.num_dop_bins; i++)
          vel_dest_ptr[i + Info.rpg_dop_bin_off] = 
                              (Moment_t) vel_src_ptr[i + Info.rda_dop_bin_off];	

       /* Indicate velocity is enabled. */
       rpg_hd->msg_type |= VEL_ENABLED_BIT;

    }
    else{

       /* The number of data bins is non-zero, the data is not in the expected 
          format or the data is not available so set all bins to below threshold. */
       for( i = 0; i < Info.num_dop_bins; i++ )	
          vel_dest_ptr[i + Info.rpg_dop_bin_off] = 0;

    }

    mom = (Generic_moment_t *) ORPGGDR_get_data_block( (char *) gbd, ORPGGDR_DSW );
    if( (mom != NULL) && (mom->data_word_size == BYTE_MOMENT_DATA) ){

       wid_src_ptr = (unsigned char*) &mom->gate.b[0];  
       for (i = 0; i < Info.num_dop_bins; i++)
          wid_dest_ptr[i + Info.rpg_dop_bin_off] = 
                              (Moment_t) wid_src_ptr[i + Info.rda_dop_bin_off];	

       /* Indicate velocity is enabled. */
       rpg_hd->msg_type |= VEL_ENABLED_BIT;

    }
    else{

       /* The number of data bins is non-zero, the data is not in the expected 
          format or the data is not available so set all bins to below threshold. */
       for( i = 0; i < Info.num_dop_bins; i++ )	
          wid_dest_ptr[i + Info.rpg_dop_bin_off] = 0;

    }

    /* Terminate the arrays with invalid data. */
    rpg_hd->vel_offset = 0;
    rpg_hd->spw_offset = 0;
    if( Info.num_dop_bins > 0 ){

       if( Info.rpg_dop_bin_off > 0 ){

          memset( vel_dest_ptr, 0, Info.rpg_dop_bin_off*sizeof(Moment_t) );
          memset( wid_dest_ptr, 0, Info.rpg_dop_bin_off*sizeof(Moment_t) );

       }

       if( (Info.rpg_dop_bin_off + Info.num_dop_bins) < (BASEDATA_DOP_SIZE - 1) ){

	  vel_dest_ptr[Info.rpg_dop_bin_off + Info.num_dop_bins] = BASEDATA_INVALID;
	  wid_dest_ptr[Info.rpg_dop_bin_off + Info.num_dop_bins] = BASEDATA_INVALID;

       }

       /* Set offsets to velocity and spectrum width data in the radial header. */
       rpg_hd->vel_offset = (unsigned short) ((char *) vel_dest_ptr - rpg_radial);    
       rpg_hd->spw_offset = (unsigned short) ((char *) wid_dest_ptr - rpg_radial);    

       /* Set the size of the radial message dependent on the availability of 
          Velocity and/or Spectrum Width data. */
       if( (rpg_hd->vel_offset > rpg_hd->ref_offset)
                              ||
           (rpg_hd->spw_offset > rpg_hd->ref_offset) ){

          if( rpg_hd->vel_offset > rpg_hd->spw_offset )
             rpg_hd->msg_len = (rpg_hd->vel_offset + 
                               (rpg_hd->n_dop_bins*sizeof(Moment_t)) + 1)/sizeof(short);

          else
             rpg_hd->msg_len = (rpg_hd->spw_offset + 
                               (rpg_hd->n_dop_bins*sizeof(Moment_t)) + 1)/sizeof(short);

       }

    }

    /* Restore any reflectivity data needed for Super Resolution processing. */
    if( PBD_saved_ind >= 0 ){

       if( rpg_hd->azm_reso == HALF_DEGREE_AZM ){

          SR_restore_SuperRes_refl_data( rpg_radial, gbd->base.azimuth, 
                                         gbd->base.elevation );

          PBD_saved_ind = -1;

       }

    }

    /* Move additional data to RPG radial. */
    fields_added = Move_additional_data( (char *) gbd, rpg_radial );

    /* Do we need to add in the DP data? */
    if( radial_index >= 0 )
       Add_DP_data( (char *) gbd, rpg_radial, radial_index, fields_added );

    return (0);

/* End of PD_move_data */
}

/*******************************************************************

   Description:
      With generic type radial format, additional moments other
      than the standard issue (reflectivity, velocity, spectrum
      width) may be included with this radial.  These moments need
      to be copied to the rpg radial. 

      This function checks if the radial has DP data already residing
      in the radial and moves the data if it does.

   Inputs:
      rda_msg - Pointer to Generic type (Message 1) message.

   Outputs:
      rpg_radial - Pointer to RPG radial mesage.

   Returns:
      On error returns -1, 0 if DP data not moved, or 1 if DP data 
      moved.   If DP data moved, this means there is DP data already
      in this radial.   For Doppler split cuts, it doesn't need to 
      be moved from the surveillance scan.

*******************************************************************/
static int Move_additional_data( char *rda_msg, char *rpg_radial ){

   Generic_basedata_t *generic_hd = (Generic_basedata_t *) rda_msg;
   Base_data_header *rpg_hd = (Base_data_header *) rpg_radial;
   Generic_moment_t *mom_block, *mom;
   char str_type[5];
   int dualpol_added, offset, add_mom, max_bins, field_added, i;
   int size = 0, bin_offset = 0;
   float range;
    
   offset = rpg_hd->msg_len*sizeof(short);
   offset += (offset % sizeof(int));

   /* Go through all additional moments (up to MAX_ADD_MOMENTS), and append each
      to rpg_msg. */
   add_mom = 0;
   dualpol_added = 0;
   field_added = 0;
   for( i = 0; i < generic_hd->base.no_of_datum; i++ ){

      Generic_any_t *data_block = (Generic_any_t *)
                 (rda_msg + sizeof(RDA_RPG_message_header_t) + generic_hd->base.data[i]);

      /* Convert the name to a string so we can do string compares. */
      memset( str_type, 0, 5 );
      memcpy( str_type, data_block->name, sizeof(data_block->name) );

      /* If not a moment data block, ignore it. */
      if( str_type[0] != 'D' )
         continue;

      /* Ignore if one of the standard 3 moments.  These moments have
         already been copied to the RPG radial message. */
      if( (strstr( str_type, "DREF" ) != NULL)
                      ||
          (strstr( str_type, "DVEL" ) != NULL)
                      ||
          (strstr( str_type, "DSW" ) != NULL) )
         continue;

      /* Check to see if any of the Dual Pol data items are added. */
      if( strstr( str_type, "DRHO" ) != NULL ){

         field_added = PBD_DRHO_MOVED;
         max_bins = BASEDATA_RHO_SIZE;

      }      
      else if( strstr( str_type, "DPHI" ) != NULL ){

         field_added = PBD_DPHI_MOVED;
         max_bins = BASEDATA_PHI_SIZE;

      }
      else if( strstr( str_type, "DZDR" ) != NULL ){

         field_added = PBD_DZDR_MOVED;
         max_bins = BASEDATA_ZDR_SIZE;
                      
      }
      else if( strstr( str_type, "DSNR" ) != NULL ){

         field_added = 0;
         max_bins = BASEDATA_SNR_SIZE;

      }
      else if( strstr( str_type, "DRFR" ) != NULL ){

         field_added = 0;
         max_bins = BASEDATA_RFR_SIZE;
                      
      }
      else{

         /* ????? */
         field_added = 0;
         continue;

      }

      mom = (Generic_moment_t *) data_block;

      /* Is the data in the expected resolution? */
      if( (mom->data_word_size != BYTE_MOMENT_DATA)
                          &&
          (mom->data_word_size != SHORT_MOMENT_DATA) ){

         LE_send_msg( GL_INFO, "%d-bit Moment Data, %d or %d-bit Data Expected\n",
                      mom->data_word_size, BYTE_MOMENT_DATA, SHORT_MOMENT_DATA );
         LE_send_msg( GL_INFO, " ... Skipping Moment\n" );

         field_added = 0; 
         continue;

      }

      /* The data field is recognized ... let's move it. */
      if( field_added )
         dualpol_added |= field_added;

      /* Set the offset in the RPG radial header. */
      rpg_hd->offsets[rpg_hd->no_moments + add_mom] = offset;

      /* Move the data block header ..... */
      mom_block = (Generic_moment_t *) (rpg_radial + offset);
      memcpy( (char *) mom_block, mom, sizeof(Generic_moment_t) );
      offset += sizeof(Generic_moment_t);

      /* Calculate the bin offset value.   This is used to determine the number
         of pad values (0's) to prepend to the data. */
      range = (float) (mom->first_gate_range - mom->bin_size/2);
      bin_offset = Round( range / (float) mom->bin_size );
                      
      /* Ensure the number of gates is correct and not more than the max. */
      if( (mom->no_of_gates + bin_offset) > max_bins ){

         int diff = (mom->no_of_gates + bin_offset) - max_bins;
         mom_block->no_of_gates -= diff;

      }

      if( mom->data_word_size == BYTE_MOMENT_DATA ){

         memcpy( rpg_radial + offset + bin_offset, 
                 &mom->gate.b[0], mom_block->no_of_gates*sizeof(char) ); 
         size = mom_block->no_of_gates*sizeof(char);

      }
      else if( mom->data_word_size == SHORT_MOMENT_DATA ){

         memcpy( rpg_radial + offset + bin_offset*sizeof(short), 
                 &mom->gate.u_s[0], mom_block->no_of_gates*sizeof(short) ); 
         size = mom_block->no_of_gates*sizeof(short);

      }

      /* Adjust the offset based on number of bins.  The number of bins is affected
         by the bin_offset value. */
      if( mom_block->no_of_gates > 0 ){

         int pad_size = 0;

         /* Set any lending data before actual start of radial to below 
            threshold. */
         if( bin_offset > 0 ){

            if( mom->data_word_size == BYTE_MOMENT_DATA ){

               pad_size = bin_offset*sizeof(char);
               memset( rpg_radial + offset, 0, pad_size );

            }
            else if( mom->data_word_size == SHORT_MOMENT_DATA ){

               pad_size = bin_offset*sizeof(short);
               memset( rpg_radial + offset, 0, pad_size );

            }

            /* Adjust the first gate range ... */
            mom_block->first_gate_range -= bin_offset*mom->bin_size;

            /* Adjust the number of bins. */
            mom_block->no_of_gates += bin_offset;

         }

         /* Adjust the size of the radial, in bytes. */
         offset += size + pad_size;

      }

      /* Ensure the offset is defined on a word boundary. */
      offset += (offset % sizeof(int));

      add_mom++;
      if( (rpg_hd->no_moments + add_mom) >= MAX_ADD_MOMENTS )
         break;
      
   }

   /* Increase the size of RPG radial to accomodate the additional data.
      Make sure the message length in word aligned. */
   offset += (offset % sizeof(int)); 
   rpg_hd->msg_len = offset/sizeof(short);

   /* Set the number of additional moments. */
   rpg_hd->no_moments += add_mom;

   /* Set the DUALPOL_TYPE flag if dual pol data provided in this
      radial. */
   if( dualpol_added )
      rpg_hd->msg_type |= DUALPOL_TYPE; 

   return dualpol_added;

/* End of Move_additional_data() */
}

/*******************************************************************

   Description:
      Adds DP data to Doppler split cut radial from data from 
      Surveillance split cut radial.

   Inputs:
      rda_msg - Pointer to Generic type (Message 1) message.
      index - index of radial from Surveillance split cut.
      fields_added - bit map of DP fields already in radial.

   Outputs:
      rpg_radial - Pointer to RPG radial mesage.

   Returns:
      On error returns -1, 0 otherwise.

*******************************************************************/
static int Add_DP_data( char *rda_msg, char *rpg_radial, int index,
                        int fields_added ){

   Base_data_header *rpg_hd = (Base_data_header *) rpg_radial;
   int dualpol_added, offset, add_mom, max_bins;
    
   offset = rpg_hd->msg_len*sizeof(short);
   offset += (offset % sizeof(int));

   add_mom = 0;
   dualpol_added = 0;

   /* Add ZDR if needed. */
   if( !(fields_added & PBD_DZDR_MOVED) 
                     && 
       (PBD_saved_ref[index].has_zdr)
                     &&
       ((rpg_hd->no_moments + add_mom) < MAX_ADD_MOMENTS ) ){

      /* Set flag to indicate this radial has DP data. */
      dualpol_added = 1;

      /* Set the offset in the RPG radial header. */
      rpg_hd->offsets[rpg_hd->no_moments + add_mom] = offset;

      /* Determine the maximum number of bins allowed. */
      max_bins = BASEDATA_ZDR_SIZE;

      /* Insert the ZDR data. */
      offset = Insert_data( rpg_radial, add_mom, offset, max_bins, 
                            &PBD_saved_ref[index].zdr_mom );

      /* Increment the number of added data fields. */
      add_mom++;

   }

   /* Add PHI if needed. */
   if( !(fields_added & PBD_DPHI_MOVED) 
                     && 
       (PBD_saved_ref[index].has_phi) 
                     &&
       ((rpg_hd->no_moments + add_mom) < MAX_ADD_MOMENTS ) ){

      /* Set flag to indicate this radial has DP data. */
      dualpol_added = 1;

      /* Set the offset in the RPG radial header. */
      rpg_hd->offsets[rpg_hd->no_moments + add_mom] = offset;

      /* Determine the maximum number of bins allowed. */
      max_bins = BASEDATA_PHI_SIZE;

      /* Insert the PHI data. */
      offset = Insert_data( rpg_radial, add_mom, offset, max_bins,
                            &PBD_saved_ref[index].phi_mom );

      /* Increment the number of added data fields. */
      add_mom++;

   }

   /* Add RHO if needed. */
   if( !(fields_added & PBD_DRHO_MOVED) 
                     && 
       (PBD_saved_ref[index].has_rho) 
                     &&
       ((rpg_hd->no_moments + add_mom) < MAX_ADD_MOMENTS ) ){

      /* Set flag to indicate this radial has DP data. */
      dualpol_added = 1;

      /* Set the offset in the RPG radial header. */
      rpg_hd->offsets[rpg_hd->no_moments + add_mom] = offset;

      /* Determine the maximum number of bins allowed. */
      max_bins = BASEDATA_RHO_SIZE;

      /* Insert the RHO data. */
      offset = Insert_data( rpg_radial, add_mom, offset, max_bins,
                            &PBD_saved_ref[index].rho_mom );

      /* Increment the number of added data fields. */
      add_mom++;

   }
      
   /* Increase the size of RPG radial to accomodate the additional data.
      Make sure the message length in word aligned. */
   offset += (offset % sizeof(int)); 
   rpg_hd->msg_len = offset/sizeof(short);

   /* Set the number of additional moments. */
   rpg_hd->no_moments += add_mom;

   /* Set the DUALPOL_TYPE flag if dual pol data provided in this
      radial. */
   if( dualpol_added )
      rpg_hd->msg_type |= DUALPOL_TYPE; 

   return 0;

/* End of Add_DP_data() */
}

/******************************************************************

   Description:
      Inserts DP data into a radial. 

   Inputs:
      rpg_radial - RPG radial. 
      add_mom - moment index.
      offset - radial offset to place the DP data.
      max_bins - maximum number of bins in a radial.
      mom - DP data.

   Returns:
      Offset to the end of the radial.
      
******************************************************************/
static int Insert_data( char *rpg_radial, int add_mom, int offset, 
                        int max_bins, Generic_moment_t *mom ){

   Base_data_header *rpg_hd = (Base_data_header *) rpg_radial;
   Generic_moment_t *mom_block = (Generic_moment_t *) (rpg_radial + offset);
   int size = 0, bin_offset = 0;
   float range;

   /* Set the offset in the RPG radial header. */
   rpg_hd->offsets[rpg_hd->no_moments + add_mom] = offset;

   /* Move the data block header ..... */
   memcpy( mom_block, mom, sizeof(Generic_moment_t) );
   offset += sizeof(Generic_moment_t);

   /* Calculate the bin offset.   This value is used to determine the 
      number of pad values (0's) to prepend to the data. */
   range = (float) (mom->first_gate_range - mom->bin_size/2);
   bin_offset = Round( range / (float) mom->bin_size );

   /* Ensure the number of gates is correct and not more than the max. */
   if( (mom->no_of_gates + bin_offset) > max_bins ){

      int diff = (mom->no_of_gates + bin_offset) - max_bins;
      mom_block->no_of_gates -= diff;

   }

   if( mom->data_word_size == BYTE_MOMENT_DATA ){

      memcpy( rpg_radial + offset + bin_offset, 
              &mom->gate.b[0], mom_block->no_of_gates*sizeof(char) );
      size = mom_block->no_of_gates*sizeof(char);

   }
   else if( mom->data_word_size == SHORT_MOMENT_DATA ){

      memcpy( rpg_radial + offset + bin_offset, 
              &mom->gate.u_s[0], mom_block->no_of_gates*sizeof(short) );
      size = mom_block->no_of_gates*sizeof(short);

   }

   /* Set any lending data before actual start of radial to below
         threshold. */
   if( mom_block->no_of_gates > 0 ){

      if( bin_offset > 0 ){

         int pad_size = 0;

         if( mom->data_word_size == BYTE_MOMENT_DATA ){

            pad_size = bin_offset*sizeof(char);
            memset( rpg_radial + offset, 0, pad_size );

         }
         else if( mom->data_word_size == SHORT_MOMENT_DATA ){

            pad_size = bin_offset*sizeof(short);
            memset( rpg_radial + offset, 0, pad_size );

         }

         /* Adjust the first gate range ... */
         mom_block->first_gate_range -= bin_offset*mom->bin_size;

         /* Adjust the number of bins. */
         mom_block->no_of_gates += bin_offset;

         /* Adjust the size of the radial, in bytes. */
         offset += size + pad_size;

      }

   }

   /* Ensure the offset is defined on a word boundary. */
   offset += (offset % sizeof(int));

   return offset;

/* End of Insert_data(). */
}


/* VCP sizes, in shorts (taken from ICD). */
#define PBD_VCP_MIN_SIZE		23
#define PBD_VCP_MAX_SIZE   		594

/**********************************************************************
  
   Description:
      The VCP message is written to Linear Buffer.  A system status
      log message is written indicating the VCP data is available.
  
   Inputs:
      vcp_data - RDA/RPG VCP data.

   Outputs:
      The RDA VCP data is stored in the RDA VCP LB.
  
   Returns:
      0 on success, -1 upon failure.
  
   Notes:
      This is a variable length message depending on the VCP.  We
      cannot cast directly to the structure before writing to the
      LB.  Rather we have to interrogate the msg hdr to get the
      actual msg size.

**********************************************************************/
int PD_process_rda_vcp_message( char *vcp_data ){

   int ret, ind, vcp_num, size;
   char *spt = NULL;
   VCP_message_header_t *vcp_hdr = NULL;

   /* Get the size of the message from the message header. */
   spt = vcp_data + sizeof(RDA_RPG_message_header_t);
   vcp_hdr = (VCP_message_header_t *) spt;

   /* Make sure the size is not improbable. */
   if( (vcp_hdr->msg_size < PBD_VCP_MIN_SIZE)
                        ||
       (vcp_hdr->msg_size > PBD_VCP_MAX_SIZE) ){

      LE_send_msg( GL_INFO, "Improbable Size of RDA VCP Message (%d)\n",
                   vcp_hdr->msg_size*sizeof(short) );
      return -1;

   }

   /* Check if this VCP is defined.  If not, add the VCP definition. */
   vcp_num = vcp_hdr->pattern_number;
   if( (ret = ORPGVCP_index( vcp_num )) < 0 ){

      LE_send_msg( GL_INFO, "VCP %4d is Not Defined.   Adding ...\n",
                   vcp_num );

      /* Search for another VCP which has the "ORPGVCP_DATA_DEFINED_VCP" bit
         set.   If one is found, remove it. */
      for( ind = 0; ind < VCPMAX; ind++ ){

         int vnum = 0;

         if( (vnum = ORPGVCP_get_vcp_num( ind )) > 0 ){
         
            if( ORPGVCP_is_vcp_data_defined( vnum ) == 1 ){

               /* Another data defined VCP found.   Remove it. */
               ORPGVCP_delete( vnum );

               LE_send_msg( GL_INFO, "Deleting Data Defined VCP %4d\n", vnum );

               break;

            }

         }

      }

      if( (vcp_num >= VCP_MIN_CLEAR_AIR) && (vcp_num <= VCP_MAX_CLEAR_AIR) )
         Add_VCP( spt, CLEAR_AIR_MODE, ORPGVCP_RDA_DEFINED_VCP ); 

      else
         Add_VCP( spt, PRECIPITATION_MODE, ORPGVCP_RDA_DEFINED_VCP ); 
     
   }

   /* Make a local copy of this VCP data. */
   size = vcp_hdr->msg_size*sizeof(short);
   memcpy( (void *) &PBD_rda_vcp_data, spt, size );

   /* Return Normal. */
   return 0;

/* End PD_process_rda_vcp_message() */
} 

/**********************************************************************
  
   Description:
      The RDA/RPG loopback data is interrogated to determine whether
      data communication link is connected to a real RDA or the 
      wideband simulator.
  
   Inputs:
      loopback_data - RDA/RPG Loopback data.

**********************************************************************/
void PD_process_rda_rpg_loopback_message( char *loopback_data ){

   RDA_RPG_loop_back_message_t *payload = (RDA_RPG_loop_back_message_t *) 
                         loopback_data + sizeof(RDA_RPG_message_header_t);
   char *str = (char *) payload->pattern;
   char *substr = NULL;

   /* Check for the Wideband Simulator String.   If found, clear the 
      PBD_is_rda flag. */
   substr = strstr( str, "Wideband Simulator" );
   if( substr != NULL )
      PBD_is_rda = PBD_IS_WB_SIMULATOR;

   return;

/* End of PD_process_rda_rpg_loopback_message() */
}

#define MIN_SNR_THRESHOLD       -12.0
#define AZI_RATE_SCALE          (22.5/16384)
#define MAX_PRFS		5
#define INITIAL_PRF		4

/**************************************************************************

   Description: 
      This function initializes the VCP table structure from the VCP data 
      received from by the RPG.   This data is assumed to not be defined
      at the RPG.   We define it here so that the RPG can play back the 
      data.   

   Input:

   Output: 

   Return:
      -1 on error, 0 otherwise.

   Note:
      Not all RDACNT data may be set.   Furthermore some of the information
      is derived using "best-guess" such the the RDCCON data. 

**************************************************************************/
static int Add_VCP( char *vcp_data, int wx_mode, int where_defined ){

   int i, j, ret, pos, allowable_prfs_n_ele = 0;
   float vcp_time;
   Vcp_struct *vcp = (Vcp_struct *) vcp_data;
   Ele_attr *elev = NULL;

   static Vcp_alwblprf_t allowable_prfs;
   static short rdccon[ECUTMAX];

   /* Check to see if this VCP is already defined. */
   if( (pos = ORPGVCP_add( (int) vcp->vcp_num, (int) wx_mode,
                           where_defined )) < 0 ){

      LE_send_msg( GL_ERROR, "ORPGVCP_add( %d, %d ) Failed: %d\n",
                   vcp->vcp_num, wx_mode, pos );
      return( -1 );

   }

   /* Initialize some data. */
   memset( rdccon, 0, sizeof(short)*ECUTMAX );
   memset( &allowable_prfs, 0, sizeof(Vcp_alwblprf_t) );
   allowable_prfs.vcp_num = vcp->vcp_num;
   allowable_prfs.num_alwbl_prf = MAX_PRFS;

   for( i = 0; i < MAX_PRFS; i++ )
      allowable_prfs.prf_num[i] = INITIAL_PRF + i;

   /* Set fields in the VCP attributes section. */
   if( (ORPGVCP_set_num_elevations( vcp->vcp_num, vcp->n_ele ) < 0 )
                                ||
       (ORPGVCP_set_pulse_width( vcp->vcp_num, vcp->pulse_width ) < 0 )
                                ||
       (ORPGVCP_set_vel_resolution( vcp->vcp_num, vcp->vel_resolution ) < 0 ) ){

      LE_send_msg( GL_ERROR, "Error setting # elevs/pulse width/vel resolution.\n" );
      LE_send_msg( GL_ERROR, "--->VCP: %d, # Elevs: %d, Pulse Width: %d, Vel Resol: %d\n",
                   vcp->vcp_num, vcp->n_ele, vcp->pulse_width, vcp->vel_resolution );

      ORPGVCP_delete( vcp->vcp_num );
      return( -1 );

   }

   /* Set the fields for each elevation cut. */
   for( i = 0; i < vcp->n_ele; i++ ){

      float azi_rate, elev_angle; 

      elev = (Ele_attr *) vcp->vcp_ele[i];
      azi_rate = (float) elev->azi_rate * ORPGVCP_RATE_BAMS2DEG;
      elev_angle = (float) elev->ele_angle * ORPGVCP_ELVAZM_BAMS2DEG;

      if( (ORPGVCP_set_elevation_angle( vcp->vcp_num, i, elev_angle ) < -1.0 )
                                    ||
          (ORPGVCP_set_azimuth_rate( vcp->vcp_num, i, azi_rate ) < 0.0 )
                                    ||
          (ORPGVCP_set_waveform( vcp->vcp_num, i, elev->wave_type ) < 0 ) 
                                    ||
          (ORPGVCP_set_phase_type( vcp->vcp_num, i, elev->phase ) < 0 ) ){
       
         LE_send_msg( GL_ERROR, "Error setting elev angle/azm rate/waveform\n" );
         LE_send_msg( GL_ERROR, "--->VCP: %d, Elev Angle: %f, Azm Rate: %f, Wave Type: %d, Phase: %d\n",
                      vcp->vcp_num, elev_angle, azi_rate, elev->wave_type, elev->phase );

         ORPGVCP_delete( vcp->vcp_num );
         return( -1 );

      }

      /* Set the super resolution word. */
      if( ORPGVCP_set_super_res( vcp->vcp_num, i, (elev->super_res & VCP_SUPER_RES_MASK) ) < 0 )
         LE_send_msg( GL_ERROR, "Error setting super resolution.\n" );

      /* Set the Dual Pol word. */
      if( elev->super_res & VCP_DUAL_POL_ENABLED ){

         if( ORPGVCP_set_dual_pol( vcp->vcp_num, i, VCP_DUAL_POL_ENABLED ) < 0 )
            LE_send_msg( GL_ERROR, "Error setting dual pol enabled.\n" );

      }

      /* Set the surveillance PRF number and pulse counts. */
      if( (elev->wave_type == VCP_WAVEFORM_CS) 
                               ||
          (elev->wave_type == VCP_WAVEFORM_BATCH)
                               ||
          (elev->wave_type == VCP_WAVEFORM_STP) ){

         if( (ORPGVCP_set_prf_num( vcp->vcp_num, i, ORPGVCP_SURVEILLANCE, 
                                   elev->surv_prf_num ) < 0)
                                     ||
             (ORPGVCP_set_pulse_count( vcp->vcp_num, i, ORPGVCP_SURVEILLANCE, 
                                       elev->surv_pulse_cnt ) < 0) ){

            LE_send_msg( GL_ERROR, "Error setting surveillance prf/pulse count\n" );
            LE_send_msg( GL_ERROR, "--->VCP: %d, PRF #: %d, Pulse Count: %d\n",
                         vcp->vcp_num, elev->surv_prf_num, elev->surv_pulse_cnt );

         }

      }

      /* Set the Doppler PRF number and pulse counts. */
      if( (elev->wave_type == VCP_WAVEFORM_CD) 
                               ||
          (elev->wave_type == VCP_WAVEFORM_BATCH)
                               ||
          (elev->wave_type == VCP_WAVEFORM_CDBATCH) ){

         unsigned short edge_angle = (unsigned short) elev->azi_ang_1;
         float azi_ang = (float) edge_angle * ORPGVCP_ELVAZM_BAMS2DEG;

         if( (ORPGVCP_set_edge_angle( vcp->vcp_num, i, ORPGVCP_DOPPLER1,
                                      azi_ang ) < 0.0 )
                                    ||
             (ORPGVCP_set_prf_num( vcp->vcp_num, i, ORPGVCP_DOPPLER1,
                                   elev->dop_prf_num_1 ) < 0 )
                                    ||
             (ORPGVCP_set_pulse_count( vcp->vcp_num, i, ORPGVCP_DOPPLER1,
                                       elev->pulse_cnt_1 ) < 0 ) ){

            LE_send_msg( GL_ERROR, "Error setting Doppler Sector 1 prf/pulse count/sector angle\n" );
            LE_send_msg( GL_ERROR, "--->VCP: %d, PRF #: %d, Edge Angle: %f, Pulse Count: %d\n",
                         vcp->vcp_num, elev->dop_prf_num_1, azi_ang, elev->pulse_cnt_1 );

         }

         /* Set the pulse count. */
         allowable_prfs.pulse_cnt[i][elev->dop_prf_num_1-1] = elev->pulse_cnt_1;
         allowable_prfs.pulse_cnt[i][NAPRFELV-1] = elev->dop_prf_num_1;
         allowable_prfs_n_ele++;
                                    
         edge_angle = (unsigned short) elev->azi_ang_2;
         azi_ang = (float) edge_angle * ORPGVCP_ELVAZM_BAMS2DEG;

         if( (ORPGVCP_set_edge_angle( vcp->vcp_num, i, ORPGVCP_DOPPLER2,
                                      azi_ang ) < 0.0 )
                                    ||
             (ORPGVCP_set_prf_num( vcp->vcp_num, i, ORPGVCP_DOPPLER2, 
                                   elev->dop_prf_num_2 ) < 0 )
                                    ||
             (ORPGVCP_set_pulse_count( vcp->vcp_num, i, ORPGVCP_DOPPLER2, 
                                       elev->pulse_cnt_2 ) < 0 ) ){

            LE_send_msg( GL_ERROR, "Error setting Doppler Sector 2 prf/pulse count/sector angle\n" );
            LE_send_msg( GL_ERROR, "--->VCP: %d, PRF #: %d, Edge Angle: %f, Pulse Count: %d\n",
                         vcp->vcp_num, elev->dop_prf_num_2, azi_ang, elev->pulse_cnt_2 );

         }

         edge_angle = (unsigned short) elev->azi_ang_3;
         azi_ang = (float) edge_angle * ORPGVCP_ELVAZM_BAMS2DEG;
                                    
         if( (ORPGVCP_set_edge_angle( vcp->vcp_num, i, ORPGVCP_DOPPLER3, 
                                      azi_ang ) < 0.0 )
                                    ||
             (ORPGVCP_set_prf_num( vcp->vcp_num, i, ORPGVCP_DOPPLER3, 
                                   elev->dop_prf_num_3 ) < 0 )
                                    ||
             (ORPGVCP_set_pulse_count( vcp->vcp_num, i, ORPGVCP_DOPPLER3, 
                                       elev->pulse_cnt_3 ) < 0 ) ){

            LE_send_msg( GL_ERROR, "Error setting Doppler Sector 3 prf/pulse count/sector angle\n" );
            LE_send_msg( GL_ERROR, "--->VCP: %d, PRF #: %d, Edge Angle: %f, Pulse Count: %d\n",
                         vcp->vcp_num, elev->dop_prf_num_3, azi_ang, elev->pulse_cnt_3 );

         }

      }

      /* Set the Signal-to_Noise thresholds. */
      if( (ORPGVCP_set_threshold( vcp->vcp_num, i, ORPGVCP_REFLECTIVITY, 
                                  ( (float) elev->surv_thr_parm)/8.0 ) < MIN_SNR_THRESHOLD )
                                       ||
          (ORPGVCP_set_threshold( vcp->vcp_num, i, ORPGVCP_VELOCITY, 
                                  ( (float) elev->vel_thrsh_parm)/8.0 ) < MIN_SNR_THRESHOLD )
                                       ||
          (ORPGVCP_set_threshold( vcp->vcp_num, i, ORPGVCP_SPECTRUM_WIDTH, 
                                  ( (float) elev->spw_thrsh_parm)/8.0 ) < MIN_SNR_THRESHOLD ) 
                                       ||
          (ORPGVCP_set_threshold( vcp->vcp_num, i, ORPGVCP_DIFFERENTIAL_Z, 
                                  ( (float) elev->zdr_thrsh_parm)/8.0 ) < MIN_SNR_THRESHOLD )
                                       ||
          (ORPGVCP_set_threshold( vcp->vcp_num, i, ORPGVCP_CORRELATION_COEF, 
                                  ( (float) elev->corr_thrsh_parm)/8.0 ) < MIN_SNR_THRESHOLD )
                                       ||
          (ORPGVCP_set_threshold( vcp->vcp_num, i, ORPGVCP_DIFFERENTIAL_PHASE,
                                  ( (float) elev->phase_thrsh_parm)/8.0 ) < MIN_SNR_THRESHOLD ) ){

         LE_send_msg( GL_ERROR, "Error setting moment thresholds\n" );
         LE_send_msg( GL_ERROR, "--->VCP: %d, SNR Thresholds (Z/V/W): %5.1f/%5.1f/%5.1f dB\n",
                      vcp->vcp_num, ((float) elev->surv_thr_parm)/8.0, 
                      ((float) elev->vel_thrsh_parm)/8.0, ((float) elev->spw_thrsh_parm)/8.0 );
         LE_send_msg( GL_ERROR, "--->VCP: %d, SNR Thresholds (ZDR/CC/PHI): %5.1f/%5.1f/%5.1f dB\n",
                      vcp->vcp_num, ((float) elev->zdr_thrsh_parm)/8.0, 
                      ((float) elev->corr_thrsh_parm)/8.0, ((float) elev->phase_thrsh_parm)/8.0 );

      }

   }

   /* Set the allowable PRF information. */
   if( ((ret = ORPGVCP_set_allowable_prf_vcp_num( pos, vcp->vcp_num )) < 0 )
                                  ||
       ((ret = ORPGVCP_set_allowable_prfs( vcp->vcp_num, 
                                           allowable_prfs.num_alwbl_prf )) < 0 ) ){

      LE_send_msg( GL_ERROR, "Error setting allowable prfs: %d\n", ret );
      LE_send_msg( GL_ERROR, "--->VCP: %d, Pos: %d, Number Allowable PRFs: %d\n", 
                   pos, vcp->vcp_num, allowable_prfs.num_alwbl_prf );

   }

   for( i = 0; i < allowable_prfs.num_alwbl_prf; i++ ){

      if( (ret = ORPGVCP_set_allowable_prf( vcp->vcp_num, i, allowable_prfs.prf_num[i] )) < 0 ){

         LE_send_msg( GL_ERROR, "Error setting allowable prf: %d\n", ret );
         LE_send_msg( GL_ERROR, "--->VCP: %d, PRF Index: %d, PRF: %d\n", 
                      vcp->vcp_num, i, allowable_prfs.prf_num[i] );

      }


   } /*End of "For All Allowable PRFs" loop. */

   for( i = 0; i < allowable_prfs_n_ele; i++ ){

      for( j = 1; j <= PRFMAX; j++ ){

         if( (ret = ORPGVCP_set_allowable_prf_pulse_count( vcp->vcp_num, i, j, 
                                                           allowable_prfs.pulse_cnt[i][j-1] )) < 0 ){
 
            LE_send_msg( GL_ERROR, "Error setting allowable prf pulse count: %d\n", ret );
            LE_send_msg( GL_ERROR, "--->VCP: %d, Elev: %d, PRF: %d, Pulse Cnt: %d\n", 
                         vcp->vcp_num, i, j, allowable_prfs.pulse_cnt[i][j] );

         }

      } /* End of "For All Allowable PRF Elevation Cuts" loop. */

      if( (ORPGVCP_set_allowable_prf_default( vcp->vcp_num, i, 
                                              allowable_prfs.pulse_cnt[i][NAPRFELV-1] ) < 0 )){

         LE_send_msg( GL_ERROR, "Error setting allowable prf default\n" );

      }


   } /* End of "For All Elevation Cuts" loop. */

   /* Compute the VCP time (duration). Add a 1 second of overhead for each 
      elevation transition. */
   vcp_time = (float) (vcp->n_ele - 1);
   for( i = 0; i < vcp->n_ele; i++ ){

      float rate;

      elev = (Ele_attr *) vcp->vcp_ele[ i ];
      rate = (float) elev->azi_rate * ORPGVCP_RATE_BAMS2DEG;
      vcp_time += 360.0/rate;

   }

   /* Set the VCP is data defined. */
   ORPGVCP_set_vcp_is_data_defined( vcp->vcp_num );
   
   /* Set the VCP time. */
   if( ORPGVCP_set_vcp_time( vcp->vcp_num, (int) (vcp_time + 0.5) ) < 0 ){

      LE_send_msg( GL_ERROR, "Error setting VCP time\n" );
      ORPGVCP_delete( vcp->vcp_num );
      return( -1 );

   }

   /* Verify the RDA to RPG elevation mapping. */
   if( Verify_rdccon( vcp, rdccon ) < 0 ){

      LE_send_msg( GL_ERROR, "rdccon Could Not Be Verified\n" );
      ORPGVCP_delete( vcp->vcp_num );
      return( -1 );

   }

   /* Set the RDA to RPG elevation mapping. */
   for( i = 0; i < vcp->n_ele; i++ ){

      if( ORPGVCP_set_rpg_elevation_num( vcp->vcp_num, i, rdccon[i] ) < 0 ){

         LE_send_msg( GL_ERROR, "Error setting VCP %d rdccon[%d]: %d\n",
                      vcp->vcp_num, i, rdccon[i] );
         ORPGVCP_delete( vcp->vcp_num );
         return( -1 );

      }

   }/* End of "For All Elevation Cuts" loop. */

   /* Write the VCP to adaptation data through the VCP API. */
   if( (ret = ORPGVCP_write( )) < 0 ){

      LE_send_msg( GL_ERROR, "ORPGVCP_write( ) Failed: %d\n",
                   ret );
      ORPGVCP_delete( vcp->vcp_num );
      return( -1 );

   }

   return( 0 );
}

/****************************************************************************

   Description:
      Builds the contents of the rdccon array.

   Inputs:
      vcp - VCP data

   Outputs:
      rdccon - RDA to RPG elevation mapping table.

   Returns:
      -1 on error, 0 otherwise.

   Notes:
      This is a best-guess on the RDA to RPG mapping table.
 
****************************************************************************/
static int Verify_rdccon( Vcp_struct *vcp, short *rdccon ){

   int cnt = 0;
   int ele_angle = -200, wave_type = VCP_WAVEFORM_UNKNOWN;
   int i;
   Ele_attr *elev;

   /* Check the first element of rdccon.  If 0, then the entire table needs
      to be built. */
   if( rdccon[0] == 0 ){

      /* Do For All elevation cuts in the VCP. */
      for( i = 0; i < vcp->n_ele; i++ ){

         elev = (Ele_attr *) vcp->vcp_ele[ i ];

         /* Two adjacent elevations have different angles. */
         if( elev->ele_angle != ele_angle ){

            cnt++;
            ele_angle = elev->ele_angle;

         }

         /* Two adjacent elevations have same angle but the angles
            are not considered part of a split cut. */
         else if( ((elev->wave_type == VCP_WAVEFORM_CD) 
                                    || 
                   (elev->wave_type == VCP_WAVEFORM_STP))
                                    &&
                   (wave_type != VCP_WAVEFORM_CS) ){

            cnt++;
            ele_angle = elev->ele_angle;

         } 

         rdccon[i] = cnt;
         wave_type = elev->wave_type;

      }

   } /* End of "For All Elevation Cuts" loop. */

   /* The entries in rdccon need to be monotonically increasing and the total number
      of entries must equal the number of elevations in the VCP. */  
   for( i = 1; i < vcp->n_ele; i++ ){

      if( (rdccon[i] < rdccon[i-1]) || (rdccon[i] == 0) ){

         LE_send_msg( GL_ERROR, "(rdccon[%d]: %d !< rdccon[%d]: %d) || (rdccon[%d]: %d == 0)\n",
                      i, rdccon[i], i-1, rdccon[i-1], i, rdccon[i] );     
         return( -1 );

      }

   } /* End of "For All Elevation Cuts" loop. */

   return( 0 );

} /* End of Verify_rdccon() */

