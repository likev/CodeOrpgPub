/* 
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2012/12/17 16:14:54 $
 * $Id: translate_proc_msgs.c,v 1.9 2012/12/17 16:14:54 steves Exp $
 * $Revision: 1.9 $
 * $State: Exp $
 */  

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>            /* EXIT_FAILURE                            */
#include <orpg.h>
#include <vcp.h>

#include <basedata.h>
#include <generic_basedata.h>
#include <orpggdr.h>
#include <a309.h>

#define GLOBAL_DEFINED
#include <translate_main.h>

/* Used for VCP translation. */
typedef struct {

   /* Segment 1 */
   unsigned short edge_angle1;

   short dopp_prf_num1;

   short dopp_prf_pulse1;  

   short not_used_1;

   /* Segment 2 */
   unsigned short edge_angle2;

   short dopp_prf_num2;

   short dopp_prf_pulse2;  

   short not_used_2;

   /* Segment 3 */
   unsigned short edge_angle3;

   short dopp_prf_num3;

   short dopp_prf_pulse3;

   short not_used_3;

} Prf_info_t;

#ifdef BUILD13_OR_EARLIER

/* Added (appended) to Radial Block header with Major Version 2 of Message 31. 
   The "len" field also indicates whether this information is present. */
#define GENERIC_RAD_DBZ0_MAJOR          2
typedef struct {

   float h_dBZ0;                      /* Horizontal dBZ0. */

   float v_dBZ0;                      /* Vertical dBZ0. */

} Generic_rad_dBZ0_t;

/* Stores the elevation indices for supplemental cuts. */
static int Supple_cuts[2] = { -1, -1 };
static int Supple_cuts_elev_map_size = 0;
static int Supple_cuts_elev_map[VCP_MAXN_CUTS];

/* Function Prototypes. */
static char* Message_31_convert( char *radar_data, int offset );

#endif

/*******************************************************************

   Description:   Process the RDA Control Command.   Converts the 
                  elevation number in an elevation restart command
                  if VCP translation is active.

   Inputs:        rda_msg - RDA Control Command.

   Outputs:       

   Returns:       It returns 0 on success or -1 on failure.

*********************************************************************/
int Process_rda_control ( char *rda_msg /* RDA control message. */ ){

   ORDA_control_commands_t *ctrl_cmd = 
                                (ORDA_control_commands_t *) rda_msg;

   /* Translate the RDA Control message, if necessary. */
   if( !Is_translation_table_installed() )
      return 0;

   /* Is VCP Translation active? */
   if( !Trans_info.active )
      return 0;

   /* Check the Restart Volume/Elevation halfword.   If it is does
      not specify an elevation restart, just return.   Otherwise,
      because VCP translation is active, figure would what cut needs 
      to be restarted. */
   if( ctrl_cmd->restart_elev > RESTART_VOLUME ){

      int elev_num, i;

      /* The Current_table should not be NULL if RPG is requesting an
         elevation restart.   If it is, write an error message to the 
         RPG Status Log and return error. */
      if( Current_table == NULL ){

         LE_send_msg( GL_STATUS | LE_RPG_WARN_STATUS, 
                      "RDA Elevation Restart Commanded But Current Table is NULL\n" );
         return -1;

      } 
   
      /* Get the elevation number from the RDA Control Command. */
      elev_num = ctrl_cmd->restart_elev & 0xf; 

      /* Search for match in translation table. */
      for( i = 0; i < Current_table->elev_cut_map_size; i++ ){

         if( elev_num == Current_table->elev_cut_map[i] ){

            ctrl_cmd->restart_elev = RESTART_VOLUME + (i+1);
            LE_send_msg( GL_INFO, "Restarting Elevation Cut %d, RPG Commanded %d\n",
                         i+1, elev_num );
            return 0;

         }

      }

      /* If it gets here, not match found. */
      return -1;

   }

   return 0;

} /* End of Process_rda_control(). */

/*******************************************************************

   Description:   Process the RDA status message.   Converts the 
                  vcp number according to VCP translation table.

   Inputs:        rda_msg - RDA status data.

   Outputs:       

   Returns:       It returns 0 on success or -1 on failure.

*********************************************************************/
int Process_status ( char *rda_msg /* RDA status message */ ){

   ORDA_status_msg_t *status = (ORDA_status_msg_t *) rda_msg;
   int ret, temp_vcp;

   Trans_info.active = 0;

   temp_vcp = (int) status->vcp_num;
   Trans_info.vcp_external = Trans_info.vcp_internal = abs( temp_vcp );

   LE_send_msg( GL_INFO, "Processing RDA Status\n" );

   /* If translation table not installed, do nothing.  Also if the 
      vcp_num in RDA Status is negative, it is a local pattern .....
      local patterns are never translated. */
   if( (Is_translation_table_installed()) && (temp_vcp > 0) ){

      /* See if the VCP in the RDA status message is a VCP that needs
         translation. */
      Current_table = Find_in_translation_table( (int) status->vcp_num, 
                                                 TRNS_INCOMING_VCP );

      /* By default, if no translation necessary, the "translate_to_vcp" is
         the same as "status->vcp_num".  Otherwise, "translate_to_vcp" is
         the VCP number of the VCP to translate to. */
      status->vcp_num = Current_table->translate_to_vcp;

      /* Let the user know. */
      if( temp_vcp != status->vcp_num ){

         Trans_info.active = 1;
         Trans_info.vcp_internal = (int) status->vcp_num;

         if( TRNS_verbose )
            LE_send_msg( GL_INFO, "-->RDA Status VCP number %d changed to %d\n",
                         temp_vcp, status->vcp_num ); 

      }
   
   }

   /* Update the "Translation Active" message. */
   if( (ret = ORPGDA_write( ORPGDAT_SUPPL_VCP_INFO, (char *) &Trans_info, 
                            sizeof( Trans_info_t ), TRANS_ACTIVE_MSG_ID )) < 0 ){

      LE_send_msg( GL_ERROR, "ORPGDA_write( ORPGDAT_SUPPL_VCP_INFO ) Failed: %d\n", 
                   ret );
      ORPGTASK_exit( GL_EXIT_FAILURE );

   }
      
   return 0;

} /* End of Process_status() */

