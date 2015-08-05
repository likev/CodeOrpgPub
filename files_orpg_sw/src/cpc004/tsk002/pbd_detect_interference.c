/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2013/07/22 17:42:50 $
 * $Id: pbd_detect_interference.c,v 1.3 2013/07/22 17:42:50 steves Exp $
 * $Revision: 1.3 $
 * $State: Exp $
 */

#include <pbd.h>
#include <orpgsun.h>

/* Static global variables. */
static site_info Site_info;
static double Sun_az, Sun_el;
static int Log_code = GL_INFO; 

/* Set the following thresholds to be the same value. */
static float Noise_thr = 2.5;
static float P_noise_thr = 2.5;

/* local functions */

/********************************************************************

   Description:
      This function initializes the site_info data structure 
      needed by the novas library.

   Inputs:
      radial - pointer to radial message.

   Outputs:

   Returns: 
      There is no return value defined for this function.

********************************************************************/
void ID_site_init( char *radial ){

   Base_data_header *rpg_hd = (Base_data_header *) radial;

   Site_info.latitude = (double) rpg_hd->latitude; 
   Site_info.longitude = (double) rpg_hd->longitude; 
   Site_info.height = (double) (rpg_hd->height + rpg_hd->feedhorn_height);
   Site_info.temperature = 0.0;
   Site_info.pressure = 0.0;

   /* Get the Sun's position now and output its location. */
   ORPGSUN_NovasComputePos( Site_info, &Sun_az, &Sun_el );
   LE_send_msg( GL_INFO, "Sun Position At Current Time: Azm: %6.2f deg, Elv: %6.2f deg\n",
                Sun_az, Sun_el );

   /* Use the time from the radial to determined Sun position. */
   if( PBD_DEBUG ){

      /* Get the radial time. */
      time_t time_value = (time_t) (((rpg_hd->date-1)*86400) + (rpg_hd->time/1000));

      /* Get the Sun's position. */
      ORPGSUN_NovasComputePosAtTime( Site_info, &Sun_az, &Sun_el, time_value  );

      LE_send_msg( GL_INFO, "Sun Position At Start of Volume: Azm: %6.2f deg, Elv: %6.2f deg\n",
                   Sun_az, Sun_el );

   }
    
   /* Do Sun Spike messages go to RPG Status Log? */
   Log_code = GL_INFO;
   if( PBD_sun_spike_msgs ){

      Log_code = GL_STATUS | LE_RPG_INFO_MSG; 
      LE_send_msg( GL_INFO, "Sun Spike Messages written to RPG Status Log\n" );

   }

   /* Return to caller. */
   return;

/* End of ID_site_init() */
}

