#include <stdlib.h>
#include <orpg.h>
#include <vcp.h>

void VM_write_vcp_data( Vcp_struct *vcp ){

   static char *reso[] = { "0.5 m/s", "1.0 m/s" };
   static char *width[] = { "SHORT", "LONG" };
   static char *wave_form[] = { "UNK", "CS", "CD/W", "CD/WO", "BATCH", "STP" };
   static char *phase[] = { "CON", "RAN", "SZ2" };

   int i;
   short wform, phse;

   /* Write out VCP data. */
   LE_send_msg( GL_INFO, "\n\nVCP %d Data:\n", vcp->vcp_num );
   LE_send_msg( GL_INFO, "--->VCP Header:\n" ); 
   LE_send_msg( GL_INFO, "       Size (shorts): %4d   Type: %4d   # Elevs: %4d\n",
            vcp->msg_size, vcp->type, vcp->n_ele );
   LE_send_msg( GL_INFO, "       Clutter Group: %4d   Vel Reso: %s   Pulse Width: %s\n\n",
            vcp->clutter_map_num, reso[ vcp->vel_resolution/4 ], 
            width[ vcp->pulse_width/4 ] );

   /* Do For All elevation cuts. */
   for( i = 0; i < vcp->n_ele; i++ ){

      Ele_attr *elev = (Ele_attr *) &vcp->vcp_ele[i][0];

      float elev_angle = ORPGVCP_BAMS_to_deg( ORPGVCP_ELEVATION_ANGLE, elev->ele_angle );

      wform = elev->wave_type;
      phse = elev->phase;
      LE_send_msg( GL_INFO, "--->Elevation %d:\n", i+1 );
      LE_send_msg( GL_INFO, "       Elev Angle: %5.2f   Wave Type: %s   Phase: %s   Super Res Flag: %3d, Surv PRF: %2d   Surv Pulses: %4d\n",
                   elev_angle, wave_form[ wform ], phase[ phse ], 
                   elev->super_res, elev->surv_prf_num, elev->surv_pulse_cnt );
      LE_send_msg( GL_INFO, "       Az Rate: %4d (0x%4x) (BAMS)   SNR Threshold: %5.2f  %5.2f  %5.2f (dB)\n",
                   elev->azi_rate, elev->azi_rate, (float) elev->surv_thr_parm/8.0, 
                   (float) elev->vel_thrsh_parm/8.0, (float) elev->spw_thrsh_parm/8.0 );

      LE_send_msg( GL_INFO, "       PRF Sector 1:\n" );
      LE_send_msg( GL_INFO, "       --->Edge Angle: %5.2f   Dop PRF: %2d   Dop Pulses: %4d\n",
                   elev->azi_ang_1/10.0, elev->dop_prf_num_1, elev->pulse_cnt_1 );

      LE_send_msg( GL_INFO, "       PRF Sector 2:\n" );
      LE_send_msg( GL_INFO, "       --->Edge Angle: %5.2f   Dop PRF: %2d   Dop Pulses: %4d\n",
                   elev->azi_ang_2/10.0, elev->dop_prf_num_2, elev->pulse_cnt_2 );

      LE_send_msg( GL_INFO, "       PRF Sector 3:\n" );
      LE_send_msg( GL_INFO, "       --->Edge Angle: %5.2f   Dop PRF: %2d   Dop Pulses: %4d\n",
                   elev->azi_ang_3/10.0, elev->dop_prf_num_3, elev->pulse_cnt_3 );

   }   

}