/*******************************************************************

   Description:   Includes all necessary processing for the RDA/RPG
                  VCP message and the RPG/RDA VCP message.  

   Inputs:        rda_msg - VCP data.

   Outputs:       size - size of the VCP (translated size if VCP
                         was translated). 

   Returns:       It returns 0 on success or -1 on failure.

   Notes:         For RDA/RPG VCP translation, it is assumed the RDA
                  VCP is larger than the RPG VCP. 

*********************************************************************/
int Process_vcp ( char *rda_msg /* VCP Message */, int *size ){

   RDA_RPG_message_header_t *hdr = (RDA_RPG_message_header_t *) rda_msg;
   VCP_ICD_msg_t *vcp;
   char *temp;
   int vcp_size_bytes, i, ncuts, vcp_internal;

   static VCP_ICD_msg_t new_vcp;

   /* RPG/RDA VCP message ...... */
   if( hdr->type == RPG_RDA_VCP )
      return ( Process_RPG_RDA_vcp( rda_msg, size ) );

   vcp = (VCP_ICD_msg_t *) (rda_msg + sizeof( RDA_RPG_message_header_t)); 

   LE_send_msg( GL_INFO, "Processing RDA/RPG VCP %d\n", vcp->vcp_msg_hdr.pattern_number );

   /* Find the current VCP in the translation table. Translate the RDA/RPG VCP 
      message, if necessary. */
   Current_table = Find_in_translation_table( (int) vcp->vcp_msg_hdr.pattern_number,
                                              TRNS_INCOMING_VCP );

   /* Test whether translation is necessary. */
   if( (vcp->vcp_msg_hdr.pattern_number == Current_table->translate_to_vcp)
                                        ||
       (vcp->vcp_msg_hdr.pattern_number != Current_table->incoming_vcp) ){

      LE_send_msg( GL_INFO, "-->No Translation Required for RDA/RPG VCP %d\n", 
                   vcp->vcp_msg_hdr.pattern_number );
      return 0;

   }

   LE_send_msg( GL_INFO, "-->Translating RDA/RPG VCP %d to VCP %d\n", 
                vcp->vcp_msg_hdr.pattern_number,
                Current_table->translate_to_vcp );

   if( vcp->vcp_elev_data.number_cuts != Current_table->elev_cut_map_size ){

      LE_send_msg( GL_ERROR, "-->Translation Failed! Incorrect Number of Cuts in Translation Table\n" );
      return -1;

   }

   vcp_internal = (double) Current_table->translate_to_vcp;

   /* Copy the VCP header. */
   memcpy( (void *) &new_vcp, rda_msg + sizeof( RDA_RPG_message_header_t), 
           VCP_ATTR_SIZE*sizeof(short) );

   /* Copy all mapped elevations.  Why do we do this instead of just reading
      the translated VCP from RPG default VCP data?   Because the metadata
      should be representative of the VCP which actually executed at the RDA 
      .... this includes any velocity increment or PRFs which might be 
      different than those defined by default. */
   vcp_size_bytes = VCP_ATTR_SIZE*sizeof(short);
   temp = ((char *) &new_vcp) + vcp_size_bytes;
   ncuts = 0;
   for( i = 0; i < new_vcp.vcp_elev_data.number_cuts; i++ ){

      if( Current_table->elev_cut_map[i] < 0 )
         continue; 

      memcpy( temp, &vcp->vcp_elev_data.data[i], ELE_ATTR_SIZE*sizeof(short) );
      vcp_size_bytes += ELE_ATTR_SIZE*sizeof(short);
      temp = ((char *) &new_vcp) + vcp_size_bytes;

      ncuts++;

   }

   /* Change the pattern number to the translated VCP. */
   new_vcp.vcp_msg_hdr.pattern_number = Current_table->translate_to_vcp;

   /* Change the size of the VCP data and number of cuts. */
   new_vcp.vcp_msg_hdr.msg_size = VCP_ATTR_SIZE + ncuts*ELE_ATTR_SIZE; 
   new_vcp.vcp_elev_data.number_cuts = ncuts;

   /* Copy the data back to input buffer. */
   memcpy( rda_msg + sizeof(RDA_RPG_message_header_t), &new_vcp, vcp_size_bytes );

   /* Change the size of the message field in the Message Header. */
   hdr->size = sizeof( RDA_RPG_message_header_t )/sizeof( short ) +
               new_vcp.vcp_msg_hdr.msg_size; 

   /* Return the new size, in bytes, of the VCP message ... size includes 
      the RDA/RPG message Header and the VCP data. */
   *size = hdr->size*sizeof(short);

   return 0;

} /* End of Process_vcp() */

