/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2012/09/19 19:52:17 $
 * $Id: a3053a.c,v 1.30 2012/09/19 19:52:17 steves Exp $
 * $Revision: 1.30 $
 * $State: Exp $
 */
#define A3053A_C
#include <prfselect.h>

/* Macro defintions. */
#define PROCESS          1
#define ABORT            2
#define UR_SCALE         0.1

/* Static global variables. */

/* File scope static global variables. */
static int Seq_cnt;

/* Function prototypes. */
static void Get_delta_pri( Base_data_header *bhd );
static void Abort_processing( char **inbuf, int *status, int reason );
static void Finish_prf_processing( int vcpatnum, int vcp_num_cuts, 
                                   int *disposition );
static void Check_for_active_vcp_sequence( int vcpatnum, 
                                           int autoprf,
                                           int *disposition,
                                           Prfselect_t *o_prfselect );

/*\////////////////////////////////////////////////////////////////////////////

   Description:
      Buffer control routine for PRF selection algorithm.  Coordinates all
      processing by this function.

      This function will alter PRFs in the VCP if 1) Auto PRF is active and 
      2) there are allowable PRFs for the VCP.  

      The function also support cell-based and storm-based PRF selection.

      This function also checks if any VCP sequences are active.  If a
      sequence is active, this function will attempt to apply the selected
      PRFs to the new VCP in the sequence, regardless if that VCP is the
      same as the current VCP.

      The output of this function is:

         1) disposition of this VCP data.   If disposition is destroy,
            any subsequent data in the output buffer can be ignored.
         2) the VCP to change to on the next VCP restart
         3) the size of the VCP (if non-zero, VCP data is passed)
         4) VCP data (only if size is non-zero)

   Notes:
      The Storm-based PRF selection is activated by operator command.  It
      relies on data from A3d09.  Storm-based PRF will disable itself 
      temporarily (for the current volume scan) if Accd09 data is not 
      available or it is deemed to be too old.  Consequently we have 2 flags:
      Storm_based_PRF_selection, which is operator comtrolled, and a local
      version Local_storm_based_PRF_selection. It is the "local" version 
      which determines what happens in the current volume scan.

////////////////////////////////////////////////////////////////////////////\*/
int A3053A_buffer_control(){

   int status, autoprf, prfprocess, in_status, ret;
   int radial_status, ref_flag, vel_flag, spw_flag;
   int vcpatnum, vcp_num_cuts, volnum, disposition;
   char *inbuf, *outbuf, *scratchbuf;

   Scan_Summary *scansum = NULL;
   Prfselect_t *o_prfselect = NULL;

   /* Variable initializations. */
   status = 0;
   autoprf = 0;
   prfprocess = 0;
   radial_status = 0;
   vcpatnum = 0;
   vcp_num_cuts = 0;
   disposition = FORWARD;
   inbuf = NULL;

   /* Initialize data for Storm-based PRF selection. */
   ST_init();

   /* Initialize data for Cell-based PRF selection. */
   CT_init();

   /* DO UNTIL Program Stat= "Stop" or "Abort", or Radial Stat= "End_Vol"
      Acquire input buffer of radial data. */
   while(1){

      inbuf = RPGC_get_inbuf_by_name( "BASEDATA", &in_status );
      if( (in_status == NORMAL) && (inbuf != NULL) ){

         Base_data_header *bhd = (Base_data_header *) inbuf;

         /* If Radial Status is Beginning of Volume, then ..... */
         radial_status = bhd->status;
         if( radial_status == GOODBVOL ){

            RPGC_log_msg( GL_INFO, "Start of Volume Processing ....\n" );

            /* Get delta PRI. */
            Get_delta_pri( bhd );

            /* Check for disabled moments...ABORT if reflectivity data missing. */
            RPGC_what_moments( bhd, &ref_flag, &vel_flag, &spw_flag );
            if( !ref_flag ){

               /* No reflectivity moment.  ABORT processing. */
               RPGC_log_msg( GL_INFO, "Aborting Because: PROD_DISABLED_MOMENT\n" );
               Abort_processing( &inbuf, &status, PROD_DISABLED_MOMENT );
               break;

            }

            /* Get output buffer for AUTO-PRF. */
            outbuf = scratchbuf = NULL;
            A3053B_get_buffers( &outbuf, &scratchbuf, &autoprf );
            if( outbuf == NULL ){

               /* We don't need to abort here .... should have already aborted
                  int A3053B_get_buffers() */
               status = ABORT;
               break;

            }

            o_prfselect = (Prfselect_t *) outbuf;

            /* Perform beginning of volume scan initialization. */
            RPGC_log_msg( GL_INFO, "Perform Beginning of Volume Initialization ....\n" );

            /* Get Volume Number, Volume Coverage Pattern # (VCP #), 
               and Current Date & Time. */
            volnum = RPGC_get_buffer_vol_num( inbuf );
            scansum = RPGC_get_scan_summary( volnum );
            if( scansum == NULL ){
               
               RPGC_log_msg( GL_INFO, "Aborting Because: RPGC_get_scan_summary( %d ) Failed\n",
                             volnum );
               Abort_processing( &inbuf, &status, 0 );
               break;

            }

            vcpatnum = scansum->vcp_number;
            vcp_num_cuts = ORPGVCP_get_num_elevations( vcpatnum );
 
            /* If Auto-PRF is not active, clear the Storm/Cell-Based PRF Selection
               flags. */
            if( !autoprf ){

               Cell_based_PRF_selection = Storm_based_PRF_selection = 0;
               Local_storm_based_PRF_selection = Storm_based_PRF_selection;
               Prf_status.state = PRF_COMMAND_MANUAL_PRF;
               Prf_status.error_code = PRF_STATUS_NO_ERRORS;
               Prf_status.num_storms = 0;
               disposition = DESTROY;
               memset( &Prf_status.storm_data[0], 0, MAX_STORMS*sizeof(Storm_data_t) );

            }

            /* Read adaptation data to determine the minimum PRF.  Changing the Minimum
               PRF is only allowed if storm/cell-based PRF selection is enabled.  Note:  
               The minimum PRF to process needs to be determined before A30531_prf_init() 
               is called. */
            if( (Cell_based_PRF_selection)
                          ||
                (Storm_based_PRF_selection) )
               CF_read_adapt();

            else{

               int ret = 0;
               double dtemp = DOP_PRF_BEG;

               /* Read adaptation data to determine the minimum PRF for
                  the legacy PRF Selection algorithm.  A different 
                  minimum is assigned for Storm/Cell based PRF Selection.  
                  
                  Note:  The minimum PRF to process needs to be determined 
                         before A30531_prf_init() is called. */
               if( (ret = DEAU_get_values( "alg.prfselect.min_prf_legacy", &dtemp, 1 )) < 0 ){

                  RPGC_log_msg( GL_INFO,
                                "DEAU_get_values( alg.prfselect.min_prf_legacy ) Failed (%d)\n",
                                ret );

                  /* Default to DOP_PRF_BEG. */
                  Min_PRF = DOP_PRF_BEG;

               }
               else
                  Min_PRF = (int) dtemp;

               RPGC_log_msg( GL_INFO, "Minimum Allowable Legacy PRF: %d\n", Min_PRF );

            }

            /* If Auto-PRF is active, perform PRF initialization. */
            if( (autoprf) && (scratchbuf != NULL) ){

               /* Initialize tables for PRF selection processing. */
               RPGC_log_msg( GL_INFO, "Perform PRF Selection Initialization ....\n" );
               if( A30531_prf_init( (Base_data_header *) inbuf, (void *) scratchbuf, 
                                    (void *) (scratchbuf + (EPWR_SIZE*sizeof(float))) ) < 0 ){

                  RPGC_log_msg( GL_INFO, "Aborting Because: A30531_prf_init Failed\n" ) ;
                  Abort_processing( &inbuf, &status, 0 );
                  break;

               }

               prfprocess = 1;

            }

            status = PROCESS;

            /* Read in the Storm Info published by the tracking and forecast 
               algorithm .... */
            CF_update_storm_info();

            /* If storm or cell PRF selection enabled, set the maximum number of 
               storms.... */
            if( (Storm_based_PRF_selection) || (Cell_based_PRF_selection) ){

               /* Initialize the maximum number of storms to process. */
               Max_num_storms = 0;

               /* If the number of storms identified by SCIT is non-zero, then ... */
               if( A3cd09.numstrm > 0 ){

                  /* Set the number of Storms/Cells to track (Max_num_storms). */
                  if( Cell_based_PRF_selection )
                     Max_num_storms = 1;

                  if( Storm_based_PRF_selection )
                     Max_num_storms = Adapt_max_num_storms;

               }

               /* If maximum number of storms to process is 0, then disable Storm/Cell-based
                  PRF selection.  Storm-based is only disabled for this volume scan. */
               if( Max_num_storms == 0 ){

                  /* No storms detected, default to Legacy PRF Selection. */
                  Cell_based_PRF_selection = Local_storm_based_PRF_selection = 0;
                  RPGC_log_msg( GL_INFO, "No Storms To Track ... Default to Legacy PRF Selection\n" );

               }

            }

            /* Storm based PRF selection is enabled.  The following function with identify 
               the storms that we need to track.*/
            if( Local_storm_based_PRF_selection )
               ST_identify_storms();

            /* If Cell-based PRF Selection is active and we do not have a cell to
               track, find one. */
            if( Cell_based_PRF_selection ){

               /* Check to see if we need to select a new cell. */
               if( Num_cells == 0 )
                  CT_identify_storm( &Prf_command.storm_id[0] );

               else
                  CT_update_storm( );

               /* Check if Cell_based_PRF_selection still active.  If not 
                  active, go back to storm-based. */
               if( (!Cell_based_PRF_selection) 
                               && 
                   (Storm_based_PRF_selection) ){

                  RPGC_log_msg( GL_INFO, "!! Falling Back to Storm Based PRF Selection !!\n" );
                  Local_storm_based_PRF_selection = Storm_based_PRF_selection;
                  Max_num_storms = Adapt_max_num_storms;
                  ST_identify_storms();

               }

            }
          
            /* Check if a SZ2 VCP.  If so, read the clutter map and clutter bypass map
               which are used to create a filter map used for SZ2 PRF selection. */
            SZ2_prf_selection = 0;
            if( (vcpatnum >= VCP_MIN_SZ2) && (vcpatnum <= VCP_MAX_SZ2) ){

               int vcp_ind;

               RPGC_log_msg( GL_INFO, "SZ2 PRF Selection Processing This Volume\n" );
               SZ2_prf_selection = 1;
               SZ2_read_clutter();

               Z_snr = DEF_Z_SNR;
               V_snr = DEF_V_SNR;
               vcp_ind = ORPGVCP_index( vcpatnum );
               if( vcp_ind >= 0 ){

                  VCP_ICD_msg_t *vcp = (VCP_ICD_msg_t *) ORPGVCP_ptr( vcp_ind );
                  if( vcp != NULL ){

                     VCP_elevation_cut_data_t *elev =
                         (VCP_elevation_cut_data_t *) &vcp->vcp_elev_data.data[0];
                     Z_snr = elev->refl_thresh/8.0f;

                     elev = (VCP_elevation_cut_data_t *) &vcp->vcp_elev_data.data[1];
                     V_snr = elev->vel_thresh/8.0f;

                  }

               }

               LE_send_msg( GL_INFO, "The VCP Z SNR Threshold: %f, V SNR Threshold: %f\n",
                            Z_snr, V_snr );


            }

            RPGC_log_msg( GL_INFO, "Done with Start of Volume Initialization ....\n" );

         }

         if( (status == PROCESS) && (prfprocess) ){

            /* Process radial for AUTO-PRF. */
            Base_data_radial *radial = (Base_data_radial *) inbuf;

            /* Do we do non-SZ2 or SZ2 PRF selection? */
            if( SZ2_prf_selection )
               SZ2_echo_overlay( &radial->hdr, radial->ref, (float *) scratchbuf,
                                 (float *) (scratchbuf + (EPWR_SIZE*sizeof(float))) );

            else 
               A30532_echo_overlay( &radial->hdr, radial->ref, (float *) scratchbuf,
                                    (float *) (scratchbuf + (EPWR_SIZE*sizeof(float))) );

         }

         /* Release the input buffer. */
         if( inbuf != NULL ){

            RPGC_rel_inbuf( inbuf );
            inbuf = NULL;

         }

         /* If at end of first elevation and AUTOPRF is turned ON, clear the flag
            for PRF processing. */
         if( prfprocess &&  
             (((radial_status == PSEND_ELEV) || (radial_status == END_ELEV)) 
                                            ||
              ((radial_status == PSEND_VOL) || (radial_status == END_VOL))) ) 
            prfprocess = 0;

         /* If AUTO PRF selection function hasn't completed it's processing,
            continue with the next radial.  If Auto PRF is not active, 
            then no need to process the remaining radials in the cut. */
         if( (!prfprocess) || (status != PROCESS) )
            break;

      }
      else{

         /* Input buffer not successfully acquired, abort processing for
            volume scan. */
         Abort_processing( &inbuf, &status, 0 );
         break;

      }

   } /* End of while loop. */


   /* ABORT condition processing.  Destroy buffers not yet released. */
   if( status == ABORT ){

      RPGC_log_msg( GL_INFO, "Status == ABORT.  Release any inputs and destroy outputs ....\n" );
      if( inbuf != NULL ){

         RPGC_rel_inbuf( inbuf );
         inbuf = NULL;

      }

      RPGC_rel_outbuf( scratchbuf, DESTROY );
      RPGC_rel_outbuf( outbuf, DESTROY);

      return 0;

   }

   /* If Auto PRF is active, then finish up Auto PRF processing. */
   if( autoprf ){

      RPGC_log_msg( GL_INFO, "Autoprf Active.  Finishing prf processing ....\n" );
      Finish_prf_processing( vcpatnum, vcp_num_cuts, &disposition );

      /* Release PRF scratch buffer. */
      RPGC_rel_outbuf( scratchbuf, DESTROY );

   }

   /* Finish the output buffer processing ..... */
   if( o_prfselect != NULL ){

      RPGC_log_msg( GL_INFO, "Finish PRFSEL output buffer processing ....\n" );

      /* Set the size and change to VCP in output buffer. */
      o_prfselect->size = sizeof( Vcp_struct );
      o_prfselect->change_to_vcp = vcpatnum;

      /* Check for active VCP sequence ... processing includes switching VCP 
         if VCP sequence is active. */
      Check_for_active_vcp_sequence( vcpatnum, autoprf, &disposition, o_prfselect );

      /* Set disposition and current VCP table in output buffer.  The size and
         change to VCP may have been altered in Check_for_active_vcp_sequence(). */
      o_prfselect->disposition = disposition;
      memcpy( &o_prfselect->vcp, &PS_curr_vcp_tab, o_prfselect->size );

      /* Release the output buffer. */
      RPGC_rel_outbuf( outbuf, FORWARD );

   }
   else
      RPGC_log_msg( GL_INFO, "o_prfselect == NULL !!!!!!!!!\n" );

   /* Publish PRF Selection Algorithm status. */
   ret = RPGC_data_access_write( ORPGDAT_PRF_COMMAND_INFO, (char *) &Prf_status,
                                       sizeof(Prf_status_t), ORPGDAT_PRF_STATUS_MSGID );
   if( ret < 0 )
      RPGC_log_msg( GL_INFO,
             "RPGC_data_access_write( ORPGDAT_PRF_COMMAND_INFO ) Failed: %d\n", ret );

   else{

      RPGC_log_msg( GL_INFO, "PRF Status Published\n" );
      RPGC_log_msg( GL_INFO, "-->State:          %d\n", Prf_status.state );
      RPGC_log_msg( GL_INFO, "-->Error Code:     %d\n", Prf_status.error_code );
      RPGC_log_msg( GL_INFO, "-->Num Storms:     %d\n", Prf_status.num_storms );
      RPGC_log_msg( GL_INFO, "-->Storms Tracked: %d\n", Prf_status.num_storms_tracked );

   }

   /* If have stopped processing radials in mid volume scan, cycle through
      the rest of the radials of the scan without processing any. */
   if( radial_status != END_VOL )
      A3053J_dummy_processor();

   /* Return to main(). */
   return 0;

/* End of A3053A_buffer_control() */
}

