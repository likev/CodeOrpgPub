/*
 * RCS info
 * $Author: steves $
 * $Locker:  $
 * $Date: 2007/02/23 21:23:45 $
 * $Id: write_vcp_template.c,v 1.6 2007/02/23 21:23:45 steves Exp $
 * $Revision: 1.6 $
 * $State: Exp $
*/


#include <write_vcp_template.h>

/* Global variables. */
extern int Verbose_mode;

/* Static global variables. */
static FILE *Fd = NULL;

/*\//////////////////////////////////////////////////////////////////////////
//
//   Description:
//      Construct and open the VCP file.   The file name format is vcp_xx,
//      where xx is the pattern number.
//
//   Inputs:
//      pattern_num - VCP number.
//
//   Returns:
//      File Handle or NULL if cannot open the file.
//
//////////////////////////////////////////////////////////////////////////\*/
FILE* Get_filename( int pattern_num ){

   char file_name[10];
   char ext[4];

  /* Construct the name of the vcp file name.  Must be of format "vcp_#" */
   memset( file_name, 0, sizeof(file_name) );
   memset( ext, 0, sizeof(ext) );
   strcat( file_name, "vcp_" );
   sprintf( ext, "%d", pattern_num );
   strcat( file_name, ext );

   /* Open the file by name "vcp_#". */
   Fd = fopen( file_name, "w+" );
   if( Fd == NULL )
      fprintf( stderr, "Unable to open file %s\n", file_name );
   
   return(Fd);

} /* End of Get_filename() */


/*\//////////////////////////////////////////////////////////////////////////
//
//   Description:
//      Initialize some Vol_info_t data structure elements to common
//      defaults.   Values which are required fields are set to 0.
//
//   Inputs:
//      vol_info - pointer to Vol_info_t data structure.
//
//   Returns:
//      -1 if vol_info is a NULL pointer, 0 otherwise.
//
//////////////////////////////////////////////////////////////////////////\*/
int Init_VCP_attr( Vol_info_t *vol_info ){

   if( vol_info == NULL )
      return(-1);

   memset( vol_info, 0, sizeof(Vol_info_t) );

   /* Set some defaults. */
   vol_info->where_defined = ORPGVCP_RPG_DEFINED_VCP;
   vol_info->pulse_width = ORPGVCP_SHORT_PULSE; 
   vol_info->vel_reso = 0.5;
   vol_info->cluttermap_group = 1;

   return(0);

} /* End of Init_VCP_attr */

/*\//////////////////////////////////////////////////////////////////////////
//   Description:
//      Initialize some Cut_info_t data structure elements to common
//      defaults.   Values which are required fields are set to 0.
//
//   Inputs:
//      cut_info - pointer to Cut_info_t data structure.
//
//   Returns:
//      -1 if cut_info is a NULL pointer, 0 otherwise.
//
//////////////////////////////////////////////////////////////////////////\*/
int Init_elevation_attr( Cut_info_t *cut_info ){

   if( cut_info == NULL )
      return(-1);

   memset( cut_info, 0, sizeof(Cut_info_t) );

   /* Set some defaults. */
   cut_info->prf_sector[0].edge_angle = -1.0;
   cut_info->prf_sector[1].edge_angle = -1.0;
   cut_info->prf_sector[2].edge_angle = -1.0;

   cut_info->surv_snr = 2.0;
   cut_info->vel_snr = 3.5;
   cut_info->spw_snr = 3.5;

   return(0);

} /* End of Init_elevation_attr */