#ifdef BUILD13_OR_EARLIER
/*******************************************************************

   Description:   Includes all necessary processing for the RDA/RPG
                  VCP message and removing supplemental cuts.  

   Inputs:        rda_msg - VCP data.

   Outputs:       size - size of the VCP (translated size if VCP
                         was translated). 

   Returns:       It returns 0 if VCP definition does not have
                  supplemental cuts, or 1 if it does.

*********************************************************************/
int Remove_suppl_cuts_from_vcp ( char *rda_msg, int *size ){

   RDA_RPG_message_header_t *hdr = (RDA_RPG_message_header_t *) rda_msg;
   VCP_ICD_msg_t *vcp;
   char *temp;
   int has_supple_cuts, vcp_size_bytes, i, ncuts, cnt;
   float elev_angle, first_elev_angle;

   static VCP_ICD_msg_t new_vcp;
   static char buf[128], tmp[4];

   vcp = (VCP_ICD_msg_t *) (rda_msg + sizeof( RDA_RPG_message_header_t));

   /* Initialize the has_supple_cuts flag to No. */
   has_supple_cuts = 0;

   /* Copy the VCP header. */
   memcpy( (void *) &new_vcp, rda_msg + sizeof( RDA_RPG_message_header_t),
           VCP_ATTR_SIZE*sizeof(short) );

   /* Copy all mapped elevations.  Why do we do this instead of just reading
      the translated VCP from RPG default VCP data?   Because the metadata
      should be representative of the VCP which actually executed at the RDA 
      .... this includes any velocity increment or PRFs which might be 
      different than those defined by default. */
   vcp_size_bytes = VCP_ATTR_SIZE*sizeof(short);
   temp = ((char *) &new_vcp) + vcp_size_bytes;
   ncuts = 0;
   first_elev_angle = 0.5;
   cnt = 0;
   Supple_cuts[0] = Supple_cuts[1] = -1;
   Supple_cuts_elev_map_size = 0;
   memset( &Supple_cuts_elev_map[0], 0, VCP_MAXN_CUTS*sizeof(int) );

   for( i = 0; i < new_vcp.vcp_elev_data.number_cuts; i++ ){

      Ele_attr *ele = (Ele_attr *) &vcp->vcp_elev_data.data[i];
      elev_angle = ORPGVCP_BAMS_to_deg( ORPGVCP_ELEVATION_ANGLE, ele->ele_angle ); 

      /* Is this the first cut in the VCP? */
      if( i == 0 )
         first_elev_angle = elev_angle;

      /* Assume the lowest cut is part of a split cut so we check
         cuts above the lowest split cut (i.e., i > 1). */
      else if( (i > 1) && (elev_angle == first_elev_angle ) ){

         LE_send_msg( GL_INFO, "RDA VCP has Supplemental Cuts (cut #: %d)\n",
                      i+1 );
         has_supple_cuts = 1;

         /* Set the cuts we believe are supplemental cuts. */
         if( cnt > 1 ){

            LE_send_msg( GL_INFO, "More Supplemental Cuts than Expected: %d\n", cnt+1 );
            continue;

         }

         Supple_cuts[cnt] = i+1;         
         cnt++;
         continue;

      }
      
      memcpy( temp, &vcp->vcp_elev_data.data[i], ELE_ATTR_SIZE*sizeof(short) );
      vcp_size_bytes += ELE_ATTR_SIZE*sizeof(short);
      temp = ((char *) &new_vcp) + vcp_size_bytes;

      ncuts++;

   }

   /* If VCP has supplemental cuts, then set the elevation mapping table. */
   if( has_supple_cuts ){

      LE_send_msg( GL_INFO, "VCP Has Supplemental Cuts\n" );
      Supple_cuts_elev_map_size = new_vcp.vcp_elev_data.number_cuts;
      cnt = 0;
      for( i = 1; i <= Supple_cuts_elev_map_size; i++ ){

         cnt++;
         if( (i == Supple_cuts[0])
                   ||
             (i == Supple_cuts[1]) ){

            Supple_cuts_elev_map[i-1] = -1;
            cnt--;

         }
         else
            Supple_cuts_elev_map[i-1] = cnt;   

      }

      /* Print out the Supplemental Cuts Elevation Mapping table. */
      memset( &buf[0], 0, 128 );
      LE_send_msg( GL_INFO, "Supplemental Cuts Elevation Map Size: %d\n", 
                   Supple_cuts_elev_map_size );

      for( i = 0; i < Supple_cuts_elev_map_size; i++ ){

         memset( &tmp[0], 0, 4 );
         sprintf( &tmp[0], " %2d", Supple_cuts_elev_map[i] );
         strcat( buf, &tmp[0] );

      }

      LE_send_msg( GL_INFO, "%s\n", buf );
         
   }

   /* Change the size of the VCP data and number of cuts. */
   new_vcp.vcp_msg_hdr.msg_size = VCP_ATTR_SIZE + ncuts*ELE_ATTR_SIZE;
   new_vcp.vcp_elev_data.number_cuts = ncuts;

   /* Copy the data back to input buffer. */
   memcpy( rda_msg + sizeof(RDA_RPG_message_header_t), &new_vcp, vcp_size_bytes );

   /* Change the size of the message field in the Message Header. */
   hdr->size = sizeof( RDA_RPG_message_header_t )/sizeof( short ) +
               new_vcp.vcp_msg_hdr.msg_size;

   /* Return the new size, in bytes, of the VCP message ... size includes 
      the RDA/RPG message Header and the VCP data. */
   *size = hdr->size*sizeof(short);

   /* Return the flag to indicate if the VCP has supplemental cuts. */
   return has_supple_cuts;

} /* End of Remove_suppl_cuts_from_vcp() */
#endif


