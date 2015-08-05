/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2014/11/07 21:24:34 $
 * $Id: crda_download_vcp.c,v 1.46 2014/11/07 21:24:34 steves Exp $
 * $Revision: 1.46 $
 * $State: Exp $
 */
#include <crda_control_rda.h>
#include <orpg.h>
#include <itc.h>
#include <vcp.h>
#include <orpgadpt.h>
#include <siteadp.h>

/* File scope global variables. */
/* Flag which indicates if the Current VCP Table has been allocated. */
static int Vcp_tab_initialized = DV_FAILURE;

/* Structure defining current VCP table. */
static Vol_stat_gsm_t *Curr_vcp_tab;

/* Local functions. */
static int Validate_vcp_selection( int vcp_num );
static int Convert_vcp_to_icd_format( Vcp_struct *vcp,
                                      short **message_data,
                                      int *message_size );
static void Write_vcp_data( Vcp_struct *vcp );

/*\///////////////////////////////////////////////////////////////
//
//   Description:   
//      Allocate buffer space for Current VCP Table.
//
//   Inputs:
//   
//   Outputs:
//
//   Returns:  
//      DV_FAILURE on failure, or DV_SUCCESS on success.
//
//////////////////////////////////////////////////////////////\*/
int DV_init_adaptation_data(){

   /* Allocate storage to hold Current VCP Table. */
   if ( (Curr_vcp_tab = (Vol_stat_gsm_t *) calloc( (size_t) 1, 
                               (size_t) sizeof(Vol_stat_gsm_t) )) == 0 ){

      LE_send_msg( GL_MEMORY, "Current VCP Table Calloc Failed\n" );
      Vcp_tab_initialized = DV_FAILURE;

   }
   else{

      /* Set flag Current VCP table has been allocated. */
      Vcp_tab_initialized = DV_SUCCESS;

   }

   return ( Vcp_tab_initialized );

/* End of DV_init_adaptation_data() */
}

