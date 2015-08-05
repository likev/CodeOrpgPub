/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/12/09 23:01:40 $
 * $Id: pbd_handle_sails.c,v 1.13 2014/12/09 23:01:40 steves Exp $
 * $Revision: 1.13 $
 * $State: Exp $
 */  

#include <pbd.h>

/* Macro Definitions. */
#define EST_TRANSITION_TIME	1 

/**************************************************************************

   Description:
      Updates the passed VCP definition to include any supplemental
      scans when SAILS is active.

   Inputs:
      vcp - VCP definition.

   Outputs:
      download_vcp - modified VCP definition to include SAILS.

   Returns:
      0 on success, -1 on failure.

   Notes:
      The insertion angle locations are determined by partitioning the VCP.
      The initial partition is the time from the end of the first split
      cut to the start of the first split cut in the next volume.  We
      assume the VCP retrace time is 10 sec.   

      This partition is divided into sub-partitions based on the number of
      SAILS cuts in the VCP.  For example if there is 1 SAILS cut, we 
      partition into 2 equal sub-partitions.   The SAILS cuts is then
      inserted between the sub-partitions as best we can considering 
      elevation sweeps and not allowing split cuts to be divided.

**************************************************************************/
int HS_update_vcp_for_sails( Vcp_struct *vcp, Vcp_struct **download_vcp ){

   int ind, i, j, k, m, n_suppl_cuts = 1, time_steps[VCP_MAXN_CUTS];
   int last_cut_ind = 0, n_remaining_cuts = 0, n_added_cuts = 0, start_time = 0;
   int insert_cut_ind[PBD_MAX_INSERTED_CUTS], n_inserted = 0, n_local_cuts = 0;
   int partition_time = 0, sails_cut_time = 0,last_elev_angle = 0;
   int insert_time = 0, delta_time = 0;
   float rate = 0.0, scan_time = 0.0, fp_ele;
   short elev_angle;
   unsigned char vel_resolution = 2;
   Vcp_struct *vcp_adapt = NULL, *dl_vcp = NULL;
   Ele_attr *ele = NULL, vcp_local[VCP_MAXN_CUTS];

   /* For testing SZ2 PRF selection. */
   int is_SZ2_VCP = 0;
   
   /* Check if vcp is NULL.  If so, return error. */
   if( vcp == NULL ){

      LE_send_msg( GL_ERROR, "Unabled to Insert Supplemental Cut(s).\n" );
      LE_send_msg( GL_ERROR, "--->VCP Pointer Undefined (NULL)\n" );
      return -1;

   }

   /* SAILS not allowed for MPDA so return error. */
   if( (vcp->vcp_num >= VCP_MIN_MPDA) && (vcp->vcp_num < VCP_MAX_MPDA) )
      return -1;
   
   /* Check if this is an SZ2 VCP. */
   is_SZ2_VCP = 0;
   if((vcp->vcp_num >= VCP_MIN_SZ2) && (vcp->vcp_num <= VCP_MAX_SZ2))
      is_SZ2_VCP = 1;

   LE_send_msg( GL_INFO, "Modify VCP %d for SAILS\n", vcp->vcp_num );

   /* Do some initialization. */
   memset( &time_steps[0], 0, VCP_MAXN_CUTS*sizeof(int) );
   memset( &vcp_local, 0, VCP_MAXN_CUTS*sizeof(Ele_attr) );
   n_suppl_cuts = 0;

   /* Make a local copy of the VCP with any supplemental scans removed. */
   n_suppl_cuts = HS_strip_supplemental_cuts( vcp, &vcp_local[0], &n_local_cuts );
 
   LE_send_msg( GL_INFO, "--->%d Supplemental Cuts Need to be added for each SAILS cut added\n",
                n_suppl_cuts );

   /* To update the VCP definition to include supplemental cuts is a
      multistep process. 

      1. First we establish a VCP definition that does not include any
         supplemental cuts. 

      2. The next step is to determine where the supplemental cut(s)
         should be inserted.  This will be based on where the previous 
         VCP terminated ... i.e., it may depend on AVSET.

      3. The final step is the insert the supplemental cuts and
         updating the VCP definition. */

   /* Get the baseline definition to do some rudimentary validation .... */
   ind = ORPGVCP_index( vcp->vcp_num );
   vcp_adapt = (Vcp_struct *) ORPGVCP_ptr( ind );

   LE_send_msg( GL_INFO, "--->Baseline VCP %d Definition\n", vcp_adapt->vcp_num );
   LE_send_msg( GL_INFO, "------>Size: %d shorts, Cuts: %d\n", vcp_adapt->msg_size, 
                vcp_adapt->n_ele );

   /* Validate the number of cuts in the working copy. */
   if( n_local_cuts != vcp_adapt->n_ele ){

      LE_send_msg( GL_ERROR, "Unable to Insert Supplemental Cut(s).\n" );
      LE_send_msg( GL_ERROR, "--->N Local Cuts (%d) != Baseline # Cuts (%d)\n", 
                   n_local_cuts, vcp_adapt->n_ele );
      return -1;

   }

   /* The following sections of code determine where the supplemental 
      cut should be inserted ... */
   last_cut_ind = n_local_cuts;

   /* If the current VCP number is different from the previous VCP
      number, then the last elevation angle processed for the previous
      VCP can not be used. However if this is the last elevation of 
      the VCP, we know what the termination is for this VCP so this
      termination angle can be used. */
   last_elev_angle = PBD_last_elevation_angle;

   /* If the last elevation of the VCP, bypass this check since
      we know what the termination angle is for the current vcp. */
   if( (!PBD_last_ele_flag) 
                && 
       (vcp->vcp_num != PBD_old_vcp_number) )
      last_elev_angle = PBD_UNDEF_LAST_ELEV_ANG;

   /* The last elevation angle is less than PBD_MIN_LAST_ELEV_ANG, this
      angle cannot be used. */
   if( last_elev_angle < PBD_MIN_LAST_ELEV_ANG ){

      last_elev_angle = PBD_UNDEF_LAST_ELEV_ANG;
      LE_send_msg( GL_INFO, "HS_update_vcp_for_sails: last elev_angle <-- %d\n",
                   PBD_UNDEF_LAST_ELEV_ANG );

   }


   /* Do For All elevations .... */
   ele = &vcp_local[0];

   /* The rate must be determined from the passed data directly instead
      of via the more convenient ORPGVCP_get_azimuth_rate() function.  
      This is because if SZ2 PRF selection is enabled, the rate changes
      with the PRF. */
   rate = ele->azi_rate * ORPGVCP_AZIMUTH_RATE_FACTOR;
   scan_time = 360.0/rate;
   time_steps[0] = (int) (scan_time + 0.5) + EST_TRANSITION_TIME;

   /* PBD debug statements. */
   if( PBD_DEBUG ){

      LE_send_msg( GL_INFO, "time_steps[0]: %d\n", time_steps[0] );
      LE_send_msg( GL_INFO, "--->rate: %f, scan_time: %f\n", 
                   rate, scan_time );
   }

   for( i = 1; i < n_local_cuts; i++ ){

      /* Calculate the elapsed time for this elevation cut. */
      ele = &vcp_local[i];
      rate = ele->azi_rate * ORPGVCP_AZIMUTH_RATE_FACTOR;
      scan_time = 360.0/rate;
      fp_ele = ORPGVCP_BAMS_to_deg( ORPGVCP_ELEVATION_ANGLE, 
                                    ele->ele_angle ) * 10.0;
      elev_angle = (short) Round(fp_ele);

      /* Save the cumulative time for this VCP. */
      time_steps[i] = time_steps[i-1] + (int) (scan_time+0.5) + EST_TRANSITION_TIME;
  
      /* PBD debug statements. */
      if( PBD_DEBUG ){

         LE_send_msg( GL_INFO, "time_steps[%d]: %d\n", i, time_steps[i] );
         LE_send_msg( GL_INFO, "--->rate: %f, scan_time: %f, elev_angle: %d\n", 
                      rate, scan_time, elev_angle );
      }   

      /* If elevation angle matches the last elevation angle of the 
         previous (or current) VCP, assume the next VCP also terminates
         at the same angle. */
      if( elev_angle == last_elev_angle ){

         last_cut_ind = i + 1;
         LE_send_msg( GL_INFO, "SAILS Modified VCP To Terminate at %d (deg*10)\n",
                      elev_angle );
         LE_send_msg( GL_INFO, "------>Last Cut Index: %d\n", last_cut_ind );
         break;

      }
 
   }

   /* We want the time difference between the end of a low level scans and the start
      of the next low level scan to be close to a constant as we can make it.  

      "partition_time" is the time difference between the end of the first low-elevation scan
      and the start of the next low-elevation scan in the next volume.  Not included are 
      elevation transition times or retrace times. 

      At approximate intervals of "partition_time" from the end of the first split cut,
      we need to add a SAILS cut. */
   delta_time = time_steps[last_cut_ind-1] - time_steps[n_suppl_cuts-1];
   partition_time = (int) ((float) delta_time / (float) (PBD_N_sails_cuts + 1));

   /* PBD debug statements. */
   if( PBD_DEBUG ){

      LE_send_msg( GL_INFO, "1.0 PBD_N_sails_cuts: %d, last_cut_ind: %d, n_suppl_cuts: %d\n",
                   PBD_N_sails_cuts, last_cut_ind, n_suppl_cuts );
      LE_send_msg( GL_INFO, "--->partition_time: %d\n", partition_time );

   }

   LE_send_msg( GL_INFO, "Total Elapsed Time for VCP (approx): %d sec ...\n",
                time_steps[last_cut_ind-1] + PBD_N_sails_cuts*time_steps[n_suppl_cuts-1] );

   /* We want to parition the time so that the low-elevation scans, i.e., the SAILS cuts as
      well as the normal beginning of elevation low-elevation scans are equally spaced
      in time ... at least as equally as we can make them. */
   n_inserted = 0;
   sails_cut_time = time_steps[n_suppl_cuts-1];
   start_time = sails_cut_time;
   insert_time = start_time;
   if( PBD_DEBUG )
      LE_send_msg( GL_INFO, "1.1 -->sails_cut_time: %d\n", sails_cut_time );

   for( j = 0; j < PBD_N_sails_cuts; j++ ){

      /* Determine where to insert the next SAILS cuts. */
      insert_time += partition_time;
      LE_send_msg( GL_INFO, "Insert Cut(s) Around the %d sec Mark of VCP\n", insert_time );

      insert_cut_ind[j] = -1;
      for( i = 0; i < last_cut_ind; i++ ){

         Ele_attr *elev = (Ele_attr *) &vcp_local[i];

         /* Insert supplemental cuts at the "insert_time" point in time.
            Make sure we are not inserting in the middle of a split cut. */
         if( (insert_time <= time_steps[i])
                               && 
              (elev->wave_type != VCP_WAVEFORM_CD) ){
       
            /* PBD debug statements. */
            if( PBD_DEBUG ){

               LE_send_msg( GL_INFO, "1.2 --->insert_time: %d <= time_steps[%d]: %d\n",
                            insert_time, i, time_steps[i] );
               LE_send_msg( GL_INFO, "1.3 --->elev->wave_type: %d\n", elev->wave_type );

            }

            /* If the current cut is part of a split cut, insert angle before "i" in
               order to not break up the split cut.  Otherwise, find the minimum
               delta time between "insert_time" and "time_steps" values.  */
            if( (i-1) >= 0 ){

               Ele_attr *lelev = (Ele_attr *) &vcp_local[i-1];

               if( lelev->wave_type == VCP_WAVEFORM_CD ){

                  /* Previous cut is part of a split cut so we do not want to insert before. */
                  insert_cut_ind[j] = i;
                  n_inserted++;

                  /* PBD debug statements. */
                  if( PBD_DEBUG )
                     LE_send_msg( GL_INFO, "2.0 --->inserted_cut_ind[%d]: %d, n_inserted: %d\n", 
                                  j, insert_cut_ind[j], n_inserted );

               }
               else{ 

                  /* Put supplemental cuts after cut having smallest delta between end of cut
                     and insert_time. */
                  int delta_low = abs(insert_time - time_steps[i-1]);
                  int delta_high = abs(insert_time - time_steps[i]);

                  /* PBD debug statements. */
                  if( PBD_DEBUG ) 
                     LE_send_msg( GL_INFO, "2.1 --->Cut # %d, delta_low: %d; Cut # %d, delta_high: %d\n", 
                                  i-1, delta_low, i, delta_high );

                  /* Find the smallest delta. */
                  if( delta_low < delta_high )
                     insert_cut_ind[j] = i;

                  else 
                     insert_cut_ind[j] = i+1;

                  n_inserted++;

                  /* PBD debug statements. */
                  if( PBD_DEBUG ) 
                     LE_send_msg( GL_INFO, "2.2 --->inserted_cut_ind[%d]: %d, n_inserted: %d\n", 
                                  j, insert_cut_ind[j], n_inserted );
                   
               }

            }
            else{ 

               /* There is no previous cut ... Should never happen but is here for 
                  completeness. */
               insert_cut_ind[j] = i;
               n_inserted++;

               /* PBD debug statements. */
               if( PBD_DEBUG ) 
                  LE_send_msg( GL_INFO, "2.3 --->inserted_cut_ind[%d]: %d, n_inserted: %d\n", 
                               j, insert_cut_ind[j], n_inserted );

            }

            break;

         }

      }

   }

   /* Validate insert_cut_ind.  It must be greater than 0 and less than 
      (VCP_MAXN_CUTS-1). */
   for( i = 0; i < n_inserted; i++ ){

      if( (insert_cut_ind[i] <= 0) 
                   || 
          (insert_cut_ind[i] >= (VCP_MAXN_CUTS-1)) ){
   
         LE_send_msg( GL_ERROR, "Unabled to Insert Supplemental Cut(s).\n" );
         LE_send_msg( GL_ERROR, "--->Bad Insert Cut Index: %d\n", insert_cut_ind[i] );
         return -1;

      }

      LE_send_msg( GL_INFO, "--->Supplemental Cuts to be Inserted Before the %5.2f Cut (# %d)\n",
                   ORPGVCP_get_elevation_angle( vcp_adapt->vcp_num, insert_cut_ind[i] ),
                                                insert_cut_ind[i]+1 );

   }

   /* Allocate space for the new VCP. */
   dl_vcp = calloc( 1, sizeof(Vcp_struct) );
   if( dl_vcp == NULL ){

      LE_send_msg( GL_ERROR, "calloc Failed for %d Bytes\n", sizeof(Vcp_struct) );
      return -1;

   }

   /* Save velocity resolution from passed VCP definition. */
   vel_resolution = vcp->vel_resolution;

   /* Transfer the VCP header data. */
   memcpy( (void *) &dl_vcp->msg_size, (void *) &vcp_adapt->msg_size, 
           VCP_ATTR_SIZE*sizeof(short) );

   /* Insert the supplemental cuts at the insert points. */
   k = 0;
   m = 0;
   for( j = 0; j < PBD_N_sails_cuts; j++ ){

      /* Index m tracks to elevation index of the "copy from" vcp 
         table. */
      if( j > 0 )
         m = insert_cut_ind[j-1];

      /* Transfer cut data from where we left off up to the next 
         supplemental cuts. */
      memcpy( (void *) &dl_vcp->vcp_ele[k][0], 
              (void *) &vcp_local[m],
              (insert_cut_ind[j]-m)*sizeof(Ele_attr) );

      /* Increment k.  k tracks to elevation index of the "copy to"
         vcp table.*/
      k += (insert_cut_ind[j]-m);

      /* Insert the supplemental cuts. */
      if( (PBD_test_SZ2_PRF_selection)
                    &&
               (is_SZ2_VCP) ){

         /* Insert the default PRF into the supplement cut. */
         for( i = 0; i < n_suppl_cuts; i++ ){

            memcpy( (void *) &dl_vcp->vcp_ele[k][0], 
                    (void *) &vcp_adapt->vcp_ele[i][0], 
                    sizeof(Ele_attr) );

            /* Increment k. */
            k++;

         }

      }
      else{

         for( i = 0; i < n_suppl_cuts; i++ ){

            memcpy( (void *) &dl_vcp->vcp_ele[k][0], 
                    (void *) &vcp_local[i], 
                    sizeof(Ele_attr) );
       
            /* Increment k. */
            k++;

         }

      }

   }

   /* Transfer cut data above the last supplemental cut. */
   m = insert_cut_ind[PBD_N_sails_cuts-1];
   n_remaining_cuts = vcp_adapt->n_ele - m;
   memcpy( (void *) &dl_vcp->vcp_ele[k][0], (void *) &vcp_local[m], 
           n_remaining_cuts*sizeof(Ele_attr) );

   /* Set the new number of cuts, size and velocity resolution. */
   n_added_cuts = PBD_N_sails_cuts*n_suppl_cuts;
   dl_vcp->n_ele = vcp_adapt->n_ele + n_added_cuts;
   dl_vcp->msg_size = vcp_adapt->msg_size + 
                      n_added_cuts*(sizeof(Ele_attr)/sizeof(short));
   dl_vcp->vel_resolution = vel_resolution;

   /* Prepare for return to caller. */
   *download_vcp = dl_vcp;

   /* Return to caller. */
   return 0;

/* End of HS_update_vcp_for_sails(). */
}