/********************************************************************

   Description:
      This function checks to see if there is interference, and if 
      so is the interference cause by the Sun or some other source.

   Inputs:
      radial - pointer to radial message.

   Outputs:

   Returns: 
      There is no return value defined for this function.

********************************************************************/
void ID_check_interference( char *radial ){

   Base_data_header *rpg_hd = (Base_data_header *) radial;
   float h_rad_noise = rpg_hd->horiz_noise;
   float v_rad_noise = rpg_hd->vert_noise;

   float h_thresh = PBD_h_shrt_pulse_noise_thr;
   float v_thresh = PBD_v_shrt_pulse_noise_thr;
   float h_blue_sky = PBD_h_shrt_pulse_noise;
   float v_blue_sky = PBD_v_shrt_pulse_noise;

   /* Set the thresholds for H and V noise. */
   if( !PBD_is_short_pulse ){

      h_thresh = PBD_h_long_pulse_noise_thr;
      v_thresh = PBD_v_long_pulse_noise_thr;
      h_blue_sky = PBD_h_long_pulse_noise;
      v_blue_sky = PBD_v_long_pulse_noise;

   }
      
   /* Check the radial noise values. */
   if( (h_rad_noise > h_thresh)
                 ||
       (v_rad_noise > v_thresh) ){

      /* Get the radial time. */
      time_t time_value = (time_t) (((rpg_hd->date-1)*86400) + (rpg_hd->time/1000));

      /* Get the Sun's position. */
      ORPGSUN_NovasComputePosAtTime( Site_info, &Sun_az, &Sun_el, time_value  );

      /* Write out more information, if needed. */
      if( PBD_DEBUG ){

         LE_send_msg( GL_INFO, "Radial Noise Value > Threshold\n" );
         LE_send_msg( GL_INFO, "--->H Radial Noise: %6.2f dB, H Thresh: %6.2f dB, Blue Sky: %6.2f dB\n",
                      h_rad_noise, h_thresh, h_blue_sky );
         LE_send_msg( GL_INFO, "--->V Radial Noise: %6.2f dB, V Thresh: %6.2f dB, Blue Sky: %6.2f dB\n",
                      v_rad_noise, v_thresh, v_blue_sky );

         LE_send_msg( GL_INFO, "--->Sun is at Azm: %6.2f deg, Elv: %6.2f deg\n",
                      Sun_az, Sun_el );

      }

      /* Is the noise spike close to the Sun's estimated position? */
      if( (fabs(Sun_az - rpg_hd->azimuth) <= 2.0)
                        &&
          (fabs(Sun_el - rpg_hd->elevation) <= 2.0) ){

         /* Is the H noise greater than threshold. */
         if( h_rad_noise > PBD_i_detect.h_sun_noise_lvl ){

            PBD_i_detect.h_sun_noise_lvl = h_rad_noise;
            PBD_i_detect.h_sun_noise_azm = rpg_hd->azimuth;
            PBD_i_detect.h_sun_noise_ele = rpg_hd->elevation;

         }

         /* Is the V noise greater than threshold. */
         if( v_rad_noise > PBD_i_detect.v_sun_noise_lvl ){

            PBD_i_detect.v_sun_noise_lvl = v_rad_noise;
            PBD_i_detect.v_sun_noise_azm = rpg_hd->azimuth;
            PBD_i_detect.v_sun_noise_ele = rpg_hd->elevation;

         }

      }
      else {

         /* Is the H noise greater than threshold. */
         if( h_rad_noise > h_thresh ){

            /* Increment the number of radials exceeding threshold. */
            PBD_i_detect.h_noise_cnt++;

            /* Output additional information, if needed. */
            if( PBD_DEBUG ){

               LE_send_msg( GL_INFO, "Interference Radial Found\n" );
               LE_send_msg( GL_INFO, "--->H Radial Noise: %6.2f dB, H Thresh: %6.2f dB\n",
                             h_rad_noise, h_thresh );
               LE_send_msg( GL_INFO, "--->H Radial Count: %d\n", PBD_i_detect.h_noise_cnt );

            }

            /* Is this the maximum so far? */
            if( h_rad_noise > PBD_i_detect.h_max_noise_lvl ){

               PBD_i_detect.h_max_noise_lvl = h_rad_noise;
               PBD_i_detect.h_max_noise_azm = rpg_hd->azimuth;
               PBD_i_detect.h_max_noise_ele = rpg_hd->elevation;

               /* Output additional information, if needed. */
               if( PBD_DEBUG ){

                  LE_send_msg( GL_INFO, "H Maximum Interference Found\n" );
                  LE_send_msg( GL_INFO, "--->H Radial Noise: %6.2f dB, Azm: %6.2f deg, Elv: %6.2f deg\n",
                               h_rad_noise, rpg_hd->azimuth, rpg_hd->elevation );

               }

            }
            
         }
 
         if( v_rad_noise > v_thresh ){

            /* Increment the number of radials exceeding threshold. */
            PBD_i_detect.v_noise_cnt++;

            /* Output additional information, if needed. */
            if( PBD_DEBUG ){

               LE_send_msg( GL_INFO, "Interference Radial Found\n" );
               LE_send_msg( GL_INFO, "--->V Radial Noise: %6.2f dB, V Thresh: %6.2f dB\n",
                             v_rad_noise, v_thresh );
               LE_send_msg( GL_INFO, "--->V Radial Count: %d\n", PBD_i_detect.v_noise_cnt );

            }

            /* Is this the maximum so far? */
            if( v_rad_noise > PBD_i_detect.v_max_noise_lvl ){

               PBD_i_detect.v_max_noise_lvl = v_rad_noise;
               PBD_i_detect.v_max_noise_azm = rpg_hd->azimuth;
               PBD_i_detect.v_max_noise_ele = rpg_hd->elevation;

               /* Output additional information, if needed. */
               if( PBD_DEBUG ){

                  LE_send_msg( GL_INFO, "V Maximum Interference Found\n" );
                  LE_send_msg( GL_INFO, "--->V Radial Noise: %6.2f dB, Azm: %6.2f deg, Elv: %6.2f deg\n",
                               v_rad_noise, rpg_hd->azimuth, rpg_hd->elevation );

               }

            }

         }

      }

   }

   return;

/* End of ID_check_interference() */
}