/*\//////////////////////////////////////////////////////////////////////////
//
//   Description:
//      Writes out the VCP attributes section.
//
//   Inputs:
//      pattern_num - VCP number.
//      wx_mode - Weather mode.
//      num_elev_cuts - Number of elevation cuts in VCP.
//      where_defined - where the VCP will be defined (RPG or RPG and RDA).
//      pulse_width - pulse width (either SHORT or LONG)
//
//   Returns:
//      -1 if argument is NULL pointer, -2 if Fd is NULL pointer, 0 otherwise.
//
//////////////////////////////////////////////////////////////////////////\*/
int Write_VCP_attr( Vol_info_t *vol_info ){
 
   /* Verify the vol_info argument is a valid address. */
   if( vol_info == NULL )
      return(-1);

   /* Verify the Fd has been defined. */
   if( Fd == NULL )
      return(-2);

   /* Write out the VCP_attr section. */
   fprintf( Fd, "VCP_attr {\n\n" );

   /* Write out the VCP number. */
   fprintf( Fd, "    pattern_num       %-4d\n", vol_info->pattern_num );
               
   /* Write out the weather mode. */
   if( vol_info->wx_mode == 1 )
      fprintf( Fd, "    wx_mode           B\n" );
   else
      fprintf( Fd, "    wx_mode           A\n" );
            
   /* Write out the number of elevation cuts. */
   fprintf( Fd, "    num_elev_cuts     %-2d\n", vol_info->num_elev_cuts );
               
   /* Write out where the VCP is defined. */
   if( vol_info->where_defined == ORPGVCP_RPG_DEFINED_VCP )
      fprintf( Fd, "    where_defined     RPG\n" );
   else if( vol_info->where_defined == (ORPGVCP_RPG_DEFINED_VCP + ORPGVCP_RDA_DEFINED_VCP) )
      fprintf( Fd, "    where_defined     BOTH\n" );
   else
      fprintf( Fd, "    where_defined     RPG\n" );

   /* Write out the pulse width. */
   if( vol_info->pulse_width == ORPGVCP_SHORT_PULSE )
      fprintf( Fd, "    pulse_width       SHORT\n" );
   else if( vol_info->pulse_width == ORPGVCP_LONG_PULSE )
      fprintf( Fd, "    pulse_width       LONG\n" );
   else 
      fprintf( Fd, "    pulse_width       SHORT\n" );

   /* Write out the Doppler resolution. */
   if( vol_info->vel_reso == 1.0 )
      fprintf( Fd, "    velocity_reso     1.0\n" );
   else
      fprintf( Fd, "    velocity_reso     0.5\n" );

   /* Write out the clutter map group number. */
      fprintf( Fd, "    clutmap_grp       %1d\n", vol_info->cluttermap_group );

   /* End of VCP_attr section. */
   fprintf( Fd, "\n}\n" );

   /* Flush the data to disk. */
   fflush( Fd );

   return(0);

} /* End of Write_VCP_attr() */

/*\//////////////////////////////////////////////////////////////////////////
//
//   Description:
//      Controls the writing of the Elev_attr section.
//
//   Inputs:
//      flag = START_ELEV_ATTR (starts new Elev_attr section)
//             NO_ACTION (writes out information contained in cut_info)
//             END_ELEV_ATTR (end the Elev_attr section) 
//      cut_info - contains elevation angle specific information.
//      num_alwb_prfs - number of allowable prfs
//      alwb_prfs - the allowable prfs (stored as PRIs).
//
//   Returns:
//      -1 if cut_info is NULL pointer, -2 if Fd is NULL pointer, 0 otherwise.
//
//////////////////////////////////////////////////////////////////////////\*/
int Write_elevation_attr( int flag, Cut_info_t *cut_info,
                          int num_alwb_prfs, int alwb_prfs[] ){

   int pri;

   /* Verify the cut_info argument is a valid address. */
   if( cut_info == NULL )
      return(-1);

   /* Verify the Fd has been defined. */
   if( Fd == NULL )
      return(-2);
   
   /* If this is the start of the Elev_attr section, write header and 
      allowable PRFs. */
   if( flag == START_ELEV_ATTR ){
      
      fprintf( Fd, "\nElev_attr {\n\n" );
      fprintf( Fd, "    allowable_prfs     " );

      for( pri = 0; pri < num_alwb_prfs; pri++ ){

         if( alwb_prfs[pri] != 0 )
            fprintf( Fd, "%2d", alwb_prfs[ pri ] );

      }

      fprintf( Fd, "\n\n" );
         
   }

   Write_elevation_info( cut_info, num_alwb_prfs, alwb_prfs );

   /* If no more elevations, end the "Elev_attr section. */
   if( flag == END_ELEV_ATTR )
      fprintf( Fd, "}\n" );

   return(0);

}