/*\////////////////////////////////////////////////////////////////////
//
//
//   Description:  
//      The vcp number is validated.  If valid number, the corresponding 
//      VCP data is sent to Convert_vcp_to_icd_format to converted to 
//      RDA/RPG ICD format.
//
//   Inputs:  
//      rda_command - RDA Control command.
//
//   Outputs:
//      message_data - pointer to pointer where VCP data is to be stored.
//      message_size - pointer to size of message, in shorts.
//
//   Returns:   
//      DV_FAILURE on error, or DV_SUCCESS on success. 
//
/////////////////////////////////////////////////////////////////////\*/
int DV_process_vcp_download( Rda_cmd_t *rda_command,
                             short **message_data,
                             int *message_size ){

   int vcp_ind, ret;
   Vcp_struct *vcp, *vcp_baseline;

   int param3 = rda_command->param3;

   /* Cannot download VCP if RDA in LOCAL control. */
   if( CR_control_status == CS_LOCAL_ONLY ){

      LE_send_msg( GL_ERROR, "RDA in LOCAL control.  Cannot download VCP.\n" );
      return( DV_FAILURE );

   }

   /* Parameter definitions for the Download VCP command (see also rda_commands.5
      man page):

      Parameter 1:
         Defines the VCP number.  Can be 0 if "current_vcp" or a valid VCP number. 

      Parameter 2:
         Defines whether VCP data is available within the command.
         A value of 1 indicates VCP data was passed with the command and 
         rda_command->msg should be NON-NULL.  

      Parameter 3:
         Is used to support VCP Translation.  A value of VCP_DO_NOT_TRANSLATE
         indicates that the VCP is not to be translated prior to handing off
         to the RDA.  */

   /* If VCP number is not zero, validate selection, then data needs
      to be extracted from command or adaptation data, and converted 
      to ICD format. */
   if( rda_command->param1 != 0 ){

      if( (vcp_ind = Validate_vcp_selection( rda_command->param1 )) < 0 )
         return ( DV_FAILURE );
            
      /* VCP data is not passed with command.  Use baseline (adaptation) 
         version. */
      if( (rda_command->param2 == 0) || (strlen( rda_command->msg ) == 0) ){

         /* Get pointer to VCP data. */
         vcp = (Vcp_struct *) ORPGVCP_ptr( vcp_ind );

      }
      else{

         /* VCP data is passed with command. */
         vcp = (Vcp_struct *) rda_command->msg; 

      }

      /* Code added to support VCP Translation. */
      if( param3 == VCP_DO_NOT_TRANSLATE ){

            LE_send_msg( GL_INFO, "Case 1: Setting Spare 1: VCP DO NOT TRANSLATE\n" );
            vcp->spare1 = VCP_DO_NOT_TRANSLATE;

      }

      /* Reformat VCP data to ICD format. */
      return( Convert_vcp_to_icd_format( vcp, message_data, 
                                         message_size ) );

   } 
   else{

      /* VCP number if zero.  VCP data is passed with command. */
      if( (rda_command->param2 == 1) && (strlen( rda_command->msg ) != 0) ){

         vcp = (Vcp_struct *) rda_command->msg; 

         /* Code added to support VCP Translation. */
         if( param3 == VCP_DO_NOT_TRANSLATE ){

            LE_send_msg( GL_INFO, "Case 2: Setting Spare 1: VCP DO NOT TRANSLATE\n" );
            vcp->spare1 = VCP_DO_NOT_TRANSLATE;

         }

         /* Reformat VCP data to ICD format. */
         return( Convert_vcp_to_icd_format( vcp, message_data, message_size ) );

      }
      else{ 

         /* VCP number is zero and VCP is not passed with command.. Check if the 
            current VCP table space has been allocated.  If not, re-attempt 
            initialization.  If it fails again, return error.  

            Read the VCP data from adaptation data (if MPDA VCP) or Volume Status. */
         if( Vcp_tab_initialized != DV_SUCCESS ){

            if( (ret = DV_init_adaptation_data()) != DV_SUCCESS )
               return ( DV_FAILURE );

         }

         /* Read VCP data from Volume Status.  VCP specified is the Current VCP.
            If read fails or data is not of the expected size, return error. */
         if( ((ret = ORPGDA_read( ORPGDAT_GSM_DATA, (char*) Curr_vcp_tab, 
                           sizeof( Vol_stat_gsm_t ), VOL_STAT_GSM_ID )) < 0)
                                 ||
               (ret != sizeof( Vol_stat_gsm_t )) ){

            LE_send_msg( GL_STATUS | LE_RPG_WARN_STATUS, 
                         "Current VCP Read Failed!  Cannot Download Current VCP to RDA.\n" );
            LE_send_msg( GL_ERROR, "Read Returned: %d, Size Expected: %d\n", ret,
                         sizeof( Vol_stat_gsm_t ) );
            return ( DV_FAILURE );

         }
           
         /* Adjust pointer to point to VCP data. */
         vcp = (Vcp_struct *) &(Curr_vcp_tab->current_vcp_table);

         /* If this is an MPDA VCP, read the VCP data from adaptation data.  
            Then change only those field which can be modified. */
         if( (vcp->vcp_num >= VCP_MIN_MPDA) && (vcp->vcp_num <= VCP_MAX_MPDA) ){

            int i;

            vcp_ind = ORPGVCP_index( vcp->vcp_num );
            vcp_baseline = (Vcp_struct *) ORPGVCP_ptr( vcp_ind );

            if( (vcp_baseline == NULL) || (vcp_baseline->vcp_num != vcp->vcp_num) ){

               LE_send_msg( GL_ERROR, "Unable to download VCP %d\n", vcp->vcp_num );
               return( DV_FAILURE );

            }

            /* Set the velocity resolution. */
            vcp_baseline->vel_resolution = vcp->vel_resolution;

            /* Transfer the SNR threshold data. */
            for( i = 0; i < vcp_baseline->n_ele; i++ ){

               Ele_attr *elv_info = (Ele_attr *) &vcp_baseline->vcp_ele[i][0];
               Ele_attr *elv_tmp = (Ele_attr *) &vcp->vcp_ele[i][0];

               elv_info->surv_thr_parm = elv_tmp->surv_thr_parm;
               elv_info->vel_thrsh_parm = elv_tmp->vel_thrsh_parm;
               elv_info->spw_thrsh_parm = elv_tmp->spw_thrsh_parm;

            }

            /* Code added to support VCP Translation. */
            if( param3 == VCP_DO_NOT_TRANSLATE ){

               LE_send_msg( GL_INFO, "Case 3: Setting Spare 1: VCP DO NOT TRANSLATE\n" );
               vcp->spare1 = VCP_DO_NOT_TRANSLATE;

            }

            /* Reformat VCP data to ICD format. */
            return( Convert_vcp_to_icd_format( vcp_baseline, message_data, message_size ) );

         }

         /* Code added to support VCP Translation. */
         if( param3 == VCP_DO_NOT_TRANSLATE ){

            LE_send_msg( GL_INFO, "Case 4: Setting Spare 1: VCP DO NOT TRANSLATE\n" );
            vcp->spare1 = VCP_DO_NOT_TRANSLATE;

         }

         /* Reformat VCP data to ICD format. */
         return( Convert_vcp_to_icd_format( vcp, message_data, message_size ) );

      }

   }
 
/* End of DV_process_vcp_download() */
}