/********************************************************************

   Description:
      This module writes an RPG Status Log message for any 
      interference found.

   Inputs:

   Outputs:

   Returns: 
      There is no return value defined for this function.

********************************************************************/
void ID_output_interference_msg( char *radial ){

   Base_data_header *rpg_hd = (Base_data_header *) radial;
   float h_thresh = PBD_h_shrt_pulse_noise_thr;
   float v_thresh = PBD_v_shrt_pulse_noise_thr;
   int time_to_output = 0;

   /* Is it time to output an interference message??? */
   if( (rpg_hd->status == GENDVOL)
                       ||
       ((rpg_hd->status == GOODBVOL) && (rpg_hd->elev_num == 1)
                                     &&
        ((PBD_i_detect.h_noise_cnt != 0) || (PBD_i_detect.v_noise_cnt != 0))) )
      time_to_output = 1;
                    
   /* If end of volume or beginning of volume but the detection counts are 
      not zero (can happen on aborted volume scan), check if we need to 
      output an interference message. */
   if( !time_to_output )
      return;

   /* Additional Info .... */
   if( PBD_DEBUG ){

      LE_send_msg( GL_INFO, "Is there an Interference Message to Output????\n" );
      LE_send_msg( GL_INFO, "--->Radial Status: %d, Elev #: %d, Elev Ind: %d\n", 
                   rpg_hd->status, rpg_hd->elev_num, rpg_hd->rpg_elev_ind );

   }

   /* Set the thresholds for H and V noise. */
   if( !PBD_is_short_pulse ){

      h_thresh = PBD_h_long_pulse_noise_thr;
      v_thresh = PBD_v_long_pulse_noise_thr;

   }

   /* Check the thresholds to see if they are valid. */
   if( (h_thresh == PBD_UNDEFINED_NOISE) 
                    ||
       (v_thresh == PBD_UNDEFINED_NOISE) ){

      LE_send_msg( GL_INFO, 
             "V and/or H Noise Thresholds Undefined ... No Interference Messages.\n" );
      LE_send_msg( GL_INFO, "--->H Thresh: %6.2f, V Thresh: %6.2f\n", h_thresh, v_thresh );

      return;

   }


   /* Output a Sun Spike message, if needed. */
   if( (PBD_i_detect.h_sun_noise_lvl > h_thresh)
                         ||
       (PBD_i_detect.v_sun_noise_lvl > v_thresh) ){

      LE_send_msg( Log_code, 
                   "Sun Spike Detected at H:[%4.1f/%4.1f/%4.1f] V:[%4.1f/%4.1f/%4.1f]\n",
                   PBD_i_detect.h_sun_noise_azm,
                   PBD_i_detect.h_sun_noise_ele,
                   PBD_i_detect.h_sun_noise_lvl,
                   PBD_i_detect.v_sun_noise_azm,
                   PBD_i_detect.v_sun_noise_ele,
                   PBD_i_detect.v_sun_noise_lvl );

   }

   /* Output Excess Noise detected messages, if needed. */
   if( PBD_i_detect.h_noise_cnt > 0 ){

      LE_send_msg( GL_STATUS | LE_RPG_INFO_MSG, 
                   "Excess Noise Detected at %5d radials in H: max at [%4.1f/%4.1f/%6.2f]\n",
                   PBD_i_detect.h_noise_cnt,
                   PBD_i_detect.h_max_noise_azm,
                   PBD_i_detect.h_max_noise_ele,
                   PBD_i_detect.h_max_noise_lvl );
   }

   if( PBD_i_detect.v_noise_cnt > 0 ){

      LE_send_msg( GL_STATUS | LE_RPG_INFO_MSG, 
                   "Excess Noise Detected at %5d radials in V: max at [%4.1f/%4.1f/%6.2f]\n",
                   PBD_i_detect.v_noise_cnt,
                   PBD_i_detect.v_max_noise_azm,
                   PBD_i_detect.v_max_noise_ele,
                   PBD_i_detect.v_max_noise_lvl );
   }

   /* Initialize the Noise values ... these will be set when PMD is sent to the 
      RPG from the RDA prior to start of volume. */
   LE_send_msg( GL_INFO, "Initializing Noise Threshold .....\n" );

   /* H & V Short Pulse noise values. */
   PBD_h_shrt_pulse_noise = PBD_UNDEFINED_NOISE;
   PBD_v_shrt_pulse_noise = PBD_UNDEFINED_NOISE;
   PBD_h_shrt_pulse_noise_thr = PBD_UNDEFINED_NOISE;
   PBD_v_shrt_pulse_noise_thr = PBD_UNDEFINED_NOISE;

   /* H & V Long Pulse noise values. */
   PBD_h_long_pulse_noise = PBD_UNDEFINED_NOISE;
   PBD_v_long_pulse_noise = PBD_UNDEFINED_NOISE;
   PBD_h_long_pulse_noise_thr = PBD_UNDEFINED_NOISE;
   PBD_v_long_pulse_noise_thr = PBD_UNDEFINED_NOISE;
   
   return;

/* End of ID_output_interference_msg() */
}