/*************************************************************************

   Description:   Includes all necessary processing for the RDA/RPG VCP 
                  message.  

                  The RPG to RDA VCP message may need to be translated.
                  If translation is required, a local copy of the default
                  VCP data for the translated VCP is made.  PRF information
                  from the translated VCP is applied to this local copy.
                  For those cuts in the translated VCP which are not
                  in the VCP to be translated, the PRF information is
                  copied to all cuts below 7.0 deg elevation.

   Inputs:        rda_msg - VCP data.

   Outputs:       size - size of the VCP message, if translated.

   Returns:       It returns 0 on success or -1 on failure.

***************************************************************************/
int Process_RPG_RDA_vcp ( char *rda_msg, int *size){

   VCP_ICD_msg_t *vcp = NULL, *s_vcp = NULL;
   int vcp_size_shorts, vcp_size_bytes, ind, prf_set, i;
   RDA_RPG_message_header_t *hdr = NULL;
   Prf_info_t prf_info;

   static VCP_ICD_msg_t new_vcp;
   
   /* Translate the RDA/RPG VCP message, if necessary. */
   if( !Is_translation_table_installed() )
      return -1;

   /* Is VCP Translation active? */
   if( !Trans_info.active )
      return 0;

   /* Initialize the PRF info. */
   memset( &prf_info, 0, sizeof(Prf_info_t) );
   prf_set = 0;

   /* Translate the VCP if a translatable VCP. */
   vcp = (VCP_ICD_msg_t *) (rda_msg + sizeof(RDA_RPG_message_header_t));
   vcp_size_bytes = vcp->vcp_msg_hdr.msg_size*sizeof(short);

   LE_send_msg( GL_INFO, "Processing RPG/RDA VCP %d\n", vcp->vcp_msg_hdr.pattern_number );

   /* Test whether translation is necessary. */
   if( (vcp->vcp_msg_hdr.pattern_number != Current_table->translate_to_vcp)
                                        ||
       (vcp->vcp_msg_hdr.pattern_number == Current_table->incoming_vcp) 
                                        ||
       (vcp->vcp_elev_data.spare8 == VCP_DO_NOT_TRANSLATE) ){

      LE_send_msg( GL_INFO, "-->No Translation Required for RPG/RDA VCP %d\n", 
                   vcp->vcp_msg_hdr.pattern_number );

      vcp->vcp_elev_data.spare8 = 0;
      return 0;

   }

   LE_send_msg( GL_INFO, "-->Translating RPG/RDA VCP %d to VCP %d\n", 
                vcp->vcp_msg_hdr.pattern_number,
                Current_table->incoming_vcp );

   /* Get the default VCP data for the VCP to be translated. */
   ind = ORPGVCP_index( Current_table->incoming_vcp );
   s_vcp = (VCP_ICD_msg_t *) ORPGVCP_ptr( ind );
   if( s_vcp == NULL ){

      LE_send_msg( GL_INFO, "-->Translation Failed!  VCP %d Not Found\n",
                   Current_table->translate_to_vcp );
      return -1;

   }
            

   /* Make a local copy of the VCP to be translated.  This local copy will
      be modified with PRF information from the VCP to be translated. */
   vcp_size_bytes = s_vcp->vcp_msg_hdr.msg_size*sizeof(short);
   memcpy( (void *) &new_vcp, (void *) s_vcp, vcp_size_bytes );

   /* Move the velocity increment from the VCP to be translated. */
   new_vcp.vcp_elev_data.doppler_res = vcp->vcp_elev_data.doppler_res;

   /* Get the PRF information so we can apply it to any cut < 7.0 deg in 
      the experimental VCP that is not in the operational VCP. */
   for( i = 0; i < Current_table->elev_cut_map_size; i++ ){

      VCP_elevation_cut_data_t *cut = NULL;

      if( Current_table->elev_cut_map[i] <= 0 )
         continue;

      cut = &vcp->vcp_elev_data.data[Current_table->elev_cut_map[i]-1];
      memcpy( &new_vcp.vcp_elev_data.data[i], cut, ELE_ATTR_SIZE*sizeof(short) );

      /* Get the PRF number that is applied at the lowest CD cut.  This prf
         will be used to set all excluded Doppler cuts of the translated
         VCP < 7.0 deg elevation. */
      if( (!prf_set) && (Current_table->elev_cut_map[i] <= 2)
                        &&
          (new_vcp.vcp_elev_data.data[i].waveform == VCP_WAVEFORM_CD) ){
               
         memcpy( &prf_info, &cut->edge_angle1, sizeof(Prf_info_t) );
         prf_set = 1;

      }

   }

   /* Apply the PRFs information from input buffer to translated VCP. */
   if( prf_set ){

      for( i = 0; i < Current_table->elev_cut_map_size; i++ ){

         /* Skip cut if common between the two VCPs. */
         if( Current_table->elev_cut_map[i] > 0 )
            continue;

         /* If cut is CD/W or BATCH and elevation angle is less than 7.0 deg,
            apply PRF information. */
         if( (new_vcp.vcp_elev_data.data[i].waveform == VCP_WAVEFORM_CD)
                                 ||
             (new_vcp.vcp_elev_data.data[i].waveform == VCP_WAVEFORM_BATCH) ){

            float elev_angle = ORPGVCP_get_elevation_angle( Current_table->incoming_vcp, i );
                                                            
            if( elev_angle < 7.0 ){

               memcpy( &new_vcp.vcp_elev_data.data[i].edge_angle1, &prf_info, 
                       sizeof(Prf_info_t) );

               /* Set the pulse cnt information based on PRF. */
               new_vcp.vcp_elev_data.data[i].dopp_prf_pulse1 =
                     ORPGVCP_get_allowable_prf_pulse_count( Current_table->incoming_vcp, i,
                                                               prf_info.dopp_prf_num1 );
               new_vcp.vcp_elev_data.data[i].dopp_prf_pulse2 =
                     ORPGVCP_get_allowable_prf_pulse_count( Current_table->incoming_vcp, i,
                                                               prf_info.dopp_prf_num2 );
               new_vcp.vcp_elev_data.data[i].dopp_prf_pulse3 =
                     ORPGVCP_get_allowable_prf_pulse_count( Current_table->incoming_vcp, i,
                                                               prf_info.dopp_prf_num3 );

            }

         }

      }

   }

   /* Copy the translated VCP to input buffer. */
   memcpy( rda_msg + sizeof(RDA_RPG_message_header_t), &new_vcp, vcp_size_bytes );

   /* Set the new size of the VCP message. */
   hdr = (RDA_RPG_message_header_t *) rda_msg;
   vcp = (VCP_ICD_msg_t *) (rda_msg + sizeof(RDA_RPG_message_header_t));

   vcp_size_shorts = vcp->vcp_msg_hdr.msg_size;
   hdr->size = vcp_size_shorts + sizeof(RDA_RPG_message_header_t)/sizeof(short);

   *size = hdr->size*sizeof(short);

   return 0;
   
}