/*\///////////////////////////////////////////////////////////////////
//
//   Description:  
//      The vcp number is validated.  
//
//   Inputs:  
//      vcp_num - Volume Coverage Pattern to validate.
//
//   Outputs:
//
//   Returns:   
//      DV_FAILURE on error, or vcp index otherwise.
//
/////////////////////////////////////////////////////////////////////\*/
static int Validate_vcp_selection( int vcp_num ){

   int vcp_ind;

   if( (vcp_ind = ORPGVCP_index( vcp_num )) < 0 ){

      /* Selection not valid.  Report error and return. */
      LE_send_msg( GL_ERROR, "Requested VCP For Download Unrecognized\n" );
      return ( DV_FAILURE );

   }

   /* Valid VCP selection.  Return index of VCP in VCP table. */
   return (vcp_ind);

/* End of Validate_vcp_selection() */
}

/*\//////////////////////////////////////////////////////////////////
//
//   Description:  
//      The vcp data is converted to RPG/RDA format and stored in
//      message data.
//
//   Inputs:  
//      vcp - The VCP data to be downloaded.
//
//   Outputs:
//      message_data - Pointer to pointer where VCP data is to be 
//                     stored.
//      message_size - Size of the message, in shorts.
//
//   Returns:   
//      DV_FAILURE on error, DV_SUCCESS otherwise.
//
//////////////////////////////////////////////////////////////////\*/
static int Convert_vcp_to_icd_format( Vcp_struct *vcp, short **message_data,
                                      int *message_size ){

   Vcp_struct *vcp_data;
   Ele_attr *rpg_vcp_data;

   short number_elevations;
   int i;

   /* Allocate space for VCP. */
   if( (vcp_data = (Vcp_struct *) calloc( 1, sizeof(Vcp_struct) )) == NULL ){

      LE_send_msg( GL_MEMORY, "VCP Data Calloc Failed\n" );
      return ( DV_FAILURE );

   }

   /* Transfer the VCP information to local buffer. */
   memcpy( vcp_data, vcp, sizeof(Vcp_struct) );

   /* Set the velocity resolution. */
   vcp_data->vel_resolution = CR_velocity_resolution;
   LE_send_msg( GL_INFO, "Convert_vcp_to_icd_format: Setting Vel Reso to %d\n",
                vcp_data->vel_resolution );

   /* Make a copy of the last VCP downloadd. */
   memcpy( &CR_last_downloaded_vcp_data, vcp_data, sizeof(Vcp_struct) );
   CR_last_commanded_vcp = vcp_data->vcp_num;

   /* Do For All elevation cuts .... */
   number_elevations = vcp->n_ele;

   for( i = 0; i < number_elevations; i++ ){

      /* Cast start of elevation attributes to Ele_attr type structure. */
      rpg_vcp_data = (Ele_attr *) &(vcp_data->vcp_ele[i][0]);

      /* Set the SR flag (VCP_HALFDEG_RAD) regardless of current SR state. 
         RDA treats this flag as the "allowable" flag. */
      if( (rpg_vcp_data->wave_type == VCP_WAVEFORM_CS)
                              ||
          (rpg_vcp_data->wave_type == VCP_WAVEFORM_CD) )
         rpg_vcp_data->super_res |= VCP_HALFDEG_RAD;

      /* Sectors 1/2/3 azimuth angles need to be converted to BAMS. */
      rpg_vcp_data->azi_ang_1 =  (short) ORPGVCP_deg_to_BAMS( ORPGVCP_AZIMUTH_ANGLE,
                                                (float) rpg_vcp_data->azi_ang_1/10.0 );
      rpg_vcp_data->azi_ang_2 =  (short) ORPGVCP_deg_to_BAMS( ORPGVCP_AZIMUTH_ANGLE,
                                                (float) rpg_vcp_data->azi_ang_2/10.0 );
      rpg_vcp_data->azi_ang_3 =  (short) ORPGVCP_deg_to_BAMS( ORPGVCP_AZIMUTH_ANGLE,
                                                (float) rpg_vcp_data->azi_ang_3/10.0 );

   /* End of "for" loop. */
   }

   /* Set the message_data pointer and message_size (in shorts). */
   *message_data = (short *) vcp_data; 
   *message_size = vcp_data->msg_size;

   Write_vcp_data( vcp_data );

   return ( DV_SUCCESS );

/* End of Convert_vcp_to_icd_format() */
}