/********************************************************************

   Description:
      Performs an initialization for interference detection.

   Inputs:
      vcp - passes the VCP number. If -1, the VCP number is 
            undefined.

   Returns: 
      There is no return value defined for this function.

********************************************************************/
void ID_init_interference_data( int vcp ){

   double dtemp = 0;
   int width;

   /* Get the noise threshold for interference. */
   if( DEAU_get_values( "pbd.i_noise_thr", &dtemp, 1 ) < 0 ){

      LE_send_msg( GL_ERROR, "DEAU_get_values( pbd.i_noise_thr ) Failed\n" );
      Noise_thr = 2.5;

   }
   else
      Noise_thr = (float) dtemp;

   LE_send_msg( GL_INFO, "Interference Threshold: %6.2f\n", Noise_thr );
   
   /* Has the interference threshold changed? */
   if( Noise_thr != P_noise_thr ){

      LE_send_msg( GL_INFO, "Interference Threshold Has Changed !!!!!\n" );

      PBD_h_shrt_pulse_noise_thr = PBD_h_shrt_pulse_noise + Noise_thr;
      PBD_v_shrt_pulse_noise_thr = PBD_v_shrt_pulse_noise + Noise_thr;

      /* H & V Long Pulse noise values. */
      PBD_h_long_pulse_noise_thr = PBD_h_long_pulse_noise + Noise_thr;
      PBD_v_long_pulse_noise_thr = PBD_v_long_pulse_noise + Noise_thr;

      LE_send_msg( GL_INFO, "H & V Noise Levels and Thresholds\n" );
      LE_send_msg( GL_INFO, "--->PBD_h_shrt_pulse_noise: %f, Threshold: %f\n",
                   PBD_h_shrt_pulse_noise, PBD_h_shrt_pulse_noise_thr );
      LE_send_msg( GL_INFO, "--->PBD_v_shrt_pulse_noise: %f, Threshold: %f\n",
                   PBD_v_shrt_pulse_noise, PBD_v_shrt_pulse_noise_thr );
      LE_send_msg( GL_INFO, "--->PBD_h_long_pulse_noise: %f, Threshold: %f\n",
                   PBD_h_long_pulse_noise, PBD_h_long_pulse_noise_thr );
      LE_send_msg( GL_INFO, "--->PBD_v_long_pulse_noise: %f, Threshold: %f\n",
                   PBD_v_long_pulse_noise, PBD_v_long_pulse_noise_thr );

      /* Save the Previous Noise Threshold Value. */
      P_noise_thr = Noise_thr;

   }
   

   /* Initial state. */
   if( vcp == -1 ){

      LE_send_msg( GL_INFO, "Initializing Interference Data\n" );

      PBD_h_shrt_pulse_noise = PBD_UNDEFINED_NOISE;
      PBD_v_shrt_pulse_noise = PBD_UNDEFINED_NOISE;
      PBD_h_long_pulse_noise = PBD_UNDEFINED_NOISE;
      PBD_v_long_pulse_noise = PBD_UNDEFINED_NOISE;
      PBD_is_short_pulse = -1;


      PBD_h_shrt_pulse_noise_thr = PBD_h_shrt_pulse_noise;
      PBD_v_shrt_pulse_noise_thr = PBD_v_shrt_pulse_noise;
      PBD_h_long_pulse_noise_thr = PBD_h_long_pulse_noise;
      PBD_v_long_pulse_noise_thr = PBD_v_long_pulse_noise;

      PBD_i_detect.h_noise_cnt = 0;
      PBD_i_detect.v_noise_cnt = 0;
      PBD_i_detect.h_sun_noise_lvl = 0;
      PBD_i_detect.h_sun_noise_azm = PBD_UNDEFINED_ANGLE;
      PBD_i_detect.h_sun_noise_ele = PBD_UNDEFINED_ANGLE;
      PBD_i_detect.v_sun_noise_lvl = 0;
      PBD_i_detect.v_sun_noise_azm = PBD_UNDEFINED_ANGLE;
      PBD_i_detect.v_sun_noise_ele = PBD_UNDEFINED_ANGLE;
      PBD_i_detect.h_max_noise_lvl = 0;
      PBD_i_detect.h_max_noise_azm = PBD_UNDEFINED_ANGLE;
      PBD_i_detect.h_max_noise_ele = PBD_UNDEFINED_ANGLE;
      PBD_i_detect.v_max_noise_lvl = 0;
      PBD_i_detect.v_max_noise_azm = PBD_UNDEFINED_ANGLE;
      PBD_i_detect.v_max_noise_ele = PBD_UNDEFINED_ANGLE;

      return;

   }

   /* Initialization will depend on vcp .*/
   PBD_is_short_pulse = 1;
   width = ORPGVCP_get_pulse_width( vcp );
   if( width == ORPGVCP_LONG_PULSE )
      PBD_is_short_pulse = 0;

   PBD_i_detect.h_noise_cnt = 0;
   PBD_i_detect.v_noise_cnt = 0;
   PBD_i_detect.h_sun_noise_azm = PBD_UNDEFINED_ANGLE;
   PBD_i_detect.h_sun_noise_ele = PBD_UNDEFINED_ANGLE;
   PBD_i_detect.v_sun_noise_azm = PBD_UNDEFINED_ANGLE;
   PBD_i_detect.v_sun_noise_ele = PBD_UNDEFINED_ANGLE;
   PBD_i_detect.h_max_noise_azm = PBD_UNDEFINED_ANGLE;
   PBD_i_detect.h_max_noise_ele = PBD_UNDEFINED_ANGLE;
   PBD_i_detect.v_max_noise_azm = PBD_UNDEFINED_ANGLE;
   PBD_i_detect.v_max_noise_ele = PBD_UNDEFINED_ANGLE;

   /* Initialization depends on pulse width */
   if( PBD_is_short_pulse ){

      PBD_i_detect.h_sun_noise_lvl = PBD_h_shrt_pulse_noise;
      PBD_i_detect.v_sun_noise_lvl = PBD_v_shrt_pulse_noise;
      PBD_i_detect.h_max_noise_lvl = PBD_h_shrt_pulse_noise;
      PBD_i_detect.v_max_noise_lvl = PBD_v_shrt_pulse_noise;

   }
   else{

      PBD_i_detect.h_sun_noise_lvl = PBD_h_long_pulse_noise;
      PBD_i_detect.v_sun_noise_lvl = PBD_v_long_pulse_noise;
      PBD_i_detect.h_max_noise_lvl = PBD_h_long_pulse_noise;
      PBD_i_detect.v_max_noise_lvl = PBD_v_long_pulse_noise;

   }

   return;

/* End of ID_init_interference_data() */
}