/*******************************************************************

   Description:   List the converted VCP message. 

   Inputs:        rda_msg - VCP data.

   Outputs:

   Returns:       It returns 0 on success or -1 on failure.

*********************************************************************/
void Write_vcp_data( char *rda_msg ){

   static char *reso[] = { "0.5 m/s", "1.0 m/s" };
   static char *width[] = { "SHORT", "LONG" };
   static char *wave_form[] = { "UNK", "CS", "CD/W", "CD/WO", "BATCH", "STP" };
   static char *phase[] = { "CON", "RAN", "SZ2" };

   int i;
   short wform, phse;
   VCP_ICD_msg_t *vcp = (VCP_ICD_msg_t *) (rda_msg + sizeof(RDA_RPG_message_header_t) );

   /* Write out VCP data. */
   LE_send_msg( GL_INFO, "VCP %d Data:\n", vcp->vcp_msg_hdr.pattern_number );
   LE_send_msg( GL_INFO, "--->VCP Header:\n" ); 
   LE_send_msg( GL_INFO, "       Size (shorts): %4d   Type: %4d   # Elevs: %4d\n",
                vcp->vcp_msg_hdr.msg_size, vcp->vcp_msg_hdr.pattern_type, 
                vcp->vcp_elev_data.number_cuts );

   LE_send_msg( GL_INFO, "       Clutter Group: %4d   Vel Reso: %s   Pulse Width: %s\n\n",
            vcp->vcp_elev_data.group, reso[ vcp->vcp_elev_data.doppler_res/4 ], 
            width[ vcp->vcp_elev_data.pulse_width/4 ] );


   /* Do For All elevation cuts. */
   for( i = 0; i < vcp->vcp_elev_data.number_cuts; i++ ){

      Ele_attr *elev = (Ele_attr *) &vcp->vcp_elev_data.data[i];

      float elev_angle = ORPGVCP_BAMS_to_deg( ORPGVCP_ELEVATION_ANGLE, elev->ele_angle );
      float azi_ang_1 = ORPGVCP_BAMS_to_deg( ORPGVCP_AZIMUTH_ANGLE, elev->azi_ang_1 );
      float azi_ang_2 = ORPGVCP_BAMS_to_deg( ORPGVCP_AZIMUTH_ANGLE, elev->azi_ang_2 );
      float azi_ang_3 = ORPGVCP_BAMS_to_deg( ORPGVCP_AZIMUTH_ANGLE, elev->azi_ang_3 );

      wform = elev->wave_type;
      phse = elev->phase;
      LE_send_msg( GL_INFO, "--->Elevation %d:\n", i+1 );
      LE_send_msg( GL_INFO, "       Elev Angle: %5.2f   Wave Type: %s   Phase: %s   Surv PRF: %2d   Surv Pulses: %4d\n",
                   elev_angle, wave_form[ wform ], phase[ phse ], 
                   elev->surv_prf_num, elev->surv_pulse_cnt );
      LE_send_msg( GL_INFO, "       Az Rate: %4d (0x%4x) (BAMS)   SNR Threshold: %5.2f  %5.2f  %5.2f (dB)\n",
                   elev->azi_rate, elev->azi_rate, (float) elev->surv_thr_parm/8.0, 
                   (float) elev->vel_thrsh_parm/8.0, (float) elev->spw_thrsh_parm/8.0 );

      LE_send_msg( GL_INFO, "       PRF Sector 1:\n" );
      LE_send_msg( GL_INFO, "       --->Edge Angle: %5.2f   Dop PRF: %2d   Dop Pulses: %4d\n",
                   azi_ang_1, elev->dop_prf_num_1, elev->pulse_cnt_1 );

      LE_send_msg( GL_INFO, "       PRF Sector 2:\n" );
      LE_send_msg( GL_INFO, "       --->Edge Angle: %5.2f   Dop PRF: %2d   Dop Pulses: %4d\n",
                   azi_ang_2, elev->dop_prf_num_2, elev->pulse_cnt_2 );

      LE_send_msg( GL_INFO, "       PRF Sector 3:\n" );
      LE_send_msg( GL_INFO, "       --->Edge Angle: %5.2f   Dop PRF: %2d   Dop Pulses: %4d\n",
                   azi_ang_3, elev->dop_prf_num_3, elev->pulse_cnt_3 );

   }   

}