/**************************************************************************

   Description:
      Receives the VCP definition that is to be downloaded.   Checks how 
      many supplemental cuts there should be and strips any supplemental 
      cuts within the VCP definition.

   Inputs:
      vcp - VCP definition

   Outputs:
      vcp_local - VCP definition with any supplemental cuts removed.
      n_cuts - number of elevation cuts in vcp_local.

   Returns:
      The number of supplemental cuts (to be possibly added if SAILS is 
      active. 

**************************************************************************/
int HS_strip_supplemental_cuts( Vcp_struct *vcp, Ele_attr *vcp_local,
                                int *n_cuts ){

   int i, n_suppl_cuts = 0, n_local_cuts = 0;
   Ele_attr *ele = NULL;
   unsigned char wave_types[2];

   /* Do some initialization. */
   *n_cuts = n_local_cuts;
   memset( &wave_types[0], 0, 2 );

   /* The supplemental scan(s) will always have the same elevation
      as the lowest cut(s). */
   ele = (Ele_attr *) &vcp->vcp_ele[0][0];
   memcpy( &vcp_local[0], ele, sizeof(Ele_attr) );
   n_suppl_cuts = 1;
   wave_types[0] = ele->wave_type;
   if( ele->wave_type == VCP_WAVEFORM_CS ){

      ele = (Ele_attr *) &vcp->vcp_ele[n_suppl_cuts][0];

      /* This is the Doppler cut of a split cut. */
      if( ele->wave_type == VCP_WAVEFORM_CD ){

         wave_types[1] = ele->wave_type;
         memcpy( &vcp_local[n_suppl_cuts], ele, sizeof(Ele_attr) );
         n_suppl_cuts++;

      }

   }

   /* Make a working copy of the passed VCP with any supplemental 
      scans removed. */
   n_local_cuts = n_suppl_cuts;
   for( i = n_suppl_cuts; i < vcp->n_ele; i++ ){

      /* Check whether this cut is a supplemental cut.  It will 
         have a matching elevation angle to the lowest cut if it is. */
      ele = (Ele_attr *) &vcp->vcp_ele[i][0];
      if( vcp_local[0].ele_angle == ele->ele_angle ){

         /* This check to allow a SAILS VCP to include a wave_type 
            at the same angle but not of the wave_type(s) of the 
            SAILS cut(s). */
         if( (ele->wave_type == wave_types[0])
                             ||
             (ele->wave_type == wave_types[1]) )
         continue;

      }

      /* Copy this data. */
      memcpy( &vcp_local[n_local_cuts], ele, sizeof(Ele_attr) );
      n_local_cuts++;

   }

   /* Return to caller. */
   *n_cuts = n_local_cuts;
   return( n_suppl_cuts );

/* End of HS_strip_supplemental_cuts() */
}