/**********************************************************************
  
   Description:
      The RDA/RPG Performance/Maintenance data is needed to extract
      horizontal and vertical noise values to be used for interference
      detection.
  
   Inputs:
      pmd_data - RDA Performance/Maintenance data.

**********************************************************************/
void ID_process_rda_perf_maint_msg( char *pmd_data ){

   Pmd_t *pmd = (Pmd_t *)
                (pmd_data + sizeof(RDA_RPG_message_header_t));

   /* H & V Short Pulse noise values. */
   PBD_h_shrt_pulse_noise = pmd->h_shrt_pulse_noise;
   PBD_v_shrt_pulse_noise = pmd->v_shrt_pulse_noise;
   PBD_h_shrt_pulse_noise_thr = PBD_h_shrt_pulse_noise + Noise_thr;
   PBD_v_shrt_pulse_noise_thr = PBD_v_shrt_pulse_noise + Noise_thr;

   /* H & V Long Pulse noise values. */
   PBD_h_long_pulse_noise = pmd->h_long_pulse_noise;
   PBD_v_long_pulse_noise = pmd->v_long_pulse_noise;
   PBD_h_long_pulse_noise_thr = PBD_h_long_pulse_noise + Noise_thr;
   PBD_v_long_pulse_noise_thr = PBD_v_long_pulse_noise + Noise_thr;

   LE_send_msg( GL_INFO, "H & V Noise Levels and Thresholds\n" );
   LE_send_msg( GL_INFO, "--->PBD_h_shrt_pulse_noise: %f, Threshold: %f\n",
                PBD_h_shrt_pulse_noise, PBD_h_shrt_pulse_noise_thr );
   LE_send_msg( GL_INFO, "--->PBD_v_shrt_pulse_noise: %f, Threshold: %f\n",
                PBD_v_shrt_pulse_noise, PBD_v_shrt_pulse_noise_thr );
   LE_send_msg( GL_INFO, "--->PBD_h_long_pulse_noise: %f, Threshold: %f\n",
                PBD_h_long_pulse_noise, PBD_h_long_pulse_noise_thr );
   LE_send_msg( GL_INFO, "--->PBD_v_long_pulse_noise: %f, Threshold: %f\n",
                PBD_v_long_pulse_noise, PBD_v_long_pulse_noise_thr );

   return;

/* End of ID_process_rda_rpg_loopback_message() */
}


