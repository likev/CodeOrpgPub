/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2009/05/05 14:00:56 $
 * $Id: verify_rdacnt.c,v 1.11 2009/05/05 14:00:56 steves Exp $
 * $Revision: 1.11 $
 * $State: Exp $
*/


#include <stdio.h>
#include <stdlib.h>

#include <rdacnt.h>
#include <orpg.h>
#include <vcp.h>
#include <rpg_port.h>

#include <write_vcp_template.h>

/* Static global variables. */
static int Generate_templates;
static Rdacnt *Rdacnt_info = NULL;

static int Get_command_line_options( int argc, char *argv[] );

/*\///////////////////////////////////////////////////////////////////////
//
//   Description:
//      Reads the RDACNT data structure and outputs the data elements
//      in human-readable form.
//
///////////////////////////////////////////////////////////////////////\*/
int main( int argc, char *argv[] ){

   int flag, ret, i, j, k;
   int ele_cnt;
   int cut_num;
   int alwb_prfs[ MAX_DOP_PRI ];
   Vcp_struct *vcp;
   Vcp_alwblprf_t *alwb_prf;
   Cut_info_t cut_info;
   Vol_info_t vol_info;

   FILE *fd = NULL;

   /* Get command line arguments. */
   if( Get_command_line_options( argc, argv ) < 0 ){

      fprintf( stderr, "Get command line options Failed\n" );
      exit(0);

   }

   /* Allocate memory to store the RDACNT data. */
   Rdacnt_info = calloc( 1, sizeof( Rdacnt ) );
   if( Rdacnt_info == NULL ){

      fprintf( stderr, "Unable to calloc %d bytes\n", sizeof( Rdacnt ) );
      exit(1);

   } 

   /* Read the RDACNT data. */
   ret = ORPGDA_read( ORPGDAT_ADAPTATION, (void *) Rdacnt_info, sizeof( Rdacnt ),
                      RDACNT );
   if( ret != sizeof( Rdacnt ) ){

      fprintf( stderr, "ORPGDA_read( ORPGDAT_ADAPTATION, RDACNT ) Failed (%d)\n", ret );
      exit(2);

   }

   /* Write out the Weather Mode Table. */
   fprintf( stdout, "\n\nWeather Mode Table:\n" );
   for( j = 0; j < WXMAX; j++ ){

      if( j == 0 )
         fprintf( stdout, "--->Clear Air:\n       " );

      else
         fprintf( stdout, "---> Precipitation:\n       " );
      for( i = 0; i < WXVCPMAX; i++ ){

         if( Rdacnt_info->rdcwxvcp[j][i] != 0 )
            fprintf( stdout, "%4d ", Rdacnt_info->rdcwxvcp[j][i] );

      }
      fprintf( stdout, "\n" );

   }


   /* Write out the VCP Where Defined Table. */
   fprintf( stdout, "\n\nVCP Where Defined Table:\n" );
   for( j = 0; j < COMPMAX; j++ ){

      if( j == VCP_RDA_DEFINED )
         fprintf( stdout, "---> RDA Defined:\n        " );

      else if( j == VCP_RPG_DEFINED )
         fprintf( stdout, "---> RPG Defined:\n        " );

      for( i = 0; i < VCPMAX; i++ ){
    
         if( Rdacnt_info->rdc_where_defined[j][i] != 0 )
            fprintf( stdout, "%4d", 
                     (Rdacnt_info->rdc_where_defined[j][i] & ORPGVCP_VCP_MASK) );

      }
      fprintf( stdout, "\n" );

   }


   /* Do For Each VCP. */
   for( j = 0; j < VCPMAX; j++ ){

      static char *reso[] = { "0.5 m/s", "1.0 m/s" };
      static char *width[] = { "SHORT", "LONG" };
      static char *wave_form[] = { "UNK", "CS", "CD/W", "CD/WO", "BATCH", "STP" };
      static char *phase[] = { "CNST", "", "SZ2" };
      short  wv_frm, phse;
      char half_deg_azm[2], qtr_km_ref[2], tredop[2], dualpol[2];

      vcp = (Vcp_struct *) Rdacnt_info->rdcvcpta[j];
      if( vcp->vcp_num == 0 )
         continue;

      /* If the flag for generating the template file is set, 
         file is the Vol_attr section of the template file. */
      if( Generate_templates ){

         int where_defined = 0, wx_mode = 0;

         fd = Get_filename( vcp->vcp_num );
         if( fd == NULL ){

            fprintf( stderr, "Unable to Open Template File for VCP %d\n",
                     vcp->vcp_num );
            exit(1);

         }

         for( k = 0; k < WXMAX; k++ ){

            for( i = 0; i < WXVCPMAX; i++ ){

               if( Rdacnt_info->rdcwxvcp[k][i] == vcp->vcp_num ){

                  wx_mode = k + 1;
                  break;

               }

            }

            if( wx_mode > 0 )
               break;

         }

         for( k = 0; k < COMPMAX; k++ ){

            for( i = 0; i < VCPMAX; i++ ){

               if( Rdacnt_info->rdc_where_defined[k][i] == vcp->vcp_num ){

                  if( k == VCP_RDA_DEFINED )
                     where_defined = (ORPGVCP_RPG_DEFINED_VCP + ORPGVCP_RDA_DEFINED_VCP);

                  else
                     where_defined = ORPGVCP_RPG_DEFINED_VCP;

                  break;

               }

            } /* End of "for( i < VCPMAX )" loop. */

            if( where_defined > 0 )
               break;

         } /* End of "for( k < COMPMAX )" loop. */

         vol_info.pattern_num = vcp->vcp_num;
         vol_info.wx_mode = wx_mode;
         vol_info.num_elev_cuts = vcp->n_ele;
         vol_info.where_defined = where_defined;
         vol_info.pulse_width = vcp->pulse_width;
         vol_info.cluttermap_group = 1;
         if( vcp->vel_resolution == ORPGVCP_VEL_RESOLUTION_LOW )
            vol_info.vel_reso = 1.0;
         else
            vol_info.vel_reso = 0.5;
         Write_VCP_attr( &vol_info );

      }

      /* Write out rdcvcpta data. */
      fprintf( stdout, "\n\nVCP %d Data:\n", vcp->vcp_num );
      fprintf( stdout, "--->VCP Header:\n" ); 
      fprintf( stdout, "       Size (shorts): %4d   Type: %4d   # Elevs: %4d\n",
               vcp->msg_size, vcp->type, vcp->n_ele );
      fprintf( stdout, "       Clutter Group: %4d   Vel Reso: %s   Pulse Width: %s\n\n",
               vcp->clutter_map_num, reso[ vcp->vel_resolution/4 ], 
               width[ vcp->pulse_width/4 ] );

      /* Do For All elevation cuts. */
      for( i = 0; i < vcp->n_ele; i++ ){

         Ele_attr *elev = (Ele_attr *) &vcp->vcp_ele[i][0];

         float elev_angle = ORPGVCP_BAMS_to_deg( ORPGVCP_ELEVATION_ANGLE, elev->ele_angle );

         wv_frm = elev->wave_type;
         phse = elev->phase;
         if( elev->super_res & VCP_HALFDEG_RAD )
            memcpy( half_deg_azm, "Y", 2 );

         else
            memcpy( half_deg_azm, "N", 2 );


         if( elev->super_res & VCP_QRTKM_SURV )
            memcpy( qtr_km_ref, "Y", 2 );

         else
            memcpy( qtr_km_ref, "N", 2 );

         if( elev->super_res & VCP_300KM_DOP )
            memcpy( tredop, "Y", 2 );

         else
            memcpy( tredop, "N", 2 );

         if( elev->super_res & VCP_DUAL_POL_ENABLED )
            memcpy( dualpol, "Y", 2 );

         else
            memcpy( dualpol, "N", 2 );

         fprintf( stdout, "--->Elevation %d:\n", i+1 );
         fprintf( stdout, "       Elev Angle: %5.2f   Wave Type: %s   Phase: %s   Surv PRF: %2d   Surv Pulses: %4d\n",
                  elev_angle, wave_form[ wv_frm ], phase[ phse ], 
                  elev->surv_prf_num, elev->surv_pulse_cnt );
         fprintf( stdout, "       Az Rate: %4d (0x%4x) (BAMS)   SNR Threshold: %5.2f  %5.2f  %5.2f  %5.2f  %5.2f  %5.2f (dB)\n",
                  elev->azi_rate, elev->azi_rate, (float) elev->surv_thr_parm/8.0, 
                  (float) elev->vel_thrsh_parm/8.0, (float) elev->spw_thrsh_parm/8.0,
                  (float) elev->zdr_thrsh_parm/8.0, (float) elev->phase_thrsh_parm/8.0,
                  (float) elev->corr_thrsh_parm/8.0 );
         fprintf( stdout, "       1/4 km Ref: %s  300 km Doppler: %s  1/2 deg Rads: %s  Dual Pol: %s\n",
                  half_deg_azm, qtr_km_ref, tredop, dualpol );

         fprintf( stdout, "       PRF Sector 1:\n" );
         fprintf( stdout, "       --->Edge Angle: %5.2f   Dop PRF: %2d   Dop Pulses: %4d\n",
                  elev->azi_ang_1/10.0, elev->dop_prf_num_1, elev->pulse_cnt_1 );

         fprintf( stdout, "       PRF Sector 2:\n" );
         fprintf( stdout, "       --->Edge Angle: %5.2f   Dop PRF: %2d   Dop Pulses: %4d\n",
                  elev->azi_ang_2/10.0, elev->dop_prf_num_2, elev->pulse_cnt_2 );

         fprintf( stdout, "       PRF Sector 3:\n" );
         fprintf( stdout, "       --->Edge Angle: %5.2f   Dop PRF: %2d   Dop Pulses: %4d\n",
                  elev->azi_ang_3/10.0, elev->dop_prf_num_3, elev->pulse_cnt_3 );

      }

      /* Write out rdccon data. */
      fprintf( stdout, "\n\n--->RDA -> RPG Elevation Mapping (VCP %3d)\n", vcp->vcp_num );
      fprintf( stdout, "       " );
      for( i = 0; i < vcp->n_ele; i++ ){

         if( Rdacnt_info->rdccon[j][i] != 0 )
            fprintf( stdout, "%2d ", Rdacnt_info->rdccon[j][i] );

      }
      fprintf( stdout, "\n" );
    
      /* Write out vcp_times data. */
      fprintf( stdout, "\n\n--->VCP %3d Duration:  %4d (Sec)\n", 
               vcp->vcp_num, Rdacnt_info->vcp_times[j] );

      fprintf( stdout, "\n\n--->Allowable PRFs (VCP %3d)\n", vcp->vcp_num );
      
      alwb_prf = (Vcp_alwblprf_t *) &Rdacnt_info->alwblprf[j][0];
      fprintf( stdout,"       " );
      for( i = 0; i < alwb_prf->num_alwbl_prf; i++ ){

         fprintf( stdout, "%2d ", alwb_prf->prf_num[i] );
         alwb_prfs[i] = alwb_prf->prf_num[i];

      }

      fprintf( stdout, "\n" );
      
      ele_cnt = 0;
      for( cut_num = 0; cut_num < vcp->n_ele; cut_num++ ){
      
         Ele_attr *elev = (Ele_attr *) &vcp->vcp_ele[cut_num][0];

         /* Initialize the Cut Information. */
         Init_elevation_attr( &cut_info );

         /* Do waveform processing. */
         if( (elev->wave_type & 0xff) != VCP_WAVEFORM_CS ){

            fprintf( stdout, "--->Pulse Counts for Elev Cut # %2d\n", 
                     Rdacnt_info->rdccon[j][cut_num] ); 
            fprintf( stdout, "       " );
            for( k = 0; k < NAPRFELV-1; k++ ){

               if( alwb_prf->pulse_cnt[ele_cnt][k] == 0 )
                  continue; 

               fprintf( stdout, "%4d ", alwb_prf->pulse_cnt[ele_cnt][k] );
 
               /* Gather pulse count information for Doppler PRFs. */
               cut_info.def_dop_p_cnt[ k+1 ] = (int) alwb_prf->pulse_cnt[ele_cnt][k];

            } /* End of "for k < NAPRFELV-1" loop. */
            
            cut_info.def_dop_prf = alwb_prf->pulse_cnt[ele_cnt][NAPRFELV-1];
            fprintf( stdout, "\n       Default PRF: %2d\n", 
                     alwb_prf->pulse_cnt[ele_cnt][NAPRFELV-1] );

            cut_info.prf_sector[0].edge_angle = elev->azi_ang_1/10.0;
            cut_info.prf_sector[0].dop_prf = elev->dop_prf_num_1;

            cut_info.prf_sector[1].edge_angle = elev->azi_ang_2/10.0;
            cut_info.prf_sector[1].dop_prf = elev->dop_prf_num_2;

            cut_info.prf_sector[2].edge_angle = elev->azi_ang_3/10.0;
            cut_info.prf_sector[2].dop_prf = elev->dop_prf_num_3;

            ele_cnt++;

         } /* End of "if( elev->wave_type .... ) */
         else {

            cut_info.def_dop_prf = 0;
            for( k = 0; k <= MAX_DOP_PRI; k++ )
               cut_info.def_dop_p_cnt[k] = 0;

         } /* CS waveform types. */

         if( Generate_templates ){

            /* Set the flag depending on whether we are at the first elevation cut, 
               an intermediate elevation cut, or the last elevation cut. */
            if( cut_num == 0 )
               flag = START_ELEV_ATTR;
            else if( cut_num == vcp->n_ele-1 )
               flag = END_ELEV_ATTR;
            else 
               flag = NO_ACTION;

            /* File in the cut_info data structure. */
            cut_info.cut_num = cut_num;
            cut_info.elev_ang_deg = ORPGVCP_BAMS_to_deg( ORPGVCP_ELEVATION_ANGLE, 
                                                         elev->ele_angle ); 
            cut_info.scan_rate_deg = ((float) elev->azi_rate)*BAMS_TO_DPS; 
            cut_info.waveform = (int) elev->wave_type; 
            cut_info.def_surv_prf = (int) elev->surv_prf_num; 
            cut_info.def_surv_p_cnt = (int) elev->surv_pulse_cnt;
            cut_info.surv_snr = (float) elev->surv_thr_parm/8.0; 
            cut_info.vel_snr = (float) elev->vel_thrsh_parm/8.0; 
            cut_info.spw_snr = (float) elev->spw_thrsh_parm/8.0;
            
            Write_elevation_attr( flag, &cut_info, 
                                  (int) alwb_prf->num_alwbl_prf, 
                                  alwb_prfs ); 

         }

      }

   }   

   return (0);

}
/*\//////////////////////////////////////////////////////////////////////////
//
//   Description:
//      Process command line arguments.
//
//   Inputs:
//      argc - number of command line arguments.
//      argv - the command line arguments.
//
//   Returns:
//      exits on error, or returns 0 on success.
//
///////////////////////////////////////////////////////////////////////////\*/
static int Get_command_line_options( int argc, char *argv[] ){

   extern char *optarg;
   extern int optind;
   int c, err;

   Generate_templates = 0;

   err = 0;
   while( (c = getopt (argc, argv, "ht")) != EOF ){

      switch (c) {

         case 't':
            Generate_templates = 1;
            break;

         case 'h':
         case '?':
         default:
            err = 1;
            break;
      }

   }

   if( err == 1 ){              /* Print usage message */

      printf ("Usage: %s [options]\n", MISC_string_basename(argv [0]));
      printf ("\toptions:\n");
      printf ("\t\t-h Help (print usage msg and exit)\n");
      printf ("\t\t-t Generate template files\n");
      exit (1);

   }

   return(0);

}
