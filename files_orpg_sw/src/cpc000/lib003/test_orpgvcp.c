/* 
 * RCS info
 * $Author $
 * $Locker:  $
 * $Date: 2012/09/13 16:06:14 $
 * $Id: test_orpgvcp.c,v 1.1 2012/09/13 16:06:14 steves Exp $
 * $Revision: 1.1 $
 * $State: Exp $
 *
 *
 */  

/* Includes for the gui start stop options */
#include "orpg.h"
#include <orpgvcp.h>

static int Get_all_elevation_angles( int vcp, int buffer_size, 
                                     int vs_num, float *elev_angles,
                                     unsigned short *suppl,
                                     int fallback );
static int Get_vcp_suppl_data( int vcp, int buffer_size,
                               unsigned short *suppl_flags );
/**************************************************************************
 *   Function: Read_archive_II_command
 *
 *   Description: Reads the Archive II status LB and returns the status. 
 *
 *   Input: rpg_lb (rpg LB id)
 *
 *   Output: status
 *
 *   Return: status
 *
 *   Notes:
 **************************************************************************/
int main( int argc, char *argv[] ){

   int response;
   int vcp_num, vol_num, num_elevs;
   int pulse_width;
   int clutter_map;
   int pattern_type;
   int vel_reso;
   float elev_angle;
   int waveform;
   int super_res;
   int dual_pol;
   float azi_rate;
   int config;
   int phase;
   int prf1, prf2, prf3;
   int puls_cnt1, puls_cnt2, puls_cnt3;
   float edge_ang1, edge_ang2, edge_ang3;
   float angles[ECUTMAX];
   unsigned short suppl[ECUTMAX];

   int i;

   ORPGMISC_init( argc, argv, 1000, 0, -1, 0 );

   while(1){

      fprintf( stdout, "\nSelect from the following commands:\n" );
      fprintf( stdout, "--->1 - Call Standard ORPGVP functions\n" );
      fprintf( stdout, "--->2 - Call Modified ORPGVP functions\n" );
      fprintf( stdout, "--->3 - Call Modified ORPGVCP functions with Volume Number\n" );
      fprintf( stdout, "--->4 - Test Get_all_elevations (ORPGPRQ)\n" );
      fprintf( stdout, "--->0 - Exit Tool\n" );

      scanf( "%d", &response );
      if( (response <= 0) || (response > 4))
         exit(1);

      fprintf( stdout, "\nEnter VCP Number\n" );
      scanf( "%d", &vcp_num );
      if( (vcp_num <= 0) || (vcp_num >= 255) )
         exit(1);

      switch (response){

         case 1:

            pulse_width = ORPGVCP_get_pulse_width( vcp_num );
            clutter_map = ORPGVCP_get_clutter_map_num( vcp_num );
            pattern_type = ORPGVCP_get_pattern_type( vcp_num );
            vel_reso = ORPGVCP_get_vel_resolution( vcp_num );
            num_elevs = ORPGVCP_get_num_elevations( vcp_num );

            fprintf( stderr, "VCP %d Attributes:\n", vcp_num );
            fprintf( stderr, "--->Num Elevs:     %d\n", num_elevs );
            fprintf( stderr, "--->Pulse Width:   %d\n", pulse_width );
            fprintf( stderr, "--->Clutter Map:   %d\n", clutter_map );
            fprintf( stderr, "--->Pattern Type:  %d\n", pattern_type );
            fprintf( stderr, "--->Vel Reso:      %d\n", vel_reso );
            
            for( i = 0; i < num_elevs; i++ ){
               
               elev_angle = ORPGVCP_get_elevation_angle( vcp_num, i );
               waveform = ORPGVCP_get_waveform( vcp_num, i );
               super_res = ORPGVCP_get_super_res( vcp_num, i );
               dual_pol = ORPGVCP_get_dual_pol( vcp_num, i );
               azi_rate = ORPGVCP_get_azimuth_rate( vcp_num, i );
               config = ORPGVCP_get_configuration( vcp_num, i );
               phase = ORPGVCP_get_phase_type( vcp_num, i );

               fprintf( stderr, "--->Elevation: %d\n", i );
               fprintf( stderr, "------>Elev angle: %f\n", elev_angle );
               fprintf( stderr, "------>waveform:   %d\n", waveform );
               fprintf( stderr, "------>Super Res:  %d\n", super_res );
               fprintf( stderr, "------>Dual Pol:   %d\n", dual_pol );
               fprintf( stderr, "------>Azm Rate:   %f\n", azi_rate );
               fprintf( stderr, "------>Config:     %d\n", config );
               fprintf( stderr, "------>Phase:      %d\n", phase );
               if( waveform != VCP_WAVEFORM_CS ){

                  prf1 = ORPGVCP_get_prf_num( vcp_num, i, ORPGVCP_DOPPLER1 );
                  prf2 = ORPGVCP_get_prf_num( vcp_num, i, ORPGVCP_DOPPLER2 );
                  prf3 = ORPGVCP_get_prf_num( vcp_num, i, ORPGVCP_DOPPLER3 );
                  edge_ang1 = ORPGVCP_get_edge_angle( vcp_num, i, ORPGVCP_DOPPLER1 );
                  edge_ang2 = ORPGVCP_get_edge_angle( vcp_num, i, ORPGVCP_DOPPLER2 );
                  edge_ang3 = ORPGVCP_get_edge_angle( vcp_num, i, ORPGVCP_DOPPLER3 );
                  puls_cnt1 = ORPGVCP_get_pulse_count( vcp_num | ORPGVCP_RDAVCP | ORPGVCP_VOLNUM, i, 
                                              ORPGVCP_DOPPLER1, vol_num );
                  puls_cnt2 = ORPGVCP_get_pulse_count( vcp_num | ORPGVCP_RDAVCP | ORPGVCP_VOLNUM, i, 
                                              ORPGVCP_DOPPLER2, vol_num );
                  puls_cnt3 = ORPGVCP_get_pulse_count( vcp_num | ORPGVCP_RDAVCP | ORPGVCP_VOLNUM, i, 
                                              ORPGVCP_DOPPLER3, vol_num );

                  fprintf( stderr, "------>PRF1:  %d, Pulse Cnt:  %d, Edge Angle1: %f\n", prf1, puls_cnt1, edge_ang1 );
                  fprintf( stderr, "------>PRF2:  %d, Pulse Cnt:  %d, Edge Angle2: %f\n", prf2, puls_cnt2, edge_ang2 );
                  fprintf( stderr, "------>PRF3:  %d, Pulse Cnt:  %d, Edge Angle3: %f\n", prf3, puls_cnt3, edge_ang3 );

               }

            }

            memset( &angles[0], 0, sizeof(float)*ECUTMAX );
            num_elevs = ORPGVCP_get_all_elevation_angles( vcp_num, ECUTMAX, &angles[0] );
            fprintf( stderr, "VCP: %d, All Elevations: %d\n", vcp_num, num_elevs );
            for( i = 0; i < num_elevs; i++ )
               fprintf( stderr, "--->Elev #: %d, angle: %f\n", i, angles[i] );

            break;

         case 2:

            pulse_width = ORPGVCP_get_pulse_width( vcp_num | ORPGVCP_RDAVCP );
            clutter_map = ORPGVCP_get_clutter_map_num( vcp_num | ORPGVCP_RDAVCP );
            pattern_type = ORPGVCP_get_pattern_type( vcp_num | ORPGVCP_RDAVCP );
            vel_reso = ORPGVCP_get_vel_resolution( vcp_num | ORPGVCP_RDAVCP );
            num_elevs = ORPGVCP_get_num_elevations( vcp_num | ORPGVCP_RDAVCP );

            fprintf( stderr, "RDA VCP %d Attributes:\n", vcp_num );
            fprintf( stderr, "--->Num Elevs:     %d\n", num_elevs );
            fprintf( stderr, "--->Pulse Width:   %d\n", pulse_width );
            fprintf( stderr, "--->Clutter Map:   %d\n", clutter_map );
            fprintf( stderr, "--->Pattern Type:  %d\n", pattern_type );
            fprintf( stderr, "--->Vel Reso:      %d\n", vel_reso );
            
            for( i = 0; i < num_elevs; i++ ){
               
               elev_angle = ORPGVCP_get_elevation_angle( vcp_num | ORPGVCP_RDAVCP, i );
               waveform = ORPGVCP_get_waveform( vcp_num | ORPGVCP_RDAVCP, i );
               super_res = ORPGVCP_get_super_res( vcp_num | ORPGVCP_RDAVCP, i );
               dual_pol = ORPGVCP_get_dual_pol( vcp_num | ORPGVCP_RDAVCP, i );
               azi_rate = ORPGVCP_get_azimuth_rate( vcp_num | ORPGVCP_RDAVCP, i );
               config = ORPGVCP_get_configuration( vcp_num | ORPGVCP_RDAVCP, i );
               phase = ORPGVCP_get_phase_type( vcp_num | ORPGVCP_RDAVCP, i );

               fprintf( stderr, "--->Elevation: %d\n", i );
               fprintf( stderr, "------>Elev angle: %f\n", elev_angle );
               fprintf( stderr, "------>waveform:   %d\n", waveform );
               fprintf( stderr, "------>Super Res:  %d\n", super_res );
               fprintf( stderr, "------>Dual Pol:   %d\n", dual_pol );
               fprintf( stderr, "------>Azm Rate:   %f\n", azi_rate );
               fprintf( stderr, "------>Config:     %d\n", config );
               fprintf( stderr, "------>Phase:      %d\n", phase );
               if( waveform != VCP_WAVEFORM_CS ){

                  prf1 = ORPGVCP_get_prf_num( vcp_num | ORPGVCP_RDAVCP, i, ORPGVCP_DOPPLER1 );
                  prf2 = ORPGVCP_get_prf_num( vcp_num | ORPGVCP_RDAVCP, i, ORPGVCP_DOPPLER2 );
                  prf3 = ORPGVCP_get_prf_num( vcp_num | ORPGVCP_RDAVCP, i, ORPGVCP_DOPPLER3 );
                  edge_ang1 = ORPGVCP_get_edge_angle( vcp_num | ORPGVCP_RDAVCP, i, ORPGVCP_DOPPLER1 );
                  edge_ang2 = ORPGVCP_get_edge_angle( vcp_num | ORPGVCP_RDAVCP, i, ORPGVCP_DOPPLER2 );
                  edge_ang3 = ORPGVCP_get_edge_angle( vcp_num | ORPGVCP_RDAVCP, i, ORPGVCP_DOPPLER3 );
                  puls_cnt1 = ORPGVCP_get_pulse_count( vcp_num | ORPGVCP_RDAVCP | ORPGVCP_VOLNUM, i, 
                                              ORPGVCP_DOPPLER1, vol_num );
                  puls_cnt2 = ORPGVCP_get_pulse_count( vcp_num | ORPGVCP_RDAVCP | ORPGVCP_VOLNUM, i, 
                                              ORPGVCP_DOPPLER2, vol_num );
                  puls_cnt3 = ORPGVCP_get_pulse_count( vcp_num | ORPGVCP_RDAVCP | ORPGVCP_VOLNUM, i, 
                                              ORPGVCP_DOPPLER3, vol_num );

                  fprintf( stderr, "------>PRF1:  %d, Pulse Cnt:  %d, Edge Angle1: %f\n", prf1, puls_cnt1, edge_ang1 );
                  fprintf( stderr, "------>PRF2:  %d, Pulse Cnt:  %d, Edge Angle2: %f\n", prf2, puls_cnt2, edge_ang2 );
                  fprintf( stderr, "------>PRF3:  %d, Pulse Cnt:  %d, Edge Angle3: %f\n", prf3, puls_cnt3, edge_ang3 );

               }

            }

            memset( &angles[0], 0, sizeof(float)*ECUTMAX );
            num_elevs = ORPGVCP_get_all_elevation_angles( vcp_num | ORPGVCP_RDAVCP, ECUTMAX, &angles[0] );
            fprintf( stderr, "VCP: %d, All Elevations: %d\n", vcp_num, num_elevs );
            for( i = 0; i < num_elevs; i++ )
               fprintf( stderr, "--->Elev #: %d, angle: %f\n", i, angles[i] );

            break;

         case 3:

            fprintf( stdout, "\nEnter Volume Number\n" );
            scanf( "%d", &vol_num );

            pulse_width = ORPGVCP_get_pulse_width( vcp_num | ORPGVCP_RDAVCP | ORPGVCP_VOLNUM, vol_num );
            clutter_map = ORPGVCP_get_clutter_map_num( vcp_num | ORPGVCP_RDAVCP | ORPGVCP_VOLNUM, vol_num );
            pattern_type = ORPGVCP_get_pattern_type( vcp_num | ORPGVCP_RDAVCP | ORPGVCP_VOLNUM, vol_num );
            vel_reso = ORPGVCP_get_vel_resolution( vcp_num | ORPGVCP_RDAVCP | ORPGVCP_VOLNUM, vol_num );
            num_elevs = ORPGVCP_get_num_elevations( vcp_num | ORPGVCP_RDAVCP | ORPGVCP_VOLNUM, vol_num );

            fprintf( stderr, "RDA VCP %d (Volume %d) Attributes:\n", vcp_num, vol_num );
            fprintf( stderr, "--->Num Elevs:     %d\n", num_elevs );
            fprintf( stderr, "--->Pulse Width:   %d\n", pulse_width );
            fprintf( stderr, "--->Clutter Map:   %d\n", clutter_map );
            fprintf( stderr, "--->Pattern Type:  %d\n", pattern_type );
            fprintf( stderr, "--->Vel Reso:      %d\n", vel_reso );
            
            for( i = 0; i < num_elevs; i++ ){
               
               elev_angle = ORPGVCP_get_elevation_angle( vcp_num | ORPGVCP_RDAVCP | ORPGVCP_VOLNUM, i, vol_num );
               waveform = ORPGVCP_get_waveform( vcp_num | ORPGVCP_RDAVCP | ORPGVCP_VOLNUM, i, vol_num );
               super_res = ORPGVCP_get_super_res( vcp_num | ORPGVCP_RDAVCP | ORPGVCP_VOLNUM, i, vol_num );
               dual_pol = ORPGVCP_get_dual_pol( vcp_num | ORPGVCP_RDAVCP | ORPGVCP_VOLNUM, i, vol_num );
               azi_rate = ORPGVCP_get_azimuth_rate( vcp_num | ORPGVCP_RDAVCP | ORPGVCP_VOLNUM, i, vol_num );
               config = ORPGVCP_get_configuration( vcp_num | ORPGVCP_RDAVCP | ORPGVCP_VOLNUM, i, vol_num );
               phase = ORPGVCP_get_phase_type( vcp_num | ORPGVCP_RDAVCP | ORPGVCP_VOLNUM, i, vol_num );

               fprintf( stderr, "--->Elevation: %d\n", i );
               fprintf( stderr, "------>Elev angle: %f\n", elev_angle );
               fprintf( stderr, "------>waveform:   %d\n", waveform );
               fprintf( stderr, "------>Super Res:  %d\n", super_res );
               fprintf( stderr, "------>Dual Pol:   %d\n", dual_pol );
               fprintf( stderr, "------>Azm Rate:   %f\n", azi_rate );
               fprintf( stderr, "------>Config:     %d\n", config );
               fprintf( stderr, "------>Phase:      %d\n", phase );
               if( waveform != VCP_WAVEFORM_CS ){

                  prf1 = ORPGVCP_get_prf_num( vcp_num | ORPGVCP_RDAVCP | ORPGVCP_VOLNUM, i, 
                                              ORPGVCP_DOPPLER1, vol_num );
                  prf2 = ORPGVCP_get_prf_num( vcp_num | ORPGVCP_RDAVCP | ORPGVCP_VOLNUM, i, 
                                              ORPGVCP_DOPPLER2, vol_num );
                  prf3 = ORPGVCP_get_prf_num( vcp_num | ORPGVCP_RDAVCP | ORPGVCP_VOLNUM, i, 
                                              ORPGVCP_DOPPLER3, vol_num );
                  edge_ang1 = ORPGVCP_get_edge_angle( vcp_num | ORPGVCP_RDAVCP | ORPGVCP_VOLNUM, i, 
                                              ORPGVCP_DOPPLER1, vol_num );
                  edge_ang2 = ORPGVCP_get_edge_angle( vcp_num | ORPGVCP_RDAVCP | ORPGVCP_VOLNUM, i, 
                                              ORPGVCP_DOPPLER2, vol_num );
                  edge_ang3 = ORPGVCP_get_edge_angle( vcp_num | ORPGVCP_RDAVCP | ORPGVCP_VOLNUM, i, 
                                              ORPGVCP_DOPPLER3, vol_num );
                  puls_cnt1 = ORPGVCP_get_pulse_count( vcp_num | ORPGVCP_RDAVCP | ORPGVCP_VOLNUM, i, 
                                              ORPGVCP_DOPPLER1, vol_num );
                  puls_cnt2 = ORPGVCP_get_pulse_count( vcp_num | ORPGVCP_RDAVCP | ORPGVCP_VOLNUM, i, 
                                              ORPGVCP_DOPPLER2, vol_num );
                  puls_cnt3 = ORPGVCP_get_pulse_count( vcp_num | ORPGVCP_RDAVCP | ORPGVCP_VOLNUM, i, 
                                              ORPGVCP_DOPPLER3, vol_num );

                  fprintf( stderr, "------>PRF1:  %d, Pulse Cnt:  %d, Edge Angle1: %f\n", prf1, puls_cnt1, edge_ang1 );
                  fprintf( stderr, "------>PRF2:  %d, Pulse Cnt:  %d, Edge Angle2: %f\n", prf2, puls_cnt2, edge_ang2 );
                  fprintf( stderr, "------>PRF3:  %d, Pulse Cnt:  %d, Edge Angle3: %f\n", prf3, puls_cnt3, edge_ang3 );

               }

            }

            memset( &angles[0], 0, sizeof(float)*ECUTMAX );
            num_elevs = ORPGVCP_get_all_elevation_angles( vcp_num | ORPGVCP_RDAVCP | ORPGVCP_VOLNUM, 
                                                          ECUTMAX, &angles[0], vol_num );
            fprintf( stderr, "VCP: %d, All Elevations: %d\n", vcp_num, num_elevs );
            for( i = 0; i < num_elevs; i++ )
               fprintf( stderr, "--->Elev #: %d, angle: %f\n", i, angles[i] );

            break;

         case 4:

            fprintf( stdout, "\nEnter Volume Number\n" );
            scanf( "%d", &vol_num );

            memset( &angles[0], 0, sizeof(float)*ECUTMAX );
            memset( &suppl[0], 0, sizeof(unsigned short)*ECUTMAX );
            num_elevs = Get_all_elevation_angles( vcp_num, ECUTMAX, vol_num, 
                                                  angles, suppl, 0 );
            fprintf( stderr, "VCP: %d, All Elevations: %d\n", vcp_num, num_elevs );
            for( i = 0; i < num_elevs; i++ )
               fprintf( stderr, "--->Elev #: %d, angle: %f, suppl: %x\n", 
                        i, angles[i], suppl[i] );

            memset( &angles[0], 0, sizeof(float)*ECUTMAX );
            memset( &suppl[0], 0, sizeof(unsigned short)*ECUTMAX );
            num_elevs = Get_all_elevation_angles( vcp_num, ECUTMAX, vol_num, 
                                                  angles, suppl, 1 );
            fprintf( stderr, "VCP: %d, All Elevations (fallback): %d\n", vcp_num, num_elevs );
            for( i = 0; i < num_elevs; i++ )
               fprintf( stderr, "--->Elev #: %d, angle: %f, suppl: %x\n", 
                        i, angles[i], suppl[i] );

            break;

         case 0:
         default:
            exit(1);

      }

   }

   return 0;

} /* End of main() */