/*\////////////////////////////////////////////////////////////////////

      Description:
        This function returns the Delta PRI to be used for subsequent
        processing.

      Inputs: 
        bhd - pointer to Base Data header.

      Outputs:

      Returns: 

////////////////////////////////////////////////////////////////////\*/
static void Get_delta_pri( Base_data_header *bhd ){

   int ur_i, ipri, iprf, k;
   int *unambigr = NULL;
   float ur;

   /* Variable initializations. */
   PS_delta_pri = ORPGVCP_get_delta_pri( );

   /* Get the unambiguous range from the radial header. */
   ur_i = bhd->unamb_range;
   ur = RPGC_NINT( (float) ur_i  * UR_SCALE );

   if( (unambigr = ORPGVCP_unambiguous_range_ptr()) == NULL ){

      RPGC_log_msg( GL_INFO, "ORPGVCP_unambiguous_range_ptr() Returned NULL\n" );
      return;

   }

   /* Determine delta PRI number from unambiguous range in header. */
   k = 0;
   for( ipri = 1; ipri <= DELPRI_MAX; ipri++ ){

      for( iprf = 1; iprf <= PRFMAX; iprf++ ){

         if( ur == unambigr[k] ){

            if( ipri != PS_delta_pri ){

               RPGC_log_msg( GL_INFO, "Delta_pri Mismatch (%d != %d)\n",
                             PS_delta_pri, ipri );
               PS_delta_pri = ipri;

            } /* End of "if( ur == unambigr[k] )" */

            return;

         } /* End of "for( iprf = 1; iprf <= PRFMAX; iprf++ )" */

         k++;

      } /* End of "for( ipri = 1; ipri <= DELPRI_MAX; ipri++ )" */

   }

/* End of Get_delta_pri() */
}