/*************************************************************************************

   Description:
      Processes the Radial Message.   Changes the VCP number, elevation number, 
      and radial status.

   Inputs:
      rda_msg - buffer containing the radial data message.
      has_supple_cuts - Indicates if VCP has supplemental cuts. 

   Outputs:

   Returns: NULL on error, of pointer to radial otherwise.

**************************************************************************************/
char* Process_radial( char *radar_data, int offset, int has_supple_cuts ){

   char *rda_msg = radar_data + offset;
   char *retval = radar_data;

   Generic_vol_t *ele_hdr = NULL;
   Generic_basedata_t *hdr = (Generic_basedata_t *) rda_msg;

   /* Translate the RDA/RPG Radial message, if necessary. */
   if( !Is_translation_table_installed() )
      return NULL;
 
   /* Get the volume header from the radial. */
   ele_hdr = (Generic_vol_t *) ORPGGDR_get_data_block( rda_msg, ORPGGDR_RVOL );

   /* If hdr is NULL, then can't find the volume header. */
   if( ele_hdr == NULL ){

      LE_send_msg( GL_INFO, "Can't file volume header in radial message\n" );
      return NULL;

   }

   /* For efficiency, if the VCP in the message is the same as the incoming VCP, 
      no need for lookup ... */
   if( (Current_table == NULL) || (ele_hdr->vcp_num != Current_table->incoming_vcp) )
      Current_table = Find_in_translation_table( (int) ele_hdr->vcp_num,
                                                 TRNS_INCOMING_VCP );

   if( ele_hdr->vcp_num == Current_table->incoming_vcp ){

      /* Test whether translation is necessary. */
      if( ele_hdr->vcp_num != Current_table->translate_to_vcp ){

         /* Inform the Operator that Experimental VCP is executing. */
         if( hdr->base.status == GOODBVOL )
            LE_send_msg( GL_STATUS | LE_RPG_WARN_STATUS, 
                         "VCP TRANSLATION: Incoming VCP %d -- Translated --> %d\n", 
                         ele_hdr->vcp_num, Current_table->translate_to_vcp ); 

         /* If elevation mapping table value negative, discard
            this elevation. */
         if( Current_table->elev_cut_map[hdr->base.elev_num-1] < 0 )
            return NULL;

         /* Translate the VCP in the radial header. */
         ele_hdr->vcp_num = Current_table->translate_to_vcp;

         /* Translate the elevation cut number. */
         hdr->base.elev_num = Current_table->elev_cut_map[hdr->base.elev_num-1];

         /* Change the radial status if necessary. */
         if( (hdr->base.elev_num == 1) && (hdr->base.status == GOODBEL) )
            hdr->base.status = GOODBVOL;

         if( hdr->base.status == GENDEL ){

            int i;

            for( i = Current_table->elev_cut_map_size - 1; i >= 0; i-- ){

               if( Current_table->elev_cut_map[i] > 0 ){

                  if( hdr->base.elev_num == Current_table->elev_cut_map[i] )
                     hdr->base.status = GENDVOL;

                  break;

               }

            } /* End of "for" loop. */

         } /* End of "if( hdr->status == GENDEL )" */

      } /* End of "if( hdr->vcp_num != Current_table->translate_vcp_vcp )" */

   } /* End of "if( hdr->vcp_num == Current_table->incoming_vcp )" */

#ifdef BUILD13_OR_EARLIER
   /* Check if this radial is a supplemental cut and if so, toss it on the 
      floor. */
   if( has_supple_cuts ){

      /* A supplemental cut with have RDA elevation index greater than 2. */
      if( hdr->base.elev_num == Supple_cuts[0]
                             ||
          hdr->base.elev_num == Supple_cuts[1] )
         return NULL;

      else
         hdr->base.elev_num = Supple_cuts_elev_map[hdr->base.elev_num-1];

   }
    
   /* Check if the major number is not the major number desired. */
   if( ele_hdr->major_version == GENERIC_RAD_DBZ0_MAJOR ){

      /* Write out message at start of volume/elevation. */
      if( (hdr->base.status == GOODBVOL)
                     ||
          (hdr->base.status == GOODBEL) )
         LE_send_msg( GL_INFO, "Convert Major Version 2 to Major Version 1\n" );

      /* Convert the radial message. */
      retval = Message_31_convert( radar_data, offset );

   }
#endif

   return retval;

} /* End of Process_radial() */