/************************************************************************

   Gets all elevation angles for the specified VCP. 

   Inputs:
      vcp - VCP number.
      buffer_size - number of elements of array elev_angles.
      vs_num - volume scan number [1,80].
      elev_angle - buffer of size "buffer_size" to hold the angles,
                   in 0.1 deg.

   Outputs:
      elev_angle - elevation angles for VCP "vcp", in 0.1 degs.
      suppl - elevation cut supplemental flags

   Returns:
      Number of elevation angles in VCP.

************************************************************************/
static int Get_all_elevation_angles( int vcp, int buffer_size, 
                                     int vs_num, float *elev_angles,
                                     unsigned short *suppl, int fallback ){

   VCP_ICD_msg_t *rdavcp = NULL;
   RDA_rdacnt_t *rdacnt = NULL;
   char *buf = NULL;
   int i, ind, ret, rda_num_elevs, num_elevs, elev_num, old_elev_num;
   
   static int initLB = 1;

   /* Check to make sure the volume scan number is valid, i.e., [1, 80]. */
   if( (vs_num < 1) || (vs_num > MAX_VSCAN) )
      ret = 0; 

   else{

      /* This function can be called with negative vcp number.  In this
         case, get the vcp number from volume status. */
      if( vcp <= 0 )
         vcp = ORPGVST_get_vcp();

      /* Set the write permission in case this is the first access. */
      if( initLB ){

         ORPGDA_write_permission( ORPGDAT_ADAPTATION );
         initLB = 0;

      }

      /* Read the RDA_RDACNT data, if available.  If the vcp number matches
         use this data, otherwise, use the RDACNT data (via ORPGVCP 
         functions. ) */ 
      ret = ORPGDA_read( ORPGDAT_ADAPTATION, (void *) &buf, LB_ALLOC_BUF,
                         RDA_RDACNT );

      if( fallback )
         ret = -1;

   }

   /* Some error occurred.  Use RDACNT data. */
   if( ret <= 0 ){

      if( (ret < 0) && (ret != LB_NOT_FOUND) )
         LE_send_msg( GL_ERROR,
             "ORPGDA_read( ORPGDAT_ADAPTATION, RDA_RDACNT) Failed (%d)\n",
              ret );


      num_elevs = ORPGVCP_get_all_elevation_angles( vcp, buffer_size,
                                                    elev_angles );
      Get_vcp_suppl_data( vcp, buffer_size, suppl );
      return( num_elevs );

   }
   /* Assign pointers. */
   rdacnt = (RDA_rdacnt_t *) buf;

   /* Determine which entry to use. */
   if( vs_num == 0 )
      ind = rdacnt->last_entry;

   else
      ind = vs_num % 2;

   rdavcp = (VCP_ICD_msg_t *) &rdacnt->data[ind].rdcvcpta[0];

   /* Extract the vcp number and check it against what was passed in. */
   if( rdavcp->vcp_msg_hdr.pattern_number != vcp ){

      /* Numbers don't match.  Use RDACNT data. */
      free(buf);
      num_elevs = ORPGVCP_get_all_elevation_angles( vcp, buffer_size,
                                                    elev_angles );
      Get_vcp_suppl_data( vcp, buffer_size, suppl );
      return( num_elevs );

   }

   /* VCP numbers match.   Return the elevation angles for the 
      current VCP. */
   rda_num_elevs = rdavcp->vcp_elev_data.number_cuts;
   num_elevs = 0;
   old_elev_num = elev_num = 0;
   for( i = 0; i < rda_num_elevs; i++ ){

      elev_num = rdacnt->data[ind].rdccon[i];

      /* Only interested in unique cuts.  Therefore only include
         once. */
      if( elev_num == old_elev_num )
         continue;

      /* Convert the angle, in BAMS, to degs. */
      elev_angles[num_elevs] = ORPGVCP_BAMS_to_deg( ORPGVCP_ELEVATION_ANGLE,
                                        rdavcp->vcp_elev_data.data[i].angle );
      suppl[num_elevs] = rdacnt->data[ind].suppl[i];

      num_elevs++;
      old_elev_num = elev_num;

      /* Have we reached the buffer size limit? */
      if( num_elevs >= buffer_size )
         break;

   }

   /* Free VCP buffer. */
   free(buf);

   return num_elevs;

}
/************************************************************************

   Gets VCP supplemental data for the specified VCP. 

   Inputs:
      vcp - VCP number.

   Outputs:
      suppl_flags - elevation cut supplemental flags

   Returns:
      The number of elevation cuts. 

************************************************************************/
static int Get_vcp_suppl_data( int vcp, int buffer_size,
                               unsigned short *suppl_flags ){

   int ind, num_elevs = 0, max_ind = 0;
   int new_ind = 0, old_ind = -1;
   VCP_ICD_msg_t *c_vcp = NULL;
   short *rdccon = NULL;
   float first_angle = -1.0;

   /* Get VCP from RPG. */
   if( (ind = ORPGVCP_index( vcp )) < 0 )
      return -1;

   if( (c_vcp = (VCP_ICD_msg_t *) ORPGVCP_ptr( ind )) == NULL )
      return -1;

   if( (rdccon = ORPGVCP_elev_indicies_ptr( ind )) == NULL )
      return -1;

   /* Do For All elevation cuts in the VCP ... */
   for( ind = 0; ind < c_vcp->vcp_elev_data.number_cuts; ind++ ){

      int waveform = (int) c_vcp->vcp_elev_data.data[ind].waveform;
      int phase  = (int) c_vcp->vcp_elev_data.data[ind].phase;
      int super_res = (int) c_vcp->vcp_elev_data.data[ind].super_res;
      float elev_angle = ORPGVCP_BAMS_to_deg( ORPGVCP_ELEVATION_ANGLE,
                                              c_vcp->vcp_elev_data.data[ind].angle );

      /* Need to stop when the buffer size is reached. */
      if( max_ind >= buffer_size )
         break;

      /* Only care about unique cuts. */
      new_ind= rdccon[ind];
      if( new_ind == old_ind )
         continue;

      /* Set waveform bit. */
      if( waveform == VCP_WAVEFORM_CS )
         suppl_flags[num_elevs] |= RDACNT_IS_CS;

      else if( waveform == VCP_WAVEFORM_CD )
         suppl_flags[num_elevs] |= RDACNT_IS_CD;

      else if( waveform == VCP_WAVEFORM_CDBATCH )
         suppl_flags[num_elevs] |= RDACNT_IS_CDBATCH;

      else if( waveform == VCP_WAVEFORM_BATCH )
         suppl_flags[num_elevs] |= RDACNT_IS_BATCH;

      else if( waveform == VCP_WAVEFORM_STP )
         suppl_flags[num_elevs] |= RDACNT_IS_SPRT;

      /* Set phase bit. */
      if( phase == VCP_PHASE_SZ2 )
         suppl_flags[num_elevs] |= RDACNT_IS_SZ2;

      /* Set super resolution  bit. */
      if( super_res & VCP_HALFDEG_RAD )
         suppl_flags[num_elevs] |= RDACNT_IS_SR;

      /* Until I figure out a better way to determine supplemental
         cuts, if the elevation angle matches the lowest cut, it
         is supplemental. */
      if( (rdccon[ind] > 1) && (elev_angle == first_angle) )
         suppl_flags[num_elevs] |= RDACNT_SUPPL_SCAN;

      /* Set old_ind to new_ind. */
      old_ind = new_ind;

      /* Increment the number of elevations. */
      num_elevs++;

   }

   return( num_elevs );

}