/*\////////////////////////////////////////////////////////////////////

      Description:
        This function Performs abort processing.

      Inputs: 
        inbuf - pointer to pointer to input buffer.
        status - pointer to processing status.
        reason - reason for the abort, or 0.

      Outputs:

      Returns: 

////////////////////////////////////////////////////////////////////\*/
static void Abort_processing( char **inbuf, int *status, int reason ){

   /* Set the processing state to ABORT. */
   *status = ABORT;

   /* Release the input buffer if input buffer aquired. */
   if( *inbuf != NULL ){

      RPGC_rel_inbuf( *inbuf );
      *inbuf = NULL;

   }

   /* Abort task processing. */
   if( reason )
      RPGC_abort_because( reason );

   else
      RPGC_abort();

/* End of Abort_processing() */
}
 
/*\////////////////////////////////////////////////////////////////////

      Description:
        This function performs end of elevation/volume processing.

      Inputs: 
         vcppatnum - VCP number.
         vcp_num_cuts - Number of elevation cuts within the VCP.

      Outputs:
         disposition - output buffer disposition.

      Returns: 
         No return value defined for this function.

////////////////////////////////////////////////////////////////////\*/
static void Finish_prf_processing( int vcpatnum, int vcp_num_cuts, 
                                   int *disposition ){

   /* Insert new PRFs in the current VCP table. If no allowable PRFS,
      the disposition will be DESTROY.  We still need to forward the
      PRF buffer since the downstream consumer is expecting it.  Let
      the downstream consumer destroy the buffer. */
   A30533_change_vcp( vcpatnum, vcp_num_cuts, 
                      (Ele_attr *) PS_curr_vcp_tab.vcp_ele[0],
                      disposition );

   /* If an MPDA VCP, we do not want the current VCP table to
      be downloaded to RDA, even if AUTOPRF is on.  We do want to
      make the VCP data available for viewing at HCI. */
   if( (vcpatnum >= VCP_MIN_MPDA) && (vcpatnum <= VCP_MAX_MPDA) ){

      RPGC_log_msg( GL_INFO, "PRF Selection VCP Disposition <- DESTROY Because:\n" );
      RPGC_log_msg( GL_INFO, "--->VCP_MIN_MPDA (%d) <= VCP (%d) <= VCP_MAX_MPDA (%d)\n",
                    VCP_MIN_MPDA, vcpatnum, VCP_MAX_MPDA );
      *disposition = DESTROY;

   } /* End of "if( (vcpatnum >= VCP_MIN_MPDA) && (vcpatnum <= VCP_MAX_MPDA))" */

/* End of Finish_prf_processing() */
}