/*\/////////////////////////////////////////////////////////////////
//
//   Description:  
//      The default vcp number for the specified weather mode is
//      determined from adaptation data.
//
//   Inputs:  
//      weather_mode - Weather mode (Maintenance, Clear Air, or
//                     Precipitation).
//
//   Outputs:
//
//   Returns:  
//      DV_FAILURE on error, default vcp number for weather mode 
//      otherwise. 
//
//   Notes:
//      The default VCP for the weather mode is assumed to be the 
//      first VCP number listed in the weather mode table for the 
//      weather mode.
//
///////////////////////////////////////////////////////////////////\*/
int DV_get_default_vcp_for_wxmode( int weather_mode ){

   int	vcp_num	= 0;
   int	vcp_ind	= 0;
   double value1, value2;

   /* The default weather mode is defined in the site adaptation data */
   /* group.  It can be either "Precipitation" or "Clear Air".        */
   if(  (DEAU_get_values( "site_info.def_mode_A_vcp", &value1, 1) > 0)
                               &&
        (DEAU_get_values( "site_info.def_mode_B_vcp", &value2, 1) > 0) ){

      if( PRECIPITATION_MODE == weather_mode )
         vcp_num = (int) value1;
       
      else if( CLEAR_AIR_MODE == weather_mode )
         vcp_num = (int) value2;
       
      else
         return( DV_FAILURE );
       
      /* Verify that the selected VCP is a valid VCP. */
      if( (vcp_ind = Validate_vcp_selection( vcp_num )) < 0 )
         return ( DV_FAILURE );

   }
   else{

      LE_send_msg( GL_ERROR, "DEAU_get_values(site_info.def_mode_x_vcp) failed\n" );
      return ( DV_FAILURE );

   }
            
   return (vcp_num);

} /* End of DV_get_default_vcp_for_wxmode() */