/******************************************************************

   Description:
      Updates the VCP definition when SAILS is active and AVSET 
      has changed the termination angle from what is expected.
      Also checks if SAILS has been turned on/off since start of 
      volume.  If the state has changed and the last commanded VCP
      is SAILS allowable, an appropriate change to the VCP
      definition is made and the modified VCP is downloaded.  

   Inputs:
      Vs_gsm - pointer to Volume Status.
      last_elevation_angle - Last elevation angle scanned last
                             VCP.
   Returns:
      Always returns 0.

******************************************************************/
int HS_update_vcp( Vol_stat_gsm_t *Vs_gsm, int last_elevation_angle ){

   Vcp_struct *vcp = NULL;
   char *download_vcp = NULL;
   int current_sails_state = -1;
   int update_vcp = 0;

   static int local_last_commanded_vcp = 0;
   static int n_sails_cuts = PBD_DEFAULT_N_SAILS_CUTS;

   /* Check if the last commanded vcp number is the same VCP number we might 
      be updating.  If not and last commanded VCP does not support SAILS, 
      return to caller. If the last commanded VCP supports SAILS, then 
      determine what to do depending on whether the last elevation in the 
      VCP has changed or the SAILS state has changed. 

      Note:  The last commanded VCP should be the VCP we expect to execute 
             in the next volume scan. */
   if( PBD_last_commanded_vcp != PBD_vcp_number ){

      short vcp_flags = ORPGVCP_get_vcp_flags( PBD_last_commanded_vcp );

      /* We don't want to output this message every elevation. */
      if( PBD_last_commanded_vcp != local_last_commanded_vcp ){

         LE_send_msg( GL_INFO, "Last Commanded VCP (%d) different from Current (%d)\n",
                      PBD_last_commanded_vcp, PBD_vcp_number );
         LE_send_msg( GL_INFO, "--->SAILS VCP will not be updated for AVSET.\n" );
         local_last_commanded_vcp = PBD_last_commanded_vcp;

      }

      /* Check if the last commanded VCP allows SAILS.   If not, return.
         Otherwise, continue on ...... */
      if( (vcp_flags & VCP_FLAGS_ALLOW_SAILS) == 0 )
         return 0;

   }

   /* Get the current SAILS state.  Treat no SAILS cuts as SAILS disabled. */
   current_sails_state = ORPGINFO_is_sails_enabled();
   if( (current_sails_state) && (PBD_N_sails_cuts == 0) )
      current_sails_state = 0;

   /* SAILS information. */
   if( PBD_last_ele_flag ){

      LE_send_msg( GL_INFO, "HS_update_vcp ........\n" );
      LE_send_msg( GL_INFO, "--->PBD_last_elevation_angle: %d, last_elevation_angle: %d\n",
                   PBD_last_elevation_angle, last_elevation_angle );
      LE_send_msg( GL_INFO, "--->PBD_vcp_number: %d, PBD_old_vcp_number: %d\n",
                   PBD_vcp_number, PBD_old_vcp_number );
      LE_send_msg( GL_INFO, "--->PBD_sails_enabled: %d, current_sails_state: %d\n", 
                   PBD_sails_enabled, current_sails_state );
      LE_send_msg( GL_INFO, "--->Is this the Last Elevation? %d\n", PBD_last_ele_flag );
      LE_send_msg( GL_INFO, "--->PBD_N_sails_cuts_this_vol: %d\n", PBD_N_sails_cuts_this_vol );

   }

   /* Take in account AVSET Termination Angle.  If the same VCP as last, check 
      if this VCP terminated at a different angle than last.  If a different 
      VCP, get the last elevation angle in the last VCP and compare to the current
      last elevation angle.  If the current VCP does not have SAILS cuts but
      SAILS is enabled, update the VCP. */ 
   update_vcp = 0;
   if( PBD_vcp_number == PBD_old_vcp_number ){

      if( PBD_last_elevation_angle != last_elevation_angle ) 
         update_vcp = 1;

   }
   else{

      /* Is this the last elevation of the VCP? */
      if( PBD_last_ele_flag ){

         /* Determine how many cuts this VCP should have had. */
         int n_ele = ORPGVCP_get_num_elevations( PBD_vcp_number );
         int el_angle = (int) ((ORPGVCP_get_elevation_angle( PBD_vcp_number, n_ele-1 )) * 10.0);
         
         /* Did we terminate the VCP? */
         if( PBD_last_elevation_angle != el_angle )
            update_vcp = 1;
         
         /* Is SAILS active but the current VCP has no SAILS Cuts? */
         if( (current_sails_state) && (PBD_N_sails_cuts_this_vol == 0) )
            update_vcp = 1;

      }

   }

   /* Check if the SAILS state has changed since the start of volume.  
      Check if the number of sails cuts (per adaptation data) has changed. */
   if( (update_vcp) 
            ||
        (PBD_sails_enabled != current_sails_state)
            ||
        (n_sails_cuts != PBD_N_sails_cuts) ){

      /* Write out informational message on why we are modifying and 
         downloading the VCP. */
      if( PBD_sails_enabled != current_sails_state ){

         if( current_sails_state == 1 ){
 
            LE_send_msg( GL_INFO, "SAILS has been enabled ... was disabled\n" );
            LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, "SAILS State=ON\n" );

         }
         else if( current_sails_state == 0 ){

            LE_send_msg( GL_INFO, "SAILS has been disabled ... was enabled\n" );
            LE_send_msg( GL_STATUS | LE_RPG_GEN_STATUS, "SAILS State=OFF\n" );

         }

         if( current_sails_state >= 0 )
            PBD_sails_enabled = current_sails_state;

      }

      if( PBD_last_elevation_angle != last_elevation_angle ) 
         LE_send_msg( GL_INFO, "Termination Angle for VCP Has Changed: Was %d, Is %d\n",
                      last_elevation_angle, PBD_last_elevation_angle );

      if( n_sails_cuts != PBD_N_sails_cuts ){

         LE_send_msg( GL_INFO, "Number of SAILS Cuts has changed.  Was %d, Is %d\n",
                      n_sails_cuts, PBD_N_sails_cuts );

         n_sails_cuts = PBD_N_sails_cuts;

      }

      /* Get VCP data to potentially modify for SAILS. */
      vcp = calloc( 1, sizeof(Vcp_struct) );
      if( vcp != NULL ){

         int vcp_num = PBD_vcp_number;

         if( PBD_vcp_number == PBD_last_commanded_vcp )
            memcpy( (void *) vcp, (void *) &Vs_gsm->current_vcp_table,
                    sizeof(Vcp_struct) );

         else{

            /* Get VCP data from adaptation data. */
            int index = ORPGVCP_index( PBD_last_commanded_vcp );
            short *ptr = ORPGVCP_ptr( index );

            /* If ptr == NULL, some error occurred. */
            if( ptr == NULL ){

               LE_send_msg( GL_ERROR, "Error Retrieving VCP %d data\n",
                            PBD_last_commanded_vcp );
               return 0;

            }

            vcp_num = PBD_last_commanded_vcp;
            memcpy( (void *) vcp, (void *) ptr, sizeof(Vcp_struct) );

         }
            
         /* This call does the actual SAILS processing. */
         HS_check_sails( vcp_num, vcp, (Vcp_struct **) &download_vcp );
         free( vcp );

         /* Download this VCP to RDA. */
         if( download_vcp != NULL ){

            /* Pass the VCP definition. */
            if( ORPGRDA_send_cmd( COM4_DLOADVCP, PBD_INITIATED_RDA_CTRL_CMD, 
                                  0, 1, 0, 0, 0, (char *) download_vcp, 
                                  sizeof(Vcp_struct) ) < 0 )
               LE_send_msg( GL_ERROR, "Unable to Command VCP Download\n" );

            else{

               /* Set the last commanded VCP. */
               PBD_last_commanded_vcp = PBD_vcp_number;

               /* Write out the VCP data ... */
               if( PBD_verbose )
                  VM_write_vcp_data( (Vcp_struct *) download_vcp );

            }

            /* Free buffer that was used to download the modified VCP. */
            free(download_vcp);

         }

      }

   }

   return 0;