/*\////////////////////////////////////////////////////////////////////

      Description:
        This function performs VCP sequence processing.

      Inputs: 
         vcpatnum - VCP pattern number currently in execution.
         autoprf - auto PRF active flag.
         o_prfselect - output buffer.

      Outputs:
         dispositionm - disposition of the output buffer.
         o_prfselect - output buffer.

      Returns: 
         No return value defined for this function.

////////////////////////////////////////////////////////////////////\*/
static void Check_for_active_vcp_sequence( int vcpatnum, int autoprf, 
                                           int *disposition, 
                                           Prfselect_t *o_prfselect ){

   int active_seq, num_seq, retval;
   double dtemp;

   char *temp_buf = NULL;
   Vcp_seq_t *vcp_seq = NULL;

   /* Determine if a sequence is currently active. */
   if( (retval = DEAU_get_values( "VCP_sequence.active_sequence", &dtemp, 1 )) < 0 ){

      RPGC_log_msg( GL_INFO, "DEAU_get_values( VCP_sequence.active_sequence ) Failed (%d)\n",
                   retval );
      return;

   } 

   active_seq = (int) dtemp;

   /* If the active_sequence is >= 0, then active_sequence is "active" */
   if( active_seq < 0 ){

      Seq_cnt = 0;
      return;

   }

   /* A sequence is currently active.  Get the number of defined sequences. */
   if( (retval = DEAU_get_values( "VCP_sequence.number_sequences", &dtemp, 1 )) < 0 ){

      RPGC_log_msg( GL_INFO, "DEAU_get_values( VCP_sequence.number_sequences ) Failed (%d)\n",
                   retval );
      return;

   }

   num_seq = (int) dtemp;

   /* Get the sequence data. */
   if( (retval = DEAU_get_binary_value( "VCP_sequence.sequences", &temp_buf, 0 )) < 0 ){

      RPGC_log_msg( GL_INFO, "DEAU_get_binary_value( VCP_sequence.sequences ) Failed (%d)\n",
                   retval );
      return;

   }

   vcp_seq = (Vcp_seq_t *) temp_buf;

   /* Ensure that the active sequence is less than the number of defined sequences. */
   if( active_seq < num_seq ){

      int seq_num, pos;
      Vcp_struct *vcp_ptr = NULL;

      RPGC_log_msg( GL_INFO, "PRF Selection VCP Sequence Active\n" );

      seq_num = Seq_cnt % vcp_seq[active_seq].num_in_seq;
      Seq_cnt++;

      if( vcp_seq[active_seq].vcps[ seq_num ] != vcpatnum ){

         RPGC_log_msg( GL_INFO, "--->Changing to Next VCP In Sequence: %d\n", 
                      vcp_seq[active_seq].vcps[ seq_num ] );

         o_prfselect->change_to_vcp = vcp_seq[active_seq].vcps[ seq_num ];

         /* Get the VCP data from adaptation data. */
         pos = ORPGVCP_index( (int) o_prfselect->change_to_vcp );
         if( pos >= 0 )
            vcp_ptr = (Vcp_struct *) ORPGVCP_ptr( pos );

         if( vcp_ptr != NULL ){

            /* Copy the VCP data to current VCP table. */
            memcpy( (void *) &PS_curr_vcp_tab, (void *) vcp_ptr, o_prfselect->size );

            /* If Auto-PRF is active and change to vcp is not an MPDA VCP, then .. */
            if( (autoprf) && (vcpatnum < VCP_MIN_MPDA) && (vcpatnum > VCP_MAX_MPDA) ){

               int num_cuts = ORPGVCP_get_num_elevations( o_prfselect->change_to_vcp );
               /* This is where we apply the PRF changes to the new VCP. */
               A30533_change_vcp( o_prfselect->change_to_vcp, num_cuts, 
                                  (Ele_attr *) PS_curr_vcp_tab.vcp_ele[0], disposition );

            }

         }
         else{

            RPGC_log_msg( GL_INFO, "Error Retrieving VCP Data for VCP %d\n", 
                         o_prfselect->change_to_vcp );
            o_prfselect->size = 0;

         } /* End of "if( vcp_ptr != NULL )" */

      } /* End of "if( vcp_seq[active_seq].vcps[ seq_num ] != vcpatnum )" */

   } /* End of "if( active_seq < num_seq )" */

   free( temp_buf );

/* End of Check_for_active_vcp_sequence( ) */
}