/********************************************************************************
   
   Description:
      Writes out the VCP definition to log file.

   Inputs:
      vcp - pointer to VCP data ... format specified in vcp.h.

********************************************************************************/
static void Write_vcp_data( Vcp_struct *vcp ){

   static char *reso[] = { "0.5 m/s", "1.0 m/s" };
   static char *width[] = { "SHORT", "LONG" };
   static char *wave_form[] = { "UNK", "CS", "CD/W", "CD/WO", "BATCH", "STP" };
   static char *phase[] = { "CON", "RAN", "SZ2" };

   int i, expected_size;
   short wform, phse;

   /* Write out VCP data. */
   LE_send_msg( GL_INFO, "\n\nVCP %d Data:\n", vcp->vcp_num );
   LE_send_msg( GL_INFO, "--->VCP Header:\n" );
   LE_send_msg( GL_INFO, "       Size (shorts): %4d   Type: %4d   # Elevs: %4d\n",
            vcp->msg_size, vcp->type, vcp->n_ele );
   LE_send_msg( GL_INFO, "       Clutter Group: %4d   Vel Reso: %s   Pulse Width: %s\n\n",
            vcp->clutter_map_num, reso[ vcp->vel_resolution/4 ],
            width[ vcp->pulse_width/4 ] );

   /* Do some validation. */
   expected_size = VCP_ATTR_SIZE + vcp->n_ele*(sizeof(Ele_attr)/sizeof(short));
   if( vcp->msg_size != expected_size )
      LE_send_msg( GL_ERROR, "VCP Size: %d Not Expected: %d\n",
                   vcp->msg_size, expected_size );

   /* Do For All elevation cuts. */
   for( i = 0; i < vcp->n_ele; i++ ){

      Ele_attr *elev = (Ele_attr *) &vcp->vcp_ele[i][0];

      float elev_angle = ORPGVCP_BAMS_to_deg( ORPGVCP_ELEVATION_ANGLE, elev->ele_angle );
      float azi_1 = ORPGVCP_BAMS_to_deg( ORPGVCP_AZIMUTH_ANGLE, elev->azi_ang_1 );
      float azi_2 = ORPGVCP_BAMS_to_deg( ORPGVCP_AZIMUTH_ANGLE, elev->azi_ang_2 );
      float azi_3 = ORPGVCP_BAMS_to_deg( ORPGVCP_AZIMUTH_ANGLE, elev->azi_ang_3 );

      wform = elev->wave_type;
      phse = elev->phase;
      LE_send_msg( GL_INFO, "--->Elevation %d:\n", i+1 );
      LE_send_msg( GL_INFO, "       Elev Angle: %5.2f   Wave Type: %s   Phase: %s   Super Res Flag: %3d, Surv PRF: %2d   Surv Pulses: %4d\n",
                   elev_angle, wave_form[ wform ], phase[ phse ],
                   elev->super_res, elev->surv_prf_num, elev->surv_pulse_cnt );
      LE_send_msg( GL_INFO, "       Az Rate: %5.2f (0x%4x BAMS)   SNR Threshold: %5.2f  %5.2f  %5.2f (dB)\n",
                   elev->azi_rate*ORPGVCP_AZIMUTH_RATE_FACTOR, elev->azi_rate, (float) elev->surv_thr_parm/8.0,
                   (float) elev->vel_thrsh_parm/8.0, (float) elev->spw_thrsh_parm/8.0 );

      LE_send_msg( GL_INFO, "       PRF Sector 1:\n" );
      LE_send_msg( GL_INFO, "       --->Edge Angle: %5.2f   Dop PRF: %2d   Dop Pulses: %4d\n",
                   azi_1, elev->dop_prf_num_1, elev->pulse_cnt_1 );

      LE_send_msg( GL_INFO, "       PRF Sector 2:\n" );
      LE_send_msg( GL_INFO, "       --->Edge Angle: %5.2f   Dop PRF: %2d   Dop Pulses: %4d\n",
                   azi_2, elev->dop_prf_num_2, elev->pulse_cnt_2 );

      LE_send_msg( GL_INFO, "       PRF Sector 3:\n" );
      LE_send_msg( GL_INFO, "       --->Edge Angle: %5.2f   Dop PRF: %2d   Dop Pulses: %4d\n",
                   azi_3, elev->dop_prf_num_3, elev->pulse_cnt_3 );

   }

}