/* End of HS_update_vcp(). */
}

/******************************************************************

   Description:
      Check is SAILS is active and whether this VCP supports SAILS.
      If both true, call HS_update_vcp_for_sails().

   Inputs:
      vcp_num - Volume Coverage Pattern to validate.
      vcp - volume coverage pattern data (optional)

   Outputs:
      download_vcp - set to non-NULL if the VCP to be download was 
                     modified, e.g., SAILS is enabled and the VCP
                     is allowed to be modifed.

   Returns:
      Always returns 0.

   Notes:
      If vcp == NULL, then the call to HS_update_vcp_for_sails() 
      returns error. 

******************************************************************/
int HS_check_sails( int vcp_num, Vcp_struct *vcp, 
                    Vcp_struct **download_vcp ){

   int local_sails_enabled = 0, allow_sails = 0;
   short vcp_flags = 0, max_n_sails_cuts = 0;

   /* Check if SAILS is allowed. */
   vcp_flags = ORPGVCP_get_vcp_flags( vcp_num );
   if( vcp_flags & VCP_FLAGS_ALLOW_SAILS ){

      LE_send_msg( GL_INFO, "SAILS Allowed for this VCP (%d).\n", vcp_num );
      allow_sails = 1;

      /* Get the maximum number of allowable SAILS cuts. */
      max_n_sails_cuts = 
         (vcp_flags & VCP_MAX_SAILS_CUTS_MASK) >> VCP_MAX_SAILS_CUTS_SHIFT;

   }

   /* Get the SAILS enabled state. */
   local_sails_enabled = ORPGINFO_is_sails_enabled();

   /* We normally only update PBD_sails_enabled at start of volume.  If
      the RDA is not in Operate State and a VCP is downloaded, we should
      be able to update the VCP for SAILS if needed. */
   if( PH_get_rda_status( RS_RDA_STATUS ) != RDA_STATUS_OPERATE ){

      PBD_sails_enabled = local_sails_enabled;

      /* Also check the DEAU value for the number of SAILS cuts allowed. 
         The default is used if the following function call fails. */
      if( (PBD_N_sails_cuts = ORPGSAILS_get_req_num_cuts()) < 0 ){

         LE_send_msg( GL_ERROR, "ORPGSAILS_get_req_num_cuts() Failed\n" );
         PBD_N_sails_cuts = 1;

      }

      /* Clip the maximum, if necessary. */
      if( PBD_N_sails_cuts > max_n_sails_cuts ){

         LE_send_msg( GL_INFO, "PBD_N_sails_cuts Truncated to %d From %d For VCP: %d\n",
                      max_n_sails_cuts, PBD_N_sails_cuts, vcp_num );
         PBD_N_sails_cuts = max_n_sails_cuts;

      }

   }

   /* Treat no SAILS cuts as SAILS disabled. */
   if( (local_sails_enabled) && (PBD_N_sails_cuts == 0) )
      local_sails_enabled = 0;

   /* Check if SAILS is active. */
   if( allow_sails ){

      /* Is SAILS enabled? */
      if( local_sails_enabled ){

         /* SAILS is enabled. */
         LE_send_msg( GL_INFO, "SAILS Enabled and Allowed for this VCP\n" );
         HS_update_vcp_for_sails( vcp, download_vcp );

      }
      else{

         int n_cuts = 0;
         Vcp_struct *vcp_local = NULL;

         vcp_local = (Vcp_struct *) calloc( 1, sizeof(Vcp_struct) );
         if( vcp_local == NULL ){

            /* calloc failed so there is nothing to do .... */
            LE_send_msg( GL_INFO, "calloc Failed for %d bytes\n",
                         sizeof(Vcp_struct) );
            return 0;

         }

         /* SAILS is not enabled.  Check if there are any supplemental cuts 
            and strip them if there are. */
         HS_strip_supplemental_cuts( vcp, (Ele_attr *) &vcp_local->vcp_ele[0][0],
                                     &n_cuts );

         /* Same number of cuts .... no supplemental cuts stripped so 
            no need for the vcp_local data. */
         if( vcp->n_ele == n_cuts )
            free(vcp_local);

         else{

            /* Different number of cuts so there must have been some supplemental
               cuts stripped.  Copy header from original VCP definition to vcp_local. */
            memcpy( (void *) &vcp_local->msg_size, (void *) &vcp->msg_size,
                    VCP_ATTR_SIZE*sizeof(short) );

            /* Set the new size and number of elevations. */
            vcp_local->n_ele = n_cuts;
            vcp_local->msg_size = VCP_ATTR_SIZE +
                                  n_cuts*(sizeof(Ele_attr)/sizeof(short));

            /* Prepare for return to caller ... Set the download_vcp 
               pointer.  download_vcp should be a complete VCP definition
               with any supplemental cuts stripped. */
            *download_vcp = vcp_local;

         }

      }

   }

   return 0;

/* End of HS_check_sails() */
}