#ifdef BUILD13_OR_EARLIER
/******************************************************************

   Description:
      Converts message 31, major version 2 to major version 1. 

   Inputs:
      rda_msg - RDA message 31.
      hdr_off - offset from start of message to RDA/RPG Message 
                Header.

   Returns:
      Pointer to radial message on success, NULL on failure.

******************************************************************/
static char* Message_31_convert( char *radar_data, int hdr_off ){

   static int buf_size = 0;
   static char *buf = NULL;

   /* Set pointers that are at fixed locations. */
   char *rda_msg = radar_data + hdr_off;
   Generic_basedata_t *gbd = (Generic_basedata_t *) rda_msg;
   RDA_RPG_message_header_t *msg_hd = (RDA_RPG_message_header_t *) &gbd->msg_hdr;
   Generic_basedata_header_t *gbhd = (Generic_basedata_header_t *) &gbd->base;

   /* The following pointer are set when parsing the radial. */
   Generic_vol_t *vol_hd = NULL;
   Generic_elev_t *elv_hd = NULL;
   Generic_rad_t *rad_hd = NULL;
   Generic_moment_t *refhd = NULL, *velhd = NULL, *spwhd = NULL;
   Generic_moment_t *zdrhd = NULL, *rhohd = NULL, *phihd = NULL;

   /* The sizes are set when parsing the radial. */
   int vol_hd_size = 0, elv_hd_size = 0, rad_hd_size = 0;
   int ref_size = 0, vel_size = 0, spw_size = 0;
   int zdr_size = 0, rho_size = 0, phi_size = 0;
   int offset = 0;

   /* Number of blocks. */
   int no_of_datum = gbhd->no_of_datum;
   int size = msg_hd->size*sizeof(short) + hdr_off;
   int i, cnt = 0;

   /* Allocate temporary buffer. */
   if( (size > buf_size) 
            ||
       (buf == NULL) ){

      if( buf != NULL )
         free(buf);

      buf = calloc( size + 1000, 1 );
      if( buf == NULL ){

         LE_send_msg( GL_INFO, "calloc Failed for %d bytes\n", size );
         return(radar_data);

      }

      buf_size = size + 1000;

   }

   /* Initialize the size of the radial.  Note: We include
      the number of bytes needed for the data block offsets. */
   size = ALIGNED_SIZE( sizeof(Generic_basedata_header_t) + 
                        sizeof(RDA_RPG_message_header_t) + 
                        hdr_off +
                        no_of_datum*sizeof(int) );

   /* Copy the RDA/RPG message header and standard radial header to 
      temporary buffer. */
   memcpy( buf, radar_data, size );

   /* Parse the radial message. */
   for( i = 0; i < no_of_datum; i++ ){

      Generic_any_t *data_block = NULL;
      char type[5];

      offset = gbhd->data[i];

      data_block = (Generic_any_t *) (rda_msg + sizeof(RDA_RPG_message_header_t) + offset );
      memset( type, 0, 5 );
      memcpy( type, data_block->name, 4 );

      if( strstr( type, "RVOL" ) != NULL ){

         vol_hd = (Generic_vol_t *) data_block;
         vol_hd_size = ALIGNED_SIZE( sizeof(Generic_vol_t) );

      }
      else if( strstr( type, "RELV" ) != NULL ){

         elv_hd = (Generic_elev_t *) data_block;
         elv_hd_size = ALIGNED_SIZE( sizeof(Generic_elev_t) );

      }
      else if( strstr( type, "RRAD" ) != NULL ){

         rad_hd = (Generic_rad_t *) data_block;
         rad_hd_size = ALIGNED_SIZE( sizeof(Generic_rad_t) );

      }
      else if( strstr( type, "DREF" ) != NULL ){

         refhd = (Generic_moment_t *) data_block;
         ref_size = sizeof( Generic_moment_t);
         if( refhd->data_word_size == 8 )
            ref_size += refhd->no_of_gates*sizeof(unsigned char);

         else if( refhd->data_word_size == 16 )
            ref_size += refhd->no_of_gates*sizeof(unsigned short);

         else
            ref_size += refhd->no_of_gates*sizeof(unsigned char);

      }
      else if( strstr( type, "DVEL" ) != NULL ){

         velhd = (Generic_moment_t *) data_block;
         vel_size = ALIGNED_SIZE( sizeof( Generic_moment_t) );
         if( velhd->data_word_size == 8 )
            vel_size += velhd->no_of_gates*sizeof(unsigned char);

         else if( velhd->data_word_size == 16 )
            vel_size += velhd->no_of_gates*sizeof(unsigned short);

         else
            vel_size += velhd->no_of_gates*sizeof(unsigned char);

      }
      else if( strstr( type, "DSW" ) != NULL ){

         spwhd = (Generic_moment_t *) data_block;
         spw_size = ALIGNED_SIZE( sizeof( Generic_moment_t) );
         if( spwhd->data_word_size == 8 )
            spw_size += spwhd->no_of_gates*sizeof(unsigned char);

         else if( spwhd->data_word_size == 16 )
            spw_size += spwhd->no_of_gates*sizeof(unsigned short);

         else
            spw_size += spwhd->no_of_gates*sizeof(unsigned char);

      }
      else if( strstr( type, "DZDR" ) != NULL ){

         zdrhd = (Generic_moment_t *) data_block;
         zdr_size = ALIGNED_SIZE( sizeof( Generic_moment_t) );
         if( zdrhd->data_word_size == 8 )
            zdr_size += zdrhd->no_of_gates*sizeof(unsigned char);

         else if( zdrhd->data_word_size == 16 )
            zdr_size += zdrhd->no_of_gates*sizeof(unsigned short);

         else
            zdr_size += zdrhd->no_of_gates*sizeof(unsigned char);

      }
      else if( strstr( type, "DRHO" ) != NULL ){

         rhohd = (Generic_moment_t *) data_block;
         rho_size = ALIGNED_SIZE( sizeof( Generic_moment_t) );
         if( rhohd->data_word_size == 8 )
            rho_size += ALIGNED_SIZE( rhohd->no_of_gates*sizeof(unsigned char));

         else if( rhohd->data_word_size == 16 )
            rho_size += rhohd->no_of_gates*sizeof(unsigned short);

         else
            rho_size += rhohd->no_of_gates*sizeof(unsigned char);

      }
      else if( strstr( type, "DPHI" ) != NULL ){

         phihd = (Generic_moment_t *) data_block;
         phi_size = ALIGNED_SIZE( sizeof( Generic_moment_t) );
         if( phihd->data_word_size == 8 )
            phi_size += phihd->no_of_gates*sizeof(unsigned char);

         else if( phihd->data_word_size == 16 )
            phi_size += phihd->no_of_gates*sizeof(unsigned short);

         else
            phi_size += phihd->no_of_gates*sizeof(unsigned short);

      }

   }

   /* Reset pointers. */
   gbd = (Generic_basedata_t *) (buf + hdr_off);
   msg_hd = (RDA_RPG_message_header_t *) &gbd->msg_hdr;
   gbhd = (Generic_basedata_header_t *) &gbd->base;

   /* Copy the Volume Header Block. */
   memcpy( (buf + size), vol_hd, vol_hd_size );

   /* Change the version. */
   vol_hd = (Generic_vol_t *) (buf + size);
   vol_hd->major_version = 1;
   vol_hd->minor_version = 0;

   /* Offset to Volume Header Block.  This is the size of the 
      Generic_basedata_header plus 4 bytes for every data block. */
   gbhd->data[0] = ALIGNED_SIZE( sizeof(Generic_basedata_header_t) + 
                                 no_of_datum*sizeof(int) );

   /* Increment the total size. */
   size += vol_hd_size;

   /* Copy the Elevation Header Block and increment the total size. */
   memcpy( (buf + size), elv_hd, elv_hd_size );
   size += elv_hd_size;

   /* Offset to Elevation Header Block. */
   gbhd->data[1] = gbhd->data[0] + vol_hd_size;

   /* Copy the Radial Header Block and increment the total size. */
   memcpy( (buf + size), rad_hd, rad_hd_size );

   /* Change the block size. */
   rad_hd = (Generic_rad_t *) (buf + size);
   rad_hd->len = rad_hd_size;

   /* Increment the total size. */
   size += rad_hd_size;

   /* Offset to Radial Header Block. */
   gbhd->data[2] = gbhd->data[1] + elv_hd_size;


   /* Fill out the moments */
   cnt = 3;
   offset = rad_hd_size;

   /* Did radial have reflectivity? */
   if( refhd != NULL ){

      memcpy( (buf + size), refhd, ref_size );

      ref_size = ALIGNED_SIZE( ref_size );
      size += ref_size;

      /* Set the data block offset. */
      gbhd->data[cnt] = gbhd->data[cnt-1] + offset;
      offset = ref_size;

      /* Increment the number of blocks. */
      cnt++;

   }

   /* Did radial have velocity? */
   if( velhd != NULL ){

      memcpy( (buf + size), velhd, vel_size );

      vel_size = ALIGNED_SIZE( vel_size );
      size += vel_size;

      /* Set the data block offset. */
      gbhd->data[cnt] = gbhd->data[cnt-1] + offset;
      offset = vel_size;

      /* Increment the number of blocks. */
      cnt++;

   }

   /* Did radial have spectrum width? */
   if( spwhd != NULL ){

      memcpy( (buf + size), spwhd, spw_size );

      spw_size = ALIGNED_SIZE( spw_size );
      size += spw_size;

      /* Set the data block offset. */
      gbhd->data[cnt] = gbhd->data[cnt-1] + offset;
      offset = spw_size;

      /* Increment the number of blocks. */
      cnt++;

   }

   /* Did radial have differential reflectivity? */
   if( zdrhd != NULL ){

      memcpy( (buf + size), zdrhd, zdr_size );

      zdr_size = ALIGNED_SIZE( zdr_size );
      size += zdr_size;

      gbhd->data[cnt] = gbhd->data[cnt-1] + offset;
      offset = zdr_size;

      /* Increment the number of blocks. */
      cnt++;

   }

   /* Did radial have differential phase? */
   if( phihd != NULL ){

      memcpy( (buf + size), phihd, phi_size );
 
      phi_size = ALIGNED_SIZE( phi_size );
      size += phi_size;

      gbhd->data[cnt] = gbhd->data[cnt-1] + offset;
      offset = phi_size;

      /* Increment the number of blocks. */
      cnt++;

   }

   /* Did radial have correlation coefficient? */
   if( rhohd != NULL ){

      memcpy( (buf + size), rhohd, rho_size );

      rho_size = ALIGNED_SIZE( rho_size );
      size += rho_size;

      gbhd->data[cnt] = gbhd->data[cnt-1] + offset;
      offset = rho_size;

      /* Increment the number of blocks. */
      cnt++;

   }

   /* Update the size fields in the various headers. */
   gbhd->radial_length = size - hdr_off;
   msg_hd->size = (size - hdr_off + 1)/sizeof(short);

   /* Set the number of datum and data offsets. */ 
   gbhd->no_of_datum = cnt;

   /* Verify cnt matches no_of_datum. */
   if( cnt != no_of_datum ){

      LE_send_msg( GL_INFO, "cnt (%d) != no_of_datum (%d)\n",
                   cnt, no_of_datum );
      return(radar_data);

   }

LE_send_msg( 0, "Leaving Message 31\n" );
   return (buf);

/* End of Message_31_convert(). */
}
#endif