/*\//////////////////////////////////////////////////////////////////////////
//
//   Description:
//      Writes elevation information.
//
//   Inputs:
//      cut_info - elevation cut data structure.
//      num_alwb_prfs - number of allowable PRFs
//      alwb_prfs - the allowable PRFS
//   
//   Returns:
//      -1 if cut_info is NULL pointer, -2 if Fd is NULL pointer, 0 otherwise.
//
//////////////////////////////////////////////////////////////////////////\*/
int Write_elevation_info( Cut_info_t *cut_info, int num_alwb_prfs, int alwb_prfs[] ){

   int i, pri;
   short waveform, phase;

   /* Verify the cut_info argument is a valid address. */
   if( cut_info == NULL )
      return(-1);

   /* Verify the Fd has been defined. */
   if( Fd == NULL )
      return(-2);
   
   fprintf( Fd, "    Elev_%d {\n\n", cut_info->cut_num+1 );
   fprintf( Fd, "        elev_ang_deg      %-5.2f\n", cut_info->elev_ang_deg );

   waveform = cut_info->waveform;
   phase = cut_info->phase;

   switch( waveform ){

      case 1:
         fprintf( Fd, "        waveform_type     CS\n" );
         break;

      case 2:
         fprintf( Fd, "        waveform_type     CD\n" );
         break;

      case 3:
         fprintf( Fd, "        waveform_type     CDBATCH\n" );
         break;

      case 4:
         fprintf( Fd, "        waveform_type     BATCH\n" );
         break;

   } /* End of "switch" */

   switch( phase ){

      case 2:
         fprintf( Fd, "        phase             SZ2\n" );
         break;

   } /* End of "switch" */

   /* For CS and Batch waveforms, must supply Surveillance PRF/pulse count. */
   if( (waveform == 1 /* CS */) 
                || 
       (waveform == 4 /* BATCH */) ){

      fprintf( Fd, "        surv_prf          %-3d\n", cut_info->def_surv_prf );
      fprintf( Fd, "        surv_pulses       %-3d\n", cut_info->def_surv_p_cnt );

   }

   /* For all waveforms other than CS, must provide Doppler PRF/pulse counts. */
   if( waveform != 1 ){
      
      if( cut_info->prf_sector[0].edge_angle < 0.0 )
         fprintf( Fd, "        dop_prf           %1d\n", cut_info->def_dop_prf );

      fprintf( Fd, "        dop_pulses        " );

      for( pri = 1; pri <= MAX_DOP_PRI; pri++ ){

         if( cut_info->def_dop_p_cnt[pri] != 0 ){

            for( i = 0; i < num_alwb_prfs; i++ ){

               if( pri == alwb_prfs[i] ){

                  fprintf( Fd, "%-3d  ", cut_info->def_dop_p_cnt[pri] );
                  break;

               }

            }

         }

      }

      fprintf( Fd, "\n" );

      if( cut_info->prf_sector[0].edge_angle >= 0.0 ){

         for( i = 1; i <= MAX_PRF_SECTORS; i++ ){

            fprintf( Fd, "        Sector_%1d {\n\n", i );
            fprintf( Fd, "           edge_angle     %-6.2f\n", cut_info->prf_sector[i-1].edge_angle );
            fprintf( Fd, "           dop_prf        %1d\n", cut_info->prf_sector[i-1].dop_prf );
            fprintf( Fd, "\n        }\n" );
         }

      }

   }

   /* The defaults for Batch and CD/WO is 3.5 dB for surv_snr */
   if( (waveform == 3 /* CDBATCH */) || (waveform == 4 /* BATCH */) )
      cut_info->surv_snr = 3.5;

   fprintf( Fd, "        scan_rate_dps     %-6.3f\n", cut_info->scan_rate_deg );
   fprintf( Fd, "        SNR_thresh_dB     %-6.2f  %-6.2f  %-6.2f\n\n    }\n\n",
            cut_info->surv_snr, cut_info->vel_snr, cut_info->spw_snr );

   fflush( Fd );

   return(0);

} /* End of Write_elevation_info() */

/*\//////////////////////////////////////////////////////////////////////////
//
//   Description:
//      Flushes the file buffer and closes the file.
//
//////////////////////////////////////////////////////////////////////////\*/
void Close_file(){

   if( Fd != NULL ){

      fflush( Fd );
      fclose( Fd );
      Fd = NULL;

   }

} /* End of Close_file() */ 
